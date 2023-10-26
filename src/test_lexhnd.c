#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include "lexhnd.h"
#include "cu32.h"

#define CORPUS_SIZE 710813
#define WORD_SZ 80

int main(int argc, char* argv[])
{
    int32_t n_of_n_words = argc > 1 ? strtol(argv[1],NULL,10) : 25; 
    printf("Analisando com %d novos tokens\n", n_of_n_words);
    
    clock_t corpus_s = clock();
    FILE* fptr = fopen("./test_res/wordlist.txt","r");
    if(fptr == NULL) 
    { 
        printf("File error!\n");
        return -1;
    }

     
    char buffer[WORD_SZ] = {'\0'};
    char32_t** corpus = calloc(CORPUS_SIZE,sizeof(char32_t*)); 

    size_t i = 0;
    while(fgets(buffer,WORD_SZ-1,fptr))
    {
        buffer[strcspn(buffer,"\n")] = '\0'; 
       char32_t* word = calloc(WORD_SZ, sizeof(char32_t));
        
        if(word == NULL)
        {
            printf("Word allocation error!\n");
            return -1;
        } 


        u8to32(buffer,word);
        corpus[i] = word;
        i++;
    }

    fclose(fptr);

    
    clock_t corpus_end = clock();
    double sec = ((double) corpus_end - corpus_s) / CLOCKS_PER_SEC;
    printf("Carregou o corpus em %lf s\n", sec); 

    clock_t proc_s = clock();
    lexhnd_result* res = lexhnd_run(corpus,i,15,n_of_n_words);
    clock_t proc_e = clock();


    sec = (double) (proc_e - proc_s) / CLOCKS_PER_SEC;
    printf("Iteracoes em %lfs\n", sec);

    double old_h = 0;
    for(int i=0;i<15;i++)
    {
        double h = res->priors[i] + res->posteriors[i];
        printf("%3d pri %20lf pos %20lf h %20lf delta %20lf\n", 
                i,
                res->priors[i],
                res->posteriors[i],
                h,
                i == 0? h : h - old_h
              );
        old_h = h;
    }
    

    for(size_t j=0;j<CORPUS_SIZE;j++)
    {
        free(corpus[j]);
    }
    
    free(corpus);

    
}
