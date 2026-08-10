#ifndef EXEC_ENGINE_HPP
#define EXEC_ENGINE_HPP

#include "exec_common.hpp"
#include "exec_library.hpp"
#include "exec_window.hpp"



// engine data (client side)
class engine_t : public no_copy_t {
private:
    static const std::unordered_map<std::string, conf_t::flags_t> all_flags;

    std::string cfnm_; // main configuration file path
    conf_t cdef_; // default configuration
    conf_t cini_; // initial configuration read at the start
    uint32_t runs_; // current run count between updates
    float tacc_; // partial timestamp accumulator
    conf_t ccur_; // current configuration
    main_window_t mctl_; // main window
    options_window_t octl_; // options window
    T2IV tray_; // tray icon dimensions
    T4IV area_; // drawing area position and dimensions
    T3IV ppos_; // mouse pointer position (z = flags)
    uint64_t tcur_; // current, dilation-adjusted timestamp
    uint64_t tpre_; // previous raw timestamp
    std::vector<MENU> mspr_; // per-sprite context menu
    std::vector<MENU> mctx_; // main context menu
    ENGD *engd_;

    std::unordered_map<lib_id_t, std::unique_ptr<library_t>> libs_;

    static conf_t get_def_conf(const std::string_view base);

    void save_ini_conf() const;

    static conf_t load_ini_conf(const conf_t &def, const std::string &cfnm);

    static int16_t fixup_min(conf_t::spin_t &spin, const conf_t::spin_t &def) {
        auto retn = spin.min();
        spin.set_min(def.min());
        return retn;
    }

    void build_library_structure(const std::string &base);

public:
    engine_t(const std::string_view fcnf, const std::string_view base,
            const T2IV tray, const T4IV area);

    const library_t &get_library(lib_id_t lib_id) const;
    void main_loop();
};

#endif // EXEC_ENGINE_HPP
