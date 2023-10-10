#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "minseg.h"
#include "lexicon.h"
#include "cu32.h"

int main()
{
    lexicon* lex = lexicon_create();
    if(lex == NULL) return -1;

    lexicon_error lerr;
    lexicon_populate_from_wordlist_file(lex, "./test_res/wordlist.txt", &lerr);
    if(lerr) { printf("Error on creating lexicon\n"); return -1; }

    char sentence[144];
    
    printf("Digite uma frase ate 144 caracteres sem espacos: ");
    fgets(sentence,144,stdin);

    clock_t start = clock(), diff;

    sentence[strcspn(sentence,"\n")] = 0;
    char32_t* sentence32 = malloc((u8strlen(sentence) + 1) * sizeof(char32_t));
    if(sentence32 == NULL) return -1; 
    u8to32(sentence,sentence32);

    minseg_error merr = 0;
    minseg* res = minseg_create(lex,sentence32,&merr);
    if(res == NULL) return -1;

    diff = start - clock();
    int8_t msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("\nTempo decorrido: %ds %dms\nSegmentacao encontrada: ", msec/1000, msec%1000);
    for(size_t i=0;i<res->size;i++) 
    {
        char buff[100];
        u32to8(res->segments[i],buff);
        printf("%s ", buff);
    }

    free(sentence32); sentence32 = NULL;
    minseg_free(res);
    return 0;

}
    
