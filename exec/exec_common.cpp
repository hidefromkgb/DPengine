#include "exec_common.hpp"
#include "exec_engine.hpp"
#include "exec_parser.hpp"



// TODO: implement a test system? e.g. instead of the screen the characters
//       would print their actions to STDOUT, and if the same PRNG, config,
//       and resolution are set, then everything is deterministic and these
//       output logs can be validated by comparing them to a reference log.

// TODO: content downloading
// TODO: speech bubbles
// TODO: the go button



conf_t::lang_map_t conf_t::get_lang_map(const std::string &file_name) {
    INCBIN("../exec/loc/en.lang", DefLang);
    long size = (file_name.empty()) ? DefLang_end - DefLang : 0;
    auto file = (file_name.empty()) ? DefLang
                                    : rLoadFile(file_name.c_str(), &size);
    lang_map_t retn;
    if (file) {
        token_t text({}, std::string_view(file, size));
        for (int32_t idx = 0; next_line(text); idx++)
            if (!text.first.empty()) retn[idx] = text.first;
        if (!file_name.empty()) file = (typeof(file))realloc(file, 0);
    }
    return retn;
}

void eProcessMenuItem(MENU *item) {
}

void eExecuteEngine(char *fcnf, char *base, ulong xico, ulong yico,
                    long  xpos, long  ypos, ulong xdim, ulong ydim) {
    T2IV ico{{(decltype(ico.x))xico, (decltype(ico.y))yico}};
    T4IV scr{{(decltype(scr.x))(xpos), (decltype(scr.y))(ypos),
              (decltype(scr.z))(xdim - xpos), (decltype(scr.w))(ydim - ypos)}};
    engine_t engc(fcnf, base, ico, scr);

    engc.main_loop();
}
