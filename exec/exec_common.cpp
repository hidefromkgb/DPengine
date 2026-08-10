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



conf_t::lang_map_t conf_t::get_lang_map(const std::string_view &file) {
    lang_map_t retn;
    int32_t idx = -1; // first iteration spent on filling token_t
    for (token_t text({}, file); !is_empty(text);
            text = next_token(text.second, 0, DEF_CRLF, 0), idx++)
        if (!text.first.empty()) {
            if (text.first.back() == DEF_LFCR)
                text.first.remove_suffix(sizeof(DEF_LFCR));
            retn[idx] = text.first;
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
