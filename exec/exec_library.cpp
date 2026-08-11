#include <cassert>

#include "exec_library.hpp"
#include "exec_parser.hpp"



bhv_id_desc_t library_t::build_bhv_id_desc(const input_t &in) {
    std::unordered_map<int, std::array<int, bhv_type_t::max_ + 1>> bhv_grp;
    bhv_id_desc_t retn;
    for (auto &b : in.behaviours) {
        bhv_id_internal_t iid{init_bhv_id(b.movement, b.group)};
        if (!find_in_map(bhv_grp, iid.group)) bhv_grp[iid.group] = {};
        iid.index += bhv_grp[iid.group][iid.type]++;
        retn.v.emplace_back(iid._);
        if (!b.name.empty() && !find_in_map(retn.m, b.name)) {
            retn.m.emplace(b.name, iid._);
        } else {
            printf("[%s] WARNING, behaviour name collision: '%s'\n",
                    in.name.c_str(), b.name.c_str());
        }
    }
    return retn;
}

library_t::input_t::input_t(const std::string &base, const std::string &dir) {
    auto config = concat_path({base, dir, DEF_CONF});
    if (auto file = rLoadFile(config.c_str(), nullptr)) {
        name = dir;
        for (token_t text({}, file); next_line(text); ) {
            auto line = next_token(text.first);
            switch (str_hash(line.first)) {
                //case str_hash("Name"):
                //case str_hash("BehaviorGroup"):
                default: break;
                case str_hash("Categories"):
                    for (line.first = {}; !is_empty(line);
                            line = next_token(line.second, 0))
                        if (!line.first.empty()) {
                            auto category = ascii_to_lower(line.first);
                            category[0] = ascii_to_upper(category[0]);
                            categories.emplace_back(category);
                        }
                    break;
                case str_hash("Behavior"):
                    behaviours.emplace_back(line.second);
                    break;
                case str_hash("Effect"):
                    effects.emplace_back(line.second);
                    break;
                case str_hash("Speak"):
                    speeches.emplace_back(line.second);
                    break;
                case str_hash("Interaction"):
                    interactions.emplace_back(line.second);
                    break;
            }
        }
        file = (typeof(file))realloc(file, 0);
    }
}

bhv_id_t library_t::init_bhv_id(movement_flags_t move, int16_t grp) {
    bhv_id_internal_t iid;
    iid.index = 1;
    switch (move) {
        case move_none:  iid.type = stationary; break;
        case move_mouse: iid.type = mouseover;  break;
        case move_drag:  iid.type = dragged;    break;
        case move_sleep: iid.type = sleeping;   break;
        case move_all: case move_dh: case move_horz: case move_diag:
        case move_hv:  case move_dv: case move_vert: iid.type = moving; break;
    }
    iid.group = grp;
    return iid._;
}

const behaviour_t *library_t::get(bhv_id_t id) const {
    bhv_id_internal_t iid = {id};
    if (!iid.index) return nullptr;
    auto ig = find_in_map(groups_, iid.group);
    return (ig && (iid.index <= ig->bhv[iid.type].size()))
            ? &ig->bhv[iid.type][iid.index - 1].get()
            : nullptr;
}

const speech_t *library_t::select_speech(uint32_t *seed, uint32_t chance,
        bhv_id_t prev, bhv_id_t curr) const {
    auto b_prev = get(prev), b_curr = get(curr);
    if (!b_prev || !b_curr) return nullptr;
    auto index = behaviour_t::select_speech(seed, chance, *b_prev, *b_curr);
    assert(std::abs(index) <= speeches_.size());
    return (index) ? speeches_[std::abs(index) - 1].get() : nullptr;
}

