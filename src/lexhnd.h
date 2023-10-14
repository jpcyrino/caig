#ifndef __LEXHND_H__
#define __LEXHND_H__

#include <stdlib.h>
#include "lexicon.h"

typedef enum 
lexhnd_error
{
    LEXHND_NORMAL,
    LEXHND_ERROR_MEMORY_ALLOCATION,
    LEXHND_ERROR_ALPHABET_FULL,
    LEXHND_ERROR_CHAR_NOT_FOUND,
    LEXHND_ERROR_LEXICON_ERROR,
    LEXHND_ERROR_MINSEG_ERROR
} lherror;

typedef struct 
lexhnd_components lhcomponents;
typedef struct 
lexhnd_alphabet lhalphabet;
typedef struct 
lexhnd_cycle lhcycle;

struct 
lexhnd_cycle
{
    lexicon* lex;
    double prior_length;
    double posterior_length;   
};

struct 
lexhnd_alphabet
{
    char32_t* alphabet;
    size_t alphabet_sz;
    uint64_t* char_counts;
};

struct 
lexhnd_components
{
    char32_t** corpus;
    size_t corpus_sz;
    lhcycle* cycles;
    uint8_t n_cycles;
    lhalphabet* alphabet;    
};


lhcomponents* 
lexhnd_run(
        char32_t** corpus, 
        size_t corpus_size, 
        uint8_t iterations, 
        uint8_t n_new_words,
        lherror* error,
        uint8_t* error_code
        );

void 
lexhnd_free(lhcomponents* lh);

#endif
