#include <cassert>

#include "exec_library.hpp"
#include "exec_parser.h"
#include "exec_prng.h"



library_t::group_t::~group_t() {
    RNG_FreeWeighted(&rngw);
}

library_t::group_t &library_t::group_t::operator=(group_t &&rhs) {
    RNG_FreeWeighted(&rngw);
    std_swap(rngw, rhs.rngw);
    speeches = std::move(rhs.speeches);
    for (size_t iter = sizeof(bhv) / sizeof(*bhv); iter; iter--)
        bhv[iter - 1] = std::move(rhs.bhv[iter - 1]);
    return *this;
}

const behaviour_t *library_t::group_t::random_bhv(
        uint32_t *seed, bhv_type_t type) const {
    if (bhv[type].empty()) return nullptr;
    if (type != nonzero_prob)
        return &bhv[type][RNG_LoadBounded(seed, bhv[type].size())].get();
    assert(rngw);
    return &bhv[type][RNG_LoadWeighted(rngw, seed)].get();
}

const speech_t *library_t::group_t::random_speech(uint32_t *seed) const {
    if (speeches.empty()) return nullptr;
    return &speeches[RNG_LoadBounded(seed, speeches.size())].get();
}

void library_t::group_t::add_weights(const std::vector<uint32_t> &weights) {
    assert(bhv[nonzero_prob].size() == weights.size());
    if (rngw) RNG_FreeWeighted(&rngw);
    rngw = RNG_MakeWeighted(weights.size(), weights.data());
}

void library_t::group_t::add_bhv(bhv_id_t id, const behaviour_t &b) {
    bhv_id_internal_t iid{id};
    bhv[iid.type].emplace_back(b);
    assert(iid.index == bhv[iid.type].size());
}

bhv_id_desc_t library_t::build_bhv_id_desc(const input_t &in) {
    struct count_t { int data[bhv_type_t::max_ + 1]; };
    std::unordered_map<int, count_t> bhv_grp;
    bhv_id_desc_t retn;
    for (auto &b : in.behaviours) {
        bhv_id_internal_t iid{init_bhv_id(b.movement, b.group)};
        if (!find_in_map(bhv_grp, iid.group)) bhv_grp[iid.group] = {};
        iid.index += bhv_grp[iid.group].data[iid.type]++;
        retn.v.emplace_back(iid._);
        auto b_view = get_view(b.name);
        auto in_view = get_view(in.name);
        auto b_hash = str_view_hash(b_view, 'L');
        if (b_view.size && !find_in_map(retn.m, b_hash)) {
            retn.m.emplace(b_hash, iid._);
        } else {
            printf("[%s] WARNING, behaviour name collision: '%s'\n",
                    in_view.str, b_view.str);
        }
    }
    return retn;
}

library_t::input_t::input_t(string_view_t base, string_view_t dir) {
    long size = 0;
    local_string_t *config = make_string(
            base, get_imm_view(DEF_DSEP), dir, get_imm_view(DEF_DSEP DEF_CONF));
    if (auto file = rLoadFile(get_view(config).str, &size)) {
        name = make_string(dir);
        auto line = local_make_tokenizer({});
        auto text = local_make_tokenizer({(size_t)size, file});
        while (next_line(text)) {
            next_token(line, get_token(text), DEF_CMNT, DEF_TSEP, DEF_QUOT);
            switch (str_view_hash(get_token(line), 'L')) {
                //case CHR_HASH(L, 'N','a','m','e'):
                //case CHR_HASH(L,
                //      'B','e','h','a','v','i','o','r','G','r','o','u','p'):
                default: break;
                case CHR_HASH(L, 'C','a','t','e','g','o','r','i','e','s'):
                    while (!is_empty(line)) {
                        next_token(line, get_remainder(line),
                                0, DEF_TSEP, DEF_QUOT);
                        if (!get_token(line).size) continue;
                        // non-empty here: empty tokens are skipped above
                        auto category = make_string(get_token(line));
                        auto first = get_view(category);
                        auto rest = utf8_skip_char(first);
                        first.size = rest.str - first.str;
                        ascii_to_upper(first);
                        ascii_to_lower(rest);
                        categories.emplace_back(category);
                    }
                    break;
                case CHR_HASH(L, 'B','e','h','a','v','i','o','r'):
                    behaviours.emplace_back(get_remainder(line));
                    break;
                case CHR_HASH(L, 'E','f','f','e','c','t'):
                    effects.emplace_back(get_remainder(line));
                    break;
                case CHR_HASH(L, 'S','p','e','a','k'):
                    speeches.emplace_back(get_remainder(line));
                    break;
                case CHR_HASH(L, 'I','n','t','e','r','a','c','t','i','o','n'):
                    interactions.emplace_back(get_remainder(line));
                    break;
            }
        }
        free(file);
    }
}

