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



const std::unordered_map<std::string, conf_t::flags_t> engine_t::all_flags = {
#define CASE(what) {#what, conf_t::what}
    CASE(gpu), CASE(opaque), CASE(draw), CASE(show),
    CASE(wpbo), CASE(wbgra), CASE(wregion),
    CASE(speech), CASE(cspeech), CASE(topmost), CASE(hover),
    CASE(update), CASE(filters), CASE(effects), CASE(exact),
    CASE(copies), CASE(randomsel), CASE(interaction),
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
    RNG_Free(&seed_);
}

engine_t::engine_t(string_view_t fcnf, string_view_t base,
        const T2IV tray, const T4IV area)
: engd_(init_engd())
, seed_(init_seed())
, cfnm_((fcnf.size) ? concat_path({to_string(fcnf, 'N'), DEF_CORE})
                    : std::string())
, cdef_(get_def_conf(base))
, cini_(load_ini_conf(cdef_, cfnm_))
, runs_(fixup_min(cini_.nrun, cdef_.nrun)) // do not move past ccur_()
, tacc_{}
, ccur_(cini_)
, mctl_(engd_, *this, ccur_, cini_, cdef_)
, octl_(mctl_.get_id(), ccur_, cini_, cdef_)
, tray_(tray)
, area_(area)
, tcur_{}
, tpre_{} {
    mctl_.set_options_window(octl_.get_id()); // link options window to main
    build_library_structure(cini_.base); // read configuration, load everything
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
    retn.base = to_string(base, 'N');
    retn.flgs = conf_t::show | conf_t::draw | conf_t::gpu | conf_t::hover
              | conf_t::interaction | conf_t::effects | conf_t::speech
              | conf_t::cspeech;
    retn.lang_map = conf_t::get_lang_map({});
    retn.nrun = conf_t::spin_t(  5,    0,  1000);
    retn.nsca = conf_t::spin_t(100,   25,   300);
    retn.ndil = conf_t::spin_t(100,   10,  1000);
    retn.nspk = conf_t::spin_t( 50,    0,   100);
    retn.ncdr = conf_t::spin_t(  0,    0,  1000);
    retn.spec = conf_t::spin_t(  0, -100,   100);
    retn.rgpu = conf_t::spin_t(  0,    0, 30000);
    return retn;
}

void engine_t::save_ini_conf() const {
    #define header(name) header_((const char[]){name,0})
    auto header_ = [](const char *h) { return DEF_CRLF + std::string(h); };
    auto svalue = [](const std::string &v) { return DEF_TSEP + v; };
    auto ivalue = [&](int v) { return svalue(std::to_string(v)); };
    std::string retn;
    retn = header(CFG_FLDR)
         + svalue((ccur_.base != cdef_.base) ? ccur_.base : std::string());
    retn += header(CFG_LGUI) + svalue(ccur_.lang);
    retn += header(CFG_RUNS) + ivalue(ccur_.nrun.get()) + ivalue(runs_);
    retn += header(CFG_SCAL) + ivalue(ccur_.nsca.get());
    retn += header(CFG_TDIL) + ivalue(ccur_.ndil.get());
    retn += header(CFG_RSAY) + ivalue(ccur_.nspk.get());
    retn += header(CFG_PCDR) + ivalue(ccur_.ncdr.get());
    retn += header(CFG_SPEC) + ivalue(ccur_.spec.get());
    retn += header(CFG_RGPU) + ivalue(ccur_.rgpu.get());
    auto render_flags = header(CFG_RNDR), general_flags = header(CFG_FLGS);
    for (auto &f : all_flags) {
        assert(f.second & (conf_t::render_mask | conf_t::general_mask));
        auto &target = (f.second & conf_t::render_mask) ? render_flags
                                                        : general_flags;
        if ((ccur_.flgs & f.second)) target += svalue(f.first);
    }
    retn = retn.substr(1) + render_flags + general_flags;
    rSaveFile(cfnm_.c_str(), retn.c_str(), retn.size());
    #undef header
}

