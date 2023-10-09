#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "minseg.h"
#include "lexicon.h"
#include "cu32.h"

int main()
{
    lexicon lex; 
    int error = lexicon_create(&lex);
    error += lexicon_populate_from_wordlist_file(&lex, "./test_res/wordlist.txt");

    if(error) { printf("Error on creating lexicon\n"); return -1; }

    char sentence[144];
    
    printf("Digite uma frase ate 144 caracteres sem espacos: ");
    fgets(sentence,144,stdin);

    clock_t start = clock(), diff;

    sentence[strcspn(sentence,"\n")] = 0;



    char32_t* sentence32 = malloc((u8strlen(sentence) + 1) * sizeof(char32_t));
    u8to32(sentence,sentence32);

    char wbuffer[140];
    memset(wbuffer,0,140);
    char32_t** words = malloc(u8strlen(sentence) * sizeof(char32_t*));
    error += minseg(&lex, sentence32, words);
    if(error) { printf("Minseg fault %d\n", error); }

    size_t words_len = u32strlen(sentence32);

    for(size_t i=0;i<words_len;i++)
    {
        u32to8(words[i],wbuffer);
        printf("%llu - %s\n", i, wbuffer);
    }
    
    size_t minseg_len = backtrack(words,words_len,NULL);

    char32_t** minseg_res = malloc(minseg_len * sizeof(char32_t*));

    error += backtrack(words,words_len,minseg_res);
    if(error) { printf("Backtrack allocation fault %d\n", error); return -1; }

    for(int i=0;i<minseg_len;i++) 
    {
        u32to8(minseg_res[i],wbuffer);
        printf("%s ", wbuffer);
    }

    diff = start - clock();
    int8_t msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("\nTempo decorrido: %ds %dms", msec/1000, msec%1000);

    free(sentence32); sentence32 = NULL;
    for(size_t i=0;i<words_len;i++) free(words[i]);
    free(words); words = NULL;
    for(size_t i=0;i<minseg_len;i++) free(minseg_res[i]);
    free(minseg_res); minseg_res = NULL;
    return 0;

}
    