bhv_id_t library_t::init_bhv_id(movement_flags_t move, int16_t grp) {
    bhv_id_internal_t iid;
    iid.index = 1;
    switch (move) {
        default:         iid.type = stationary; break;
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
    if (auto ig = find_in_map(groups_, bhv_id_internal_t{id}.group))
        return ig->get_bhv(id);
    return nullptr;
}

const behaviour_t *library_t::next_behaviour(
        uint32_t *seed, const behaviour_t *curr) const {
    auto check_has_nzp = [](const group_t *grp) {
        return (grp && grp->bhv_size(nonzero_prob)) ? grp : nullptr;
    };
    if (curr && curr->linked_id()) return get(curr->linked_id());

    bhv_id_internal_t iid = {(curr) ? curr->id() : bhv_id_t{}};
    auto grp = check_has_nzp(find_in_map(groups_, iid.group));
    if (!grp) {
        int best = INT32_MAX;
        for (auto &ig : groups_)
            if (check_has_nzp(&ig.second) && (!grp || (ig.first < best))) {
                grp = &ig.second;
                best = ig.first;
            }
    }
    if (!grp) {
        assert(false);
        return {};
    }
    return grp->random_bhv(seed, nonzero_prob);
}

const unit_t &library_t::follow_override(
        uint32_t *seed, const behaviour_t &bhv, bool move) const {
    const auto type = (move) ? moving : stationary;
    const bhv_id_internal_t iid = {bhv.follow_grp_id()};
    if (iid.group < 0) // custom, unless rejected by init_follow_groups()
        if (auto ig = find_in_map(groups_, iid.group))
            return *ig->get_bhv(bhv_id_internal_t{.index = 1, .type = type}._);
    return typed_bhv_internal(seed, bhv, type);
}

const speech_t *library_t::transition_speech(uint32_t *seed, uint64_t chance,
        const behaviour_t &prev, const behaviour_t &curr) const {
    if (curr.bgn_speech()) return curr.bgn_speech();
    if (prev.end_speech()) return prev.end_speech();
    if (!chance || (RNG_Load(seed) >= chance)) return nullptr;
    // in DP, random speech is also prohibited for behaviours that have an end
    // speech, here that restriction is lifted; worth remembering though
    return force_random_speech(seed, curr);
}

const speech_t *library_t::force_random_speech(
        uint32_t *seed, const behaviour_t &curr) const {
    auto ig = find_in_map(groups_, bhv_id_internal_t{curr.id()}.group);
    return (ig) ? ig->random_speech(seed) : nullptr;
}

const behaviour_t &library_t::typed_bhv_internal(
        uint32_t *seed, const behaviour_t &curr, bhv_type_t type) const {
    const behaviour_t *bhv;
    auto ig = find_in_map(groups_, bhv_id_internal_t{curr.id()}.group);
    return (ig && (bhv = ig->random_bhv(seed, type))) ? *bhv : curr;
}

static bool process_activation(tokenizer_t *line, bool def) {
    next_token(line, get_remainder(line), 0, DEF_TSEP, DEF_QUOT);
    if (get_token(line).size < 2) return def;
    // TODO: is 'any' really equivalent to 'one'? check if there are
    //       relevant interactions where 'any' means '>1'
    switch (CHR_HASH_CL(get_token(line).str[0])) {
        default: return def;
        case 't': return true; // [t]rue = 1
        // [a][l]l = 1, [a]ny = 0
        case 'a': return (CHR_HASH_CL(get_token(line).str[1]) == 'l');
        case 'o': case 'r': case 'f': return false;
    } // ^--- [o]ne = 0 / [r]andom = 0 / [f]alse = 0
}

interaction_t::input_t::input_t(string_view_t str) {
    auto line = local_make_tokenizer(str);
    name = process_string(line, DEF_TSEP, DEF_QUOT);
    chance = process_float(line, chance, DEF_TSEP, DEF_QUOT);
    proximity = process_float(line, proximity, DEF_TSEP, DEF_QUOT);
    string_view_t *temp;
    size_t size;
    size = process_array(line, &temp, DEF_TSEP, DEF_QUOT, DEF_ABGN, DEF_AEND);
    for (size_t iter = 0; iter < size; iter++)
        targets.emplace_back(make_string(temp[iter]));
    free(temp);
    target_activation_all = process_activation(line, target_activation_all);
    size = process_array(line, &temp, DEF_TSEP, DEF_QUOT, DEF_ABGN, DEF_AEND);
    for (size_t iter = 0; iter < size; iter++)
        bhv.emplace_back(make_string(temp[iter]));
    free(temp);
    reactivation_delay
            = process_float(line, reactivation_delay, DEF_TSEP, DEF_QUOT);
}

interaction_t::interaction_t(const input_t &in, string_view_t lib_name,
        const bhv_id_map_t &bhv_id_map)
: all_(in.target_activation_all)
, proximity_(in.proximity)
, chance_(double((uint64_t)1 << 32) * std_clamp(in.chance, 0.f, 1.f))
, duration_{{std::numeric_limits<decltype(duration_.x)>::max(),
            (decltype(duration_.y))(1000.f * in.reactivation_delay)}} {
    auto append_bhv = [&](string_view_t lib_id, bool target) {
        auto lib_id_hash = str_view_hash(lib_id, 'L');
        auto &retn = (target) ? targets_[lib_id_hash] : initiator_;
        if (auto bhv_id_desc = find_in_map(bhv_id_map, lib_id_hash))
            for (auto b : in.bhv)
                if (auto bhv_id = find_in_map(bhv_id_desc->m, str_hash(b, 'L')))
                    retn.emplace_back(*bhv_id);
        if (retn.empty()) {
            printf("[%s] WARNING, no interaction behaviours with '%s' in "
                   "'%s', target dropped\n",
                    lib_name.str, lib_id.str, get_view(in.name).str);
            return false;
        }
        return true;
    };
    if (append_bhv(lib_name, false)) {
        for (auto &t : in.targets) append_bhv(get_view(t), true);
        for (auto it = targets_.begin(); it != targets_.end(); )
            it = (it->second.empty()) ? targets_.erase(it) : std::next(it);
    }
    if (is_empty())
        printf("[%s] WARNING, no interaction behaviours in '%s', "
               "interaction dropped\n",
                lib_name.str, get_view(in.name).str);
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
            const float min = std_min(std_min(r, g), b);
            const float max = std_max(std_max(r, g), b);
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
            // the argument is an 8-bit channel, so the whole function is 256
            // values that can just as well be tabulated once; pow() is a lot
            // more expensive than a lookup. N.B.: a local static is filled in
            // a thread-safe manner since C++11, but the C port will have to
            // build the table before the workers are spawned instead
            static const struct lut_t {
                float lin[256];
                lut_t() {
                    for (uint32_t i = 0; i < sizeof(lin) / sizeof(*lin); i++)
                        lin[i] = std::pow(
                                (to_fp32(i) + 0.055f) * (1.f / 1.055f), 2.4f);
                }
            } lut;
            return lut.lin[int8];
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
        static string_t *ttycolor( // TODO: does WIN32 have something alike?
                uint32_t fg, uint32_t bg, string_view_t text) {
            char bg_str[16];
            size_t bg_size = sprintf(bg_str, "%d;%d;%d",
                    uint8_t(bg >> 16), uint8_t(bg >> 8), uint8_t(bg));
            if (bg == fg) return make_string(get_imm_view("\033[48;2;"),
                                        {bg_size, bg_str}, get_imm_view("m"),
                                        text, get_imm_view("\033[0m"));
            char fg_str[16];
            size_t fg_size = sprintf(fg_str, "%d;%d;%d",
                    uint8_t(fg >> 16), uint8_t(fg >> 8), uint8_t(fg));
            return make_string(get_imm_view("\033[48;2;"), {bg_size, bg_str},
                        get_imm_view(";38;2;"), {fg_size, fg_str},
                        get_imm_view("m"), text, get_imm_view("\033[0m"));
        }
    };
    // reading the inputs, freeing temporary data
    auto lib = ((extract_speech_colors_worker_data_t *)data)->lib;
    auto engd = ((extract_speech_colors_worker_data_t *)data)->engd;
    free((void *)data);

    // allocating the color buffer and drawing frame #0 of the preview to it
    auto &preview = lib->get_preview();
    auto dims = preview.dims(false);
    size_t preview_size = sizeof(uint32_t) * dims.x * dims.y;
    int64_t time = 0;
    AINF ainf = {0, (uint32_t)dims.x, (uint32_t)dims.y, UINT32_MAX,
                (typeof(ainf.time))realloc(nullptr, preview_size)};
    ainf.uuid = preview.advance(false, 0, time, ainf.fcnt);
    for (size_t i = dims.x * dims.y; i; ainf.time[--i] = 0) {}
    cEngineCallback(engd, ECB_DRAW, (intptr_t)&ainf);

    // building the color histogram. The preview is drawn from a palette, so
    // the number of distinct colors is tiny next to the number of pixels and
    // a flat open-addressed table beats a node-based map by a wide margin.
    // COLOR == 0 marks a free slot and never clashes with a stored key, as
    // the fully transparent pixels are skipped here: they could never win
    // the selection below anyway, it used to test the very same alpha
    const uint32_t bits = 9, mask = (1u << bits) - 1;
    struct slot_t { uint32_t color, value; } *histogram
                = (slot_t *)realloc(nullptr, sizeof(*histogram) * (mask + 1));
    for (uint32_t i = mask + 1; i; histogram[--i] = {}) {}
    for (size_t used = 0, i = dims.x * dims.y; i; ) {
        const uint32_t color = ainf.time[--i];
        if (!(color & 0xFF000000)) continue;
        uint32_t slot = (color * 2654435761u) >> (32 - bits);
        for (; histogram[slot].color && (histogram[slot].color != color);
             slot = (slot + 1) & mask) {}
        if (histogram[slot].color) {
            histogram[slot].value++;
            continue;
        }
        histogram[slot] = {color, 1};
        if (2 * ++used <= mask + 1) continue; // still within half capacity?
        STATIC_ASSERT(bits > 8);
        assert(false); // 256-color palettes can never get to this spot
        break;
    }
    free(ainf.time);

    // extracting the most prominent colors from the histogram; end = size - 1
    constexpr size_t end = 2;
    size_t bgn = 0;
    slot_t top_clrs[end + 1] = {};
    for (uint32_t i = mask + 1; i; ) {
        const slot_t slot = histogram[--i];
        if (!slot.color || (slot.value < top_clrs[end].value)) continue;
        size_t j = end;
        for (; j && (slot.value >= top_clrs[j - 1].value); j--) {}
        for (size_t k = end; k > j; k--)
            top_clrs[k] = top_clrs[k - 1];
        top_clrs[j] = {slot.color, slot.value};
    }
    free(histogram);

    // saving the color order for debugging
    local_string_t *colors = nullptr;
    for (size_t i = bgn; i <= end; i++) {
        local_string_t *color = hsl_t::ttycolor(
                top_clrs[i].color, top_clrs[i].color, get_imm_view("   "));
        colors = append_string(colors, get_view(color));
    }
    // sorting the colors by luminance
    // more sorting networks: bertdobbelaere.github.io/sorting_networks.html
    for (size_t i = bgn; i <= end; i++)
        top_clrs[i].value = hsl_t::get_luminance(top_clrs[i].color);
    #define SORT(v, i, j) if (v[i].value > v[j].value) std_swap(v[i], v[j])
    STATIC_ASSERT(end == 2);
    SORT(top_clrs, 0, 1);
    SORT(top_clrs, 0, 2);
    SORT(top_clrs, 1, 2);
    #undef SORT

    // emphasizing the contrast artificially in case it's insufficient
    for (; (bgn < end) && !(top_clrs[bgn].color & 0xFF000000); bgn++) {}
    hsl_t dk(top_clrs[bgn].color); // dark
    hsl_t bt(top_clrs[end].color); // bright
    float contrast = -hsl_t::get_contrast(dk, bt);
    constexpr float readable_contrast = 5.f; // W3C WCAG recommends >4.5
    constexpr auto bin_iter = 16; // number of binary search iterations
    if (-contrast < readable_contrast) {
        constexpr float min_l = 0.f, max_l = 1.f;
        const float dk_l = dk.l, bt_l = bt.l, delta_l = bt_l - dk_l;
        const float mid_part = ((dk_l - min_l) < (max_l - bt_l))
                ? 1.f - 1.f / (1.f + (dk_l - min_l) / (max_l - bt_l))
                :       1.f / (1.f + (max_l - bt_l) / (dk_l - min_l));
        for (float min = min_l, max = max_l;
                max - min > (max_l - min_l) / (1 << bin_iter); ) {
            const float mid = delta_l - 0.5f * (min + max);
            dk.l = std_clamp(dk_l + mid * mid_part, min_l, max_l);
            bt.l = std_clamp(bt_l + mid * mid_part - mid, min_l, max_l);
            contrast = hsl_t::get_contrast(dk, bt);
            ((contrast < readable_contrast) ? min : max) = delta_l - mid;
        }
    }

    // saving the results
    lib->speech_fg_ = dk.bgra();
    lib->speech_bg_ = bt.bgra();

    // printing the debug output: first prepare the contrast
    char temp[24];
    auto ct_size = sprintf(temp, "%5.2f ", std_abs(contrast));
    local_string_t *text = hsl_t::ttycolor(
            (contrast < 0.f) ? lib->speech_fg_ : lib->speech_bg_,
            (contrast < 0.f) ? lib->speech_bg_ : lib->speech_fg_,
            (string_view_t){(size_t)ct_size, temp});
    // ...then prepare the main header
    auto clr_size = sprintf(temp, " %06X / %06X: ", lib->speech_fg_ & 0xFFFFFF,
                                                    lib->speech_bg_ & 0xFFFFFF);
    string_view_t clr_view{(size_t)clr_size, temp};
    local_string_t *header = make_string(clr_view, lib->name(),
            get_imm_view(" #######################################"));
    auto hdr_view = get_view(header);
    hdr_view.size = (lib->name().size * 2 < hdr_view.size - clr_view.size)
            ? (hdr_view.size - lib->name().size)
            : (clr_view.size + lib->name().size + 1);
    ((char*)hdr_view.str)[hdr_view.size++] = ' '; // over the first cut '#'
    local_string_t *colored_header
            = hsl_t::ttycolor(lib->speech_fg_, lib->speech_bg_, hdr_view);
    // ...then concatenate everything and put it to the screen
    text = append_string(text, get_view(colored_header), get_view(colors));
    printf("%s\n", get_view(text).str);
}