conf_t engine_t::load_ini_conf(const conf_t &def, const std::string &cfnm) {
    auto process_spin = [](tokenizer_t *line, const conf_t::spin_t &def) {
        return conf_t::spin_t(
                process_float(line, def.get()), def.min(), def.max());
    };
    auto render = def.flgs & conf_t::render_mask;
    auto general = def.flgs & conf_t::general_mask;
    conf_t retn;
    retn.base = def.base;
    if (auto file = (!cfnm.empty()) ? rLoadFile(cfnm.c_str(), nullptr)
                                    : nullptr) {
        printf("Reading config from '%s'...\n", cfnm.c_str());
        auto line = local_make_tokenizer((string_view_t){});
        auto text = local_make_tokenizer((string_view_t){strlen(file), file});
        while (next_line(text)) {
            next_token(line, get_token(text), DEF_CMNT, DEF_TSEP, DEF_QUOT);
            switch (str_hash(get_token(line), 'L')) {
                default: break;
                case CHR_HASH(L, CFG_LGUI):
                    retn.lang = to_string(get_remainder(line), 'N');
                    retn.lang_map = conf_t::get_lang_map(retn.lang);
                    if (retn.lang_map.empty()) retn.lang = {};
                    break;
                case CHR_HASH(L, CFG_FLDR):
                    next_token(
                            line, get_remainder(line), 0, DEF_TSEP, DEF_QUOT);
                    if (!get_token(line).size) break;
                    retn.base = to_string(get_token(line), 'N');
                    break;
                case CHR_HASH(L, CFG_RUNS): {
                    // a little hack: set min to equal the run count
                    // (see fixup_min() in constructor for more info)
                    int16_t max = process_float(line, def.nrun.get());
                    int16_t runs = process_float(line, def.nrun.min()) + 1;
                    max = std::clamp(max, def.nrun.min(), def.nrun.max());
                    runs = std::clamp(runs, def.nrun.min(), max);
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
                        render |= process_map(line, all_flags, {})
                            & conf_t::render_mask;
                    break;
                case CHR_HASH(L, CFG_FLGS):
                    general = {};
                    while (!is_empty(line))
                        general |= process_map(line, all_flags, {})
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

void engine_t::build_library_structure(const std::string &base) {
    size_t last_category = 0;
    std::vector<std::pair<lib_id_t, conf_t::categories_t>> ctg;
    std::unordered_map<std::string, size_t> ctg_map;
    bhv_id_map_t bhv_id_map;

    std::vector<library_t::input_t> ins;
    const auto path = concat_path({base, DEF_FLDR, "Ponies"});
    for (bool synced = false; ; ccur_.flgs &= ~conf_t::update, synced = true) {
        if (!(ccur_.flgs & conf_t::update)) {
            const intptr_t find = rFindMake(path.c_str());
            while (char *file = rFindFile(find)) {
                ins.emplace_back(library_t::input_t(path, file));
                free(file);
            }
            if (!ins.empty() || synced) break;
        }
        mctl_.toggle_visibility(true); // show the progress bar
        try_get_from_github(mctl_, "DPE", "RoosterDragon", "Desktop-Ponies",
                "master", "Content", concat_path({base, DEF_FLDR}).c_str());
    }
    for (auto &i : ins) {
        // construct the behaviour ID descriptors and extract categories
        auto ib = bhv_id_map.emplace(to_string(to_view(i.name), 'L'),
                                     library_t::build_bhv_id_desc(i));
        assert_and_discard(ib.second, ib); // make sure the library is unique
        conf_t::categories_t all_categories;
        for (auto &c : i.categories) {
            if (auto ic = find_in_map(ctg_map, c)) {
                all_categories.add(*ic);
            } else {
                all_categories.add(last_category);
                ctg_map[c] = last_category++;
                mctl_.add_category(c);
            }
        }
        ctg.emplace_back(str_hash(to_view(i.name), 'L'),
                         std::move(all_categories));
    }
    for (auto &i : ins) {
        // initialize the libraries
        printf("%s\n", i.name.c_str());
        const lib_id_t hash = str_hash(to_view(i.name), 'L');
        libs_.emplace(hash, std::make_unique<library_t>(
                    hash, concat_path({path, i.name}), i, bhv_id_map));
        // process follow targets (nothing to do beside report missing ones)
        for (auto &b : i.behaviours)
            if (!b.follow_target.empty())
                if (!find_in_map(bhv_id_map, b.follow_target))
                    printf("[%s] WARNING, invalid follow target name in '%s'\n",
                            i.name.c_str(), b.name.c_str());
    }

    // order the libraries by name
    using ctg_val_t = decltype(ctg)::value_type;
    auto names = [&](ctg_val_t &a, ctg_val_t &b) {
        auto lib_a = find_in_map(libs_, a.first);
        auto lib_b = find_in_map(libs_, b.first);
        return (*lib_a)->name() < (*lib_b)->name();
    };
    std::sort(ctg.begin(), ctg.end(), names);

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

