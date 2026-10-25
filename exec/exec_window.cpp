#include <cassert>

#include "exec_engine.hpp"
#include "exec_prng.hpp"
#include "exec_window.hpp"



enum {
/// /// /// /// /// /// /// /// /// localized text constants
/*  Remove character             */ TXT_CDEL = 0,
/*  Remove all similar           */ TXT_ADEL,
/*  Sleep / wake up              */ TXT_CSLP,
/*  Sleep / wake up all similar  */ TXT_ASLP,
/*  Take control: Player 1       */ TXT_TPL1,
/*  Take control: Player 2       */ TXT_TPL2,
/*  More options...              */ TXT_OPTS,

/*  [ Desktop Ponies Engine ]    */ TXT_HEAD,
/*  OS specific options          */ TXT_SPEC,
/*  Disable transparency         */ TXT_OPAQ,
/*  Play animation               */ TXT_DRAW,
/*  Show window                  */ TXT_SHOW,
/*  Exit                         */ TXT_EXIT,
/*  Use GPU for drawing          */ TXT_RGPU,
/*  [ none ]                     */ TXT_NONE,
/*  [ default ]                  */ TXT_DFLT,

/*  Show console                 */ TXT_CONS,
/*  Use regions                  */ TXT_IRGN,
/*  Enable BGRA                  */ TXT_IBGR,
/*  Enable pixel buffers         */ TXT_IPBO,
/*  Useless on full opacity!     */ TXT_UOFO,
/*  Useless without GPU!         */ TXT_UWGL,
/*  Cannot initialize GPU!       */ TXT_CIGL,
/*  The animation base <...>     */ TXT_CTUP,
/*  Internet connection failure  */ TXT_INET,
/*  Failed to create directory   */ TXT_FDIR,
/*  Update                       */ TXT_CCUP,

/*  Desktop Ponies               */ TXT_CAPT,
/*  Enable filters               */ TXT_FLTR,
/*  Exact matching               */ TXT_EXAC,
/*  [At least one:]              */ TXT_OGRP,
/*  [All at once:]               */ TXT_AGRP,
/*  Random selection:            */ TXT_SRND,
/*  Group selection:             */ TXT_SGRP,
/*  Add                          */ TXT_BADD,
/*  Copies                       */ TXT_BDUP,
/*  Selected:                    */ TXT_SELE,
/*  Loaded:                      */ TXT_LOAD,
/*  Updated:                     */ TXT_UPTO,
/*  GO!                          */ TXT_GOGO,

/*  Update on next run           */ TXT_UONR,
/*  Always on top                */ TXT_ETOP,
/*  Enable effects               */ TXT_EEFF,
/*  Enable interactions          */ TXT_EINT,
/*  Enable speech                */ TXT_ESPK,
/*  Enable colored speech        */ TXT_ECLR,
/*  React to cursor hover        */ TXT_ERCH,

/*   runs between updates        */ TXT_RUNS,
/*   % base scaling factor       */ TXT_SCAL,
/*   % time dilation factor      */ TXT_TDIL,
/*   % random speech chance      */ TXT_RSPK,
/*   pix. cursor dodge radius    */ TXT_PCDR,

/*  Choose...                    */ TXT_CHOO,
/*  Reload                       */ TXT_RELO,
/*  Reset                        */ TXT_RESE,
/*  GUI language: English        */ TXT_LGUI,
/*  Animation base directory:    */ TXT_BDIR,
/*  Moving the animation base    */ TXT_BMOV,
/*  Confirm saving the <...>     */ TXT_BSAV,
/*  On refusal, the source <...> */ TXT_BDEL,
/*  Failed to move the <...>     */ TXT_BERR,
/*  OK                           */ TXT_BYES,
/*  Cancel                       */ TXT_BNAY,
};

