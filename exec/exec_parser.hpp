#ifndef EXEC_PARSER_HPP
#define EXEC_PARSER_HPP

#include <string>
#include <string_view>
#include <unordered_map>

#include "exec_common.hpp"

using token_t = std::pair<std::string_view, std::string_view>;

inline bool is_empty(const token_t &t) {
    return t.first.empty() && t.second.empty();
}

// default comment character
#define DEF_CMNT '\''
// default end-of-line character
#define DEF_CRLF '\n'
// non-default end-of-line character
#define DEF_LFCR '\r'
// default quotation character - inhibits tokenization
#define DEF_QUOT '"'
// default token separator
#define DEF_TSEP ','
// default dir slash (string)
#define DEF_DSEP "/"

std::string ascii_to_lower(const std::string_view &str);
std::string ascii_to_upper(const std::string_view &str);

static inline constexpr char ascii_to_lower(char c) {
    return ((c >= 'A') && (c <= 'Z')) ? c + ('a' - 'A') : c;
}
static inline constexpr char ascii_to_upper(char c) {
    return ((c >= 'a') && (c <= 'z')) ? c - ('a' - 'A') : c;
}
size_t constexpr str_hash(const std::string_view &str, bool ascii2low = true) {
    size_t retn = (str.size()) ? 14695981039346656037uLL : 0uLL;
    const size_t mult = 1099511628211uLL;
    if (ascii2low)
        for (auto c : str) retn = (retn ^ (size_t)ascii_to_lower(c)) * mult;
    else
        for (auto c : str) retn = (retn ^ (size_t)c) * mult;
    return retn;
}

std::string concat_path(const std::initializer_list<const std::string> &list);

token_t next_token(const std::string_view &str,
        char c = DEF_CMNT, char s = DEF_TSEP, char q = DEF_QUOT);

bool process_bool(
        token_t &line, bool def, char s = DEF_TSEP, char q = DEF_QUOT);

float process_float(
        token_t &line, float def, char s = DEF_TSEP, char q = DEF_QUOT);

std::vector<std::string_view> process_array(token_t &line,
        char s = DEF_TSEP, char q = DEF_QUOT, char bgn = '{', char end = '}');

T2IV process_quoted_int_pair(token_t &line,
        T2IV def, char s = DEF_TSEP, char q = DEF_QUOT);

std::string process_string(
        token_t &line, char s = DEF_TSEP, char q = DEF_QUOT);

template <typename T>
T process_map(token_t &line, const std::unordered_map<std::string, T> &map,
        T def, char s = DEF_TSEP, char q = DEF_QUOT) {
    line = next_token(line.second, 0, s, q);
    auto it = find_in_map(map, ascii_to_lower(line.first));
    return (it) ? *it : def;
}

#endif // EXEC_PARSER_HPP
