#ifndef EXEC_WINDOW_HPP
#define EXEC_WINDOW_HPP



#include "exec_common.hpp"
#include "exec_unit.hpp"



// FE2C / FC2E helper macros
#define RUN_FE2C(trgt, cmsg, data) (trgt).fe2c(&(trgt), (cmsg), (data))
#define RUN_FC2E(trgt, cmsg, data) (trgt).fc2e(&(trgt), (cmsg), (data))

class window_t {
private:
    bool visible_;
    std::vector<CTRL> controls_;

public:
    intptr_t get_id() { return intptr_t(&get(0)); }
    ~window_t() { for (auto &c : controls_) rFreeControl(&c); }

    void toggle_visibility(bool visible) {
        visible_ = visible;
        RUN_FE2C(get_root(), MSG__SHW, visible);
    }
    bool is_visible() const { return visible_; }

protected:
    size_t size() const { return controls_.size(); }
    CTRL &get(int32_t ctl);
    CTRL &get_root() { return get(0); }
    static int32_t get_type(const CTRL &ctrl) { return ctrl.flgs & FCT_TTTT; }

    window_t(std::vector<CTRL> controls);

    static CTRL *get_parent(CTRL &child) { return child.prev; }

    static CTRL &get_root(CTRL &child) {
        auto root = &child;
        for (auto curr = get_parent(*root);
                curr && (get_type(*root) != FCT_WNDW); curr = get_parent(*root))
            root = curr;
        return *root;
    }

    void set_control_text(const std::string &str, size_t ctl);
};

class conf_window_t : public window_t {
protected:
    const conf_t &def_conf_;
    const conf_t &ini_conf_;
    conf_t &conf_;

    static bool try_update_checkbox(CTRL &c, int &flag) {
        if (get_type(c) != FCT_CBOX) return false;
        auto &flags = ((conf_window_t*)get_root(c).data)->conf_.flgs;
        switch (flag) {
            default:
                if (!c.fe2c) return false;
                flag = !!(flags & conf_t::flags_t(c.data));
                RUN_FE2C(c, MSG_BCLK, flag);
                return true;
            case 0:
                flags &= ~conf_t::flags_t(c.data);
                return true;
            case 1:
                flags |= conf_t::flags_t(c.data);
                return true;
        }
    }

    static bool try_update_spinner(CTRL &c, int16_t val = 0, bool init = true) {
        if (get_type(c) != FCT_SPIN) return false;
        auto spin = (conf_t::spin_t*)c.data;
        if (!init) {
            val = spin->set(val);
        } else if (c.fe2c) {
            bool set_dims = (val == 0);
            val = spin->get();
            if (set_dims)
                RUN_FE2C(c, MSG_NDIM,
                        ((uint32_t)spin->max() << 16) | (uint16_t)spin->min());
            RUN_FE2C(c, MSG_NSET, val);
        } else {
            return false;
        }
        return true;
    }

    static int16_t update_spinner(CTRL &c, int16_t val) {
        if (get_type(c) != FCT_SPIN) return 0;
        auto spin = (conf_t::spin_t*)c.data;
        auto old_empty = !spin->get();
        spin->set(val);
        return old_empty - !spin->get();
    }

    const std::string *get_text_stock(int32_t idx) const {
        if (auto in = find_in_map(conf_.lang_map, idx)) {
            return in;
        } else if (auto id = find_in_map(def_conf_.lang_map, idx)) {
            return id;
        }
        return nullptr;
    }

    void set_control_text_stock(int32_t idx, size_t ctl) {
        set_control_text(*get_text_stock(idx), ctl);
    }

    void relocalize() {
        for (size_t i = 0; i < size(); i++)
            set_control_text_stock(get(i).uuid, i);
        if (auto prev = get_parent(get_root())) // propagate to parent windows
            RUN_FC2E(*prev, MSG__TXT, 0);
    }

    conf_window_t(std::vector<CTRL> controls, conf_t &conf,
            const conf_t &ini_conf, const conf_t &def_conf)
    : window_t(std::move(controls))
    , def_conf_(def_conf)
    , ini_conf_(ini_conf)
    , conf_(conf) {}
};

class main_window_t : public conf_window_t {
private:
    enum elements_t { MCT_CAPT = 0,
        MCT_FLTR, MCT_EXAC, MCT_OGRP, MCT_SGRP, MCT_SPEC, MCT_BADD, MCT_SRND,
        MCT_RGPU, MCT_BDUP, MCT_SELE, MCT_OPTS, MCT_GOGO, MCT_CHAR,
        MCT__MAX = MCT_CHAR
    };
    class preview_t : public no_copy_t {
    private:
        // !eligible && !visible: doesn't match selection, not visible
        // !eligible &&  visible: doesn't match selection, visible: need to hide
        //  eligible && !visible: matches selection, not visible: need to show
        //  eligible &&  visible: matches selection, visible
        enum flags_t : uint32_t { GEN_FLAGS(finalized, eligible, visible) };
        std::string name_; // padded name
        int32_t name_len_;
        int32_t name_iter_;
        int64_t name_time_;
        lib_id_t lib_id_;
        conf_t::spin_t count_;
        flags_t flags_;
        T2IV lower_left_;
        T4IV size_;
        conf_t::categories_t categories_;
        const unit_t &unit_;
        uint32_t frame_iter_;
        int64_t frame_time_;
        CTRL imagebox_; // image box control to preview the sprite
        CTRL charname_; // character name just below the image box
        CTRL spinner_;  // spin control to set ICNT

