#ifndef __LEXHND_H__
#define __LEXHND_H__

#include <stdlib.h>
#include "lexicon.h"
#include "minseg.h"


typedef struct lexhnd_result
{
    lexicon** lexicons;
    double* priors;
    double* posteriors;
} lexhnd_result;

lexhnd_result* 
lexhnd_run(
        char32_t** corpus, 
        size_t corpus_size, 
        uint8_t n_iterations, 
        uint8_t n_new_words
        ); 

#endif
