#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "exec_string.h"



struct tokenizer_t { string_view_t token, remainder; };
const size_t tokenizer_t_size = sizeof(tokenizer_t);

tokenizer_t *make_tokenizer(void *mem, string_view_t v) {
    *((tokenizer_t*)mem) = (tokenizer_t){(string_view_t){}, v};
    return (tokenizer_t*)mem;
}
bool is_empty(const tokenizer_t *t) {
    return !t->token.size && !t->remainder.size;
}
string_view_t get_token(const tokenizer_t *t) { return t->token; }
string_view_t get_remainder(const tokenizer_t *t) { return t->remainder; }

static bool utf8_is_wspace(string_view_t v) {
    const auto size = v.size;
    const auto str = v.str;
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

string_view_t utf8_skip_char(string_view_t v) { // assumes 1+ readable chars
    static const char skip[] = {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 6};
    size_t skip_size = (v.str[0] & 0x80) ? skip[(v.str[0] >> 2) & 0x0F] : 1;
    if (skip_size > v.size) skip_size = v.size; // truncated char
    return (string_view_t){v.size - skip_size, v.str + skip_size};
}

void next_token(tokenizer_t *t, string_view_t v, char c, char s, char q) {
    assert(s != q);
    auto sep = s;
    while (utf8_is_wspace(v)) v = utf8_skip_char(v);
    if (!v.size || (v.str[0] == c)) { // if comment char at start, abort
        *t = (tokenizer_t){};
        return;
    }
    if (v.str[0] == q) { // if quote at start, next quote becomes separator
        v = utf8_skip_char(v);
        sep = q;
    }
    auto found = (const char*)memchr(v.str, sep, v.size);
    if (!found) { // no separator: all of the remainder becomes the token
        *t = (tokenizer_t){v, (string_view_t){}};
        return;
    }
    t->token = (string_view_t){(size_t)(found - v.str),
                               (found > v.str) ? v.str : nullptr};
    v.size -= found + sizeof(sep) - v.str;
    v.str = found + sizeof(sep);
    if (sep != s) {
        if (!(found = (const char*)memchr(v.str, s, v.size))) {
            v = (string_view_t){};
        } else {
            //if (found > v.str) printf("tokens delimited past quotes!\n");
            v.size -= found + sizeof(s) - v.str;
            v.str = found + sizeof(s);
        }
    }
    t->remainder = v;
}

bool next_line(tokenizer_t *t) {
    next_token(t, t->remainder, 0, DEF_CRLF, 0);
    if (t->token.size && (t->token.str[t->token.size - 1] == DEF_LFCR))
        t->token.size--;
    return !is_empty(t);
}

size_t str_view_hash(string_view_t str, char mode) {
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

size_t str_hash(const string_t *str, char mode) {
    return str_view_hash(get_view(str), mode);
}

void ascii_to_lower(string_view_t str) {
    for (size_t i = 0; i < str.size; i++)
        ((char*)str.str)[i] = CHR_HASH_CL(str.str[i]);
}

void ascii_to_upper(string_view_t str) {
    for (size_t i = 0; i < str.size; i++)
        ((char*)str.str)[i] = CHR_HASH_CU(str.str[i]);
}



#define string_t_ag_p2 3 /* alloc granularity as a power of 2: (1 << 3) = 8 */
#define string_t_size(s) ((((s) >> string_t_ag_p2) + 1) << string_t_ag_p2)

string_view_t get_view(const string_t *str) {
    return (str) ? (string_view_t){((const size_t*)str)[-1], (const char*)str}
                 : (string_view_t){};
}

void free_string_internal_(string_t *const *str) {
    if (str && *str) free((void*)((size_t*)*str - 1));
}
void free_string(const string_t *str) {
    free_string_internal_((string_t *const *)&str);
}

string_t *append_string_internal_(string_t *head, size_t n,
                                  const string_view_t *v) {
    const auto str_v = get_view(head);
    size_t size = str_v.size;
    for (size_t i = 0; i < n; i++) size += v[i].size;
    if (size == str_v.size) return head; // nothing to append
    char *mem = (head) ? (char*)((size_t*)head - 1) : nullptr;
    if (!head || (string_t_size(str_v.size) < string_t_size(size)))
        mem = (char*)realloc((void*)mem, sizeof(size_t) + string_t_size(size));
    *((size_t*)mem) = size;
    mem += sizeof(size_t);
    size = str_v.size;
    for (size_t i = 0; i < n; i++) { // views into HEAD follow its reallocation
        const uintptr_t offs = (uintptr_t)v[i].str - (uintptr_t)str_v.str;
        const char *str = (head && (offs < str_v.size)) ? mem + offs : v[i].str;
        for (size_t j = 0; j < v[i].size; j++) mem[size++] = str[j];
    }
    mem[size] = 0;
    return (string_t*)mem;
}