void library_t::extract_speech_colors(
        ENGD *engd, const std::vector<library_t*> &libs) {
    auto parallel = rMakeParallel(extract_speech_colors_worker, 1);
    for (auto &l : libs) {
        if (!l->first_speech()) continue;
        auto data = (extract_speech_colors_worker_data_t *)realloc(
                nullptr, sizeof(extract_speech_colors_worker_data_t));
        data->lib = l;
        data->engd = engd;
        rLoadParallel(parallel, intptr_t(data));
    }
    rFreeParallel(parallel);
}

void library_t::load(ENGD *engd, bool speech, bool cspeech,
        const speech_t::text_fn_t &text_fn) {
    if (speech) {
        auto dims = get_preview_internal().dims(false);
        dims = {{dims.x * 2, dims.y * 8}};
        for (auto &s : speeches_) {
            s->render((cspeech) ? speech_fg_ : def_speech_fg_,
                      (cspeech) ? speech_bg_ : def_speech_bg_, dims, text_fn);
            s->schedule_upload(engd);
        }
    }
    for (auto &b : behaviours_) b->schedule_upload_full(engd);
}

void library_t::finalize() {
    for (auto &b : behaviours_) b->finalize_full();
    for (auto &s : speeches_) s->finalize();
}

struct library_t::build_ctx_t {
    const input_t &in;
    const bhv_id_map_t &bhv_id_map;