std::vector<CTRL> main_window_t::get_template(
        engine_t &engcref, const conf_t &conf) {
    auto engc = (intptr_t)&engcref;
    auto here = (intptr_t)this;
    std::array tmpl = {
    CTRL{nullptr, here, TXT_CAPT, FSW_SIZE | FCT_WNDW,  1,  1,  1,  1, FC2E},
    CTRL{nullptr, intptr_t(conf_t::filters),
                        TXT_FLTR,            FCT_CBOX,  0,  0, 19,  2, FC2E},
    CTRL{nullptr, intptr_t(conf_t::exact),
                        TXT_EXAC, FCP_VERT | FCT_CBOX,  0,  0, 19,  2, FC2E},
    CTRL{nullptr, here, TXT_OGRP, FCP_VERT | FCT_LIST,  0,  0, 19, 16, FC2E},
    CTRL{nullptr, here, TXT_SGRP, FCP_VERT | FCT_TEXT,  0,  1, 19,  2, FC2E},
    CTRL{nullptr, intptr_t(&conf.spec),
                        TXT_SPEC, FCP_VERT | FCT_SPIN,  0,  0,  9,  3, FC2E},
    CTRL{nullptr, here, TXT_BADD, FCP_BOTH | FCT_BUTN,  1, -3,  9,  3, FC2E},
    CTRL{nullptr, intptr_t(conf_t::randomsel),
             TXT_SRND, FSX_LEFT | FCP_VERT | FCT_CBOX,  0,  1, 19,  2, FC2E},
    CTRL{nullptr, intptr_t(&conf.rgpu),
                        TXT_RGPU, FCP_VERT | FCT_SPIN,  0,  0,  9,  3, FC2E},
    CTRL{nullptr, intptr_t(conf_t::copies),
                        TXT_BDUP, FCP_BOTH | FCT_CBOX,  1, -3,  9,  3, FC2E},
    CTRL{nullptr, here, TXT_SELE, FCP_VERT | FCT_PBAR,  0,  1, 19,  3, FC2E},
    CTRL{nullptr,    0, TXT_OPTS, FCP_VERT | FCT_BUTN,  0,  1,  9,  6, FC2E},
    CTRL{nullptr, engc, TXT_GOGO, FCP_BOTH | FCT_BUTN
    /* ^--- engine_t reference above!!! */ | FSB_DFLT,  1, -6,  9,  6, FC2E},
    CTRL{nullptr, here, TXT_HEAD, FCP_HORZ | FCT_SBOX,  0,  0, 41, 43, FC2E},
    };
    static_assert(tmpl.size() - 1 == MCT__MAX);
    return std::vector(tmpl.begin(), tmpl.end());
}

