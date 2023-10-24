#include <stdbool.h>
#include <uchar.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "lexicon.h"
#include "lexhnd.h"
#include "cu32.h"
#include "minseg.h"



static uint64_t 
u64_arr_sum(uint64_t* arr, size_t sz)
{
    uint64_t sum = 0;
    for(size_t i=0; i<sz;i++)
    {
        sum += arr[i];
    }
    return sum;
}

#define ALPHABET_INIT_LENGTH 100
#define ALPHABET_RESIZE_RATE 0.8


typedef struct 
lexhnd_alphabet
{
    char32_t* alphabet;
    size_t alphabet_sz;
    uint64_t* char_counts;
} alphabet;

static void 
alphabet_resize(alphabet* ab)
{
    char32_t* new_alphabet = calloc(2 * ab->alphabet_sz,sizeof(char32_t));
    if(new_alphabet == NULL) abort();

    size_t i = 0;
    while(new_alphabet[i])
    {
        new_alphabet[i] = ab->alphabet[i];
        i++;
    }

    free(ab->alphabet);
    ab->alphabet = new_alphabet;
}

static void 
alphabet_add(alphabet* ab, char32_t character)
{
    for(size_t i=0; i<ab->alphabet_sz;i++)
    {
        if(ab->alphabet[i] == character) 
        {
            ab->char_counts[i]++;
            return;
        }
        if(ab->alphabet[i] == 0)
        {
            if((float) i > ALPHABET_RESIZE_RATE * ab->alphabet_sz) 
                alphabet_resize(ab); 
            ab->alphabet[i] = character;
            ab->char_counts[i]++;
            return;
        }
    }
}

static void 
alphabet_add_word(alphabet* ab, char32_t* word)
{
    while(*word)
    {
        alphabet_add(ab,*word);
        word++;
    }
    assert(*word == 0);
}

static alphabet* 
alphabet_create()
{
    alphabet* ab = malloc(sizeof(alphabet));
    if(ab == NULL) abort();

    ab->alphabet_sz = ALPHABET_INIT_LENGTH; 
    ab->alphabet = calloc(ALPHABET_INIT_LENGTH,sizeof(char32_t));
    ab->char_counts = calloc(ALPHABET_INIT_LENGTH,sizeof(uint64_t));
    if(ab-> alphabet == NULL || ab->char_counts == NULL) abort();

    return ab;
}


static void 
alphabet_free(alphabet* ab)
{
    free(ab->alphabet);
    free(ab->char_counts);
    free(ab);
}


static double 
alphabet_char_cost(alphabet* ab, uint64_t char_count)
{
    uint64_t total_char_counts = u64_arr_sum(ab->char_counts, ab->alphabet_sz);
    
    return -1 * log2((double) char_count/total_char_counts);
}


static double 
alphabet_get_word_cost(alphabet* ab, char32_t* word)
{
    double total_cost = 0;
    bool found = false;
    while(*word)
    {
        size_t i = 0;
        while(ab->alphabet[i])
        { 
            if(ab->alphabet[i] == *word)
            {
                total_cost += alphabet_char_cost(ab,ab->char_counts[i]);
                found = true;
                break;
            }
            i++;
        }
        if(!found) return DBL_MAX;
        found = false;  
        word++;
    }
    return total_cost;
}

static void 
alphabet_setup(alphabet* ab, char32_t** corpus, size_t corpus_sz)
{
    for(size_t i=0;i<corpus_sz;i++)
    {
        alphabet_add_word(ab,corpus[i]);
    }
}

static double 
get_lexicon_bitlength(alphabet* ab, lexicon* lex)
{
    double bitlen = 0;
    size_t lexlen = lex->occupancy;
    litem** items = malloc(lexlen * sizeof(litem*));
    lexicon_get_items(lex,items);
    for(size_t i=0;i<lexlen;i++)
    {
        bitlen += alphabet_get_word_cost(ab,items[i]->key);
    }
    return bitlen;
}


typedef struct lexhnd_parse
{
    char32_t* segments;
    size_t size;
    size_t pos;
} parse;

#define PARSE_SEGMENTS_BUFFER_INIT_SZ 2000

static parse*
parse_create()
{
    parse* prs = malloc(sizeof(parse));
    if(prs == NULL) abort();
    prs->segments = malloc(PARSE_SEGMENTS_BUFFER_INIT_SZ * sizeof(char32_t));
    if(prs->segments == NULL) abort();
    prs->size = PARSE_SEGMENTS_BUFFER_INIT_SZ;
    prs->pos = 0;
    return prs;
}

static char32_t*
parse_pop(parse* parse)
{
    if(!parse->pos) return parse->segments;
    parse->pos--;
    while(1)
    {
        if(!parse->pos) return parse->segments;
        parse->pos--;
        if(!parse->pos) return parse->segments;
        if(!parse->segments[parse->pos]) 
        {
            parse->pos++;
            return parse->segments + parse->pos;
        }
    }
}


static void
parse_add(parse* parse, char32_t* str)
{
    if(parse->pos > parse->size - 50) 
    {
        parse->size = 2 * parse->size;
        parse->segments = realloc(parse->segments, parse->size * sizeof(char32_t));
        if(parse->segments == NULL) abort();  
    }
    u32strcpy(parse->segments + parse->pos,str); 
    parse->pos = parse->pos + u32strlen(str) + 1;
    
}

static void
parse_free(parse* parse)
{
    free(parse->segments);
    free(parse);
}


