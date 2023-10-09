#ifndef __MINSEG_H__
#define __MINSEG_H__

#include <uchar.h>
#include <stdint.h>
#include "lexicon.h"

#define MINSEG_ERROR_MEMORY_ALLOCATION 1
#define MINSEG_ERROR_NULL_OUT_PARAMETER 2

double lexicon_lookup(lexicon* lex, const char32_t* word);
int8_t minseg(lexicon* lex, const char32_t* sentence, char32_t** words);

// Se minseg_words for NULL, retorna o comprimento que o vetor
// minseg_words devera ter. Caso contrario, retorna codigo de erro 
// de alocação de memória. 
size_t backtrack(char32_t** words, size_t words_size,char32_t** minseg_words);

#endif