interaction_t::input_t::input_t(const std::string_view &str) {
    // TODO: is 'any' really equivalent to 'one'? check if there are
    //       relevant interactions where 'any' means '>1'
    static const std::unordered_map<std::string, bool> activations = {
        {"true", true}, {"false",  false}, {"any", false},
        {"all",  true}, {"random", false}, {"one", false},
    };
    token_t line({}, str);
    name = process_string(line);
    chance = process_float(line, chance);
    proximity = process_float(line, proximity);
    auto tgt_s = process_array(line);
    for (auto &t : tgt_s) targets.emplace_back(ascii_to_lower(t));
    target_activation_all
            = process_map(line, activations, target_activation_all);
    auto bhv_s = process_array(line);
    for (auto &b : bhv_s) bhv.emplace_back(ascii_to_lower(b));
    reactivation_delay = process_float(line, reactivation_delay);
}

interaction_t::interaction_t(const input_t &in, const std::string &lib_name,
        const bhv_id_map_t &bhv_id_map)
: all_(in.target_activation_all)
, proximity_(in.proximity)
, chance_(double(uint32_t(~0)) * std::clamp(in.chance, 0.f, 1.f))
, duration_{{std::numeric_limits<decltype(duration_.x)>::max(),
            (decltype(duration_.y))(1000.f * in.reactivation_delay)}} {
#ifdef DEV_MODE
    debug_ = in;
#endif // DEV_MODE
    auto append_bhv = [&](const std::string &lib_id, bool target) {
        auto &retn = (target) ? targets_[str_hash(lib_id)] : initiator_;
        if (auto bhv_id_desc = find_in_map(bhv_id_map, lib_id))
            for (auto &b : in.bhv)
                if (auto bhv_id = find_in_map(bhv_id_desc->m, b))
                    retn.emplace_back(*bhv_id);
        if (retn.empty()) {
            printf("[%s] WARNING, no interaction behaviours with '%s' in "
                   "'%s', target dropped\n",
                    lib_name.c_str(), lib_id.c_str(), in.name.c_str());
            return false;
        }
        return true;
    };
    if (append_bhv(lib_name, false)) {
        for (auto &t : in.targets) append_bhv(t, true);
        for (auto it = targets_.begin(); it != targets_.end(); )
            it = (it->second.empty()) ? targets_.erase(it) : std::next(it);
    }
    if (is_empty())
        printf("[%s] WARNING, no interaction behaviours in '%s', "
               "interaction dropped\n",
                lib_name.c_str(), in.name.c_str());
}



struct extract_speech_colors_worker_data_t {
    library_t *lib;
    ENGD *engd;
};

