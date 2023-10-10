#ifndef __MINSEG_H__
#define __MINSEG_H__

#include <uchar.h>
#include <stdint.h>
#include "lexicon.h"

typedef enum minseg_error
{
    MINSEG_NORMAL,
    MINSEG_MEMORY_ALLOCATION_ERROR,
    MINSEG_LEXICON_ERROR,
} minseg_error;

typedef struct minseg
{
    char32_t** segments;
    size_t size;
} minseg;

minseg* 
minseg_create(lexicon* lex, const char32_t* sentence, minseg_error* error);

void 
minseg_free (minseg* result);

#endif


