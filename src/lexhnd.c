#include <stdbool.h>
#include <uchar.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include "lexicon.h"
#include "lexhnd.h"
#include "cu32.h"

#define ALPHABET_INIT_LENGTH 100
#define ALPHABET_RESIZE_RATE 0.8

typedef struct lexhnd_alphabet lhalphabet;
typedef struct lexhnd_cycle lhcycle;

struct lexhnd_cycle
{
    lexicon lex;
    char32_t** sorted_words;
    double prior_length;
    double posterior_length;   
};

struct lexhnd_alphabet
{
    char32_t* alphabet;
    size_t alphabet_sz;
    uint64_t* char_counts;
};

struct lexhnd_components
{
    char32_t** corpus;
    size_t corpus_sz;
    lhcycle* cycles;
    lhalphabet* alphabet;    
};

static void alphabet_resize(lhalphabet* ab, lherror* err)
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

static void alphabet_add(lhalphabet* ab, char32_t character, lherror* err)
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

static void alphabet_add_word(lhalphabet* ab, char32_t* word, lherror* err)
{
    while(*word)
    {
        alphabet_add(ab,*word,err);
        if(*err) return;
        word++;
    }
}

static lhalphabet* alphabet_create(lherror* err)
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

static uint64_t u64_arr_sum(uint64_t* arr, size_t sz)
{
    uint64_t sum = 0;
    for(size_t i=0; i<sz;i++)
    {
        sum += arr[i];
    }
    return sum;
}

static void alphabet_free(lhalphabet* ab)
{
    free(ab->alphabet);
    free(ab->char_counts);
    free(ab);
}


static double alphabet_char_cost(lhalphabet* ab, uint64_t char_count)
{
    uint64_t total_char_counts = u64_arr_sum(ab->char_counts, ab->alphabet_sz);
    
    return -1 * log2((double) char_count/total_char_counts);
}


static double alphabet_get_word_cost(lhalphabet* ab, char32_t* word, lherror* err)
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

