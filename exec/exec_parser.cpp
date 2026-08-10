#include <cassert>
#include <charconv>

#include "exec_parser.hpp"



static bool utf8_is_wspace(const std::string_view &str) {
    const auto size = str.size();
    if (size == 0) return false;
    switch (str[0]) {
        case '\x09': case '\x20': return true;
        case '\xC2':
            return (size > 1) && (str[1] == '\xA0');
        case '\xE1':
            return (size > 2) && (str[1] == '\x9A') && (str[2] == '\x80');
        case '\xE3':
            return (size > 2) && (str[1] == '\x80') && (str[2] == '\x80');
        case '\xEF':
            return (size > 2) && (str[1] == '\xBB') && (str[2] == '\xBF');
        case '\xE2':
            if (size <= 2) return false;
            switch (str[1]) {
                case '\x81': return str[2] == '\x9F';
                case '\x80':
                    switch (str[2]) {
                        case '\x80': case '\x81': case '\x82': case '\x83':
                        case '\x84': case '\x85': case '\x86': case '\x87':
                        case '\x88': case '\x89': case '\x8A': case '\x8B':
                        case '\xAF': return true;
                    }
            }
    }
    return false;
}

static inline void utf8_skip_char(std::string_view &str) {
    static const char skip[] = {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 6};
    str.remove_prefix((str[0] & 0x80) ? skip[(str[0] >> 2) & 0x0F] : 1);
}



std::string concat_path(const std::initializer_list<const std::string> &list) {
    std::string retn;
    for (auto &str : list) retn += (!retn.empty()) ? DEF_DSEP + str : str;
    return retn;
}

std::string ascii_to_lower(const std::string_view &str) {
    std::string retn(str);
    for (auto &c : retn) c = ascii_to_lower(c);
    return retn;
}

std::string ascii_to_upper(const std::string_view &str) {
    std::string retn(str);
    for (auto &c : retn) c = ascii_to_upper(c);
    return retn;
}

token_t next_token(const std::string_view &str, char c, char s, char q) {
    assert(s != q);
    auto sep = s;
    auto iter = str;
    while (utf8_is_wspace(iter)) utf8_skip_char(iter);
    // abort if token begins with comment char
    if (iter.empty() || (iter[0] == c)) return {};
    // look for the next quote instead of separator if token begins with quote
    if (iter[0] == q) {
        utf8_skip_char(iter);
        sep = q;
    }
    auto found = iter.find(sep);
    if (found == std::string_view::npos) return {iter, {}};
    std::string_view token((found > 0) ? iter.data() : nullptr, found);
    iter.remove_prefix(found + sizeof(sep));
    if (sep != s) {
        found = iter.find(s);
        if (found == std::string_view::npos) iter = {};
        else iter.remove_prefix(found + sizeof(s));
        //if (found > 0) printf("'%s': tokens delimited past quotes!\n");
    }
    return {token, iter};
}

bool process_bool(token_t &line, bool def, char s, char q) {
    static const std::unordered_map<std::string, bool>
        map = { {"false", false}, {"true", true}, };
    return process_map(line, map, def, s, q);
}

float process_float(token_t &line, float def, char s, char q) {
    line = next_token(line.second, 0, s, q);
    std::from_chars(
            line.first.data(), line.first.data() + line.first.size(), def);
    return def;
}

std::vector<std::string_view> process_array(
        token_t &line, char s, char q, char bgn, char end) {
    std::vector<std::string_view> retn;
    line = next_token(line.second, 0, bgn, 0);
    if (line.second.empty()) { // no array beginning detected, array = {token}
        line = next_token(line.first, 0, s, q);
        return {line.first};
    }
    line = next_token(line.second, 0, end, 0);
    for (token_t iter({}, line.first); !is_empty(iter);
            iter = next_token(iter.second, 0, s, q))
        if (!iter.first.empty()) retn.emplace_back(iter.first);
    line = next_token(line.second, 0, s, q);
    return retn;
}

T2IV process_quoted_int_pair(token_t &line, T2IV def, char s, char q) {
    auto pair = process_array(line, s, 0, q, q);
    if (pair.size() > 0)
        std::from_chars(pair[0].data(), pair[0].data() + pair[0].size(), def.x);
    if (pair.size() > 1)
        std::from_chars(pair[1].data(), pair[1].data() + pair[1].size(), def.y);
    return def;
}

std::string process_string(token_t &line, char s, char q) {
    line = next_token(line.second, 0, s, q);
    return std::string(line.first);
}
