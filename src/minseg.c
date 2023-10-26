#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "cu32.h"
#include "lexicon.h"
#include "minseg.h"

#define SENTENCE_LENGTH 200

static double 
lexicon_lookup(lexicon* lex, const char32_t* word)
{
    size_t count = lexicon_get_count(lex, word); 
    if(count == 0) return DBL_MAX;
    double prob = (double) count/lex->total_counts;

    return -1 * log2(prob);
}

static void
forward_step(lexicon* lex, const char32_t* sentence, char32_t** words, double* parse_cost)
{
    size_t sentence_length = u32strlen(sentence);
    
    double costs[200];
    char32_t candidate_buffer[200]; 
    char32_t min_cost_candidate[200];

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
        *parse_cost = min_cost;

        words[fpos] = calloc((u32strlen(min_cost_candidate) + 1),sizeof(char32_t));
        if(words[fpos] == NULL) abort();

        u32strcpy(words[fpos],min_cost_candidate); 
    }
   
}

static void 
backtrack(char32_t** words, size_t words_size,char32_t** minseg_words, size_t* minseg_words_size)
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
            if(minseg_words[i] == NULL)  abort();
            u32strcpy(minseg_words[i],words[pos]);
        } 
        minseg_words_sz++;
        pos = pos - wordlen;
        i++;
    }


    // Reverse result array
    if(minseg_words == NULL) 
    {
        *minseg_words_size = minseg_words_sz;
        return;
    }
    
    char32_t* temp;
    for(size_t i=0;i<minseg_words_sz/2;i++)
    {
        temp = minseg_words[i];
        minseg_words[i] = minseg_words[minseg_words_sz-i-1];
        minseg_words[minseg_words_sz-i-1] = temp;
    }
}


minseg* 
minseg_create(lexicon* lex, const char32_t* sentence)
{
    minseg* result = NULL;
   
    size_t chosen_words_sz = u32strlen(sentence);
    char32_t** chosen_words = calloc(chosen_words_sz, sizeof(char32_t*));
    if(chosen_words == NULL) abort();

    double cost = 0;
    forward_step(lex,sentence,chosen_words, &cost);

    size_t segments_sz = 0;
    backtrack(chosen_words, chosen_words_sz, NULL, &segments_sz);

    result = malloc(sizeof(minseg)); if(result == NULL) abort();
    result->segments = malloc(segments_sz * sizeof(char32_t*));
    if(result->segments == NULL) abort();

    backtrack(chosen_words, chosen_words_sz, result->segments, &segments_sz);
    result->size = segments_sz;
    result->cost = cost;
    
    return result;

}


void 
minseg_free(minseg* result)
{
    for(size_t i=0;i<result->size;i++) free(result->segments[i]);
    free(result->segments);
    free(result);
}