    // filled by map_effects:
    std::unordered_map<size_t, fx_vec_t> fx_by_bhv_name;
    // filled by map_speeches:
    std::unordered_map<size_t, const speech_t*> spk_by_name;
    std::unordered_map<int, ref_vec_t<speech_t>> spk_by_group;
    // filled by create_behaviours:
    std::unordered_map<size_t, behaviour_t*> bhv_by_name;

    const bhv_id_desc_t &bhv_id_desc() const {
        auto bhv_id_desc = find_in_map(bhv_id_map, str_hash(in.name, 'L'));
        assert(bhv_id_desc);
        return *bhv_id_desc;
    }
    build_ctx_t(
            const input_t &i, const bhv_id_map_t &m) : in(i), bhv_id_map(m) {}
};

void library_t::init_interactions(const build_ctx_t &ctx) {
    for (auto &i : ctx.in.interactions) {
        auto it = std::make_unique<interaction_t>(
                i, get_view(ctx.in.name), ctx.bhv_id_map);
        if (!it->is_empty())
            interactions_.emplace_back(std::move(it));
    }
}

void library_t::map_effects(build_ctx_t &ctx) {
    const auto &bhv_id_desc = ctx.bhv_id_desc();
    for (auto &fx : ctx.in.effects) {
        auto bhv_hash = str_hash(fx.bhv, 'L');
        if (find_in_map(bhv_id_desc.m, bhv_hash)) {
            ctx.fx_by_bhv_name[bhv_hash].emplace_back(fx);
        } else {
            printf("[%s] WARNING, unused effect '%s'\n",
                   get_view(ctx.in.name).str, get_view(fx.name).str);
        }
    }
}