std::vector<CTRL> options_window_t::get_template(
        CTRL *prevptr, const conf_t &conf) {
    auto here = (intptr_t)this;
    std::array tmpl = {
    CTRL{prevptr, here, TXT_OPTS,            FCT_WNDW,  1,  1,  1,  1, FC2E},

    CTRL{nullptr, intptr_t(conf_t::update),
                        TXT_UONR,            FCT_CBOX,  0,  0, 18,  2, FC2E},
    CTRL{nullptr, intptr_t(conf_t::topmost),
                        TXT_ETOP, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
    CTRL{nullptr, intptr_t(conf_t::effects),
                        TXT_EEFF, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
    CTRL{nullptr, intptr_t(conf_t::interaction),
                        TXT_EINT, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
    CTRL{nullptr, intptr_t(conf_t::speech),
                        TXT_ESPK, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
    CTRL{nullptr, intptr_t(conf_t::cspeech),
                        TXT_ECLR, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
    CTRL{nullptr, intptr_t(conf_t::hover),
                        TXT_ERCH, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},

    CTRL{nullptr, intptr_t(&conf.nrun),
                        TXT_RUNS,            FCT_SPIN, 19,  0,  8,  3, FC2E},
    CTRL{nullptr, here, TXT_RUNS, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},
    CTRL{nullptr, intptr_t(&conf.nsca),
                        TXT_SCAL, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2E},
    CTRL{nullptr, here, TXT_SCAL, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},
    CTRL{nullptr, intptr_t(&conf.ndil),
                        TXT_TDIL, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2E},
    CTRL{nullptr, here, TXT_TDIL, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},
    CTRL{nullptr, intptr_t(&conf.nspk),
                        TXT_RSPK, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2E},
    CTRL{nullptr, here, TXT_RSPK, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},
    CTRL{nullptr, intptr_t(&conf.ncdr),
                        TXT_PCDR, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2E},
    CTRL{nullptr, here, TXT_PCDR, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},

    CTRL{nullptr, here, TXT_OPTS, FCP_VERT | FCT_TEXT
                                           | FST_SUNK,  0,  1, 49, -2, FC2E},

    CTRL{nullptr, here, TXT_LGUI, FCP_VERT | FCT_TEXT,  0,  0, 18,  3, FC2E},
    CTRL{nullptr, here, TXT_CHOO, FCP_BOTH | FCT_BUTN,  1, -3, 10,  3, FC2E},
    CTRL{nullptr, here, TXT_RELO, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
    CTRL{nullptr, here, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
    CTRL{nullptr, here, TXT_DFLT, FCP_VERT | FCT_TEXT
                                           | FST_CNTR,  0,  0, 49,  2, FC2E},

    CTRL{nullptr, here, TXT_OPTS, FCP_VERT | FCT_TEXT
                                           | FST_SUNK,  0,  1, 49, -2, FC2E},

    CTRL{nullptr, here, TXT_BDIR, FCP_VERT | FCT_TEXT,  0,  0, 18,  3, FC2E},
    CTRL{nullptr, here, TXT_CHOO, FCP_BOTH | FCT_BUTN,  1, -3, 10,  3, FC2E},
    CTRL{nullptr, here, TXT_RELO, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
    CTRL{nullptr, here, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
    CTRL{nullptr, here, TXT_DFLT, FCP_VERT | FCT_TEXT
                                           | FST_CNTR,  0,  0, 49,  2, FC2E},

    CTRL{nullptr, here, TXT_OPTS, FCP_VERT | FCT_TEXT
                                           | FST_SUNK,  0,  1, 49, -2, FC2E},

    CTRL{nullptr, here, TXT_RELO, FCP_VERT | FCT_BUTN, 29,  0, 10,  3, FC2E},
    CTRL{nullptr, here, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
    };
    static_assert(tmpl.size() - 1 == OCT__MAX);
    return std::vector(tmpl.begin(), tmpl.end());
}


    
window_t::window_t(std::vector<CTRL> controls) : visible_(false) {
    controls_ = std::move(controls);
    assert(!controls_.empty() && (get_type(get_root()) == FCT_WNDW));
    // creating the main window
    rMakeControl(&get_root(), nullptr, nullptr);

    long xmax = 0, ymax = 0, xoff = 0, yoff = 0;
    for (size_t indx = 1; indx < controls_.size(); indx++) {
        get(indx).prev = &get_root();
        rMakeControl(&get(indx), &xoff, &yoff);
        xmax = (xmax > xoff) ? xmax : xoff;
        ymax = (ymax > yoff) ? ymax : yoff;
    }
    // resizing and showing the window
    FCTL fc2e = nullptr;
    std::swap(fc2e, get_root().fc2e); // no size request forwarding for now
    RUN_FE2C(get_root(), MSG_WSZC, (uint16_t)xmax | ((uint32_t)ymax << 16));
    std::swap(fc2e, get_root().fc2e);
}

CTRL &window_t::get(int32_t ctl) {
    assert((ctl >= 0) && (size_t(ctl) < controls_.size()));
    return controls_[ctl];
}

void window_t::set_control_text(const std::string &str, size_t ctl) {
    assert(ctl < size());
    const auto type = get_type(get(ctl));
    if ((type == FCT_WNDW) || (type == FCT_LIST) || (type == FCT_PBAR)
    ||  (type == FCT_BUTN) || (type == FCT_CBOX)
    || ((type == FCT_TEXT) && !(get(ctl).flgs & FST_SUNK)))
        RUN_FE2C(get(ctl), MSG__TXT, intptr_t(str.c_str()));
}

void main_window_t::relocalize() {
    conf_window_t::relocalize();
    for (size_t i = 0; i < size(); i++) {
        try_update_checkbox(get(i));
        try_update_spinner(get(i));
    }
    auto text = (conf_.flgs & conf_t::exact) ? TXT_AGRP : TXT_OGRP;
    set_control_text_stock(text, MCT_OGRP);
    global_menu_.reload(get_global_template());
    local_menu_.reload(get_local_template());
}

void main_window_t::try_update_checkbox(CTRL &c, int flag) {
    if (conf_window_t::try_update_checkbox(c, flag)) {
        if (c.uuid == TXT_SRND) {
            auto w = (main_window_t*)get_root(c).data;
            RUN_FE2C(w->get(MCT_RGPU), MSG__ENB, flag);
            RUN_FE2C(w->get(MCT_BDUP), MSG__ENB, flag);
        } else if (c.uuid == TXT_FLTR) {
            auto w = (main_window_t*)get_root(c).data;
            RUN_FE2C(w->get(MCT_EXAC), MSG__ENB, flag);
            RUN_FE2C(w->get(MCT_OGRP), MSG__ENB, flag);
            w->categorize_previews();
        } else if (c.uuid == TXT_EXAC) {
            auto w = (main_window_t*)get_root(c).data;
            w->set_control_text_stock((flag) ? TXT_AGRP : TXT_OGRP, MCT_OGRP);
            w->categorize_previews();
        }
    }
}

intptr_t main_window_t::FC2EI(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (get_type(*ctrl)) {
        case FCT_IBOX:
            if (cmsg == MSG_IFRM)
                cEngineCallback((ENGD*)ctrl->data, ECB_DRAW, data);
            break;

        case FCT_SPIN:
            if (cmsg == MSG_NSET) {
                auto w = (main_window_t*)get_root(*ctrl).data;
                w->preview_stats_.move(update_spinner(*ctrl, data));
                w->set_progress_select(w->preview_stats_.get(),
                        w->preview_stats_.max());
            }
            break;
    }
    return 0;
}

void options_window_t::try_update_checkbox(CTRL &c, int flag) {
    if (conf_window_t::try_update_checkbox(c, flag)) {
        if (c.uuid == TXT_ESPK) {
            auto w = (options_window_t*)get_root(c).data;
            RUN_FE2C(w->get(OCT_ECLR), MSG__ENB, flag);
            RUN_FE2C(w->get(OCT_NSPK), MSG__ENB, flag);
            RUN_FE2C(w->get(OCT_TSPK), MSG__ENB, flag);
        } else if (c.uuid == TXT_ERCH) {
            auto w = (options_window_t*)get_root(c).data;
            RUN_FE2C(w->get(OCT_NCDR), MSG__ENB, flag);
            RUN_FE2C(w->get(OCT_TCDR), MSG__ENB, flag);
        }
    }
}

void options_window_t::maybe_set_control_text(
        const std::string &path, size_t ctl) {
    if (!path.empty()) {
        set_control_text(path, ctl);
    } else {
        set_control_text_stock(TXT_DFLT, ctl);
    }
}

intptr_t options_window_t::FC2E(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (get_type(*ctrl)) {
        case FCT_WNDW:
            if (cmsg == MSG_WEND) RUN_FE2C(*ctrl, MSG__SHW, 0);
            break;

        case FCT_CBOX:
            if (cmsg == MSG_BCLK) try_update_checkbox(*ctrl, !!data);
            break;

        case FCT_SPIN:
            if (cmsg == MSG_NSET) try_update_spinner(*ctrl, data, false);
            break;

        case FCT_BUTN: {
            if (cmsg != MSG_BCLK) break;
            auto w = (options_window_t*)ctrl->data;
            switch (ctrl - &w->get_root()) {
                case OCT_BCHO: {
                    auto path = (!w->conf_.base.empty()) ? w->conf_.base
                                                         : std::string();
                    auto base = rChooseDir(ctrl, path.c_str());
                    if (!base) return 0;
                    w->conf_.base = base;
                    free(base);
                    // TODO: add some checks to verify that the new dir is ok?
                    break;
                }
                case OCT_LCHO: {
                    auto path = (!w->conf_.lang.empty()) ? w->conf_.lang
                                                         : std::string();
                    auto lang = rChooseFile(ctrl, "lang", path.c_str());
                    if (!lang) return 0;
                    w->conf_.lang = lang;
                    free(lang);
                    w->conf_.lang_map = conf_t::get_lang_map(w->conf_.lang);
                    break;
                }
                case OCT_BREL: w->conf_.base = w->ini_conf_.base; break;
                case OCT_BRES: w->conf_.base = w->def_conf_.base; break;
                case OCT_LREL: w->conf_.lang_map = w->ini_conf_.lang_map;
                               w->conf_.lang = w->ini_conf_.lang; break;
                case OCT_LRES: w->conf_.lang_map = w->def_conf_.lang_map;
                               w->conf_.lang = w->def_conf_.lang; break;
                case OCT_FREL: w->conf_ = w->ini_conf_; break;
                case OCT_FRES: w->conf_ = w->def_conf_; break;
                default: assert(false); break; // no buttons except those above
            }
            w->relocalize();
            break;
        }
    }
    return 0;
}

selection_t main_window_t::collect_selection(uint32_t *seed) const {
    std::vector<uint32_t> rand_count(previews_.size(), 0);
    if (conf_.flgs & conf_t::randomsel)
        if (auto rand = conf_.rgpu.get()) {
            std::vector<size_t> pool;
            for (size_t iter = 0; iter < previews_.size(); iter++)
                if (previews_[iter]->is_eligible()) pool.emplace_back(iter);
            for (auto left = pool.size(); left && (rand > 0); rand--) {
                auto indx = RNG_LoadBounded(seed, left);
                rand_count[pool[indx]]++;
                if (!(conf_.flgs & conf_t::copies))
                    if (indx < --left) pool[indx] = pool[left];
            }
        }
    selection_t retn;
    for (size_t iter = 0; iter < previews_.size(); iter++)
        if (auto count = previews_[iter]->count() + rand_count[iter])
            retn.emplace_back(previews_[iter]->library(), count);
    return retn;
}

intptr_t main_window_t::FC2E(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (get_type(*ctrl)) {
        case FCT_WNDW:
            if ((cmsg == MSG__TXT) && !data) {
                ((main_window_t*)ctrl->data)->relocalize();
            } else if (cmsg == MSG_WSZC) {
                auto w = (main_window_t*)ctrl->data;
                if (size_t(MCT_CHAR) < w->size())
                    RUN_FE2C(w->get(MCT_CHAR), cmsg, data);
            } else if (cmsg == MSG_WEND) {
/*
                auto w = (main_window_t*)ctrl->data;
                char *fptr, *file, *temp;
                // trying to write the animation base to its new location
                if (!w->conf_.base.empty()) {
                    fptr = strdup(engc->ccur.base);
                    file = Concatenate(0, engc->cini.base, DEF_DSEP, DEF_FLDR);
                    temp = Concatenate(0, engc->tran[TXT_BSAV],
                                          "\n\n", file, "\n==>\n",
                                          fptr, "\n\n", engc->tran[TXT_BDEL]);
                    if (strcmp(engc->cini.base, engc->ccur.base)) {
                        if (!rMessage(temp, engc->tran[TXT_BMOV],
                                            engc->tran[TXT_BYES],
                                            engc->tran[TXT_BNAY])) {
                            free_(fptr);
                            fptr = 0;
                        }
                        if (!rMoveDir(file, fptr)) {
                            free_(temp);
                            temp = Concatenate(0, engc->tran[TXT_BERR],
                                                  "\n\n", file, "\n==>\n",
                                                  (fptr) ? fptr : "[X]");
                            rMessage(temp, engc->tran[TXT_BMOV],
                                           engc->tran[TXT_BYES], 0);
                        }
                    }
                    free_(temp);
                    free_(fptr);
                    free_(file);
                }
//*/
                return 1;
            }
            break;

        case FCT_CBOX:
            if (cmsg == MSG_BCLK) try_update_checkbox(*ctrl, !!data);
            break;

        case FCT_SPIN:
            if (cmsg == MSG_NSET) try_update_spinner(*ctrl, data, false);
            break;

        case FCT_LIST:
            if ((cmsg == MSG_LGST) || (cmsg == MSG_LSST)) {
                auto w = (main_window_t*)ctrl->data;
                auto &categories = (w->conf_.flgs & conf_t::exact)
                        ? w->conf_.ctg_exact
                        : w->conf_.ctg_nonex;
                if (cmsg == MSG_LGST) {
                    return categories.match(conf_t::categories_t(data), false);
                } else {
                    bool retn = categories.match(
                            conf_t::categories_t(data >> 1), false);
                    if (data & 1) {
                        categories.add(data >> 1);
                    } else {
                        categories.remove(data >> 1);
                    }
                    w->categorize_previews();
                    return retn;
                }
            }
            break;

        case FCT_SBOX:
            if (cmsg == MSG_SGIP) {
                ((main_window_t*)ctrl->data)->display_previews(data);
            } else if (cmsg == MSG_SSID) {
                return ((main_window_t*)ctrl->data)->rearrange_previews(
                        {{(uint16_t)data, (uint16_t)(data >> 16)}});
            }
            break;

        case FCT_BUTN:
            if (cmsg != MSG_BCLK) break;
            if (ctrl->uuid == TXT_OPTS) {
                if (auto opts = (CTRL*)ctrl->data) RUN_FE2C(*opts, MSG__SHW, 1);
            } else if (ctrl->uuid == TXT_BADD) {
                auto w = (main_window_t*)ctrl->data;
                auto spec = w->conf_.spec.get();
                for (auto &p : w->previews_)
                    if (int total = p->update_spinner_relative(spec)) {
                        w->preview_stats_.move(total);
                        w->set_progress_select(w->preview_stats_.get(),
                                w->preview_stats_.max());
                    }
            } else if (ctrl->uuid == TXT_GOGO) {
                ((engine_t*)ctrl->data)->display_loop();
            }
            break;
    }
    return 0;
}

void main_window_t::init_preview(const unit_t &preview,
        conf_t::categories_t categories, const std::string &name,
        library_t &lib) {
    previews_.emplace_back(std::make_unique<preview_t>(&get(MCT_CHAR),
                previews_.size(), get_min_preview_size(), FC2EI,
                std::move(categories), engd_, preview, name, lib));
}

void main_window_t::categorize_previews() {
    int16_t count = 0, total = 0;
    const bool filters = conf_.flgs & conf_t::filters;
    const bool exact = !filters || (conf_.flgs & conf_t::exact);
    const auto &ctg = (filters) ? (exact) ? conf_.ctg_exact : conf_.ctg_nonex
                                : conf_t::categories_t{};
    for (auto &p : previews_)
        if (p->categorize(ctg, exact)) {
            count += !!p->count();
            total++;
        }
    preview_stats_.set_max(total);
    preview_stats_.set(count);
    set_progress_select(preview_stats_.get(), preview_stats_.max());
    RUN_FE2C(get(MCT_CHAR), MSG_WSZC, 0);
}

void main_window_t::finalize_previews() {
    static const std::string_view spaces("    ");
    float inv_space_width = float(spaces.size())
                          / get_string_dims(get_root(), spaces.data()).x;

    for (auto &p : previews_)
        p->finalize(inv_space_width);
    categorize_previews();
}

void main_window_t::display_previews(int32_t y_scroll) {
    auto metrics = get_scrollbox_metrics();
    for (auto &p : previews_)
        p->actualize(metrics.v, y_scroll);
}

size_t main_window_t::rearrange_previews(T2IV preview_area) {
    constexpr int32_t xysp = 8;
    std::vector<T2IV> rows(1, {{}}); // x = last index + 1, y = max row height
    for (auto xmax = xysp; rows.back().x < (decltype(T2IV::x))previews_.size();
            rows.back().x++) {
        if (!previews_[rows.back().x]->is_eligible()) continue;
        auto size = previews_[rows.back().x]->get_size();
        if (rows.back().y && (xmax + size.x + xysp > preview_area.x)) {
            xmax = xysp + size.x + xysp;
            rows.emplace_back(T2IV{{rows.back().x, size.y}});
        } else {
            xmax += size.x + xysp;
            rows.back().y = std::max(rows.back().y, size.y);
        }
    }
    T2IV here = {{xysp, -xysp}};
    for (auto row = 0; row < (decltype(row))rows.size(); row++) {
        here = {{xysp, here.y + xysp + rows[row].y}};
        for (auto p = (row) ? rows[row - 1].x : 0; p < rows[row].x; p++) {
            if (!previews_[p]->is_eligible()) continue;
            previews_[p]->set_pos(here);
            here.x += previews_[p]->get_size().x + xysp;
        }
    }
    return std::max(here.y, preview_area.y);
}

void main_window_t::update_previews(intptr_t data, uint64_t time) {
    auto w = (main_window_t*)data;
    if (!w->is_visible()) return; // window hidden when the engine is active
    for (auto &p : w->previews_)
        p->render(time);
}

void main_window_t::preview_t::finalize(float inv_space_width) {
    if ((flags_ & finalized) || !unit_.is_ready(false)) return;

    auto dims = unit_.dims(false);
    size_.x = std::max(size_.x, dims.x);
    size_.y = std::max(size_.y, dims.y);

    constexpr int leeway = 6;
    auto name_size = get_string_dims(get_root(charname_), name_.data());
    if (name_size.x + leeway > size_.x) { // consider scrolling
        auto maybe_name_len
                  = std::ceil(inv_space_width * (name_size.x - size_.x));
        if (maybe_name_len < 8) { // too tight, widen the box instead
            size_.x = name_size.x + leeway;
        } else {
            name_len_ = maybe_name_len + 1;
            std::string padding(name_len_, ' ');
            name_ = padding + name_ + padding;
        }
    }
    imagebox_.xdim = -size_.x;
    imagebox_.ydim = -size_.y;

    charname_.xdim = -size_.x;
    charname_.ydim = -size_.z;

    spinner_.xdim = -size_.x;
    spinner_.ydim = -size_.w;

    flags_ |= finalized;
}

void main_window_t::preview_t::render(int64_t time) {
    if (!imagebox_.fe2c || !charname_.fe2c) return;
    if (auto uuid = unit_.advance(false, time, frame_time_, frame_iter_)) {
        if (!(flags_ & visible)) return;
        RUN_FE2C(imagebox_, MSG_IFRM, (frame_iter_ & 0x3FF) | (uuid << 10));
    }
    auto str = name_scroll_advance(time);
    if (!str.empty() && (flags_ & visible))
        RUN_FE2C(charname_, MSG__TXT, (intptr_t)str.data());
}

void main_window_t::set_progress_load(uint32_t frac, uint32_t full) {
    set_progress(TXT_LOAD, frac, full);
}

void main_window_t::set_progress_select(uint32_t frac, uint32_t full) {
    set_progress(TXT_SELE, frac, full);
}

intptr_t main_window_t::prepare_tray_icon(T2IV tray) {
    INCBIN("../exec/icon.gif", MainIcon); (void)(MainIcon_end);

    AINF icon = {};
    cEngineCallback(engd_, ECB_LOAD, ~0);
    cEngineLoadAnimAsync(
            engd_, &icon, (uint8_t*)"/Icon/", MainIcon, ELA_LOAD, 0);
    cEngineCallback(engd_, ECB_LOAD, 0);
    icon.fcnt = 0;
    icon.xdim = tray.x;
    icon.ydim = tray.y;
    icon.time = (decltype(icon.time))realloc(
            nullptr, sizeof(*icon.time) * icon.xdim * icon.ydim);
    for (size_t iter = icon.xdim * icon.ydim; iter; icon.time[--iter] = 0);
    cEngineCallback(engd_, ECB_DRAW, (intptr_t)&icon);
    auto retn = rMakeTrayIcon(global_menu_.get(),
            get_text_stock(TXT_HEAD)->data(), icon.time, icon.xdim, icon.ydim);
    free(icon.time);
    toggle_visibility(false);
    return retn;
}

void main_window_t::discard_tray_icon(intptr_t icon) {
    rFreeTrayIcon(icon);
    toggle_visibility(true);
}

void main_window_t::sync_menu_flags(uint32_t flgs) {
    // the engine calls this at every start and restart of its main loop, and
    // it is the only thing that ever puts a checkmark on the render toggles:
    // their templates below carry MFL_CCHK but never MFL_VCHK so the initial
    // state comes from whatever the engine reports back here
    for (auto item = global_menu_.get(); item->text; item++) {
        uint32_t flag = 0;
        switch (item->uuid) {
            default: continue;
            case TXT_RGPU: flag = COM_RGPU; break;
            case TXT_OPAQ: flag = COM_OPAQ; break;
            case TXT_DRAW: flag = COM_DRAW; break;
            case TXT_SHOW: flag = COM_SHOW; break;
        }
        // a flag the engine dropped while its item was checked means the
        // engine could not do what was asked; the only such case is the GPU
        if (!(flgs & flag) && (item->flgs & MFL_VCHK)
        &&  (flag == (uint32_t)COM_RGPU))
            rMessage(get_text_stock(TXT_CIGL)->data(), nullptr,
                    get_text_stock(TXT_BYES)->data(), nullptr);
        item->flgs = (flgs & flag) ? (item->flgs | MFL_VCHK)
                                   : (item->flgs & ~MFL_VCHK);
    }
}

void eProcessMenuItem(MENU *item) {
    MENU *indx;

    if (item->flgs & MFL_CCHK) {
        if (item->flgs & MFL_RCHK & ~MFL_CCHK) {
            if (item->flgs & MFL_VCHK)
                return;
            item->flgs |= MFL_VCHK;
            indx = item;
            while ((--indx)->flgs & MFL_RCHK & ~MFL_CCHK)
                indx->flgs &= ~MFL_VCHK;
            indx = item;
            while ((++indx)->flgs & MFL_RCHK & ~MFL_CCHK)
                indx->flgs &= ~MFL_VCHK;
        }
        else
            item->flgs ^= MFL_VCHK;
    }
    if (!item->chld && item->func)
        item->func(item);
}

void main_window_t::global_menu_handler(MENU *item) {
    uint32_t indx, flgs = 0;
    ENGD *engd;

    switch (item->uuid) {
        case TXT_RGPU:
            engd = (ENGD*)item->data;
            cEngineCallback(engd, ECB_GFLG, (intptr_t)&flgs);
            if (item->flgs & MFL_VCHK)
                flgs |= COM_RGPU;
            else
                flgs &= ~COM_RGPU;
            cEngineCallback(engd, ECB_SFLG, flgs);
            break;

        case TXT_SHOW:
        case TXT_DRAW:
        case TXT_OPAQ:
            engd = (ENGD*)item->data;
            indx = (item->uuid != TXT_OPAQ)? (item->uuid != TXT_DRAW)?
                    COM_SHOW : COM_DRAW : COM_OPAQ;
            cEngineCallback(engd, ECB_GFLG, (intptr_t)&flgs);
            if (item->flgs & MFL_VCHK)
                flgs |= indx;
            else
                flgs &= ~indx;
            cEngineCallback(engd, ECB_SFLG, flgs);
            break;

        case TXT_EXIT:
            cEngineCallback((ENGD*)item->data, ECB_QUIT, ~0);
            break;
    }
}

std::vector<MENU> main_window_t::get_global_template() {
    return std::vector<MENU>{
        {.text = get_text_stock(TXT_HEAD)->data(), .flgs = MFL_GRAY},
        {.text = ""},
        {.text = get_text_stock(TXT_SPEC)->data()},
        {.text = get_text_stock(TXT_RGPU)->data(), .data = (intptr_t)engd_,
         .flgs = MFL_CCHK, .uuid = TXT_RGPU, .func = global_menu_handler},
        {.text = get_text_stock(TXT_OPAQ)->data(), .data = (intptr_t)engd_,
         .flgs = MFL_CCHK, .uuid = TXT_OPAQ, .func = global_menu_handler},
        {.text = get_text_stock(TXT_DRAW)->data(), .data = (intptr_t)engd_,
         .flgs = MFL_CCHK, .uuid = TXT_DRAW, .func = global_menu_handler},
        {.text = get_text_stock(TXT_SHOW)->data(), .data = (intptr_t)engd_,
         .flgs = MFL_CCHK, .uuid = TXT_SHOW, .func = global_menu_handler},
        {.text = ""},
        {.text = get_text_stock(TXT_EXIT)->data(), .data = (intptr_t)engd_,
                           .uuid = TXT_EXIT, .func = global_menu_handler}
    };
}

std::vector<MENU> main_window_t::get_local_template() {
    using e = engine_t;
    return std::vector<MENU>{
        {.text = "", .flgs = MFL_GRAY}, // the title, see open_local_menu()
        {.text = ""},
        {.text = get_text_stock(TXT_CDEL)->data(), .data = (intptr_t)engc_,
                           .uuid = TXT_CDEL, .func = e::remove_one_handler},
        {.text = get_text_stock(TXT_ADEL)->data(), .data = (intptr_t)engc_,
                           .uuid = TXT_ADEL, .func = e::remove_all_handler},
        {.text = ""},
        {.text = get_text_stock(TXT_CSLP)->data(), .data = (intptr_t)engc_,
         .flgs = MFL_CCHK, .uuid = TXT_CSLP, .func = e::sleep_one_handler},
        {.text = get_text_stock(TXT_ASLP)->data(), .data = (intptr_t)engc_,
                           .uuid = TXT_ASLP, .func = e::sleep_all_handler},
        {.text = ""},
        {.text = get_text_stock(TXT_TPL1)->data(), .data = (intptr_t)engc_,
         .flgs = MFL_CCHK, .uuid = TXT_TPL1, .func = e::control_p1_handler},
        {.text = get_text_stock(TXT_TPL2)->data(), .data = (intptr_t)engc_,
         .flgs = MFL_CCHK, .uuid = TXT_TPL2, .func = e::control_p2_handler},
        {.text = ""},
        {.text = get_text_stock(TXT_OPTS)->data(), .data = (intptr_t)engc_,
                           .uuid = TXT_OPTS, .func = e::options_handler},
        {.text = get_text_stock(TXT_EXIT)->data(), .data = (intptr_t)engd_,
                           .uuid = TXT_EXIT, .func = global_menu_handler}
    };
}

void main_window_t::open_local_menu(
        const std::string &name, bool asleep, uint32_t player) {
    local_menu_title_ = "[ " + name + " ]";
    auto menu = local_menu_.get();
    menu[0].text = local_menu_title_.c_str();
    for (auto item = menu; item->text; item++) {
        bool check;
        switch (item->uuid) {
            case TXT_CSLP: check = asleep; break;
            case TXT_TPL1: check = (player == 1); break;
            case TXT_TPL2: check = (player == 2); break;
            default: continue;
        }
        item->flgs = (check) ? (item->flgs | MFL_VCHK)
                             : (item->flgs & ~MFL_VCHK);
    }
    rOpenContextMenu(menu);
}

MENU *main_window_t::menu_t::get() {
    assert(!items_.empty());
    return items_.data();
}
