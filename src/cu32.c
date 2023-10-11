/* CU32
 * Rotinas para conversão de strings em utf8 para
 * strings de caracteres de largura fixa de 32bits.
 * Não há conversão garantida para utf32 no momento.
 * Use para facilitar acesso randômico a caracteres 
 * unicode internamente ao programa.
 * 
 * Testado em GCC 13.2, C11
 * Autor: João Paulo Lazzarini Cyrino
 * Data 11.10.2023
 */
#include "cu32.h"
#include <uchar.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>

#define FIRST_OF_TWO_BYTES_SEQ 0xC0
#define FIRST_OF_THREE_BYTES_SEQ 0xE0
#define FIRST_OF_FOUR_BYTES_SEQ 0xF0

#define FOUR_BYTES 0x1000000
#define THREE_BYTES 0x10000
#define TWO_BYTES 0x100

size_t u8strlen(const char* u8str)
{
    size_t len = 0;
    while(*u8str)
    {
        unsigned char chr = (unsigned char) *u8str;
        if(chr > FIRST_OF_FOUR_BYTES_SEQ) u8str = u8str + 4;
        else if(chr > FIRST_OF_THREE_BYTES_SEQ) u8str = u8str + 3;
        else if(chr > FIRST_OF_TWO_BYTES_SEQ) u8str = u8str + 2;
        else u8str++;
        len++;
    }
    return len;
}

size_t u32strlen(const char32_t* u32str)
{
    size_t len = 0;
    while(*u32str++) len++;
    return len;
}


void u32strcpy(char32_t* dest, const char32_t* src)
{
    for(size_t i=0;i<u32strlen(src);i++)
    {
        dest[i] = src[i];
    }
    dest[u32strlen(src)] = '\0';
}

void u32strncpy(char32_t* dest, const char32_t* src, size_t n)
{
    for(size_t i=0;i<n;i++)
    {
        dest[i] = src[i];
    }
    dest[n] = '\0';
}

int8_t u32streq(const char32_t* str_a, const char32_t* str_b)
{
    size_t len_a = u32strlen(str_a);
    if(len_a != u32strlen(str_b)) return 0;
    for(size_t i=0;i<len_a;i++)
    {
        if(str_a[i] != str_b[i]) return 0;
    }
    return 1;
}

size_t u8to32(const char* u8str, char32_t* u32str)
{
    size_t bytelen = strlen(u8str);
    size_t runelen = u8strlen(u8str);

    for(size_t i=0;i<runelen;i++)
    {
        unsigned char chr = (unsigned char) *u8str;
        unsigned char sz = 0;
        if(chr > FIRST_OF_FOUR_BYTES_SEQ) sz = 4;
        else if (chr > FIRST_OF_THREE_BYTES_SEQ) sz = 3;
        else if (chr > FIRST_OF_TWO_BYTES_SEQ) sz = 2;
        else sz = 1;
        
        char32_t u32c = 0;
        switch(sz)
        {
            case 4: u32c += (unsigned char) *u8str++; u32c <<= 8;
            case 3: u32c += (unsigned char) *u8str++; u32c <<= 8;
            case 2: u32c += (unsigned char) *u8str++; u32c <<= 8;
            case 1: u32c += (unsigned char) *u8str++; 
        }
        u32str[i] = u32c;
    }
    u32str[runelen] = 0;
    return runelen;
}

size_t u32strmblen(const char32_t* u32str)
{
    size_t size_of_u8str = 0;
    for(int i=0; i<u32strlen(u32str);i++)
    {
        if(u32str[i] >= FOUR_BYTES) size_of_u8str += 4;
        else if(u32str[i] >= THREE_BYTES) size_of_u8str += 3;
        else if(u32str[i] >= TWO_BYTES) size_of_u8str += 2;
        else size_of_u8str++;
    }
    return size_of_u8str;
}
    

size_t u32to8(const char32_t* u32str, char* u8str)
{
    // iterate through every byte of u32str with bytepos
    // every non-zero byte is transfered to the next position (j) 
    // in u8str
    size_t j = 0;
    for(size_t i=0;i<u32strlen(u32str);i++)
    {
        if(u32str[i] >= FOUR_BYTES) 
        {
            *u8str++ = (u32str[i] >> 24);
            *u8str++ = (u32str[i] >> 16);
            *u8str++ = (u32str[i] >> 8);
            *u8str++ = u32str[i];
        } 
        else if(u32str[i] >= THREE_BYTES)
        {
            *u8str++ = (u32str[i] >> 16); 
            *u8str++ = (u32str[i] >> 8); 
            *u8str++ = u32str[i]; 
        }
        else if(u32str[i] >= TWO_BYTES)
        {
            *u8str++ = (u32str[i] >> 8); 
            *u8str++ = u32str[i];
        }
        else *u8str++ = u32str[i];
    }
    *u8str = '\0'; 

    return 0;
}

