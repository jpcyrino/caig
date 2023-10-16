#ifndef __MINSEG_H__
#define __MINSEG_H__

#include <uchar.h>
#include <stdint.h>
#include "lexicon.h"

typedef struct minseg
{
    char32_t** segments;
    size_t size;
    double cost;
} minseg;

minseg* 
minseg_create(lexicon* lex, const char32_t* sentence);

void 
minseg_free (minseg* result);

#endif


