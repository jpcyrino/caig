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

#define ALPHABET_INIT_LENGTH 100
#define ALPHABET_RESIZE_RATE 0.8

static void 
alphabet_resize(lhalphabet* ab, lherror* err)
{
    size_t new_alphabet_sz = 2 * ab->alphabet_sz; 
    char32_t* new_alphabet = calloc(2 * ab->alphabet_sz,sizeof(char32_t));
    if(new_alphabet == NULL) goto error;

    size_t i = 0;
    while(new_alphabet[i])
    {
        new_alphabet[i] = ab->alphabet[i];
        i++;
    }

    free(ab->alphabet);
    ab->alphabet = new_alphabet;
    new_alphabet = NULL;

    return;
error:
    *err = LEXHND_ERROR_MEMORY_ALLOCATION; 

}

static void 
alphabet_add(lhalphabet* ab, char32_t character, lherror* err)
{
    *err = LEXHND_NORMAL;
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
            {
                alphabet_resize(ab, err); 
                if(*err) return;
            }
            ab->alphabet[i] = character;
            ab->char_counts[i]++;
            return;
        }
    }
    *err = LEXHND_ERROR_ALPHABET_FULL;
}

static void 
alphabet_add_word(lhalphabet* ab, char32_t* word, lherror* err)
{
    while(*word)
    {
        alphabet_add(ab,*word,err);
        if(*err) return;
        word++;
    }
}

static lhalphabet* 
alphabet_create(lherror* err)
{
    lhalphabet* ab = malloc(sizeof(lhalphabet));
    if(ab == NULL) goto error1;

    ab->alphabet_sz = ALPHABET_INIT_LENGTH; 
    ab->alphabet = calloc(ALPHABET_INIT_LENGTH,sizeof(char32_t));
    ab->char_counts = calloc(ALPHABET_INIT_LENGTH,sizeof(uint64_t));
    if(ab-> alphabet == NULL || ab->char_counts == NULL) goto error2;

    return ab;

error2:
    if(ab->alphabet) free(ab->alphabet);
    if(ab->char_counts) free(ab->char_counts);
error1:
    *err = LEXHND_ERROR_MEMORY_ALLOCATION;
    free(ab);
    return NULL;
}

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

static void 
alphabet_free(lhalphabet* ab)
{
    free(ab->alphabet);
    free(ab->char_counts);
    free(ab);
}


static double 
alphabet_char_cost(lhalphabet* ab, uint64_t char_count)
{
    uint64_t total_char_counts = u64_arr_sum(ab->char_counts, ab->alphabet_sz);
    
    return -1 * log2((double) char_count/total_char_counts);
}


static double 
alphabet_get_word_cost(lhalphabet* ab, char32_t* word, lherror* err)
{
    *err = LEXHND_NORMAL;
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
        if(!found)
        {
            *err = LEXHND_ERROR_CHAR_NOT_FOUND;
            return DBL_MAX;
        }
        found = false;  
        word++;
    }
    return total_cost;
}

static void 
alphabet_setup(lhalphabet* ab, char32_t** corpus, size_t corpus_sz, lherror* err)
{
    *err = LEXHND_NORMAL;
    for(size_t i=0;i<corpus_sz;i++)
    {
        alphabet_add_word(ab,corpus[i],err);
        if(*err) goto exit;
    }

exit:
    return;
}

static double 
lexhnd_get_lexicon_bitlength(lhalphabet* ab, lhcycle* cycle, lherror* err)
{
    *err = LEXHND_NORMAL;
    double bitlen = 0;
    size_t lexlen = cycle->lex->occupancy;
    litem** items = malloc(lexlen * sizeof(litem*));
    lexicon_get_items(cycle->lex,items);
    for(size_t i=0;i<lexlen;i++)
    {
        bitlen += alphabet_get_word_cost(ab,items[i]->key,err);
        if(*err) return DBL_MAX;
    }
    return bitlen;
}

