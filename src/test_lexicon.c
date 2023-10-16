#include "lexicon.h"
#include "cu32.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char* argv[])
{
    lexicon* lex = lexicon_create();

    clock_t start = clock();
    lexicon_populate_from_wordlist_file(lex,"./test_res/wordlist.txt");


    clock_t end = clock();
    float sec = (float) (end-start) / CLOCKS_PER_SEC;
    printf("Palavras carregadas em: %fs\n"
           "Quantidade de palavras: %llu\n"
           "Quantidade de tokens: %llu\n", sec, lex->occupancy, lex->total_counts);

     

    while(1)
    {
        printf("Palavra para busca (q sai): ");
        char wd[50];
        fgets(wd,50,stdin);
        wd[strcspn(wd, "\n")] = 0;
        if(strcmp(wd,"q") == 0) break;
        char32_t u32[50];
        u8to32(wd,u32);
        printf("Contagem de %s é %llu\n", wd, lexicon_get_count(lex,u32));
    }

    size_t lex_sz = lex->occupancy;
    litem** li = malloc(lex_sz * sizeof(litem*));
    lexicon_get_items(lex, li);

    char buff[80];
    for(size_t i=0;i<lex_sz;i++)
    {
        u32to8(li[i]->key,buff);
        printf("%s - %llu\n", buff, li[i]->count);
    }

    free(li);
    lexicon_free(lex); 
    return 0;
}
