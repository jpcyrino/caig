#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "cu32.h"
#include "lexicon.h"
#include "minseg.h"

static void u32strncpy(char32_t* dest, const char32_t* src, size_t len)
{
    for(size_t i=0;i<len;i++)
    {
        dest[i] = src[i];
    }
    dest[len] = 0;
}

double lexicon_lookup(lexicon* lex, const char32_t* word)
{
    size_t count = lexicon_get_count(lex, word); 
    if(count == 0) return DBL_MAX;
    double prob = (double) count/lex->total_counts;

    return -1 * log2(prob);
}

int8_t minseg(lexicon* lex, const char32_t* sentence, char32_t** words)
{
    if(words == NULL) return MINSEG_ERROR_NULL_OUT_PARAMETER;
    int8_t error = 0;
    size_t sentence_length = u32strlen(sentence);

    double* costs = calloc(sentence_length+1, sizeof(double));
    char32_t* candidate_buffer = calloc(sentence_length + 1, sizeof(char32_t)); 
    char32_t* min_cost_candidate = calloc(sentence_length + 1,sizeof(char32_t));
    if(costs == NULL || candidate_buffer == NULL || min_cost_candidate == NULL) 
    {
        error = MINSEG_ERROR_MEMORY_ALLOCATION;
        goto exit;
    }
    
    for(size_t fpos=0;fpos<sentence_length;fpos++)
    {
        double min_cost = DBL_MAX;

        for(size_t ipos=0;ipos<=fpos;ipos++)
        {
            u32strncpy(candidate_buffer,(sentence + ipos),(fpos-ipos+1)); 

            double cost = costs[ipos] + lexicon_lookup(lex, candidate_buffer);


            if(cost < min_cost) 
            {
                min_cost = cost;
                memset(min_cost_candidate,0,sentence_length * 4);
                u32strcpy(min_cost_candidate,candidate_buffer);  
            }
            memset(candidate_buffer,0,sentence_length * 4);
        }
        costs[fpos+1] = min_cost;
        words[fpos] = calloc((u32strlen(min_cost_candidate) + 1),sizeof(char32_t));
        u32strcpy(words[fpos],min_cost_candidate); 
    }

exit:
    free(candidate_buffer); candidate_buffer = NULL;
    free(min_cost_candidate); min_cost_candidate = NULL;
    free(costs); costs = NULL;
    return error;
}

size_t backtrack(char32_t** words, size_t words_size,char32_t** minseg_words)
{
    int64_t pos = words_size-1;
    size_t minseg_words_sz = 0;
    size_t i = 0;

    while(pos >= 0)
    {
        size_t wordlen = u32strlen(words[pos]);
        if(minseg_words != NULL) 
        {
            minseg_words[i] = malloc((wordlen + 1) * sizeof(char32_t)); 
            if(minseg_words[i] == NULL) return MINSEG_ERROR_MEMORY_ALLOCATION;

            u32strcpy(minseg_words[i],words[pos]);
        } 
        minseg_words_sz++;
        pos = pos - wordlen;
        i++;
    }

    // Reverse result array
    if(minseg_words != NULL)
    {
        char32_t* temp;
        for(size_t i=0;i<minseg_words_sz/2;i++)
        {
            temp = minseg_words[i];
            minseg_words[i] = minseg_words[minseg_words_sz-i-1];
            minseg_words[minseg_words_sz-i-1] = temp;
        }
    }

    return minseg_words == NULL ? minseg_words_sz : 0;
}