static void 
lexhnd_create_first_cycle(
        lhcomponents* comps, 
        char32_t** corpus, 
        size_t corpus_sz, 
        lherror* err, 
        uint8_t* error_code
        )
{
    lexicon_error lerr = LEXICON_NORMAL;
    minseg_error merr = MINSEG_NORMAL;
    char32_t buff[2];
    double posterior = 0; 

    lexicon* lex = lexicon_create();
    if(lex == NULL) goto malloc_error;

    minseg** parses = malloc(corpus_sz * sizeof(minseg*));
    if(parses == NULL) goto malloc_error;

    // Populate lexicon with alphabet characters as 32bit strings
    for(size_t i=0;i<corpus_sz;i++)
    {
        char32_t* word = corpus[i];
        while(*word)
        {
            buff[0] = *word;
            buff[1] = 0;
            lexicon_add(lex,buff,1,&lerr);
            if(lerr) goto lexicon_error;
            word++;
        }
        // Calculate posterior with minseg
        minseg* parse = minseg_create(lex,corpus[i],&merr);
        if(merr) goto minseg_error;
        posterior += parse->cost;
        
        // Add each parse to cycle object
        parses[i] = parse;
    }

    comps->cycles[0].parses = parses;
    comps->cycles[0].lex = lex;
    comps->cycles[0].posterior_length = posterior;
    comps->cycles[0].prior_length = lexhnd_get_lexicon_bitlength(comps->alphabet, &comps->cycles[0],err);

    return;
lexicon_error:
    lexicon_free(lex);
    *err = LEXHND_ERROR_LEXICON_ERROR;
    *error_code = lerr;
    return;
minseg_error:
    *err = LEXHND_ERROR_MINSEG_ERROR;
    *error_code = merr;
    return;
malloc_error:
    *err = LEXHND_ERROR_MEMORY_ALLOCATION;
    return;
}

static void 
u32strjoin(char32_t* dest, char32_t* str1, char32_t* str2)
{
    size_t len1 = u32strlen(str1);
    size_t len2 = u32strlen(str2);
    for(size_t i=0;i<len1;i++) dest[i] = str1[i];
    for(size_t i=0;i<len2;i++) dest[len1+i] = str2[i];
    dest[len1+len2] = 0;
}

static lexicon_error
copy_lexical_items(lexicon* dest, lexicon* src)
{
    lexicon_error err = LEXICON_NORMAL;
    for(size_t i=0;i<src->capacity;i++)
    {
        if(src->table[i] == NULL) continue;
        lexicon_add(dest, src->table[i]->key, src->table[i]->count, &err);
    }
    return err;
}

#define BUFFER_SZ_TEMP_LEXICON_SEGMENT 100

