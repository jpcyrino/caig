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

    lexicon_populate_from_wordlist_file(lex, "./test_res/wordlist.txt");

    char sentence[144];
    
    printf("Digite uma frase ate 144 caracteres sem espacos: ");
    fgets(sentence,144,stdin);

    clock_t start = clock();

    sentence[strcspn(sentence,"\n")] = 0;
    char32_t* sentence32 = malloc((u8strlen(sentence) + 1) * sizeof(char32_t));
    if(sentence32 == NULL) return -1; 
    u8to32(sentence,sentence32);

    minseg* res = minseg_create(lex,sentence32);
    if(res == NULL) return -1;

    clock_t end = clock();
    float sec = (float) (end-start) / CLOCKS_PER_SEC;
    printf("\nTempo decorrido: %fs\nSegmentacao encontrada: ", sec);
    for(size_t i=0;i<res->size;i++) 
    {
        char buff[100];
        u32to8(res->segments[i],buff);
        printf("%s ", buff);
    }
    printf("\nCusto = %lf", res->cost);

    free(sentence32); sentence32 = NULL;
    minseg_free(res);
    return 0;

}
    