void library_t::map_speeches(build_ctx_t &ctx) {
    for (auto &s : ctx.in.speeches) {
        auto name_hash = str_hash(s.name, 'L');
        auto dejavu = find_in_map(ctx.spk_by_name, name_hash);
        if (dejavu)
            printf("[%s] WARNING, speech name collision: '%s'%s\n",
                    get_view(ctx.in.name).str, get_view(s.name).str,
                    (s.skip) ? ", non-selectable (dropped)" : ", selectable");
        if (!dejavu || !s.skip) {
            speeches_.emplace_back(std::make_unique<speech_t>(s));
            ctx.spk_by_name.emplace(name_hash, speeches_.back().get());
            if (!s.skip)
                ctx.spk_by_group[s.group].emplace_back(*speeches_.back());
        }
    }
}

void library_t::create_behaviours(build_ctx_t &ctx) {
    fx_vec_t fx_null;
    const auto &bhv_id_desc = ctx.bhv_id_desc();
    for (size_t i = 0; i < ctx.in.behaviours.size(); i++) {
        auto &b = ctx.in.behaviours[i];
        auto ib = find_in_map(ctx.spk_by_name, str_hash(b.bgn_speech, 'L'));
        auto ie = find_in_map(ctx.spk_by_name, str_hash(b.end_speech, 'L'));

        bhv_id_internal_t iid{bhv_id_desc.v[i]};
        bhv_id_internal_t linked_iid = {};
        if (auto il = find_in_map(bhv_id_desc.m, str_hash(b.linked_bhv, 'L'))) {
            linked_iid._ = *il;
        } else if (get_view(b.linked_bhv).size) {
            printf("[%s] WARNING, invalid linked behaviour name in '%s'\n",
                    get_view(ctx.in.name).str, get_view(b.name).str);
        }
        auto is = find_in_map(bhv_id_desc.m, str_hash(b.follow_stop_bhv, 'L'));
        auto im = find_in_map(bhv_id_desc.m, str_hash(b.follow_mov_bhv, 'L'));
        bhv_id_internal_t follow_grp_iid = {};
        follow_grp_iid.group = (is && im) ? -int16_t(i) : iid.group;
        if (!is != !im)
            printf("[%s] WARNING, inconsistent follow behaviours in '%s'\n",
                   get_view(ctx.in.name).str, get_view(b.name).str);

        auto b_hash = str_hash(b.name, 'L');
        auto ix = find_in_map(ctx.fx_by_bhv_name, b_hash);
        behaviours_.emplace_back(std::make_unique<behaviour_t>(b, iid._,
                    linked_iid._, follow_grp_iid._, (ix) ? *ix : fx_null,
                    (ib) ? *ib : nullptr, (ie) ? *ie : nullptr,
                    str_hash(b.follow_target, 'L'), get_view(library_path_)));
        groups_[iid.group].add_bhv(iid._, *behaviours_.back());
        ctx.bhv_by_name.emplace(b_hash, behaviours_.back().get());
    }
}

