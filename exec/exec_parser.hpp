#ifndef EXEC_PARSER_HPP
#define EXEC_PARSER_HPP

#include <unordered_map>

#include "exec_common.hpp"
#include "exec_string.h"

#define DEF_ABGN '{' /* default array start character */
#define DEF_AEND '}' /* default array end character */

bool process_bool(
        tokenizer_t *line, bool def, char s = DEF_TSEP, char q = DEF_QUOT);

float process_float(
        tokenizer_t *line, float def, char s = DEF_TSEP, char q = DEF_QUOT);

std::vector<string_view_t> process_array(tokenizer_t *line, char s = DEF_TSEP,
        char q = DEF_QUOT, char bgn = DEF_ABGN, char end = DEF_AEND);

T2IV process_quoted_int_pair(
        tokenizer_t *line, T2IV def, char s = DEF_TSEP, char q = DEF_QUOT);

string_t *process_string(
        tokenizer_t *line, char s = DEF_TSEP, char q = DEF_QUOT);

template <typename T>
T process_map(tokenizer_t *line, const std::unordered_map<size_t, T> &map,
        T def, char s = DEF_TSEP, char q = DEF_QUOT) {
    next_token(line, get_remainder(line), 0, s, q);
    auto it = find_in_map(map, str_view_hash(get_token(line), 'L'));
    return (it) ? *it : def;
}

#endif // EXEC_PARSER_HPP