void library_t::extract_speech_colors_worker(intptr_t data, uint64_t unused) {
    struct hsl_t {
        float h, s, l;

        hsl_t(uint32_t bgra) : h(0), s(0), l(0) {
            const float r = to_fp32(bgra >> 16);
            const float g = to_fp32(bgra >> 8);
            const float b = to_fp32(bgra);
            const float min = std::min(std::min(r, g), b);
            const float max = std::max(std::max(r, g), b);
            if ((l = (max + min) * 0.5f) <= 0.f) return;
            if ((s = max - min) <= 0.f) return;
            const float inv_diff = 1.f / (max - min);
            s /= (l > 0.5f) ? (2.f - max - min) : (max + min);
            if (r == max) {
                h = (g == min) ? (5.f + (max - b) * inv_diff)
                               : (1.f - (max - g) * inv_diff);
            } else if (g == max) {
                h = (b == min) ? (1.f + (max - r) * inv_diff)
                               : (3.f - (max - b) * inv_diff);
            } else { // (b == _max) {
                h = (r == min) ? (3.f + (max - g) * inv_diff)
                               : (5.f - (max - r) * inv_diff);
            }
            h *= 1.f / 6.f;
        }
        uint32_t bgra() const {
            const float coef = (l > 0.5f) ? (s + l - s * l)
                                          : (    l + s * l);
            if (coef <= 0.f)
                return ((uint8_t)(255 * l) * 0x010101) | 0xFF000000;
            const float mean = 2.f * l - coef;
            const float frac = 6.f * h - (int)(6.f * h);
            const float mid1 = mean + frac * (coef - mean);
            const float mid2 = coef - frac * (coef - mean);
            uint8_t r, g, b;
            switch ((int)(6.f * h)) {
                default: r = 255 * coef; g = 255 * mid1; b = 255 * mean; break;
                case 1:  r = 255 * mid2; g = 255 * coef; b = 255 * mean; break;
                case 2:  r = 255 * mean; g = 255 * coef; b = 255 * mid1; break;
                case 3:  r = 255 * mean; g = 255 * mid2; b = 255 * coef; break;
                case 4:  r = 255 * mid1; g = 255 * mean; b = 255 * coef; break;
                case 5:  r = 255 * coef; g = 255 * mean; b = 255 * mid2; break;
            }
            return b | ((uint32_t)g << 8) | ((uint32_t)r << 16) | 0xFF000000;
        }
        static float to_fp32(uint8_t int8) { return (1.f / 255.f) * int8; }
        static float to_linear_srgb(uint8_t int8) {
            return std::pow((to_fp32(int8) + 0.055f) * (1.f / 1.055f), 2.4f);
        }
        static float get_luminance(uint32_t bgra) {
            return (2126 * 255) * to_linear_srgb(bgra >> 16) // R
                 + (7152 * 255) * to_linear_srgb(bgra >> 8)  // G
                 + ( 722 * 255) * to_linear_srgb(bgra);      // B
        }
        static float get_contrast(const hsl_t &dk, const hsl_t &bt) {
            return (500 * 255 + get_luminance(bt.bgra()))
                 / (500 * 255 + get_luminance(dk.bgra()));
        }
        static std::string ttycolor( // TODO: does WIN32 have something alike?
                uint32_t fg, uint32_t bg, const std::string &text) {
            std::string retn = "\033[48;2;" + std::to_string(uint8_t(bg >> 16))
                    + ";" + std::to_string(uint8_t(bg >> 8))
                    + ";" + std::to_string(uint8_t(bg));
            if (fg != bg)
                retn += ";38;2;" + std::to_string(uint8_t(fg >> 16))
                        + ";" + std::to_string(uint8_t(fg >> 8))
                        + ";" + std::to_string(uint8_t(fg));
            return retn + "m" + text + "\033[0m";
        }
    };
    // reading the inputs, freeing temporary data
    auto lib = ((extract_speech_colors_worker_data_t *)data)->lib;
    auto engd = ((extract_speech_colors_worker_data_t *)data)->engd;
    data = (typeof(data))realloc((void *)data, 0);

    // allocating the color buffer and drawing frame #0 of the preview to it
    auto &preview = lib->get_preview();
    auto dims = preview.dims(false);
    size_t preview_size = sizeof(uint32_t) * dims.x * dims.y;
    uint32_t frame = 0;
    int64_t time = 0;
    AINF ainf = {preview.advance(false, 1, time, frame), (uint32_t)dims.x,
        (uint32_t)dims.y, 0, (typeof(ainf.time))realloc(nullptr, preview_size)};
    for (size_t i = dims.x * dims.y; i; ainf.time[--i] = 0) {}
    cEngineCallback(engd, ECB_DRAW, (intptr_t)&ainf);

    // building the color histogram
    std::unordered_map<uint32_t, uint32_t> histogram;
    for (size_t i = dims.x * dims.y; i; histogram[ainf.time[--i]]++) {}
    ainf.time = (typeof(ainf.time))realloc(ainf.time, 0);

    // extracting the most prominent colors from the histogram; end = size - 1
    constexpr size_t end = 2;
    size_t bgn = 0;
    struct {uint32_t clr, idx;} top_clrs[end + 1] = {};
    for (auto &h : histogram)
        if ((h.first & 0xFF000000) && (h.second >= top_clrs[end].idx)) {
            size_t i = end;
            for (; i && (h.second >= top_clrs[i - 1].idx); i--) {}
            for (size_t j = end; j > i; j--)
                top_clrs[j] = top_clrs[j - 1];
            top_clrs[i] = {h.first, h.second};
        }

    // saving the color order for debugging
    std::string colors;
    for (size_t i = bgn; i <= end; i++)
        colors += hsl_t::ttycolor(top_clrs[i].clr, top_clrs[i].clr, "   ");

    // sorting the colors by luminance
    // more sorting networks: bertdobbelaere.github.io/sorting_networks.html
    for (size_t i = bgn; i <= end; i++)
        top_clrs[i].idx = hsl_t::get_luminance(top_clrs[i].clr);
    #define SORT(v, i, j) if (v[i].idx > v[j].idx) std::swap(v[i], v[j])
    static_assert(end == 2);
    SORT(top_clrs, 0, 1);
    SORT(top_clrs, 0, 2);
    SORT(top_clrs, 1, 2);
    #undef SORT

    // emphasizing the contrast artificially in case it's insufficient
    for (; (bgn < end) && !(top_clrs[bgn].clr & 0xFF000000); bgn++) {}
    hsl_t dk(top_clrs[bgn].clr); // dark
    hsl_t bt(top_clrs[end].clr); // bright
    float contrast = -hsl_t::get_contrast(dk, bt);
    constexpr float readable_contrast = 5.f; // W3C WCAG recommends >4.5
    constexpr auto bin_iter = 16; // number of binary search iterations
    if (-contrast < readable_contrast) {
        constexpr float min_l = 0.f, max_l = 1.f;
        const float dk_l = dk.l, bt_l = bt.l, delta_l = bt_l - dk_l;
        const float mid_part = ((max_l - bt_l) < (dk_l - min_l))
                ?       1.f / (1.f + (max_l - bt_l) / (dk_l - min_l))
                : 1.f - 1.f / (1.f + (dk_l - min_l) / (max_l - bt_l));
        for (float min = min_l, max = max_l;
                max - min > (max_l - min_l) / (1 << bin_iter); ) {
            const float mid = 0.5f * (min + max) - delta_l;
            dk.l = std::clamp(dk_l - mid * mid_part, min_l, max_l);
            bt.l = std::clamp(bt_l - mid * mid_part + mid, min_l, max_l);
            contrast = hsl_t::get_contrast(dk, bt);
            ((contrast < readable_contrast) ? min : max) = mid + delta_l;
        }
    }

    // saving the results
    lib->speech_fg_ = dk.bgra();
    lib->speech_bg_ = bt.bgra();

    // printing the debug output
    std::string text(40, '#');
    text.replace(0, lib->name().size() + 1, lib->name() + " ");
    char temp[32];
    sprintf(temp, "%06X %06X - ", lib->speech_fg_ & 0xFFFFFF,
            lib->speech_bg_ & 0xFFFFFF);
    text = hsl_t::ttycolor(lib->speech_fg_, lib->speech_bg_, temp + text + " ");
    sprintf(temp, "%5.2f ", std::fabs(contrast));
    text = hsl_t::ttycolor((contrast < 0.f) ? lib->speech_fg_ : lib->speech_bg_,
                (contrast < 0.f) ? lib->speech_bg_ : lib->speech_fg_, temp)
         + text + colors;
    printf("%s\n", text.c_str());
}

