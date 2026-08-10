#include <cassert>

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
/*  Enable speech                */ TXT_ESAY,
/*  Enable colored speech        */ TXT_ECLR,
/*  React to cursor hover        */ TXT_ERCH,

/*   runs between updates        */ TXT_RUNS,
/*   % base scaling factor       */ TXT_SCAL,
/*   % time dilation factor      */ TXT_TDIL,
/*   % random speech chance      */ TXT_RSAY,
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
        intptr_t here, const conf_t &conf) {
    return {
        {nullptr, here, TXT_CAPT, FSW_SIZE | FCT_WNDW,  1,  1,  1,  1, FC2E},
        {nullptr, intptr_t(conf_t::filters),
                        TXT_FLTR,            FCT_CBOX,  0,  0, 19,  2, FC2E},
        {nullptr, intptr_t(conf_t::exact),
                        TXT_EXAC, FCP_VERT | FCT_CBOX,  0,  0, 19,  2, FC2E},
        {nullptr, here, TXT_OGRP, FCP_VERT | FCT_LIST,  0,  0, 19, 16, FC2E},
        {nullptr, here, TXT_SGRP, FCP_VERT | FCT_TEXT,  0,  1, 19,  2, FC2E},
        {nullptr, intptr_t(&conf.spec),
                        TXT_SPEC, FCP_VERT | FCT_SPIN,  0,  0,  9,  3, FC2E},
        {nullptr, here, TXT_BADD, FCP_BOTH | FCT_BUTN,  1, -3,  9,  3, FC2E},
        {nullptr, intptr_t(conf_t::randomsel),
             TXT_SRND, FSX_LEFT | FCP_VERT | FCT_CBOX,  0,  1, 19,  2, FC2E},
        {nullptr, intptr_t(&conf.rgpu),
                        TXT_RGPU, FCP_VERT | FCT_SPIN,  0,  0,  9,  3, FC2E},
        {nullptr, intptr_t(conf_t::copies),
                        TXT_BDUP, FCP_BOTH | FCT_CBOX,  1, -3,  9,  3, FC2E},
        {nullptr, here, TXT_SELE, FCP_VERT | FCT_PBAR,  0,  1, 19,  3, FC2E},
        {nullptr,    0, TXT_OPTS, FCP_VERT | FCT_BUTN,  0,  1,  9,  6, FC2E},
        {nullptr, here, TXT_GOGO, FCP_BOTH | FCT_BUTN
                                           | FSB_DFLT,  1, -6,  9,  6, FC2E},
        {nullptr, here, TXT_HEAD, FCP_HORZ | FCT_SBOX,  0,  0, 41, 43, FC2E},
    };
}

