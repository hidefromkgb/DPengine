#ifndef EXEC_STRING_H
#define EXEC_STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "exec.h"

#ifdef __cplusplus
extern "C" {
#endif



#define DEF_CMNT '\'' /* default comment character */
#define DEF_CRLF '\n' /* default end-of-line character */
#define DEF_LFCR '\r' /* non-default end-of-line character */
#define DEF_QUOT  '"' /* default quotation character - inhibits tokenization */
#define DEF_TSEP  ',' /* default token separator */
#define DEF_DSEP  "/" /* default directory separator (string) */

// hashes at most 32 comma-separated 'char'-s using FNV-1a; empty list yields 0
#define CHR_HASH(mode, ...) (CHR_HASH_L(CHR_HASH_O,##__VA_ARGS__)    \
            CHR_HASH_BIAS * (CHR_HASH_L(CHR_HASH_B,##__VA_ARGS__) 0) \
                       CHR_HASH_L(CHR_HASH_C##mode,##__VA_ARGS__))
#define CHR_HASH_L(f, ...)   CHR_HASH_L4(f,,,,,,,,,,,,,,,,,##__VA_ARGS__) \
                             CHR_HASH_L4(f                ,##__VA_ARGS__)
#define CHR_HASH_L4(f, ...) \
CHR_HASH_L3(f,,,,,,,,,,,,,__VA_ARGS__) CHR_HASH_L3(f,,,,,,,,,__VA_ARGS__) \
CHR_HASH_L3(f        ,,,,,__VA_ARGS__) CHR_HASH_L3(f        ,__VA_ARGS__)
#define CHR_HASH_L3(f, ...) \
CHR_HASH_L2(f         ,,,,__VA_ARGS__) CHR_HASH_L2(f      ,,,__VA_ARGS__) \
CHR_HASH_L2(f           ,,__VA_ARGS__) CHR_HASH_L2(f        ,__VA_ARGS__)
#define CHR_HASH_L2(...) CHR_HASH_L1(__VA_ARGS__, \
1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, \
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, )
#define CHR_HASH_L1(    ___,s1,s2,s3,s4,s5,s6,s7, t0,t1,t2,t3,t4,t5,t6,t7, \
u0,u1,u2,u3,u4,u5,u6,u7, v0,v1,v2,v3,v4,v5,v6,v7, __,a1,a2,a3,a4,a5,a6,a7, \
b0,b1,b2,b3,b4,b5,b6,b7, c0,c1,c2,c3,c4,c5,c6,c7, d0,d1,d2,d3,d4,d5,d6,d7, \
_, ...) ___##_(__)
#define CHR_HASH_O0(c) /* opening parentheses */
#define CHR_HASH_O1(c) (
#define CHR_HASH_B0(c) /* initial bias multiplier unless the input is empty */
#define CHR_HASH_B1(c) 1 |
#define CHR_HASH_CN0(c) /* the rest of the FNV-1a hash + closing parentheses */
#define CHR_HASH_CN1(c) ^ (size_t)(c)) * CHR_HASH_MULT
#define CHR_HASH_CL(c) /* y/A-Z/a-z/ */ \
            (char)(((c) >= 'A') && ((c) <= 'Z') ? (c) + ('a' - 'A') : (c))
#define CHR_HASH_CL0(c) /* same as CHR_HASH_CN* but with case lowering */
#define CHR_HASH_CL1(c) ^ (size_t)CHR_HASH_CL(c)) * CHR_HASH_MULT
#define CHR_HASH_CU(c) /* y/a-z/A-Z/ */ \
            (char)(((c) >= 'a') && ((c) <= 'z') ? (c) - ('a' - 'A') : (c))
#define CHR_HASH_CU0(c) /* same as CHR_HASH_CL* but upper instead of lower */
#define CHR_HASH_CU1(c) ^ (size_t)CHR_HASH_CU(c)) * CHR_HASH_MULT
#define CHR_HASH_BIAS (size_t)14695981039346656037uLL
#define CHR_HASH_MULT (size_t)1099511628211uLL



// a non-owning view of SIZE chars at STR, not necessarily 0-terminated
typedef struct { size_t size; const char *str; } string_view_t;

// the string class: owns data, manages memory, surpasses char* in everything
typedef struct string_t string_t;

// appends the views to HEAD (nullptr: new string); empty views are skipped
#define make_string(...) append_string(nullptr, __VA_ARGS__)
#define append_string(head, ...) append_string_internal_(head, \
        sizeof((string_view_t[]){__VA_ARGS__}) / sizeof(string_view_t), \
        (const string_view_t[]){__VA_ARGS__})
string_t *append_string_internal_(string_t *head, size_t n,
                                  const string_view_t *v);
void free_string_internal_(string_t *const *str);
void free_string(const string_t *str);

string_view_t get_view(const string_t *str);
#define get_imm_view(literal) (string_view_t){sizeof(literal) - 1, literal}
#define get_pchar_view(pchr) (string_view_t){(pchr) ? strlen(pchr) : 0, pchr}

#define local_string_t __attribute__((cleanup(free_string_internal_))) string_t

#define ILV_DSEP get_imm_view(DEF_DSEP) /* immutable string-literal view */



// MODE: 'N' as is, 'L' ASCII to lowercase, 'U' ASCII to uppercase
size_t str_view_hash(string_view_t str, char mode);
size_t str_hash(const string_t *str, char mode);
void ascii_to_lower(string_view_t str);
void ascii_to_upper(string_view_t str);



// splits a string into tokens: holds the first token and the remainder
typedef struct tokenizer_t tokenizer_t;
extern const size_t tokenizer_t_size;

// MEM as a tokenizer with no token yet and V as the remainder
tokenizer_t *make_tokenizer(void *mem, string_view_t v);

// sits in stack till return: never use in loops anywhere but in initialization
#define local_make_tokenizer(...) make_tokenizer( \
            __builtin_alloca(tokenizer_t_size), (string_view_t) __VA_ARGS__)

bool is_empty(const tokenizer_t *t); // both the token and the remainder
string_view_t get_token(const tokenizer_t *t);
string_view_t get_remainder(const tokenizer_t *t);

string_view_t utf8_skip_char(string_view_t v); // V without the first UTF8 char

// replaces T with the first token of V and the remainder of V
// C, S, Q: comment, separator and quotation chars (0 if none)
void next_token(tokenizer_t *t, string_view_t v, char c, char s, char q);
// the next line taken from the remainder; returns false if nothing is left
bool next_line(tokenizer_t *t);



#ifdef __cplusplus
}
#endif

#endif // EXEC_STRING_H
