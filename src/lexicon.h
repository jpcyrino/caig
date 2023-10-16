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
    uint64_t total_counts;
    uint64_t capacity;
    uint64_t occupancy;
} lexicon;

lexicon* 
lexicon_create();

void 
lexicon_free(lexicon* lexicon);

void 
lexicon_add(lexicon* lexicon, const char32_t* word, size_t count);

void 
lexicon_populate_from_wordlist_file(lexicon* lexicon, const char* filename);

void 
lexicon_get_items(lexicon* lexicon, litem** lex_items);

uint64_t 
lexicon_get_count(lexicon* lexicon, const char32_t* word);

#endif