static void 
u32strjoin(char32_t* buffer, char32_t* fst, char32_t* snd)
{
    size_t len_1 = u32strlen(fst);
    size_t len_2 = u32strlen(snd);
    size_t pos = 0;
    for(size_t i=0;i<len_1;i++)
    {
        buffer[pos] = fst[i];
        pos++;
    }
    for(size_t i=0;i<len_2;i++)
    {
        buffer[pos] = snd[i];
        pos++;
    }
    buffer[pos] = '\0';  
}

#define JOIN_BUFFER_SZ 100

static parse*
iteration_zero(alphabet* ab, char32_t** corpus, size_t corpus_sz, 
        lexhnd_result* res)
{
    
    lexicon* lex = lexicon_create();
    
    for(size_t i=0;i<ab->alphabet_sz;i++)
    {
        char32_t letter[2];
        letter[0] = ab->alphabet[i];
        letter[1] = 0;
        lexicon_add(lex,letter,ab->char_counts[i]);
    }

    double priors = get_lexicon_bitlength(ab,lex);
    double posteriors = 0;
    parse* res_parse = parse_create();
   
    char32_t join_buffer[JOIN_BUFFER_SZ];
    for(size_t i=0;i<corpus_sz;i++)
    {
        minseg* mseg = minseg_create(lex,corpus[i]);
        posteriors += mseg->cost;
        for(size_t j=0;j<mseg->size;j+=2)
        { 
            if(j+1 == mseg->size)
                parse_add(res_parse,mseg->segments[j]);      
            else
            {
                u32strjoin(join_buffer,mseg->segments[j],mseg->segments[j+1]);
                parse_add(res_parse,join_buffer);
            } 
        }
        minseg_free(mseg);
    }
    
    res->lexicons[0] = lex;
    res->priors[0] = priors;
    res->posteriors[0] = posteriors;


    return res_parse;
}

static lexicon* 
lexicon_copy(lexicon* origin)
{
    lexicon* copy = lexicon_create();
    for(size_t i=0;i<origin->capacity;i++)
    {
        if(origin->table[i] != NULL)
        {
            lexicon_add(copy,origin->table[i]->key, origin->table[i]->count);
        }
    }
    return copy;
}

static parse*
iteration_n(size_t it_n,uint8_t n_new_words, alphabet*ab, char32_t**corpus, size_t corpus_sz, 
        lexhnd_result* res, parse* old_parse)
{
    
    lexicon* candidate_new_words = lexicon_create(); 

    // Populate candidate lexicon with joint items from old parse
    while(old_parse->pos) lexicon_add(candidate_new_words,parse_pop(old_parse),1);
    litem** litems = malloc(candidate_new_words->occupancy * sizeof(litem*));
    lexicon_get_items(candidate_new_words, litems);

    // Create temporary lexicon with old lexicon + n most frequent 
    // new joint items
    lexicon* temp = lexicon_copy(res->lexicons[it_n-1]);
    for(size_t i=0;i<n_new_words;i++)
    {
        lexicon_add(temp,litems[i]->key,litems[i]->count);
    }

 
    // Minseg 1
    for(size_t i=0;i<corpus_sz;i++)
    {
        minseg* m1 = minseg_create(temp,corpus[i]);
        for(size_t j=0;j<m1->size;j++)
            parse_add(old_parse,m1->segments[j]);
        minseg_free(m1);
    }
    
    // Lexicon
    lexicon* lexicon_n = lexicon_create();
    while(old_parse->pos) lexicon_add(lexicon_n,parse_pop(old_parse),1);
    

    // Minseg 2
    double priors = get_lexicon_bitlength(ab,lexicon_n);
    double posteriors = 0;
    char32_t join_buffer[JOIN_BUFFER_SZ];
    for(size_t i=0;i<corpus_sz;i++)
    {
        minseg* m2 = minseg_create(lexicon_n,corpus[i]);
        posteriors += m2->cost;
        for(size_t j=0;j<m2->size;j+=2)
        { 
            if(j+1 == m2->size)
                parse_add(old_parse,m2->segments[j]);      
            else
            {
                u32strjoin(join_buffer,m2->segments[j],m2->segments[j+1]);
                parse_add(old_parse,join_buffer);
            } 
        }
        minseg_free(m2);
    }
    res->lexicons[it_n] = lexicon_n;
    res->posteriors[it_n] = posteriors;
    res->priors[it_n] = priors;


    lexicon_free(candidate_new_words); 
    lexicon_free(temp);
    free(litems);

    return old_parse;

}

lexhnd_result* 
lexhnd_run(
        char32_t** corpus,
        size_t corpus_size,
        uint8_t n_iterations,
        uint8_t n_new_words
        )
{
    lexhnd_result* result = malloc(sizeof(lexhnd_result*));
    if(result == NULL) abort();
    
    result->lexicons = malloc(n_iterations * sizeof(lexicon*));
    result->priors = malloc(n_iterations * sizeof(double));
    result->posteriors = malloc(n_iterations * sizeof(double));
    if(result->lexicons == NULL ||
       result->priors == NULL ||
       result->posteriors == NULL) abort();

    alphabet* ab = alphabet_create();
    alphabet_setup(ab,corpus,corpus_size);
    

    parse* prs = iteration_zero(ab,corpus,corpus_size,result);
    
    for(size_t i=1;i<n_iterations;i++)
    {
        prs = iteration_n(i,n_new_words,ab,corpus,corpus_size,result,prs);
    }
    
    parse_free(prs);
    alphabet_free(ab);

    return result;
}