void library_t::extract_speech_colors(
        ENGD *engd, const std::vector<library_t*> &libs) {
    auto parallel = rMakeParallel(extract_speech_colors_worker, 1);
    for (auto &l : libs) {
        if (l->speeches_.empty()) continue;
        auto data = (extract_speech_colors_worker_data_t *)realloc(
                nullptr, sizeof(extract_speech_colors_worker_data_t));
        data->lib = l;
        data->engd = engd;
        rLoadParallel(parallel, intptr_t(data));
    }
    rFreeParallel(parallel);
}

struct library_t::build_ctx_t {
    const input_t &in;
    const bhv_id_map_t &bhv_id_map;
    const std::string hashable_name;

    // filled by map_effects:
    std::unordered_map<std::string, eff_vec_t> eff_by_bhv_name;
    // filled by map_speeches:
    std::unordered_map<std::string, int> spk_by_name;
    std::unordered_map<int, std::vector<int>> spk_by_group;
    // filled by create_behaviours:
    std::unordered_map<std::string, behaviour_t*> bhv_by_name;

    const bhv_id_desc_t &bhv_id_desc() const {
        auto bhv_id_desc = find_in_map(bhv_id_map, hashable_name);
        assert(bhv_id_desc);
        return *bhv_id_desc;
    }

