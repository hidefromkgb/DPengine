#ifndef EXEC_STRING_H
#define EXEC_STRING_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEF_CMNT '\'' /* default comment character */
#define DEF_CRLF '\n' /* default end-of-line character */
#define DEF_LFCR '\r' /* non-default end-of-line character */
#define DEF_QUOT  '"' /* default quotation character - inhibits tokenization */
#define DEF_TSEP  ',' /* default token separator */

// a non-owning view of SIZE chars at STR, not necessarily 0-terminated
typedef struct { size_t size; const char *str; } string_view_t;

// splits a string into tokens: holds the first token and the remainder
typedef struct tokenizer_t tokenizer_t;
extern const size_t tokenizer_t_size;

// MEM as a tokenizer with no token yet and V as the remainder
tokenizer_t *make_tokenizer(void *mem, string_view_t v);

// sits in stack till return: never use in loops anywhere but in initialization
#define local_make_tokenizer(...) \
            make_tokenizer(__builtin_alloca(tokenizer_t_size), __VA_ARGS__)

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
