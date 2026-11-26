#include "errno.h"

#include "exec_parser.h"



static float to_float(string_view_t line, float def) {
    char *end = nullptr;
    errno = 0;
    float retn = (line.size) ? strtof(line.str, &end) : def;
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

size_t process_array(tokenizer_t *line,
        string_view_t **retn, char s, char q, char bgn, char end) {
    next_token(line, get_remainder(line), 0, bgn, 0);
    if (!get_remainder(line).size) { // no array start found, array = {token}
        next_token(line, get_token(line), 0, s, q);
        *retn = (typeof(*retn))realloc(nullptr, sizeof(**retn));
        **retn = get_token(line);
        return 1;
    }
    next_token(line, get_remainder(line), 0, end, 0);
    size_t capacity = 0, size = 0;
    *retn = nullptr;
    for (auto iter = local_make_tokenizer(get_token(line)); !is_empty(iter);
            next_token(iter, get_remainder(iter), 0, s, q))
        if (get_token(iter).size) {
            if (++size > capacity)
                *retn = (typeof(*retn))realloc(
                            *retn, (capacity += 8) * sizeof(**retn));
            (*retn)[size - 1] = get_token(iter);
        }
    if (size) *retn = (typeof(*retn))realloc(*retn, size * sizeof(**retn));
    next_token(line, get_remainder(line), 0, s, q);
    return size;
}

T2IV process_quoted_int_pair(tokenizer_t *line, T2IV def, char s, char q) {
    string_view_t *pair;
    auto size = process_array(line, &pair, s, 0, q, q);
    if (size > 0) def.x = to_float(pair[0], def.x);
    if (size > 1) def.y = to_float(pair[1], def.y);
    free(pair);
    return def; // 23 bits per value are more than enough here
}

string_t *process_string(tokenizer_t *line, char s, char q) {
    next_token(line, get_remainder(line), 0, s, q);
    return make_string(get_token(line));
}