    build_ctx_t(const input_t &i, const bhv_id_map_t &m)
    : in(i)
    , bhv_id_map(m)
    , hashable_name(ascii_to_lower(i.name)) {}
};

void library_t::init_interactions(const build_ctx_t &ctx) {
    for (auto &i : ctx.in.interactions) {
        auto it = std::make_unique<interaction_t>(
                i, ctx.hashable_name, ctx.bhv_id_map);
        if (!it->is_empty())
            interactions_.emplace_back(std::move(it));
    }
}

void library_t::map_effects(build_ctx_t &ctx) {
    const auto &bhv_id_desc = ctx.bhv_id_desc();
    for (auto &e : ctx.in.effects) {
        if (find_in_map(bhv_id_desc.m, e.bhv)) {
            ctx.eff_by_bhv_name[e.bhv].emplace_back(e);
        } else {
            printf("[%s] WARNING, unused effect '%s'\n",
                   ctx.in.name.c_str(), e.name.c_str());
        }
    }
}

void library_t::map_speeches(build_ctx_t &ctx) {
    for (auto &s : ctx.in.speeches) {
        auto dejavu = find_in_map(ctx.spk_by_name, s.name);
        if (dejavu)
            printf("[%s] WARNING, speech name collision: '%s'%s\n",
                    ctx.in.name.c_str(), s.name.c_str(),
                    (s.skip) ? ", non-selectable (dropped)" : ", selectable");
        if (!dejavu || !s.skip) {
            speeches_.emplace_back(std::make_unique<speech_t>(s));
            ctx.spk_by_name.emplace(s.name, -int(speeches_.size()));
            if (!s.skip)
                ctx.spk_by_group[s.group].emplace_back(int(speeches_.size()));
        }
    }
}

void library_t::create_behaviours(build_ctx_t &ctx) {
    eff_vec_t e_null;
    auto i0 = find_in_map(ctx.spk_by_group, 0); // random speeches from group 0
    const auto &bhv_id_desc = ctx.bhv_id_desc();
    for (size_t i = 0; i < ctx.in.behaviours.size(); i++) {
        std::vector<int16_t> b_spk, e_spk;
        auto &b = ctx.in.behaviours[i];
        auto ig = find_in_map(ctx.spk_by_group, b.group);
        if (auto ib = find_in_map(ctx.spk_by_name, b.bgn_speech)) {
            b_spk.emplace_back(*ib); // found behaviour-specific start speech
        } else if (ig || i0) {
            // no start speech found, substituting it with random speeches;
            // no speech can belong to >1 group, don't look for duplicates
            if (ig) b_spk.insert(b_spk.end(), ig->begin(), ig->end());
            if (i0 && (b.group != 0))
                b_spk.insert(b_spk.end(), i0->begin(), i0->end());
        }
        if (auto ie = find_in_map(ctx.spk_by_name, b.end_speech))
            e_spk.emplace_back(*ie); // found behaviour-specific end speech

        bhv_id_internal_t iid{bhv_id_desc.v[i]};
        bhv_id_internal_t linked_iid = {};
        if (auto il = find_in_map(bhv_id_desc.m, b.linked_bhv)) {
            linked_iid._ = *il;
        } else if (!b.linked_bhv.empty()) {
            printf("[%s] WARNING, invalid linked behaviour name in '%s'\n",
                    ctx.in.name.c_str(), b.name.c_str());
        }
        auto is = find_in_map(bhv_id_desc.m, b.follow_stop_bhv);
        auto im = find_in_map(bhv_id_desc.m, b.follow_mov_bhv);
        bhv_id_internal_t follow_grp_iid = {};
        follow_grp_iid.group = (is && im) ? -int16_t(i) : iid.group;
        if (!is != !im)
            printf("[%s] WARNING, inconsistent follow behaviours in '%s'\n",
                   ctx.in.name.c_str(), b.name.c_str());

        auto ie = find_in_map(ctx.eff_by_bhv_name, b.name);
        behaviours_.emplace_back(std::make_unique<behaviour_t>(b, iid._,
                    linked_iid._, follow_grp_iid._, str_hash(b.follow_target),
                    library_path_, b_spk, e_spk, (ie) ? *ie : e_null));
        groups_[iid.group].bhv[iid.type].emplace_back(*behaviours_.back());
        ctx.bhv_by_name.emplace(b.name, behaviours_.back().get());
        assert(iid.index == groups_[iid.group].bhv[iid.type].size());
    }
}