        bool within_scroll(int32_t y_visible, int32_t y_scroll) const {
            auto ymax = lower_left_.y - y_scroll;
            return (ymax >= 0) && (ymax - get_size().y < y_visible);
        }
        std::string_view name_scroll_advance(int64_t time) {
            #define ABS(v) (((v) < 0) ? -(v) : (v))
            constexpr int FRM_TEXT = FRM_WAIT * 1.5f;
            if (!name_len_ || (time <= name_time_ + FRM_TEXT)) return {};
            name_time_ = time + FRM_TEXT;
            const auto true_len = name_.size() - name_len_ * 2;
            name_[true_len + name_len_ + ABS(name_iter_)] = ' ';
            if (++name_iter_ >= name_len_) name_iter_ = -name_iter_;
            name_[true_len + name_len_ + ABS(name_iter_)] = 0;
            return {name_.data() + ABS(name_iter_), true_len + name_len_};
            #undef ABS
        }
        void toggle_visibility(bool visible) {
            if (imagebox_.fe2c) RUN_FE2C(imagebox_, MSG__SHW, visible);
            if (charname_.fe2c) RUN_FE2C(charname_, MSG__SHW, visible);
            if (spinner_.fe2c) RUN_FE2C(spinner_, MSG__SHW, visible);
        }

    public:
        ~preview_t() {
            if (imagebox_.fe2c) rFreeControl(&imagebox_);
            if (charname_.fe2c) rFreeControl(&charname_);
            if (spinner_.fe2c) rFreeControl(&spinner_);
        }
        preview_t(CTRL *p, int32_t idx, T4IV size, FCTL fc2e,
                conf_t::categories_t categories, ENGD *engd, const unit_t &unit,
                const std::string &name, lib_id_t lib_id)
        : name_(std::to_string(idx + 1) + ". " + name)
        , name_len_(0)
        , name_iter_(0)
        , name_time_(0)
        , lib_id_(lib_id)
        , count_(0, 0, 30000)
        , flags_{}
        , lower_left_{}
        , size_(size)
        , categories_(std::move(categories))
        , unit_(unit)
        , frame_iter_(0)
        , frame_time_(0) {
            auto engd_ = intptr_t(engd);
            imagebox_ = {p, engd_, idx, FCT_IBOX, 0, 0, size.x, size.y, fc2e};
            charname_
                = {p, engd_, idx, FCT_TEXT | FST_CNTR, 0, 0, size.x, 0, fc2e};
            spinner_
                = {p, intptr_t(&count_), idx, FCT_SPIN, 0, 0, size.x, 0, fc2e};
        }
        bool is_eligible() const { return flags_ & eligible; }
        void set_pos(T2IV lower_left) {
            lower_left_ = lower_left;
            flags_ &= ~visible;
            toggle_visibility(false);
        }
        void finalize(float inv_space_width);
        void render(int64_t time);
        bool categorize(const conf_t::categories_t &c, bool all_at_once) {
            const bool match = categories_.match(c, all_at_once);
            flags_ = (match) ? (flags_ | eligible) : (flags_ & ~eligible);
            return match;
        }
        void actualize(int32_t y_visible, int32_t y_scroll) {
            if (!(flags_ & finalized)) return;
            const bool e = is_eligible();
            if (e && !within_scroll(y_visible, y_scroll)) return;
            if (e != !(flags_ & visible)) return;
            flags_ ^= visible;
            const bool v = (flags_ & visible);
            if (v) {
                if (!imagebox_.fe2c) rMakeControl(&imagebox_, nullptr, nullptr);
                if (!charname_.fe2c) {
                    rMakeControl(&charname_, nullptr, nullptr);
                    RUN_FE2C(charname_, MSG__TXT, intptr_t(name_.c_str()));
                }
                if (!spinner_.fe2c) {
                    rMakeControl(&spinner_, nullptr, nullptr);
                    try_update_spinner(spinner_);
                }
                long pos[2] = {-lower_left_.x, size_.w - lower_left_.y};
                RUN_FE2C(spinner_, MSG__POS, (intptr_t)pos);
                pos[1] += size_.z;
                RUN_FE2C(charname_, MSG__POS, (intptr_t)pos);
                pos[1] += size_.y;
                RUN_FE2C(imagebox_, MSG__POS, (intptr_t)pos);
            }
            toggle_visibility(v);
        }
        T2IV get_size() const {
            return {{size_.x, size_.y + size_.z + size_.w}};
        }
        int16_t count() const { return is_eligible() * count_.get(); }
        int16_t update_spinner_relative(int16_t value) {
            if (!is_eligible()) return 0;
            auto v = conf_window_t::update_spinner(
                    spinner_, count_.get() + value);
            conf_window_t::try_update_spinner(spinner_, 1, true);
            return v;
        }
    };

