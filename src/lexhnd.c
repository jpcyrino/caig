#include <stdbool.h>
#include <uchar.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
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
    size_t new_alphabet_sz = 2 * ab->alphabet_sz; 
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
    char32_t** segments;
    size_t size;
    size_t free_pos;
} parse;

#define PARSE_SEGMENTS_BUFFER_INIT_SZ 2000

static parse*
parse_create()
{
    parse* prs = malloc(sizeof(parse));
    if(prs == NULL) abort();

    prs->segments = malloc(PARSE_SEGMENTS_BUFFER_INIT_SZ * sizeof(char32_t*));
    if(prs->segments == NULL) abort();
    for(size_t i=0;i<PARSE_SEGMENTS_BUFFER_INIT_SZ;i++) 
        prs->segments[i] = NULL;

    prs->size = PARSE_SEGMENTS_BUFFER_INIT_SZ;
    prs->free_pos = 0;
    return prs;
}

static void
parse_add(parse* parse, char32_t* str)
{
    if(parse->free_pos > parse->size-10)
    {
        parse->size = parse->size + PARSE_SEGMENTS_BUFFER_INIT_SZ;
        parse->segments = realloc(parse->segments, 
                parse->size * sizeof(char32_t*));
        if(parse->segments == NULL) abort();
        
    }

    parse->segments[parse->free_pos] = 
        malloc((u32strlen(str) + 1) * sizeof(char32_t));
    u32strcpy(parse->segments[parse->free_pos],str);
    parse->free_pos++;
}


static void
parse_free(parse* parse)
{
    for(size_t i=0;i<parse->size;i++)
        free(parse->segments[i]);
    free(parse->segments);
    free(parse);
}

typedef struct lexhnd_iteration
{
    lexicon* lexicon;
    parse* parse;
    double prior;
    double posterior;
} iteration;



static iteration
iteration_zero(alphabet* ab, char32_t** corpus, size_t corpus_sz)
{
    iteration result;
    lexicon* lex = lexicon_create();

    for(size_t i=0;i<ab->alphabet_sz;i++)
    {
        char32_t letter[2];
        letter[0] = ab->alphabet[i];
        letter[1] = 0;
        lexicon_add(lex,letter,ab->char_counts[i]);
    }

    result.lexicon = lex;
    result.prior = get_lexicon_bitlength(ab,lex);
    result.parse = parse_create();
    result.posterior = 0;
    for(size_t i=0;i<corpus_sz;i++)
    {
        minseg* mseg = minseg_create(lex,corpus[i]);
        result.posterior += mseg->cost;
        for(size_t j=0;j<mseg->size;j++)
            parse_add(result.parse,mseg->segments[j]);     
        minseg_free(mseg);
    }
    
    return result;

}

static iteration
iteration_n(size_t it_n,alphabet*ab, char32_t**corpus, size_t corpus_sz, parse* past_parse)
{
    iteration result;

    return result;
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

    //run iteration 0
    iteration it = iteration_zero(ab,corpus,corpus_size);
    parse* past_parse = it.parse;

    /* UB HERE !!!
    result->lexicons[0] = it.lexicon;
    result->priors[0] = it.prior;
    result->posteriors[0] = it.posterior;
   
    //run further iterations
    for(size_t i=1;i<n_iterations;i++)
    {
        it = iteration_n(i,ab,corpus,corpus_size,past_parse);
        parse_free(past_parse);
        result->lexicons[i] = it.lexicon;
        result->priors[i] = it.prior;
        result->posteriors[i] = it.posterior;
        past_parse = it.parse;

    } */

    parse_free(past_parse);
    alphabet_free(ab);

    return result;
}


