#include <algorithm>
#include <cassert>

#include "exec_engine.hpp"
#include "exec_library.hpp"
#include "exec_parser.hpp"
#include "exec_prng.hpp"
#include "exec_sprite.hpp"
#include "exec_web.hpp"



// config file content dir location; intentionally separate from DEF_FLDR
#define CFG_FLDR 'C','o','n','t','e','n','t'
// config file language file location
#define CFG_LGUI 'L','a','n','g','u','a','g','e'
// launches between network updates, then launches since the last update
#define CFG_RUNS 'R','u','n','s','T','i','l','l','U','p','d','a','t','e'
// the amount in % of base to which the sprites are scaled
#define CFG_SCAL 'B','a','s','e','S','c','a','l','e'
// time dilation amount in % of the master timer
#define CFG_TDIL 'T','i','m','e','D','i','l','a','t','i','o','n'
// random speech chance in %: 0 = utter nothing at random, 100 = speak always
#define CFG_RSAY 'R','a','n','d','o','m','S','p','e','e','c','h'
// radius in pixels of a circle around the cursor that everyone tries to avoid
#define CFG_PCDR 'C','u','r','s','o','r','D','o','d','g','e'
// instances added to (if < 0, removed from) everyone in previews on [Add]
#define CFG_SPEC 'G','r','o','u','p','S','e','l','e','c','t','i','o','n'
// the number of randomly picked instances appended to the cast on [GO]
#define CFG_RGPU 'R','a','n','d','o','m','S','e','l','e','c','t','i','o','n'
// rendering flags in conf_t::flgs
#define CFG_RNDR 'R','e','n','d','e','r'
// general flags in conf_t::flgs
#define CFG_FLGS 'F','l','a','g','s'



