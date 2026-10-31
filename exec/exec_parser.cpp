#include <cerrno>

#include "exec_parser.hpp"



float to_float(string_view_t line, float def) {
    char *end = nullptr;
    errno = 0;
    float retn = (line.size) ? ::strtof(line.str, &end) : def;
    return (errno || (line.str == end)) ? def : retn;
}

bool process_bool(tokenizer_t *line, bool def, char s, char q) {
    next_token(line, get_remainder(line), 0, s, q);
    return (get_token(line).size) // [t]rue = 1 / false = 0
         ? (CHR_HASH_CL(get_token(line).str[0]) == 't') : def;
}

float process_float(tokenizer_t *line, float def, char s, char q) {
    next_token(line, get_remainder(line), 0, s, q);
    return to_float(get_token(line), def);
}

std::vector<string_view_t> process_array(
        tokenizer_t *line, char s, char q, char bgn, char end) {
    std::vector<string_view_t> retn;
    next_token(line, get_remainder(line), 0, bgn, 0);
    if (!get_remainder(line).size) { // no array start found, array = {token}
        next_token(line, get_token(line), 0, s, q);
        return {get_token(line)};
    }
    next_token(line, get_remainder(line), 0, end, 0);
    for (auto iter = local_make_tokenizer(get_token(line)); !is_empty(iter);
            next_token(iter, get_remainder(iter), 0, s, q))
        if (get_token(iter).size) retn.emplace_back(get_token(iter));
    next_token(line, get_remainder(line), 0, s, q);
    return retn;
}

T2IV process_quoted_int_pair(tokenizer_t *line, T2IV def, char s, char q) {
    auto pair = process_array(line, s, 0, q, q);
    if (pair.size() > 0)
        def.x = to_float(pair[0], def.x);
    if (pair.size() > 1)
        def.y = to_float(pair[1], def.y);
    return def; // 23 bits per value are more than enough here
}

string_t *process_string(tokenizer_t *line, char s, char q) {
    next_token(line, get_remainder(line), 0, s, q);
    return make_string(get_token(line));
}