void library_t::init_random_speeches(const build_ctx_t &ctx) {
    auto i0 = find_in_map(ctx.spk_by_group, 0);
    for (auto &g : groups_) {
        if (auto ig = find_in_map(ctx.spk_by_group, g.first))
            for (auto &speech : *ig) g.second.add_speech(speech);
        if (i0 && (g.first != 0))
            for (auto &speech : *i0) g.second.add_speech(speech);
    }
}

void library_t::init_probabilities(const build_ctx_t &ctx) {
    // extracting raw probabilities
    std::unordered_map<int, std::vector<uint32_t>> prob_by_group;
    for (size_t i = 0; i < ctx.in.behaviours.size(); i++) {
        auto &b = ctx.in.behaviours[i];
        if ((!b.skip) && (b.chance > 0.f)) {
            prob_by_group[b.group].emplace_back(
                    10000.f * std_clamp(b.chance, 0.f, 1.f));
            const uint32_t index = groups_[b.group].bhv_size(nonzero_prob) + 1;
            const bhv_id_internal_t iid{.index = index, .type = nonzero_prob};
            groups_[b.group].add_bhv(iid._, *behaviours_[i]);
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
    for (auto &p : prob_by_group)
        groups_[p.first].add_weights(p.second);
}

void library_t::init_follow_groups(const build_ctx_t &ctx) {
    using bhv_int_t = bhv_id_internal_t;
    for (size_t i = 0; i < ctx.in.behaviours.size(); i++)
        if (get_view(ctx.in.behaviours[i].follow_target).size
                && !ctx.in.behaviours[i].auto_follow_img) {
            group_t grp;
            auto fs_hash = str_hash(ctx.in.behaviours[i].follow_stop_bhv, 'L');
            if (auto is = find_in_map(ctx.bhv_by_name, fs_hash))
                grp.add_bhv(bhv_int_t{.index = 1, .type = stationary}._, **is);
            auto fm_hash = str_hash(ctx.in.behaviours[i].follow_mov_bhv, 'L');
            if (auto im = find_in_map(ctx.bhv_by_name, fm_hash))
                grp.add_bhv(bhv_int_t{.index = 1, .type = moving}._, **im);

            if (grp.bhv_size(moving) && grp.bhv_size(stationary)) {
                groups_[-int16_t(i)] = std::move(grp);
            } else {
                printf("[%s] WARNING, invalid custom follow behaviours in "
                       "'%s', reverting to defaults\n",
                        get_view(ctx.in.name).str,
                        get_view(ctx.in.behaviours[i].name).str);
            }
        }
}

void library_t::init_special_behaviours(const build_ctx_t &ctx) {
    if (behaviours_.empty()) return;
    // stationary candidates by group: nonzero chance, no skip, no follow
    std::unordered_map<int, ref_vec_t<behaviour_t>> idle;
    size_t slowest = 0;
    for (size_t i = 0; i < behaviours_.size(); i++) {
        auto &b = ctx.in.behaviours[i];
        auto &bhv = *behaviours_[i];
        if (bhv.speed() < behaviours_[slowest]->speed()) slowest = i;
        if (!b.skip && (b.chance > 0.f) && !get_view(b.follow_target).size
                && !bhv.speed())
            idle[bhv_id_internal_t{bhv.id()}.group].emplace_back(bhv);
    }
    // only the lists left without dedicated behaviours get fallbacks
    for (auto &g : groups_) {
        if (g.first < 0) continue; // skip custom follow groups
        ref_vec_t<behaviour_t> fallbacks;
        if (auto ii = find_in_map(idle, g.first)) {
            fallbacks = *ii;
        } else if (auto ii = find_in_map(idle, 0)) {
            fallbacks = *ii;
        } else {
            fallbacks.emplace_back(*behaviours_[slowest]);
        }
        g.second.add_fallbacks(fallbacks);
    }
}

void library_t::init_max_effects() {
    // compute the max number of effects the sprite belonging to their library
    // can spawn: summed as effects may outlive their behaviours
    max_effects_ = 0;
    for (auto &b : behaviours_) {
        const auto d_bhv = b->duration();
        for (auto &fx : b->effects()) {
            const auto d_fx = fx->duration();
            const auto life = (!d_fx.u) ? d_bhv.u + d_bhv.v : d_fx.u;
            max_effects_ += (d_fx.v) ? life / d_fx.v + 2 : 2;
        }
    }
}

void library_t::dump_stats() const {
    auto out = (char*)realloc(nullptr, 64 + name().size + 128 * groups_.size());
    auto size = sprintf(out, "[%s] Total behaviours: %lu (max effects: %u)",
            name().str, behaviours_.size(), max_effects_);
    for (auto &g : groups_)
        size += sprintf(out + size,
            "\n[%d] %lu + %lu + %lu + %lu + %lu + %lu + %lu", g.first,
            g.second.bhv_size(nonzero_prob), g.second.bhv_size(stationary),
            g.second.bhv_size(moving), g.second.bhv_size(mouseover),
            g.second.bhv_size(dragged), g.second.bhv_size(sleeping),
            g.second.speech_size());
    printf("%s\n", out); // to keep potential threaded output in one piece
    free(out);
}

library_t::library_t(lib_id_t id, string_view_t path, const input_t &in,
        const bhv_id_map_t &bhv_id_map)
: id_(id)
, library_path_(make_string(path))
, readable_name_(make_string(get_view(in.name)))
, preview_id_(0)
, max_effects_(0)
, speech_fg_(def_speech_fg_)
, speech_bg_(def_speech_bg_) {
    build_ctx_t ctx(in, bhv_id_map);
    init_interactions(ctx);
    map_effects(ctx);
    map_speeches(ctx);
    create_behaviours(ctx);
    init_random_speeches(ctx);
    init_probabilities(ctx);
    init_follow_groups(ctx);
    init_special_behaviours(ctx);
    init_max_effects();
    dump_stats();
}
