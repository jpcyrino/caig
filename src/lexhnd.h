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
    LEXHND_ERROR_CHAR_NOT_FOUND
} lherror;

typedef struct 
lexhnd_components lhcomponents;


lhcomponents* 
lexhnd_run(char32_t** corpus, size_t corpus_size, uint8_t iterations, uint8_t n_new_words);


#endif
