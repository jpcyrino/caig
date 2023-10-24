#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "lexhnd.h"
#include "cu32.h"

#define CORPUS_SIZE 710813
#define WORD_SZ 80

int main()
{
    
    clock_t tot_s = clock(), corpus_s = clock();
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

    clock_t corpus_end = clock();
    double sec = ((double) corpus_end - corpus_s) / CLOCKS_PER_SEC;
    printf("Carregou o corpus em %lf s\n", sec); 

    clock_t proc_s = clock();
    lexhnd_result* res = lexhnd_run(corpus,i,1,10);
    clock_t proc_e = clock();


    sec = (double) (proc_e - proc_s) / CLOCKS_PER_SEC;
    printf("Primeira iteração em %lfs\n", sec);
    

    



    

    
    for(size_t j=0;j<CORPUS_SIZE;j++)
    {
        free(corpus[j]);
    }
    
    free(corpus);

    
}
