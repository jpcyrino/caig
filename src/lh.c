#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <wchar.h>
#include <float.h>
#include <math.h>


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
    if(size >= (pool->size - pool->pos))
    {
        pool->size += size;
        pool->data = realloc(pool->data, pool->size);

    }
    if( pool->pos > pool->size - 64)
    {
        pool->size *= 2;
        pool->data = realloc(pool->data, pool->size);
    }
    void* new_mem = (void*) ((char*) pool->data + pool->pos);
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
#define DEFAULT_LEXICON_ITEM_SIZE 64
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
    char key[DEFAULT_LEXICON_ITEM_SIZE];
} lexical_item;

typedef struct
{
    uint64_t size;
    uint64_t occupancy;
    uint64_t total_counts;
    lexical_item* items;
} lexicon;

static lexicon*
lexicon_create()
{
    lexicon* lex = malloc(sizeof(lexicon));
    lex->occupancy = 0;
    lex->total_counts = 0;
    lex->size = DEFAULT_LEXICON_SIZE;
    lex->items = malloc(DEFAULT_LEXICON_SIZE * sizeof(lexical_item));
      
    for(size_t i=0;i<DEFAULT_LEXICON_SIZE;i++)
    {
        lex->items[i].occ = false;
        lex->items[i].count = 0;
        memset(lex->items[i].key,0,DEFAULT_LEXICON_ITEM_SIZE);
    }
   
    return lex;
}

