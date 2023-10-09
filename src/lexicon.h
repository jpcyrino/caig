/* LEXICON
 * Estrutura de dados para representar os tokens únicos de um
 * corpus e sua contagem.
 *
 * Testado em GCC 13.2, C11
 * Autor: João Paulo Lazzarini Cyrino
 * Data: 06/10/2023
 */

#ifndef __LEXICON_H__
#define __LEXICON_H__

#include <uchar.h>

// Erros:
// ERROR_LEXICON_MEMORY_ALLOCATION - Malloc retornou ponteiro nulo.
// ERROR_LEXICON_OUT_OF_SPACE - Tabela sem espaço. Provavelmente não foi possível 
// aumentar a capacidade com o rehash automático. 
#define ERROR_LEXICON_MEMORY_ALLOCATION 1
#define ERROR_LEXICON_OUT_OF_SPACE 2 


#define LEXICON_INITIAL_CAPACITY 8000
#define LEXICON_LOAD_FACTOR 0.70

typedef struct litem
{
    char32_t* key;
    size_t count;
} litem;

typedef struct lexicon 
{
    struct litem** table;   
    size_t total_counts;
    size_t capacity;
    size_t occupancy;
} lexicon;

int lexicon_create(lexicon* lexicon);
void lexicon_free(lexicon* lexicon);

int lexicon_add(lexicon* lexicon, const char32_t* word);
int lexicon_populate_from_wordlist_file(lexicon* lexicon, const char* filename);
size_t lexicon_get_count(lexicon* lexicon, const char32_t* word);

#endif
