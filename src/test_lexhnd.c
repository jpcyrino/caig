#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lexhnd.h"
#include "cu32.h"

#define CORPUS_SIZE 710813


int main()
{
    
    clock_t tot_s = clock(), corpus_s = clock();
    FILE* fptr = fopen("./test_res/wordlist.txt","r");
    if(fptr == NULL) 
    { 
        printf("File error!\n");
        return -1;
    }

    
    char buffer[80];
    char32_t** corpus = malloc(CORPUS_SIZE * sizeof(char32_t*)); 
    
    size_t i = 0;
    while(fgets(buffer,79,fptr))
    {
        buffer[strcspn(buffer,"\n")] = '\0'; 
        char32_t* word = calloc(u8strlen(buffer) + 1, sizeof(char32_t));
        if(word == NULL)
        {
            printf("Word allocation error!\n");
            return -1;
        }
        u8to32(buffer,word);
        corpus[i] = word;
        i++;
    }
    clock_t corpus_end = clock();
    double sec = ((double) corpus_end - corpus_s) / CLOCKS_PER_SEC;
    printf("Carregou o corpus em %lf s\n", sec); 

    clock_t proc_s = clock();
    lherror err;
    uint8_t errc;
    lhcomponents* lc = lexhnd_run(corpus,CORPUS_SIZE,5,10,&err,&errc);
    if(err) 
    {
        printf("Error code %u", err);
        return -1;
    }

    clock_t proc_e = clock();
    sec = ((double) proc_e - proc_s) / CLOCKS_PER_SEC;
    printf("Processou o primeiro ciclo em %lf s\n",sec);


    printf("tamanho do lexico %llu\npriori %lf \nposteriori %lf\n", lc->cycles[0].lex->occupancy ,lc->cycles[0].prior_length, lc->cycles[0].posterior_length);
    
    clock_t tot_e = clock();
    sec = ((double) tot_e - tot_s) / CLOCKS_PER_SEC;
    printf("Tempo total decorrido %lf s\n",sec);

    lexhnd_free(lc);
    for(size_t j=0;j<CORPUS_SIZE;j++)
    {
        free(corpus[j]);
    }
    
    free(corpus);

    printf("all free\n");
}
