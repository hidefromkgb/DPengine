#include <cassert>

#include "exec_engine.hpp"
#include "exec_parser.hpp"



// config file content dir location; intentionally separate from DEF_FLDR
#define CFG_FLDR "Content"
// config file language file location
#define CFG_LGUI "Language"
//
#define CFG_RUNS "RunsTillUpdate"
//
#define CFG_SCAL "BaseScale"
//
#define CFG_TDIL "TimeDilation"
//
#define CFG_RSAY "RandomSpeech"
//
#define CFG_PCDR "CursorDodge"
//
#define CFG_SPEC "GroupSelection"
//
#define CFG_RGPU "RandomSelection"
//
#define CFG_RNDR "Render"
//
#define CFG_FLGS "Flags"



const std::unordered_map<std::string, conf_t::flags_t> engine_t::all_flags = {
#define CASE(what) {#what, conf_t::what}
    CASE(gpu), CASE(opaque), CASE(draw), CASE(show),
    CASE(wpbo), CASE(wbgra), CASE(wregion),
    CASE(speech), CASE(cspeech), CASE(topmost), CASE(hover),
    CASE(update), CASE(filters), CASE(effects), CASE(exact),
    CASE(copies), CASE(randomsel), CASE(interaction),
#undef CASE
};

engine_t::engine_t(const std::string_view fcnf, const std::string_view base,
        const T2IV tray, const T4IV area)
: cfnm_((!fcnf.empty()) ? concat_path({std::string(fcnf), DEF_CORE})
                        : std::string())
, cdef_(get_def_conf(base))
, cini_(load_ini_conf(cdef_, cfnm_))
, runs_(fixup_min(cini_.nrun, cdef_.nrun)) // do not move past ccur_()
, tacc_{}
, ccur_(cini_)
, mctl_(ccur_, cini_, cdef_)
, octl_(mctl_.get_id(), ccur_, cini_, cdef_)
, tray_(tray)
, area_(area)
, ppos_{}
, tcur_{}
, tpre_{} {
    mctl_.set_options_window(octl_.get_id()); // link options window to main
    cEngineCallback(0, ECB_INIT, (intptr_t)&engd_); // create rendering engine
    build_library_structure(cini_.base); // read configuration, load everything
}

conf_t engine_t::get_def_conf(const std::string_view base) {
    conf_t retn;
    retn.base = base;
    retn.flgs = conf_t::show | conf_t::draw | conf_t::gpu | conf_t::hover
              | conf_t::interaction | conf_t::effects | conf_t::speech
              | conf_t::cspeech;
    retn.lang_map = conf_t::get_lang_map({});
    retn.nrun = conf_t::spin_t(  5,    0,  1000);
    retn.nsca = conf_t::spin_t(100,   25,   300);
    retn.ndil = conf_t::spin_t(100,   10,  1000);
    retn.nsay = conf_t::spin_t( 50,    0,   100);
    retn.ncdr = conf_t::spin_t(  0,    0,  1000);
    retn.spec = conf_t::spin_t(  0, -100,   100);
    retn.rgpu = conf_t::spin_t(  0,    0, 30000);
    return retn;
}
    
const library_t &engine_t::get_library(lib_id_t lib_id) const {
    auto il = find_in_map(libs_, lib_id);
    assert(il);
    return *il->get();
}

void engine_t::save_ini_conf() const {
    auto header = [](const char *h) { return DEF_CRLF + std::string(h); };
    auto svalue = [](const std::string &v) { return DEF_TSEP + v; };
    auto ivalue = [&](int v) { return svalue(std::to_string(v)); };
    std::string retn;
    retn = header(CFG_FLDR)
         + svalue((ccur_.base != cdef_.base) ? ccur_.base : std::string());
    retn += header(CFG_LGUI) + svalue(ccur_.lang);
    retn += header(CFG_RUNS) + ivalue(ccur_.nrun.get()) + ivalue(runs_);
    retn += header(CFG_SCAL) + ivalue(ccur_.nsca.get());
    retn += header(CFG_TDIL) + ivalue(ccur_.ndil.get());
    retn += header(CFG_RSAY) + ivalue(ccur_.nsay.get());
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
}

