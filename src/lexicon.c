#include <stdint.h>
#include <uchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "cu32.h"
#include "lexicon.h"

#define HASH_MAGIC_NUMBER 5381

static size_t 
hash(const char32_t* key)
{
    unsigned long int hsh = HASH_MAGIC_NUMBER;
    unsigned int c;
    while((c = *key++))
    {
        hsh = ((hsh << 5) * hsh) + c;
    }
    return (size_t) hsh;
} 

lexicon* 
lexicon_create()
{
    lexicon* lex = malloc(sizeof(lexicon));
    if(lex == NULL) goto exit1;

    lex->capacity = LEXICON_INITIAL_CAPACITY; 
    lex->occupancy = 0;
    lex->total_counts = 0;
    lex->table = malloc(sizeof(litem*) * LEXICON_INITIAL_CAPACITY);
    if(lex->table == NULL) goto exit2; 

    for(int i=0;i<lex->capacity;i++) lex->table[i] = NULL;
    return lex;

    
exit2:
    free(lex->table);
exit1:
    free(lex);
    return NULL;
}

void 
lexicon_free(lexicon* lexicon)
{
    for(int i=0; i<lexicon->capacity; i++)
    {
        if(lexicon->table[i] == NULL) continue;
        free(lexicon->table[i]->key); lexicon->table[i]->key = NULL;
        free(lexicon->table[i]); lexicon->table[i] = NULL;
    }
    free(lexicon);
}

static int8_t
u32strcmp(const char32_t* str1, const char32_t* str2)
{

    size_t len = u32strlen(str1);
    if(len != u32strlen(str2)) return -1;
    for(size_t i=0;i<len;i++)
    {
        if(str1[i] != str2[i]) return -1;
    }
    return 0;
}

uint64_t 
lexicon_get_count(lexicon* lexicon, const char32_t* word)
{
    size_t hsh = hash(word) % lexicon->capacity;
    if(lexicon->table[hsh] == NULL) return 0;
    size_t i = hsh;
    while(1)
    {
        if(u32strcmp(word, lexicon->table[i]->key) == 0) 
            return lexicon->table[i]->count;
        i++;

        if(i==hsh) break;
        if(i >= lexicon->capacity) i = 0;
        if(lexicon->table[i] == NULL) break;
    }    
    return 0;
}


static void 
create_item(const char32_t* word, size_t count, litem* item)
{
    item->key = calloc((u32strlen(word) + 1), sizeof(char32_t) );
    if(item->key == NULL) abort();
    u32strcpy(item->key, word);
    item->count = count;
}

static void
add_item(litem** items, size_t* occupancy, size_t capacity, const char32_t* word, size_t count)
{
    size_t hsh = hash(word) % capacity;
    size_t slot = hsh;

    while(1)
    {
        if(slot >= capacity) slot = 0;

        if(items[slot] == NULL) 
        {
            litem* item = malloc(sizeof(litem));
            if(item == NULL) abort();
            create_item(word,count, item);
            items[slot] = item;
            *occupancy += 1;
            return;
        }

        if(u32strcmp(items[slot]->key, word) == 0)
        {
            items[slot]->count += count;         
            return;
        }
        slot++;
        if(slot == hsh) return;
    }   
}

static void 
rehash(lexicon* lexicon)
{
    size_t new_capacity = 2*lexicon->capacity;
    litem** new_list = (litem**) malloc(new_capacity * sizeof(litem*));
    if(new_list == NULL) abort();

    for(size_t i=0;i<new_capacity;i++) new_list[i] = NULL;

    // Copy existing items into new list
    for(size_t i=0;i<lexicon->capacity;i++)
    {
        if(lexicon->table[i] == NULL) continue;

        size_t slot = hash(lexicon->table[i]->key) % new_capacity;
        while(1)
        {
            if(new_list[slot] == NULL) 
            {
                new_list[slot] = lexicon->table[i];
                lexicon->table[i] = NULL;
                break;
            }

            slot++;
            if(slot >= new_capacity) slot = 0;          
        }
    }

    // Replace table
    free(lexicon->table);
    lexicon->table = new_list;
    lexicon->capacity = new_capacity;
}



void 
lexicon_add(lexicon* lexicon, const char32_t* word, size_t count)
{ 
    add_item(lexicon->table,&lexicon->occupancy,lexicon->capacity,word, count);
   
    lexicon->total_counts += count;
    if((float) lexicon->occupancy/lexicon->capacity >= LEXICON_LOAD_FACTOR) 
    {
       rehash(lexicon);
    }
}


void
lexicon_populate_from_wordlist_file(lexicon* lexicon, const char* filename)
{
    FILE *fptr = fopen(filename,"r");
    if(fptr == NULL) return;

    char buffer[80];
    while(fgets(buffer,80,fptr))
    {

        buffer[strcspn(buffer,"\n")] = 0;
        char32_t* word = malloc((u8strlen(buffer) + 1) * sizeof(char32_t));
        if(word == NULL) abort(); 

        u8to32(buffer,word);
        lexicon_add(lexicon,word,1);
        free(word);
    }
    fclose(fptr);
}

static
int compare_item_freqs(const void* item_a, const void* item_b)
{
    uint64_t a = (*((litem**) item_a))->count;
    uint64_t b = (*((litem**) item_b))->count;

    return (a < b) - (a > b);
}

void 
lexicon_get_items(lexicon* lexicon, litem** lex_items)
{
    // populate items array
    size_t li = 0;
    for(size_t i=0;i<lexicon->capacity;i++)
    {
        if(lexicon->table[i] != NULL)
        {   
            lex_items[li] = lexicon->table[i];
            li++;
        }
    }

    // sort
    qsort(lex_items,lexicon->occupancy,sizeof(litem*),compare_item_freqs);
}