static void
lexicon_release(lexicon* lex)
{
    free(lex->items);
    free(lex);
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

static double
lexicon_get_cost(lexicon* lex, char* key)
{
    uint64_t count = lexicon_get_count(lex,key);
    if(!count) return DBL_MAX;
    return log2((double) count/lex->total_counts) * -1;
}

static bool
lexicon_add_item(lexicon* lex, char* key, uint64_t count)
{

    uint64_t hash_code = hash(key) % lex->size;
    uint64_t hash_code_cp = hash_code;
    while(1)
    { 
        // When slot not occupied: add item and return t
        if(!lex->items[hash_code].occ)
        {
            strcpy(lex->items[hash_code].key,key);

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
            if(!lexicon_add_item(lex,lex->items[i].key,lex->items[i].count))
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
    return lexicon_add_item(lex,key,count); 
}

static char**
lexicon_get_keys(lexicon* lex, mem_pool* storage, size_t* out_size)
{
    *out_size = lex->occupancy * sizeof(char*);
    char** arr = (char**) mem_push(storage,*out_size);
    size_t pos = 0;
    for(size_t i=0;i<lex->size;i++)
    {
        if(lex->items[i].occ) 
        {
            arr[pos] = lex->items[i].key;
            pos++;
        }
    }
    return arr;
}

//
// MINSEG
//

static char* str_stack_next(char* str)
{
    return str + (strlen(str) + 1);
}

static char* str_stack_prev(char* str)
{
    str--;
    while(*(str - 1)) str--;
    return str;
}

#define MINSEG_DEFAULT_BUFFER_SZ 1024

static char*
minseg_forward(lexicon* lex, mem_pool* str_storage, char*str, 
        uint32_t* n_segments, double* cost, size_t* allocated_mem)
{
    *n_segments = 0;
    *cost = 0;
    *allocated_mem = 0;


    char* segment = NULL;
    size_t len = strlen(str);
    if(MINSEG_DEFAULT_BUFFER_SZ < len + 1) return NULL;
    double costs[MINSEG_DEFAULT_BUFFER_SZ + 1] = {0};
    char candidate_buff[MINSEG_DEFAULT_BUFFER_SZ];
    char min_cost_candidate[MINSEG_DEFAULT_BUFFER_SZ];

    size_t i = 0, j = 0;
    while(i < len)
    {
        double min_cost = DBL_MAX;
        j=0;
        while(j <= i)
        {
            memset(candidate_buff,0,MINSEG_DEFAULT_BUFFER_SZ);
            strncpy(candidate_buff,str+j,i-j+1);
            double cost  = costs[j] + lexicon_get_cost(lex, candidate_buff);
            
            if(cost < min_cost)
            {
                min_cost = cost;
                memset(min_cost_candidate,0,MINSEG_DEFAULT_BUFFER_SZ);
                strcpy(min_cost_candidate,candidate_buff);
            }

            j++;
        }
        costs[i+1] = min_cost;
        *cost = min_cost;

        segment = (char*) mem_push(str_storage,(strlen(min_cost_candidate)+1) * sizeof(char));
        *allocated_mem += (strlen(min_cost_candidate)+1) *sizeof(char);
        strcpy(segment,min_cost_candidate);
        *n_segments += 1;
      
        i++;
    }
    return segment;
}

static char*
minseg_backward(lexicon* lex, mem_pool* storage, char* segment, uint32_t* n_segments, size_t* allocated_mem)
{
    char* segment_pointers[MINSEG_DEFAULT_BUFFER_SZ]; // This buffer can be shorter

    int64_t segment_pointers_index=0;
    int64_t n_segment = (int64_t) *n_segments;
    while(n_segment>0)
    {
        segment_pointers[segment_pointers_index] = segment;
        size_t len = strlen(segment);
        for(size_t j=0;j<len;j++)
        {
            segment = str_stack_prev(segment);
            n_segment--;
        }
        if(n_segment > 0) segment_pointers_index++;
    } 
    
    // Release memory allocated by minseg_forward
    mem_pop(storage, *allocated_mem);

    // Rewrite at the beggining of memory previously allocated by minseg_forward

    *allocated_mem = strlen(segment_pointers[segment_pointers_index]) + 1;
    char* init_of_segments = (char*) mem_push(storage, (*allocated_mem) * sizeof(char));

    strcpy(init_of_segments,segment_pointers[segment_pointers_index]);
    segment_pointers_index--;
    *n_segments = 1;
    
    while(segment_pointers_index >= 0)
    {
        size_t mem_size = strlen(segment_pointers[segment_pointers_index]) + 1;
        char* next_segment = (char*) mem_push(storage, mem_size*sizeof(char));
        *allocated_mem += mem_size;
        strcpy(next_segment,segment_pointers[segment_pointers_index]);
        segment_pointers_index--;
        *n_segments+=1;
    }

    return init_of_segments;    
}

char* 
minseg(char* sentence, lexicon* lex, mem_pool* storage, uint32_t* out_n_segments, size_t* out_mem_size, double* out_cost)
{
    char* segs = minseg_forward(lex,storage,sentence,out_n_segments,out_cost,out_mem_size);
    segs = minseg_backward(lex,storage,segs,out_n_segments,out_mem_size);
    return segs;
}


//
// LEXHOUND
//

#define ALPHABET_DEFAULT_SIZE 64

typedef struct
{
    uint64_t total_counts;
    char letters[ALPHABET_DEFAULT_SIZE];
    uint32_t counts[ALPHABET_DEFAULT_SIZE];
} alphabet;


static void
alphabet_add(alphabet* ab, char ch)
{
    for(size_t i=0;i<ALPHABET_DEFAULT_SIZE;i++)
    {
        if(!ab->letters[i])
        {
            ab->letters[i] = ch;
            ab->counts[i] = 1;
            ab->total_counts += 1;
            return;
        }

        if(ab->letters[i] == ch)
        {
            ab->counts[i] += 1;
            ab->total_counts += 1;
            return;
        }
    }

}

static alphabet*
alphabet_create(mem_pool* storage, char* corpus, size_t corpus_sz)
{
    alphabet* ab = (alphabet*) mem_push(storage, sizeof(alphabet));
    ab->total_counts = 0;
    memset(ab->letters, 0,ALPHABET_DEFAULT_SIZE);
    memset(ab->counts, 0, ALPHABET_DEFAULT_SIZE);
    size_t pos = 0;
    while(pos < corpus_sz)
    {
        char* wd = corpus + pos;
        size_t len = strlen(wd);
        
        for(size_t i=0;i<len;i++)
        {
            alphabet_add(ab,wd[i]);
        }
        pos += 1 + len;
    }
    return ab;
}

static void
lexhound_iteration_zero()
{
    
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
    mem_pool* str_storage = create_mem_pool();
    //lexicon* le = lexicon_create(str_storage); 
    lexicon* le = lexicon_create();
    size_t count;
    load_word_list(corpus,"./test_res/wordlist.txt",&count);
    int i=0;
    size_t pos=0;
    char* str = (char*) corpus->data;
    
    alphabet* ab = alphabet_create(corpus, str, count);
    while(i < count)
    {
       lexicon_insert(le,str+pos,1);
       pos += 1 + strlen(str+pos);
       i++;
    }

    printf("Total alfabeto: %zu\n", ab->total_counts);
    size_t sz=0;
    char** wds = lexicon_get_keys(le,corpus,&sz);
    for(size_t i=0;i<le->occupancy;i++)
    {
        printf("%s %lf\n", wds[i],lexicon_get_cost(le,wds[i]));
    }
    mem_pop(corpus,sz);

    printf("Itens lexicais únicos: %zu\n", le->occupancy);

     

    uint32_t n;
    size_t am;
    double cst;

    printf("Teste minseg. Memória total alocada com corpus e alfabeto: %zu\n\n", corpus->size);
    while(1)
    {

        char buff[144];
        printf("Frase sem espaços (Posição: %zu): ", corpus->pos);
        scanf("%s",buff);

        if(strcmp(buff,"q") ==0) break;

        char* segs = minseg(buff,le,corpus,&n,&am,&cst);

        printf("Custo: %lf\nSegmentos: %u\nMemória alocada: %zu\nSegmentos abaixo:\n",cst,n,am); 
        for(int i=0;i<n;i++)
        {
            printf("%-2d %s\n",i,segs);
            segs = str_stack_next(segs);
        }
        printf("\nPosição antes da liberação de memória: %zu\n", corpus->pos);
        mem_pop(corpus,am);
        printf("Posição após a liberação de memória: %zu\n\n", corpus->pos);
    }

    printf("\nEscrevendo a memória no arquivo dump.bin...\n");
    FILE* fptr = fopen("dump.bin","wb");
    size_t bytes = fwrite((char*) corpus->data,sizeof(char),corpus->pos,fptr);
    fclose(fptr);
    printf("%zu bytes escritos\n", bytes);

    return 0;
}

