#ifndef EXEC_ENGINE_HPP
#define EXEC_ENGINE_HPP

#include "exec_common.hpp"
#include "exec_window.hpp"



class library_t;
class sprite_bank_t;

// engine data (client side)
class engine_t : public no_copy_t {
private:
    static const std::unordered_map<
        size_t, std::pair<string_view_t, conf_t::flags_t>> all_flags;

    ENGD *engd_; // rendering engine data
    uint32_t *seed_; // random number generator seed
    string_t *cfnm_; // main configuration file path
    conf_t cdef_; // default configuration
    conf_t cini_; // initial configuration read at the start
    uint16_t runs_; // current run count between updates
    float tacc_; // partial timestamp accumulator
    conf_t ccur_; // current configuration
    main_window_t mctl_; // main window
    options_window_t octl_; // options window
    T2IV tray_; // tray icon dimensions
    T4IV area_; // drawing area position and dimensions
    uint64_t tcur_; // current, dilation-adjusted timestamp
    uint64_t tpre_; // previous raw timestamp

    std::unordered_map<lib_id_t, std::unique_ptr<library_t>> libs_;

    std::unique_ptr<sprite_bank_t> bank_; // the sprites, for one run only

    static conf_t get_def_conf(string_view_t base);

    void save_ini_conf() const;

    static conf_t load_ini_conf(const conf_t &def, string_view_t cfnm);

    static int16_t fixup_min(conf_t::spin_t &spin, const conf_t::spin_t &def) {
        auto retn = spin.min();
        spin.set_min(def.min());
        return retn;
    }

    void build_library_structure(string_view_t base);

    bool load_selection(const selection_t &sel, bool show_progress);

    static uint32_t update_frame(ENGD *engd, T4FV **data, uint32_t *size,
            uint64_t time, intptr_t user, uint32_t attr,
            int32_t xptr, int32_t yptr, int32_t isel);
    static uint32_t update_flags(ENGD *engd, intptr_t user, uint32_t flgs);

    // the argument is sprite_bank_t::action_t in disguise
    void menu_handler_internal(uint32_t a);

public:
    ~engine_t();
    engine_t(string_view_t fcnf, string_view_t base,
            const T2IV tray, const T4IV area);

    void display_loop();
    void main_loop();

    // local menu handlers, ITEM->DATA is the engine
    static void remove_one_handler(MENU *item);
    static void remove_all_handler(MENU *item);
    static void sleep_one_handler(MENU *item);
    static void sleep_all_handler(MENU *item);
    static void control_p1_handler(MENU *item);
    static void control_p2_handler(MENU *item);
    static void options_handler(MENU *item);
};

#endif // EXEC_ENGINE_HPP
