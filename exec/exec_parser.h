#ifndef EXEC_PARSER_H
#define EXEC_PARSER_H

#include "exec_string.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEF_ABGN '{' /* default array start character */
#define DEF_AEND '}' /* default array end character */

bool process_bool(tokenizer_t *line, bool def, char s, char q);

float process_float(tokenizer_t *line, float def, char s, char q);

size_t process_array(tokenizer_t *line, string_view_t **retn,
        char s, char q, char bgn, char end);

T2IV process_quoted_int_pair(tokenizer_t *line, T2IV def, char s, char q);

string_t *process_string(tokenizer_t *line, char s, char q);

#ifdef __cplusplus
}
#endif

#endif // EXEC_PARSER_H