std::vector<CTRL> options_window_t::get_template(
        intptr_t prev, intptr_t here, const conf_t &conf) {
    auto prevptr = (CTRL*)prev;
    return {
        {prevptr, here, TXT_OPTS,            FCT_WNDW,  1,  1,  1,  1, FC2E},

        {nullptr, intptr_t(conf_t::update),
                        TXT_UONR,            FCT_CBOX,  0,  0, 18,  2, FC2E},
        {nullptr, intptr_t(conf_t::topmost),
                        TXT_ETOP, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
        {nullptr, intptr_t(conf_t::effects),
                        TXT_EEFF, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
        {nullptr, intptr_t(conf_t::interaction),
                        TXT_EINT, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
        {nullptr, intptr_t(conf_t::speech),
                        TXT_ESAY, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
        {nullptr, intptr_t(conf_t::cspeech),
                        TXT_ECLR, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
        {nullptr, intptr_t(conf_t::hover),
                        TXT_ERCH, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},

        {nullptr, intptr_t(&conf.nrun),
                        TXT_RUNS,            FCT_SPIN, 19,  0,  8,  3, FC2E},
        {nullptr, here, TXT_RUNS, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},
        {nullptr, intptr_t(&conf.nsca),
                        TXT_SCAL, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2E},
        {nullptr, here, TXT_SCAL, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},
        {nullptr, intptr_t(&conf.ndil),
                        TXT_TDIL, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2E},
        {nullptr, here, TXT_TDIL, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},
        {nullptr, intptr_t(&conf.nsay),
                        TXT_RSAY, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2E},
        {nullptr, here, TXT_RSAY, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},
        {nullptr, intptr_t(&conf.ncdr),
                        TXT_PCDR, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2E},
        {nullptr, here, TXT_PCDR, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},

        {nullptr, here, TXT_OPTS, FCP_VERT | FCT_TEXT
                                           | FST_SUNK,  0,  1, 49, -2, FC2E},

        {nullptr, here, TXT_LGUI, FCP_VERT | FCT_TEXT,  0,  0, 18,  3, FC2E},
        {nullptr, here, TXT_CHOO, FCP_BOTH | FCT_BUTN,  1, -3, 10,  3, FC2E},
        {nullptr, here, TXT_RELO, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
        {nullptr, here, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
        {nullptr, here, TXT_DFLT, FCP_VERT | FCT_TEXT
                                           | FST_CNTR,  0,  0, 49,  2, FC2E},

        {nullptr, here, TXT_OPTS, FCP_VERT | FCT_TEXT
                                           | FST_SUNK,  0,  1, 49, -2, FC2E},

        {nullptr, here, TXT_BDIR, FCP_VERT | FCT_TEXT,  0,  0, 18,  3, FC2E},
        {nullptr, here, TXT_CHOO, FCP_BOTH | FCT_BUTN,  1, -3, 10,  3, FC2E},
        {nullptr, here, TXT_RELO, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
        {nullptr, here, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
        {nullptr, here, TXT_DFLT, FCP_VERT | FCT_TEXT
                                           | FST_CNTR,  0,  0, 49,  2, FC2E},

        {nullptr, here, TXT_OPTS, FCP_VERT | FCT_TEXT
                                           | FST_SUNK,  0,  1, 49, -2, FC2E},

        {nullptr, here, TXT_RELO, FCP_VERT | FCT_BUTN, 29,  0, 10,  3, FC2E},
        {nullptr, here, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
    };
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
    RUN_FE2C(get_root(), MSG_WSZC, (uint16_t)xmax | ((uint32_t)ymax << 16));
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
        if (c.uuid == TXT_ESAY) {
            auto w = (options_window_t*)get_root(c).data;
            RUN_FE2C(w->get(OCT_ECLR), MSG__ENB, flag);
            RUN_FE2C(w->get(OCT_NSAY), MSG__ENB, flag);
            RUN_FE2C(w->get(OCT_TSAY), MSG__ENB, flag);
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
            if (ctrl == &w->get(OCT_BCHO)) {
                auto path = (!w->conf_.base.empty()) ? w->conf_.base : "";
                auto base = rChooseDir(ctrl, path.c_str());
                if (!base) break;
                w->conf_.base = base;
                base = (typeof(base))realloc(base, 0);
                // TODO: add some checks to verify that the new dir is ok?
            } else if (ctrl == &w->get(OCT_LCHO)) {
                auto path = (!w->conf_.lang.empty()) ? w->conf_.lang : "";
                auto lang = rChooseFile(ctrl, "lang", path.c_str());
                if (!lang) break;
                w->conf_.lang = lang;
                lang = (typeof(lang))realloc(lang, 0);
                long size = 0;
                if (auto file = rLoadFile(w->conf_.lang.c_str(), &size)) {
                    std::string_view str(file, size);
                    w->conf_.lang_map = conf_t::get_lang_map(str);
                    file = (typeof(file))realloc(file, 0);
                } else {
                    w->conf_.lang_map.clear();
                }
            } else if (ctrl == &w->get(OCT_BREL)) {
                w->conf_.base = w->ini_conf_.base;
            } else if (ctrl == &w->get(OCT_BRES)) {
                w->conf_.base = w->def_conf_.base;
            } else if (ctrl == &w->get(OCT_LREL)) {
                w->conf_.lang = w->ini_conf_.lang;
                w->conf_.lang_map = w->ini_conf_.lang_map;
            } else if (ctrl == &w->get(OCT_LRES)) {
                w->conf_.lang = w->def_conf_.lang;
                w->conf_.lang_map = w->def_conf_.lang_map;
            } else if (ctrl == &w->get(OCT_FREL)) {
                w->conf_ = w->ini_conf_;
            } else if (ctrl == &w->get(OCT_FRES)) {
                w->conf_ = w->def_conf_;
            } else {
                assert(false); // no buttons except those above
            }
            w->relocalize();
            break;
        }
    }
    return 0;
}

intptr_t main_window_t::FC2E(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    //INCBIN("../exec/icon.gif", MainIcon);

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
/*
                LINF *libs;
                auto engc = (main_window_t*)ctrl->data;
                AINF igif = {};
                intptr_t icon;
                long ilen, *irnd, *iput;

                irnd = calloc(engc->libs.size, sizeof(*irnd));
                iput = calloc(engc->libs.size, sizeof(*iput));

                // checking if random choice is enabled
                if (engc->conf_.flgs & conf_t::randomsel) {
                    // indexing random-capable libraries
                    for (ilen = icon = 0; icon < engc->libs.size; icon++)
                        if (engc->libs._[icon].wctx.icnt == 0)
                            iput[ilen++] = icon;
                    // iterating over the requested random sprites count
                    for (icon = RUN_FE2C(engc->MCT_RGPU, MSG_NGET, 0);
                        (icon > 0) && ilen; icon--) {
                        irnd[iput[data = RNG_Load(engc->seed) % ilen]]++;
                        auto copies = engc->conf_.flgs & conf_t::copies;
                        if (!copies && (data < --ilen))
                            iput[data] = iput[ilen];
                    }
                    // finally, adding the computed random values to ICNTs
                    for (icon = 0; icon < engc->libs.size; icon++)
                        engc->libs._[icon].wctx.icnt += irnd[icon];
                }
                // is there anything selected? let's find out
                for (icon = 0; icon < engc->libs.size; icon++)
                    if (engc->libs._[icon].wctx.icnt > 0)
                        break;
                if (icon >= engc->libs.size) {
                    // [TODO:] do we need to show messages here?
//                    rMessage("Nothing selected!", 0, 0);
                    free_(irnd);
                    free_(iput);
                    break;
                }
                // counting the number of selected libraries
                for (cmsg = icon = 0; icon < engc->libs.size; icon++)
                    if (engc->libs._[icon].wctx.icnt > 0)
                        cmsg++;
                SetProgress(engc, TXT_LOAD, 0, cmsg);

                cEngineCallback(engc->engd, ECB_LOAD, ~0);
                for (data = icon = 0; icon < engc->libs.size; icon++)
                    if (engc->libs._[icon].wctx.icnt > 0) {
                        LoadLib(&engc->libs._[icon], engc->engd);
                        SetProgress(engc, TXT_LOAD, ++data, cmsg);
                        RUN_FE2C(engc->MCT_SELE, MSG_PPOS, data);
                    }
                cEngineLoadAnimAsync(engc->engd, &igif, (uint8_t*)"/Icon/",
                                     MainIcon, ELA_LOAD, 0);
                cEngineCallback(engc->engd, ECB_LOAD, 0);

                // [TODO:] adapt for CTR_V_FLTR
                for (libs = engc->libs._, icon = 0; icon < engc->libs.size;
                        icon++)
                    if (AppendSpriteArr(&engc->libs._[icon], engc)) {
                        // revert random ICNT
                        engc->libs._[icon].wctx.icnt -= irnd[icon];
                        if (++libs <= &engc->libs._[icon])
                            CTR_ASSIGN(libs[-1], engc->libs._[icon]);
                    }
                free_(irnd);
                free_(iput);
                CTR_V_MGET(engc->libs, libs - engc->libs._, 1);
                igif.fcnt = 0;
                igif.xdim = engc->idim.x;
                igif.ydim = engc->idim.y;
                igif.time = calloc(sizeof(*igif.time), igif.xdim * igif.ydim);
                cEngineCallback(engc->engd, ECB_DRAW, (intptr_t)&igif);
                icon = rMakeTrayIcon(engc->mctx, engc->tran[TXT_HEAD],
                                     igif.time, igif.xdim, igif.ydim);
                free_(igif.time);
                RUN_FE2C(engc->MCT_CAPT, MSG__SHW, 0);
                engc->pcur = engc->povr = 0;
                engc->data = (engc->pmax) ? calloc(engc->pmax,
                                                  sizeof(*engc->data)) : 0;
                cEngineRunMainLoop(engc->engd, engc->dpos.x, engc->dpos.y,
                                   engc->dims.x + engc->dpos.x,
                                   engc->dims.y + engc->dpos.y, engc->ftmp,
                                   FRM_WAIT, (intptr_t)engc, eUpdFrame,
                                   eUpdFlags);
                cEngineCallback(engc->engd, ECB_GFLG, (intptr_t)&engc->ftmp);
                free_(engc->data);

                rFreeTrayIcon(icon);
                for (icon = 0; icon < engc->pcnt; icon++)
                    free_(engc->parr[icon]);
                free_(engc->parr);
                engc->parr = 0;
                engc->pmax = engc->pcnt = 0;

                // finally showing the window
                RecountSelectedLibs(engc);
                RUN_FE2C(engc->MCT_CAPT, MSG__SHW, ~0);
//*/
            }
            break;
    }
    return 0;
}

void main_window_t::init_preview(ENGD *engd, const unit_t &preview,
        conf_t::categories_t categories, const std::string &name, lib_id_t id) {
    previews_.emplace_back(std::make_unique<preview_t>(&get(MCT_CHAR),
                previews_.size(), get_min_preview_size(), FC2EI,
                std::move(categories), engd, preview, name, id));
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
