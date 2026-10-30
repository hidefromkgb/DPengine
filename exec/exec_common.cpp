#include "exec_common.hpp"
#include "exec_engine.hpp"
#include "exec_parser.hpp"



// TODO: implement a test system? e.g. instead of the screen the characters
//       would print their actions to STDOUT, and if the same PRNG, config,
//       and resolution are set, then everything is deterministic and these
//       output logs can be validated by comparing them to a reference log.

// TODO: cursor dodging within the NCDR radius
// TODO: more menu items, including the per-OS submenu



void conf_t::free_lang_map(lang_map_t *lang_map) {
    for (auto &item : *lang_map) free_string(item.second);
    lang_map->clear();
}

void conf_t::dup_strings(conf_t *conf) {
    conf->base = make_string(get_view(conf->base));
    conf->lang = make_string(get_view(conf->lang));
    for (auto &item : conf->lang_map)
        item.second = make_string(get_view(item.second));
}

conf_t::lang_map_t conf_t::get_lang_map(string_view_t file_name) {
    INCBIN("../exec/loc/en.lang", DefLang);
    long size = (!file_name.size) ? DefLang_end - DefLang : 0;
    auto file = (!file_name.size) ? DefLang : rLoadFile(file_name.str, &size);
    lang_map_t retn;
    if (file) {
        auto text = local_make_tokenizer({(size_t)size, file});
        for (int32_t idx = 0; next_line(text); idx++)
            if (get_token(text).size) retn[idx] = make_string(get_token(text));
        if (file_name.size) free(file);
    }
    return retn;
}

void eExecuteEngine(char *fcnf, char *base, ulong xico, ulong yico,
                    long  xpos, long  ypos, ulong xdim, ulong ydim) {
    T2IV ico{{(decltype(ico.x))xico, (decltype(ico.y))yico}};
    T4IV scr{{(decltype(scr.x))(xpos), (decltype(scr.y))(ypos),
              (decltype(scr.z))(xdim - xpos), (decltype(scr.w))(ydim - ypos)}};
    engine_t engc(get_pchar_view(fcnf), get_pchar_view(base), ico, scr);

    engc.main_loop();
}
