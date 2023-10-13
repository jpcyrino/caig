#include <stdbool.h>
#include <uchar.h>
#include <stdlib.h>
#include "lexicon.h"
#include "lexhnd.h"


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
    double* char_bitlength;
};

struct lexhnd_components
{
    char32_t** corpus;
    size_t corpus_sz;
    lhcycle* cycles;
    lhalphabet* alphabet;    
};