conf_t engine_t::load_ini_conf(const conf_t &def, const std::string &cfnm) {
    auto process_spin = [](token_t &line, const conf_t::spin_t &def) {
        return conf_t::spin_t(
                process_float(line, def.get()), def.min(), def.max());
    };
    auto render = def.flgs & conf_t::render_mask;
    auto general = def.flgs & conf_t::general_mask;
    conf_t retn;
    retn.base = def.base;
    if (auto file = (!cfnm.empty())
            ? rLoadFile(cfnm.c_str(), nullptr)
            : nullptr) {
        printf("Reading config from '%s'...\n", cfnm.c_str());
        for (token_t text({}, file); next_line(text); ) {
            auto line = next_token(text.first);
            switch (str_hash(line.first)) {
                default: break;
                case str_hash(CFG_LGUI):
                    retn.lang = line.second;
                    retn.lang_map = conf_t::get_lang_map(retn.lang);
                    if (retn.lang_map.empty()) retn.lang = {};
                    break;
                case str_hash(CFG_FLDR):
                    line = next_token(line.second, 0);
                    if (line.first.empty()) break;
                    retn.base = line.first;
                    break;
                case str_hash(CFG_RUNS): {
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
                case str_hash(CFG_SCAL):
                    retn.nsca = process_spin(line, def.nsca);
                    break;
                case str_hash(CFG_TDIL):
                    retn.ndil = process_spin(line, def.ndil);
                    break;
                case str_hash(CFG_RSAY):
                    retn.nsay = process_spin(line, def.nsay);
                    break;
                case str_hash(CFG_PCDR):
                    retn.ncdr = process_spin(line, def.ncdr);
                    break;
                case str_hash(CFG_SPEC):
                    retn.spec = process_spin(line, def.spec);
                    break;
                case str_hash(CFG_RGPU):
                    retn.rgpu = process_spin(line, def.rgpu);
                    break;
                case str_hash(CFG_RNDR):
                    render = {};
                    while (!is_empty(line))
                        render |= process_map(line, all_flags, {})
                            & conf_t::render_mask;
                    break;
                case str_hash(CFG_FLGS):
                    general = {};
                    while (!is_empty(line))
                        general |= process_map(line, all_flags, {})
                            & conf_t::general_mask;
                    break;
            }
        }
        file = (typeof(file))realloc(file, 0);
        retn.flgs = render | general;
    }
    return retn;
}

void engine_t::main_loop() {
    // waiting for the previews to finish loading
    cEngineCallback(engd_, ECB_LOAD, 0);
    cEngineCallback(engd_, ECB_LOAD, ~0);

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

void engine_t::build_library_structure(const std::string &base) {
    size_t last_category = 0;
    std::vector<std::pair<lib_id_t, conf_t::categories_t>> ctg;
    std::unordered_map<std::string, size_t> ctg_map;
    bhv_id_map_t bhv_id_map;

    std::vector<library_t::input_t> ins;
    auto path = concat_path({base, DEF_FLDR, "Ponies"});
    auto find = rFindMake(path.c_str());
    while (auto file = rFindFile(find)) {
        ins.emplace_back(library_t::input_t(path, file));
        file = (typeof(file))realloc(file, 0);
    }

    for (auto &i : ins) {
        // construct the behaviour ID descriptors and extract categories
        auto ib = bhv_id_map.emplace(
                    ascii_to_lower(i.name), library_t::build_bhv_id_desc(i));
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
        ctg.emplace_back(str_hash(i.name), std::move(all_categories));
    }
    for (auto &i : ins) {
        // initialize the libraries
        printf("%s\n", i.name.c_str());
        libs_.emplace(str_hash(i.name), std::make_unique<library_t>(
                    concat_path({path, i.name}), i, bhv_id_map));
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
    for (size_t ctg_idx = 0; ctg_idx < ctg.size(); ctg_idx++) {
        auto &c = ctg[ctg_idx];
        auto &il = *find_in_map(libs_, c.first);
        mctl_.init_preview(engd_, il->get_preview(engd_), std::move(c.second),
                il->name(), c.first);
        mctl_.set_progress_load(ctg_idx + 1, ctg.size());
    }
}

