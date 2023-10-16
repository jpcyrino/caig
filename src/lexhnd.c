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


typedef struct lexhnd_iteration
{
    lexicon* lexicon;
    char32_t** parse;
    double prior;
    double posterior;
} iteration;

static iteration
iteration_zero(alphabet* ab, char32_t** corpus, size_t corpus_sz)
{
    iteration result;



    return result;

}

static iteration
iteration_n(alphabet*ab, char32_t**corpus, size_t corpus_sz, char32_t** past_parse)
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

    //run iteration 0
    //run other iterations
    

    return result;
}


