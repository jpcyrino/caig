
//          Copyright João Paulo Lazzarini Cyrino 2023 
//  Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

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

#ifndef __CU32_H__
#define __CU32_H__

#include <uchar.h>

// Retorna o número de caracteres em uma string codificada
// em UTF8
// u8str = string padrão C codificada em UTF8
size_t u8strlen(const char* u8str);

// Retorna o número de caracteres em uma string de
// caracteres de 32bits
// u32str = string padrão C codificada em caracteres de 32bits
size_t u32strlen(const char32_t* u32str);

// Retorna o número de bytes necessários para codificar uma string
// em caracteres de 32bits em uma string UTF8. Útil para
// alocar memória para string UTF8 que receberá a conversão na função
// u32to8 abaixo.
// u32str = string padrão C codificada em caracteres de 32bits
size_t u32strmblen(const char32_t* u32str);

// Copia uma string de caracteres de 32bits para um buffer de caracteres
// de 32bits. Interface semelhante a strcpy.
// dest = buffer de caracteres de 32bits.
// src = string a ser copiada
void u32strcpy(char32_t* dest, const char32_t* src);

// Copia os n primeiros caracteres de uma string de caracteres de 32bits
// para um buffer de caracteres de 32bits. Interface semelhante a strncpy.
// dest = buffer de caracteres de 32bits.
// src = string a ser copiada
// n = número de caracteres a serem copiados
void u32strncpy(char32_t* dest, const char32_t* src, size_t n);

// Verifica igualdade entre strings de 32bits. Retorna 0 se diferente e 1
// se igual.
// str_a = primeira string para comparação
// str_b = segunda string para comparação
int8_t u32streq(const char32_t* str_a, const char32_t* str_b);

// Converte uma string em UTF8 em uma string de caracteres
// de largura fixa de 32bits. 
// u8str = string padrão C codificada em UTF8
// u32str = buffer previamente alocado que receberá a string convertida.
size_t u8to32(const char* u8str, char32_t* u32str);

// Converte uma string de caracteres de largura fixa para uma 
// string codificada em UTF8.
// u32str = string padrão C codificada em largura fixa de 32bits
// u8str = buffer previamente alocado que recebeá a string convertida.
size_t u32to8(const char32_t* u32str, char* u8str);
#endif
