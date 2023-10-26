#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wchar.h>


//
// MEM POOL
//
#define DEFAULT_MEM_POOL_SZ 4096

typedef struct {
    size_t size;
    size_t pos;
    void* data;
} mem_pool;

static mem_pool* create_mem_pool()
{
    mem_pool* pool = malloc(sizeof(mem_pool*));
    pool->size = DEFAULT_MEM_POOL_SZ;
    pool->pos = 0;
    pool->data = malloc(DEFAULT_MEM_POOL_SZ);
    return pool;
}


void* mem_push(mem_pool* pool, size_t size)
{
    if(pool->pos > pool->size - 64)
    {
        pool->size *= 2;
        pool->data = realloc(pool->data, pool->size);
    }
    void* new_mem = pool->data + pool->pos;
    pool->pos += size;
    return new_mem;
}

void mem_pop(mem_pool* pool, size_t size)
{
    pool->pos -= size;
}

void mem_clear(mem_pool* pool)
{
    pool->pos = 0;
}


//
// UTF-8 OPS
//
static bool is_initial_of_utf8_seq(char ch)
{
    return (ch & 0b11000000) != 0b10000000;
}

static void utf8_next(char* str, size_t* i)
{

    (void) (is_initial_of_utf8_seq(str[++(*i)]) || 
            is_initial_of_utf8_seq(str[++(*i)]) ||
            is_initial_of_utf8_seq(str[++(*i)]) ||
            ++(*i)); 
}

static void utf8_prev(char* str, size_t* i)
{

    (void) (is_initial_of_utf8_seq(str[--(*i)]) || 
            is_initial_of_utf8_seq(str[--(*i)]) ||
            is_initial_of_utf8_seq(str[--(*i)]) ||
            --(*i)); 
}

//
// LEXICON
//

 
#define HASH_MAGIC_NUMBER 5381
#define DEFAULT_LEXICON_SIZE 1024
#define MAX_OCCUPANCY_RATE 0.7

static uint64_t 
hash(const char* key)
{
    uint64_t hsh = HASH_MAGIC_NUMBER;
    uint32_t c;
    while((c = *key++))
    {
        hsh = ((hsh << 5) * hsh) + c;
    }
    return hsh;
} 

typedef struct
{
    bool occ;
    uint64_t count;
    char* key;
} lexical_item;

typedef struct
{
    uint64_t size;
    uint64_t occupancy;
    uint64_t total_counts;
    lexical_item* items;  
    mem_pool *string_storage;
} lexicon;

static lexicon*
lexicon_create(mem_pool* string_storage)
{
    lexicon* lex = malloc(sizeof(*lex));
    lex->occupancy = 0;
    lex->total_counts = 0;
    lex->size = DEFAULT_LEXICON_SIZE;
    lex->items = malloc(DEFAULT_LEXICON_SIZE * sizeof(lexical_item));
    for(size_t i=0;i<DEFAULT_LEXICON_SIZE;i++)
    {
        lex->items[i].occ = false;
        lex->items[i].count = 0;
        lex->items[i].key = NULL;
    }
    lex->string_storage = string_storage;
    return lex;
}

static uint64_t
lexicon_get_count(lexicon*lex, char* key)
{
    uint64_t hash_code = hash(key) % lex->size;
    uint64_t hash_code_cp = hash_code;

    while(1)
    {
        if(lex->items[hash_code].occ && strcmp(lex->items[hash_code].key,key) == 0)
            return lex->items[hash_code].count;

        if(++hash_code >= lex->size) hash_code=0;
        if(hash_code == hash_code_cp) return 0;
    }
}

static bool
lexicon_add_item(lexicon* lex, char* key, uint64_t count, bool copy_key)
{

    uint64_t hash_code = hash(key) % lex->size;
    uint64_t hash_code_cp = hash_code;
    while(1)
    { 
        // When slot not occupied: add item and return t
        if(!lex->items[hash_code].occ)
        {
            if(copy_key)
            {
                lex->items[hash_code].key = 
                    (char*) mem_push(lex->string_storage,strlen(key)+1);
                strcpy(lex->items[hash_code].key,key);
            }
            else
                lex->items[hash_code].key = key;

            lex->items[hash_code].count = count;
            lex->items[hash_code].occ = true;
            lex->occupancy++;
            lex->total_counts += count;
            return true;
        }

        // When slot occupied: increment count and return t
        if(lex->items[hash_code].occ && 
                strcmp(lex->items[hash_code].key,key) == 0)
        {
            lex->items[hash_code].count += count;
            lex->total_counts += count;
            return true;
        }

        // Increment hash code or go to beginning
        if(++hash_code >= lex->size) hash_code = 0;
        // Returns f on full cycle
        if(hash_code_cp == hash_code) return false;
    }
}

static bool 
lexicon_rehash(lexicon* lex)
{
    size_t old_size = lex->size;
    lex->size *= 2;
    lex->items = realloc(lex->items,lex->size * sizeof(lexical_item));
    for(size_t i=old_size;i<lex->size;i++)
        lex->items[i].occ = false;
    for(size_t i=0;i<lex->size;i++)
    {
        if(lex->items[i].occ && i < old_size) 
        {
            lex->items[i].occ = false;
            lex->occupancy--;
            lex->total_counts -= lex->items[i].count;
            if(!lexicon_add_item(lex,lex->items[i].key,lex->items[i].count,false))
                return false;
        }
    }
    return true;
}

static bool
lexicon_insert(lexicon* lex, char* key, uint64_t count)
{
    float occupancy_rate = (float) lex->occupancy/lex->size;
    if(occupancy_rate >= MAX_OCCUPANCY_RATE && !lexicon_rehash(lex)) return false;
    return lexicon_add_item(lex,key,count,true); 
}

//
// MINSEG
//

#define MINSEG_MAX_CHARS_LENGTH 1024

static char*
minseg_forward(lexicon* lex, char*str, uint32_t* segments, double* cost)
{
    if(MINSEG_MAX_CHARS_LENGTH < strlen(str) + 1) return NULL;
    double costs[MINSEG_MAX_CHARS_LENGTH + 1];
    char* candidate_buff[MINSEG_MAX_CHARS_LENGTH];

    return NULL;
}

//
// CORPUS LOAD
//
#define WORD_LIST_WD_LEN 50

static void load_word_list(mem_pool* pool, char* filename, size_t* count)
{
    FILE* fptr = fopen(filename, "r");
    if(fptr == NULL) 
    {
        printf("File not found\n");
        return;
    }

    char buffer[WORD_LIST_WD_LEN] ;
    *count = 0;
    while(fgets(buffer,WORD_LIST_WD_LEN,fptr))
    {
        buffer[strcspn(buffer,"\r\n")] = '\0'; 
        char* dest = (char*) mem_push(pool,strlen(buffer)+1);
        strcpy(dest,buffer);
        *count += 1;
    }
    fclose(fptr);
}




int main()
{
    mem_pool* corpus = create_mem_pool();
    lexicon* le = lexicon_create(corpus);
    size_t count;
    load_word_list(corpus,"./test_res/wordlist.txt",&count);
    int i=0;
    size_t pos=0;
    char* str = (char*) corpus->data;
    while(i < count)
    {
       lexicon_insert(le,str+pos,1);
       pos += 1 + strlen(str+pos);
       i++;
    }

    
    char buff[50];
    printf("Palavra a pesquisar: ");
    scanf("%s",buff);
    uint64_t cnt = lexicon_get_count(le, buff);
    printf("Contagem da palavra %s: %llu\n", buff, cnt);
    
    return 0;
}