static void 
lexhnd_cycle(lhcomponents* comps,uint64_t cycle,uint64_t n_new_words)
{
    // join neighboring segments add them to temporary lexicon
    minseg** parses = comps->cycles[cycle-1].parses;
    lexicon_error lex_err;
    lexicon* temp_lex = lexicon_create();
    if(temp_lex == NULL) {} // handle malloc error

    for(size_t i=0;i<comps->corpus_sz;i++)
    {
        for(size_t j=0;j<parses[i]->size;j+=2)
        {
            char32_t buff[BUFFER_SZ_TEMP_LEXICON_SEGMENT]; // Buffer to avoid malloc, test perf
            if(j==parses[i]->size-1) 
            {
                lexicon_add(temp_lex, parses[i]->segments[j], 1,&lex_err);
                continue;
            } 
            u32strjoin(buff, parses[i]->segments[j], parses[i]->segments[j+1]);
            lexicon_add(temp_lex, buff,1, &lex_err); 
        }
    }

    // add n most common of temp_lexicon to new_lexicon
    litem** new_items = malloc(temp_lex->occupancy * sizeof(litem*));
    if(new_items == NULL) {} // handle malloc error

    lexicon* new_lexicon = lexicon_create();
    if(new_lexicon == NULL) {} // handle malloc error

    lexicon_get_items(temp_lex,new_items);
    uint64_t n_words = n_new_words < temp_lex->occupancy ? n_new_words : temp_lex->occupancy;
    
    for(size_t i=0;i<n_words;i++)
    {
        lexicon_add(new_lexicon, new_items[i]->key, new_items[i]->count, &lex_err);
    }
    free(new_items);
    lexicon_free(temp_lex);
    
    // Add old lexicons items into new lexicon
    lex_err = copy_lexical_items(new_lexicon, comps->cycles[cycle-1].lex);

    // minseg corpus with new lexicon
    minseg_error ms_err;
    minseg** temp_parses = malloc(comps->corpus_sz * sizeof(minseg*));
    for(size_t i=0;i<comps->corpus_sz;i++)
    {
        minseg* m = minseg_create(new_lexicon,comps->corpus[i],&ms_err);
        temp_parses[i] = m;
    }

    // produce new lexicon with segments from minseg (cycle->lex), prior
    comps->cycles[cycle].lex = lexicon_create();

    for(size_t i=0;i<comps->corpus_sz;i++)
    {
        for(size_t j=0;j<temp_parses[i]->size;j++)
        {
            lexicon_add(comps->cycles[cycle].lex,temp_parses[i]->segments[j],1,&lex_err);
        }
        //minseg_free(temp_parses[i]);
    }
    lexicon_free(new_lexicon);
    free(temp_parses);

    lherror err;
    comps->cycles[cycle].prior_length = 
        lexhnd_get_lexicon_bitlength(comps->alphabet, &comps->cycles[cycle],&err);

    // minseg -> posterior
    comps->cycles[cycle].parses = malloc(comps->corpus_sz * sizeof(minseg*));
    for(size_t i=0;i<comps->corpus_sz;i++)
    {
        parses[i] = minseg_create(comps->cycles[cycle].lex,comps->corpus[i],&ms_err);
        comps->cycles[cycle].posterior_length += parses[i]->cost;
    }
}

lhcomponents* lexhnd_run(
        char32_t** corpus, 
        size_t corpus_size, 
        uint8_t iterations, 
        uint8_t n_new_words, 
        lherror* error, 
        uint8_t* error_code
        )
{
    *error = LEXHND_NORMAL;
    lhcomponents* comps = malloc(sizeof(lhcomponents));
    if(comps == NULL) goto error_exit;

    comps->n_cycles = iterations;
    comps->corpus_sz = corpus_size;
    comps->corpus = corpus;

    comps->cycles = calloc(iterations, sizeof(lhcycle));
    if(comps->cycles == NULL) goto error_exit;
    for(size_t i=0;i<iterations;i++)
    {
        comps->cycles[i].lex = NULL;
        comps->cycles[i].parses = NULL;
    }

    comps->alphabet = alphabet_create(error);
    if(*error) goto error_exit;

    alphabet_setup(comps->alphabet,corpus,corpus_size,error);
    if(*error) goto error_exit;

    lexhnd_create_first_cycle(comps, corpus,corpus_size, error, error_code);
    if(*error) goto error_exit;

    for(uint8_t i=1;i<iterations;i++)
    {
        lexhnd_cycle(comps,i,n_new_words);    
        printf("prior = %lf , posterior = %lf\n", comps->cycles[i].prior_length, comps->cycles[i].posterior_length);
    }
    
    return comps;
error_exit: 
    return NULL;
}

void lexhnd_free(lhcomponents* lh)
{
    for(size_t i=0;i<lh->n_cycles;i++)
    {
        if(lh->cycles[i].lex != NULL) lexicon_free(lh->cycles[i].lex);
        if(lh->cycles[i].parses != NULL) 
            for(size_t j=0;j<lh->corpus_sz;j++) minseg_free(lh->cycles[i].parses[j]);
    }
    free(lh->cycles);
    free(lh);
    lh = NULL;
}