void library_t::init_probabilities(const build_ctx_t &ctx) {
    // extracting raw probabilities
    std::unordered_map<int, std::vector<uint32_t>> prob_by_group;
    for (size_t i = 0; i < ctx.in.behaviours.size(); i++) {
        auto &b = ctx.in.behaviours[i];
        if ((!b.skip) && (b.chance > 0.f)) {
            prob_by_group[b.group].emplace_back(
                    10000.f * std::clamp(b.chance, 0.f, 1.f));
            groups_[b.group].bhv[nonzero_prob].emplace_back(*behaviours_[i]);
        }
    }
    // adding behaviours from group 0 (GroupAny) to all other groups
    auto ip = find_in_map(prob_by_group, 0);
    if (auto ig = find_in_map(groups_, 0))
        for (auto &p : prob_by_group)
            if (p.first != 0) {
                if (ip) p.second.insert(p.second.end(), ip->begin(), ip->end());
                groups_[p.first].append(*ig);
            }
    // initializing probabilities for the alias method
    for (auto &p : prob_by_group) {
        assert(groups_[p.first].bhv[nonzero_prob].size() == p.second.size());
        groups_[p.first].nonzero_weights = weighted_rng_t(p.second);
    }
}

void library_t::init_follow_groups(const build_ctx_t &ctx) {
    for (size_t i = 0; i < ctx.in.behaviours.size(); i++)
        if (!ctx.in.behaviours[i].follow_target.empty()
                && !ctx.in.behaviours[i].auto_follow_img) {
            group_t grp;
            if (auto is = find_in_map(
                        ctx.bhv_by_name, ctx.in.behaviours[i].follow_stop_bhv))
                grp.bhv[stationary].emplace_back(**is);
            if (auto im = find_in_map(
                        ctx.bhv_by_name, ctx.in.behaviours[i].follow_mov_bhv))
                grp.bhv[moving].emplace_back(**im);

            if (!grp.bhv[moving].empty() && !grp.bhv[stationary].empty()) {
                groups_[-int16_t(i)] = std::move(grp);
            } else {
                printf("[%s] WARNING, invalid custom follow behaviours in "
                       "'%s', reverting to defaults\n",
                        ctx.in.name.c_str(), ctx.in.behaviours[i].name.c_str());
            }
        }
}

void library_t::dump_stats() const {
    printf("Total behaviours: %lu\n", behaviours_.size());
    for (auto &g : groups_) {
        printf("[%d] %lu + %lu + %lu + %lu + %lu + %lu\n", g.first,
            g.second.bhv[nonzero_prob].size(), g.second.bhv[stationary].size(),
            g.second.bhv[moving].size(), g.second.bhv[mouseover].size(),
            g.second.bhv[dragged].size(), g.second.bhv[sleeping].size());
    }
}

library_t::library_t(std::string path, const input_t &in,
        const bhv_id_map_t &bhv_id_map)
: library_path_(std::move(path))
, readable_name_(in.name)
, preview_id_(0)
, speech_fg_(0xFF000000)
, speech_bg_(0xFFFFFFFF) {
    build_ctx_t ctx(in, bhv_id_map);
    init_interactions(ctx);
    map_effects(ctx);
    map_speeches(ctx);
    create_behaviours(ctx);
    init_probabilities(ctx);
    init_follow_groups(ctx);
    dump_stats();
}