    conf_t::spin_t preview_stats_; // min = 0, curr = selected, max = total
    std::vector<std::unique_ptr<preview_t>> previews_;

    std::vector<CTRL> get_template(intptr_t here, const conf_t &conf);
    static intptr_t FC2E(CTRL *ctrl, uint32_t cmsg, intptr_t data);
    static intptr_t FC2EI(CTRL *ctrl, uint32_t cmsg, intptr_t data);
    static void try_update_checkbox(CTRL &c, int flag = -1);
    void display_previews(int32_t y_scroll);
    size_t rearrange_previews(T2IV preview_area);

    T2IV get_scrollbox_metrics() { // u = total height, v = window height
        auto &scrollbox = get(MCT_CHAR);
        return {{(decltype(T2IV::x))(RUN_FE2C(scrollbox, MSG_SGTH, 0)),
                 (decltype(T2IV::y))(RUN_FE2C(scrollbox, MSG__GSZ, 0) >> 16)}};
    }

    void set_progress(int32_t text, uint32_t frac, uint32_t full) {
        std::string line = *get_text_stock(text) + " " + std::to_string(frac);
        if (full) line += " / " + std::to_string(full);
        set_control_text(line, MCT_SELE);
        RUN_FE2C(get(MCT_SELE), MSG_PLIM, (full) ? full : 100);
        RUN_FE2C(get(MCT_SELE), MSG_PPOS, (full) ? frac : 0);
    }

    void relocalize();

public:
    main_window_t(conf_t &conf, const conf_t &ini_conf, const conf_t &def_conf)
    : conf_window_t(
            get_template(intptr_t(this), conf), conf, ini_conf, def_conf) {}

    void add_category(const std::string &name) {
        RUN_FE2C(get(MCT_OGRP), MSG_LADD, (intptr_t)name.c_str());
    }

    void set_options_window(intptr_t opt_id) {
        get(MCT_OPTS).data = opt_id;
    }

    T4IV get_min_preview_size() {
        auto sgrp = RUN_FE2C(get(MCT_SGRP), MSG__GSZ, 0);
        auto spec = RUN_FE2C(get(MCT_SPEC), MSG__GSZ, 0);
        return {{uint16_t(spec), uint16_t(spec >> 16),
                uint16_t(sgrp >> 16), uint16_t(spec >> 16)}};
    }

    static T2IV get_string_dims(CTRL &window, const std::string_view &str) {
        AINF atmp{0, 0, 0, 0, (uint32_t*)str.data()};
        auto f = RUN_FE2C(window, MSG_WTGD, (intptr_t)&atmp);
        return {{uint16_t(f), uint16_t(f >> 16)}};
    }

    void set_progress_load(uint32_t frac, uint32_t full);
    void set_progress_select(uint32_t frac, uint32_t full);

    static void update_previews(intptr_t data, uint64_t time);

    void init_preview(ENGD *engd, const unit_t &preview,
            conf_t::categories_t categories, const std::string &name,
            lib_id_t lib_id);

    void categorize_previews();
    void finalize_previews();

    void main_loop(uint32_t fram) {
        rInternalMainLoop(&get_root(), fram, update_previews, intptr_t(this));
    }
};

class options_window_t : public conf_window_t {
private:
    enum elements_t { OCT_OPTS = 0,
        OCT_UONR, OCT_ETOP, OCT_EEFF, OCT_EINT, OCT_ESAY, OCT_ECLR, OCT_ERCH,
        OCT_NRUN, OCT_TRUN, OCT_NSCA, OCT_TSCA, OCT_NDIL, OCT_TDIL, OCT_NSAY,
        OCT_TSAY, OCT_NCDR, OCT_TCDR, OCT_LSEP, OCT_LHDR, OCT_LCHO, OCT_LREL,
        OCT_LRES, OCT_LGUI, OCT_BSEP, OCT_BHDR, OCT_BCHO, OCT_BREL, OCT_BRES,
        OCT_BDIR, OCT_FSEP, OCT_FREL, OCT_FRES,
        OCT__MAX = OCT_FRES
    };
    std::vector<CTRL> get_template(
            intptr_t prev, intptr_t here, const conf_t &conf);

    static intptr_t FC2E(CTRL *ctrl, uint32_t cmsg, intptr_t data);

    static void try_update_checkbox(CTRL &c, int flag = -1);

    void maybe_set_control_text(const std::string &path, size_t ctl);

    void relocalize() {
        conf_window_t::relocalize();
        for (size_t i = 0; i < size(); i++) {
            try_update_checkbox(get(i));
            try_update_spinner(get(i));
        }
        maybe_set_control_text(conf_.lang, OCT_LGUI);
        maybe_set_control_text(conf_.base, OCT_BDIR);
    }

public:
    options_window_t(intptr_t prev, conf_t &conf, const conf_t &ini_conf,
            const conf_t &def_conf)
    : conf_window_t(get_template(prev, intptr_t(this), conf), conf, ini_conf,
            def_conf) {
        relocalize(); // also triggers relocalization of the main window
    }
};

#endif // EXEC_WINDOW_HPP
