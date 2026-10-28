#include <cassert>

#include "exec_parser.hpp"



std::string concat_path(const std::initializer_list<const std::string> &list) {
    std::string retn;
    for (auto &str : list) retn += (!retn.empty()) ? DEF_DSEP + str : str;
    return retn;
}

std::string to_string(string_view_t str, char mode) {
    std::string retn(str.str, str.size);
    switch (mode) {
        default:
            assert(false); // unknown character transformation mode
        case 'N': // no transformation
            return retn;
        case 'L': // ASCII to lowercase
            for (auto &c : retn) c = CHR_HASH_CL(c);
            return retn;
        case 'U': // ASCII to uppercase
            for (auto &c : retn) c = CHR_HASH_CU(c);
            return retn;
    }
}

size_t str_hash(string_view_t str, char mode) {
    size_t retn = (str.size) ? CHR_HASH_BIAS : 0uLL;
    switch (mode) {
        default:
            assert(false); // unknown character transformation mode
        case 'N': // no transformation
            for (size_t i = 0; i < str.size; i++)
                retn = CHR_HASH_O1() retn CHR_HASH_CN1(str.str[i]);
            return retn;
        case 'L': // ASCII to lowercase
            for (size_t i = 0; i < str.size; i++)
                retn = CHR_HASH_O1() retn CHR_HASH_CL1(str.str[i]);
            return retn;
        case 'U': // ASCII to uppercase
            for (size_t i = 0; i < str.size; i++)
                retn = CHR_HASH_O1() retn CHR_HASH_CU1(str.str[i]);
            return retn;
    }
}

float to_float(string_view_t line, float def) {
    char *end = nullptr;
    errno = 0;
    float retn = (line.size) ? ::strtof(line.str, &end) : def;
    return (errno || (line.str == end)) ? def : retn;
}

bool process_bool(tokenizer_t *line, bool def, char s, char q) {
    static const std::unordered_map<std::string, bool>
        map = { {"false", false}, {"true", true}, };
    return process_map(line, map, def, s, q);
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

std::string process_string(tokenizer_t *line, char mode, char s, char q) {
    next_token(line, get_remainder(line), 0, s, q);
    return to_string(get_token(line), mode);
}
