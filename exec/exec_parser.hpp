#ifndef EXEC_PARSER_HPP
#define EXEC_PARSER_HPP

#include <string>
#include <unordered_map>

#include "exec_common.hpp"
#include "exec_string.h"

#define DEF_DSEP "/" /* default directory separator (string) */
#define DEF_ABGN '{' /* default array start character */
#define DEF_AEND '}' /* default array end character */

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

// C++ only: std::string -> string_view_t
inline string_view_t to_view(const std::string &str) {
    return (string_view_t){str.size(), str.c_str()};
}

// MODE: 'N' as is, 'L' ASCII to lowercase, 'U' ASCII to uppercase
std::string to_string(string_view_t str, char mode);
size_t str_hash(string_view_t str, char mode);

std::string concat_path(const std::initializer_list<const std::string> &list);

bool process_bool(
        tokenizer_t *line, bool def, char s = DEF_TSEP, char q = DEF_QUOT);

float process_float(
        tokenizer_t *line, float def, char s = DEF_TSEP, char q = DEF_QUOT);

std::vector<string_view_t> process_array(tokenizer_t *line, char s = DEF_TSEP,
        char q = DEF_QUOT, char bgn = DEF_ABGN, char end = DEF_AEND);

T2IV process_quoted_int_pair(
        tokenizer_t *line, T2IV def, char s = DEF_TSEP, char q = DEF_QUOT);

std::string process_string(
        tokenizer_t *line, char mode, char s = DEF_TSEP, char q = DEF_QUOT);

template <typename T>
T process_map(tokenizer_t *line, const std::unordered_map<std::string, T> &map,
        T def, char s = DEF_TSEP, char q = DEF_QUOT) {
    next_token(line, get_remainder(line), 0, s, q);
    auto it = find_in_map(map, to_string(get_token(line), 'L'));
    return (it) ? *it : def;
}

#endif // EXEC_PARSER_HPP
