#include "lexicon.h"
#include "cu32.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char* argv[])
{
    lexicon lex;
    clock_t start = clock(), diff;
    int error = lexicon_create(&lex);
    error += lexicon_populate_from_wordlist_file(&lex,"./test_res/wordlist.txt");
    if(error) return 1;

    diff = start - clock();
    int8_t msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("Palavras carregadas em: %ds e %dms\n"
           "Quantidade de palavras: %llu\n"
           "Quantidade de tokens: %llu\n", msec/1000, msec%1000, lex.occupancy, lex.total_counts);

     

    while(1)
    {
        printf("Palavra para busca (q sai): ");
        char wd[50];
        fgets(wd,50,stdin);
        wd[strcspn(wd, "\n")] = 0;
        if(strcmp(wd,"q") == 0) break;
        char32_t u32[50];
        u8to32(wd,u32);
        printf("Contagem de %s é %llu\n", wd, lexicon_get_count(&lex,u32));
    }
    lexicon_free(&lex);
    
    return 0;
}