decltype(engine_t::all_flags) engine_t::all_flags = {
#define CASE(what, ...) \
        {CHR_HASH(L, __VA_ARGS__), {get_imm_view(#what), conf_t::what}}
    CASE(interaction,'i','n','t','e','r','a','c','t','i','o','n'),
    CASE(randomsel,'r','a','n','d','o','m','s','e','l'), CASE(gpu, 'g','p','u'),
    CASE(cspeech,'c','s','p','e','e','c','h'), CASE(hover,'h','o','v','e','r'),
    CASE(topmost,'t','o','p','m','o','s','t'), CASE(exact,'e','x','a','c','t'),
    CASE(wregion,'w','r','e','g','i','o','n'), CASE(wbgra,'w','b','g','r','a'),
    CASE(filters,'f','i','l','t','e','r','s'), CASE(wpbo,'w','p','b','o'),
    CASE(effects,'e','f','f','e','c','t','s'), CASE(draw,'d','r','a','w'),
    CASE(opaque,'o','p','a','q','u','e'), CASE(speech,'s','p','e','e','c','h'),
    CASE(update,'u','p','d','a','t','e'), CASE(copies,'c','o','p','i','e','s'),
    CASE(show,'s','h','o','w'),
#undef CASE
};

static inline ENGD *init_engd() {
    ENGD *engd = nullptr;
    cEngineCallback(0, ECB_INIT, (intptr_t)&engd);
    return engd;
}

static inline uint32_t *init_seed() {
    uint32_t seed = time(nullptr);
    printf("[((RNG))] seed = 0x%08X\n", seed);
    return RNG_Make(seed);
}

engine_t::~engine_t() {
    free_string(cfnm_);
    RNG_Free(&seed_);
}

engine_t::engine_t(string_view_t fcnf, string_view_t base,
        const T2IV tray, const T4IV area)
: engd_(init_engd())
, seed_(init_seed())
, cfnm_((fcnf.size) ? make_string(fcnf, get_imm_view(DEF_DSEP DEF_CORE))
                    : nullptr)
, cdef_(get_def_conf(base))
, cini_(load_ini_conf(cdef_, get_view(cfnm_)))
, runs_(fixup_min(cini_.nrun, cdef_.nrun)) // do not move past ccur_()
, tacc_{}
, ccur_(cini_)
, mctl_(engd_, *this, ccur_, cini_, cdef_)
, octl_(mctl_.get_id(), ccur_, cini_, cdef_)
, tray_(tray)
, area_(area)
, tcur_{}
, tpre_{} {
    conf_t::dup_strings(&ccur_); // CCUR_ was copied from CINI_ shallowly
    mctl_.set_options_window(octl_.get_id()); // link options window to main
    build_library_structure(get_view(cini_.base)); // read conf, load everything
}

bool engine_t::load_selection(const selection_t &sel, bool show_progress) {
    if (sel.empty()) return false;
    cEngineCallback(engd_, ECB_LOAD, ~0); // interrupt to load more animations
    const auto text_fn = mctl_.text_fn();
    for (size_t iter = 0; iter < sel.size(); iter++) {
        sel[iter].first.get().load(engd_, ccur_.flgs & conf_t::speech,
                ccur_.flgs & conf_t::cspeech, text_fn);
        if (show_progress) mctl_.set_progress_load(iter + 1, sel.size());
    }
    cEngineCallback(engd_, ECB_LOAD, 0); // wait for the loading to complete
    for (auto &s : sel) s.first.get().finalize();
    return true;
}

conf_t engine_t::get_def_conf(string_view_t base) {
    conf_t retn;
    retn.base = make_string(base);
    retn.flgs = conf_t::show | conf_t::draw | conf_t::gpu | conf_t::hover
              | conf_t::interaction | conf_t::effects | conf_t::speech
              | conf_t::cspeech;
    retn.lang_map = conf_t::get_lang_map(get_view(retn.lang));
    retn.nrun = conf_t::spin_t(  5,    0,  1000);
    retn.nsca = conf_t::spin_t(100,   25,   300);
    retn.ndil = conf_t::spin_t(100,   10,  1000);
    retn.nspk = conf_t::spin_t( 50,    0,   100);
    retn.ncdr = conf_t::spin_t(  0,    0,  1000);
    retn.spec = conf_t::spin_t(  0, -100,   100);
    retn.rgpu = conf_t::spin_t(  0,    0, 30000);
    return retn;
}

static string_t *hdrs_(
        string_t *str, string_view_t head, size_t n, const string_view_t *v) {
    char tsep = DEF_TSEP, crlf = DEF_CRLF;
    if (head.size) str = append_string(str, {1, &crlf}, head);
    for (size_t i = 0; i < n; i++) str = append_string(str, {1, &tsep}, v[i]);
    return str;
}

static string_t *vals_(string_t *str, size_t n, const int32_t *v) {
    char tsep = DEF_TSEP, temp[16];
    for (size_t i = 0; i < n; i++)
        str = append_string(
                str, {1, &tsep}, {(size_t)sprintf(temp, "%d", v[i]), temp});
    return str;
}

void engine_t::save_ini_conf() const {
    #define _(...) get_imm_view(((const char[]){__VA_ARGS__, 0}))
    #define hdrs(retn, name, ...) hdrs_(retn, (string_view_t) name, \
            sizeof((string_view_t[]){__VA_ARGS__}) / sizeof(string_view_t), \
            (const string_view_t[]){__VA_ARGS__})
    #define vals(retn, ...) vals_(retn, \
            sizeof((int32_t[]){__VA_ARGS__}) / sizeof(int32_t), \
            (const int32_t[]){__VA_ARGS__})
    const auto cur = get_view(ccur_.base), def = get_view(cdef_.base);
    const bool equal = (cur.size == def.size) ? (!cur.size) ? true
                     : !strncmp(cur.str, def.str, cur.size) : false;
    local_string_t *retn = nullptr, *render = nullptr, *general = nullptr;
    retn = hdrs(retn, _(CFG_FLDR), (!equal) ? cur : (string_view_t){});
    retn = hdrs(retn, _(CFG_LGUI), get_view(ccur_.lang));
    retn = hdrs(retn, _(CFG_RUNS)); retn = vals(retn, ccur_.nrun.get(), runs_);
    retn = hdrs(retn, _(CFG_SCAL)); retn = vals(retn, ccur_.nsca.get());
    retn = hdrs(retn, _(CFG_TDIL)); retn = vals(retn, ccur_.ndil.get());
    retn = hdrs(retn, _(CFG_RSAY)); retn = vals(retn, ccur_.nspk.get());
    retn = hdrs(retn, _(CFG_PCDR)); retn = vals(retn, ccur_.ncdr.get());
    retn = hdrs(retn, _(CFG_SPEC)); retn = vals(retn, ccur_.spec.get());
    retn = hdrs(retn, _(CFG_RGPU)); retn = vals(retn, ccur_.rgpu.get());
    render = hdrs(render, _(CFG_RNDR)); general = hdrs(general, _(CFG_FLGS));
    for (auto &f : all_flags) {
        assert(f.second.second & (conf_t::render_mask | conf_t::general_mask));
        string_t **target
                = (f.second.second & conf_t::render_mask) ? &render : &general;
        if ((ccur_.flgs & f.second.second))
            *target = hdrs(*target, {}, f.second.first);
    }
    retn = append_string(retn, get_view(render), get_view(general));
    const auto retn_view = get_view(retn);
    rSaveFile(get_view(cfnm_).str, retn_view.str + 1, retn_view.size - 1);
    #undef vals
    #undef hdrs
    #undef _
}

conf_t engine_t::load_ini_conf(const conf_t &def, string_view_t cfnm) {
    auto process_spin = [](tokenizer_t *line, const conf_t::spin_t &def) {
        return conf_t::spin_t(
                process_float(line, def.get()), def.min(), def.max());
    };
    auto render = def.flgs & conf_t::render_mask;
    auto general = def.flgs & conf_t::general_mask;
    long size = 0;
    conf_t retn;
    retn.base = make_string(get_view(def.base));
    if (auto file = (cfnm.size) ? rLoadFile(cfnm.str, &size) : nullptr) {
        printf("Reading config from '%s'...\n", cfnm.str);
        auto line = local_make_tokenizer({});
        auto text = local_make_tokenizer({(size_t)size, file});
        while (next_line(text)) {
            next_token(line, get_token(text), DEF_CMNT, DEF_TSEP, DEF_QUOT);
            switch (str_view_hash(get_token(line), 'L')) {
                default: break;
                case CHR_HASH(L, CFG_LGUI):
                    free_string(retn.lang);
                    retn.lang = make_string(get_remainder(line));
                    conf_t::free_lang_map(&retn.lang_map);
                    retn.lang_map = conf_t::get_lang_map(get_view(retn.lang));
                    if (retn.lang_map.empty()) {
                        free_string(retn.lang);
                        retn.lang = nullptr;
                    }
                    break;
                case CHR_HASH(L, CFG_FLDR):
                    next_token(
                            line, get_remainder(line), 0, DEF_TSEP, DEF_QUOT);
                    if (!get_token(line).size) break;
                    free_string(retn.base);
                    retn.base = make_string(get_token(line));
                    break;
                case CHR_HASH(L, CFG_RUNS): {
                    // a little hack: set min to equal the run count
                    // (see fixup_min() in constructor for more info)
                    int16_t max = process_float(line, def.nrun.get());
                    int16_t runs = process_float(line, def.nrun.min()) + 1;
                    max = std_clamp(max, def.nrun.min(), def.nrun.max());
                    runs = std_clamp(runs, def.nrun.min(), max);
                    if ((max > def.nrun.min()) && (max == runs)) {
                        retn.flgs |= conf_t::update;
                        runs = def.nrun.min();
                    }
                    retn.nrun = conf_t::spin_t(max, runs, def.nrun.max());
                    break;
                }
                case CHR_HASH(L, CFG_SCAL):
                    retn.nsca = process_spin(line, def.nsca);
                    break;
                case CHR_HASH(L, CFG_TDIL):
                    retn.ndil = process_spin(line, def.ndil);
                    break;
                case CHR_HASH(L, CFG_RSAY):
                    retn.nspk = process_spin(line, def.nspk);
                    break;
                case CHR_HASH(L, CFG_PCDR):
                    retn.ncdr = process_spin(line, def.ncdr);
                    break;
                case CHR_HASH(L, CFG_SPEC):
                    retn.spec = process_spin(line, def.spec);
                    break;
                case CHR_HASH(L, CFG_RGPU):
                    retn.rgpu = process_spin(line, def.rgpu);
                    break;
                case CHR_HASH(L, CFG_RNDR):
                    render = {};
                    while (!is_empty(line))
                        render |= process_map(line, all_flags, {}).second
                            & conf_t::render_mask;
                    break;
                case CHR_HASH(L, CFG_FLGS):
                    general = {};
                    while (!is_empty(line))
                        general |= process_map(line, all_flags, {}).second
                            & conf_t::general_mask;
                    break;
            }
        }
        free(file);
        retn.flgs |= render | general; // keeping the update from CFG_RUNS
    }
    return retn;
}

uint32_t engine_t::update_flags(ENGD *engd, intptr_t user, uint32_t flgs) {
    // the engine reports what it actually managed to do, which is not always
    // what it was asked for; the tray menu's checkmarks come from here
    ((engine_t*)user)->mctl_.sync_menu_flags(flgs);
    return flgs;
}

uint32_t engine_t::update_frame(ENGD *engd, T4FV **data, uint32_t *size,
        uint64_t time, intptr_t user, uint32_t attr,
        int32_t xptr, int32_t yptr, int32_t isel) {
    auto &engc = *(engine_t*)user;
    auto &bank = *engc.bank_;

    // timekeeping (with dilation and partial timestamp accumulator)
    if (!engc.tpre_) engc.tpre_ = time;
    engc.tacc_ += 0.01f * engc.ccur_.ndil.get() * (time - engc.tpre_);
    engc.tpre_ = time;
    engc.tcur_ += (uint64_t)engc.tacc_;
    engc.tacc_ -= (uint64_t)engc.tacc_;

    bank.update(engc.seed_, engc.tcur_, {{{xptr, yptr}}, attr, (uint32_t)isel,
            !!(engc.ccur_.flgs & conf_t::hover), engd}); // animating
    *data = bank.display(); // passing the updated sprites to the renderer
    if (auto chr = bank.menu_request())
        engc.mctl_.open_local_menu(chr->name(), bank.is_asleep(*chr),
                bank.controller(*chr));

    // SIZE is the capacity of the sprite array rather than its length: the GPU
    // renderer is built once, out of whatever the very first frame reports
    // (see MakeRendererOGL() in core/ogl/oglstd.c), so it has to be the
    // maximum for the whole run; reporting 0 is how the engine is told to stop
    *size = bank.capacity();
    return bank.shown();
}

void engine_t::display_loop() {
    static const std::pair<conf_t::flags_t, uint32_t> render_flags[] = {
        {conf_t::gpu,    COM_RGPU}, {conf_t::wbgra,   WIN_IBGR},
        {conf_t::draw,   COM_DRAW}, {conf_t::wpbo,    WIN_IPBO},
        {conf_t::show,   COM_SHOW}, {conf_t::wregion, WIN_IRGN},
        {conf_t::opaque, COM_OPAQ},
    };

    const auto sel = mctl_.collect_selection(seed_);
    if (!load_selection(sel, true)) {
        // [TODO:] not convinced if this is needed; localize if it is
        //rMessage("Nothing selected!", nullptr, "OK", nullptr);
        return;
    }
    bank_ = std::make_unique<sprite_bank_t>(sel, seed_, tcur_,
                T2IV{{area_.xdim, area_.ydim}}, ccur_.flgs & conf_t::effects,
                ccur_.flgs & conf_t::speech, ccur_.nspk);

    uint32_t engd_flags = 0;
    for (auto &f : render_flags)
        if (ccur_.flgs & f.first) engd_flags |= f.second;

    tpre_ = 0;
    const auto icon = mctl_.prepare_tray_icon(tray_);
    cEngineRunMainLoop(engd_, area_.x, area_.y, area_.z + area_.x,
            area_.w + area_.y, engd_flags, FRM_WAIT, (intptr_t)this,
            update_frame, update_flags);
    cEngineCallback(engd_, ECB_GFLG, (intptr_t)&engd_flags);
    mctl_.discard_tray_icon(icon);

    ccur_.flgs &= ~conf_t::render_mask;
    for (auto &f : render_flags)
        if (engd_flags & f.second) ccur_.flgs |= f.first;

    bank_.reset();
}

void engine_t::main_loop() {
    // waiting for the previews to finish loading
    cEngineCallback(engd_, ECB_LOAD, 0);
    cEngineCallback(engd_, ECB_LOAD, ~0);
    for (auto &l : libs_) l.second->finalize();

    // computing preview sizes, since image sizes are now known
    mctl_.finalize_previews();

    do { // computing the colors for colored speech, in parallel
        std::vector<library_t*> distilled_libs;
        distilled_libs.reserve(libs_.size());
        for (auto &l : libs_)
            distilled_libs.emplace_back(l.second.get());
        library_t::extract_speech_colors(engd_, distilled_libs);
    } while (false);

    // starting the GUI loop
    mctl_.main_loop(FRM_WAIT);

    save_ini_conf();
}

void engine_t::menu_handler_internal(uint32_t a) {
    if (bank_) bank_->menu_action(seed_, tcur_, sprite_bank_t::action_t(a));
}

void engine_t::remove_one_handler(MENU *item) {
    ((engine_t*)item->data)->menu_handler_internal(sprite_bank_t::remove_one);
}

void engine_t::remove_all_handler(MENU *item) {
    ((engine_t*)item->data)->menu_handler_internal(sprite_bank_t::remove_all);
}

void engine_t::sleep_one_handler(MENU *item) {
    ((engine_t*)item->data)->menu_handler_internal(sprite_bank_t::sleep_one);
}

void engine_t::sleep_all_handler(MENU *item) {
    ((engine_t*)item->data)->menu_handler_internal(sprite_bank_t::sleep_all);
}

void engine_t::control_p1_handler(MENU *item) {
    ((engine_t*)item->data)->menu_handler_internal(sprite_bank_t::control_p1);
}

void engine_t::control_p2_handler(MENU *item) {
    ((engine_t*)item->data)->menu_handler_internal(sprite_bank_t::control_p2);
}

void engine_t::options_handler(MENU *item) {
}

void engine_t::build_library_structure(string_view_t base) {
    size_t last_category = 0;
    std::vector<std::pair<lib_id_t, conf_t::categories_t>> ctg;
    std::unordered_map<size_t, size_t> ctg_map;
    bhv_id_map_t bhv_id_map;

    std::vector<library_t::input_t> ins;
    local_string_t *path = make_string(
            base, get_imm_view(DEF_DSEP DEF_FLDR DEF_DSEP "Ponies"));
    for (bool synced = false; ; ccur_.flgs &= ~conf_t::update, synced = true) {
        if (!(ccur_.flgs & conf_t::update)) {
            const intptr_t find = rFindMake(get_view(path).str);
            while (char *file = rFindFile(find)) {
                ins.emplace_back(library_t::input_t(
                                get_view(path), get_pchar_view(file)));
                free(file);
            }
            if (!ins.empty() || synced) break;
        }
        mctl_.toggle_visibility(true); // show the progress bar
        local_string_t *webpath
                = make_string(base, get_imm_view(DEF_DSEP DEF_FLDR));
        try_get_from_github(mctl_, "DPE", "RoosterDragon", "Desktop-Ponies",
                "master", "Content", get_view(webpath).str);
    }
    for (auto &i : ins) {
        // construct the behaviour ID descriptors and extract categories
        auto ib = bhv_id_map.emplace(str_hash(i.name, 'L'),
                                     library_t::build_bhv_id_desc(i));
        assert_and_discard(ib.second, ib); // make sure the library is unique
        conf_t::categories_t all_categories;
        for (auto &c : i.categories) {
            auto ctg_hash = str_hash(c, 'L');
            if (auto ic = find_in_map(ctg_map, ctg_hash)) {
                all_categories.add(*ic);
            } else {
                all_categories.add(last_category);
                ctg_map[ctg_hash] = last_category++;
                mctl_.add_category(get_view(c));
            }
        }
        ctg.emplace_back(str_hash(i.name, 'L'), std::move(all_categories));
    }
    for (auto &i : ins) {
        // initialize the libraries
        printf("%s\n", get_view(i.name).str);
        const lib_id_t hash = str_hash(i.name, 'L');
        local_string_t *full
                = make_string(get_view(path), ILV_DSEP, get_view(i.name));
        libs_.emplace(hash, std::make_unique<library_t>(
                    hash, get_view(full), i, bhv_id_map));
        // process follow targets (nothing to do beside report missing ones)
        for (auto &b : i.behaviours)
            if (get_view(b.follow_target).size)
                if (!find_in_map(bhv_id_map, str_hash(b.follow_target, 'L')))
                    printf("[%s] WARNING, invalid follow target name in '%s'\n",
                            get_view(i.name).str, get_view(b.name).str);
    }

    // order the libraries by name
    using ctg_val_t = const decltype(ctg)::value_type;
    auto names = [&](ctg_val_t &a, ctg_val_t &b) {
        auto _a = (*find_in_map(libs_, a.first))->name();
        auto _b = (*find_in_map(libs_, b.first))->name();
        return strncmp(_a.str, _b.str, std_max(_a.size, _b.size)) < 0;
    };
    std::stable_sort(ctg.begin(), ctg.end(), names);

    // show the main window, initialize the previews
    mctl_.toggle_visibility(true);
    RUN_FC2E(*(CTRL*)mctl_.get_id(), MSG__TXT, 0); // relocalize
    for (size_t ctg_idx = 0; ctg_idx < ctg.size(); ctg_idx++) {
        auto &c = ctg[ctg_idx];
        auto &il = *find_in_map(libs_, c.first);
        mctl_.init_preview(
                il->load_preview(engd_), std::move(c.second), il->name(), *il);
        mctl_.set_progress_load(ctg_idx + 1, ctg.size());
    }
}

