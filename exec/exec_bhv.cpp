#include "exec_bhv.hpp"
#include "exec_parser.hpp"
#include "exec_prng.hpp"



gravity_t effect_t::select_gravity(bool left, uint32_t *seed) const {
    return random_selection<false>(gravity_[left], seed);
}

effect_t::input_t::input_t(string_view_t str) {
    static const std::unordered_map<size_t, gravity_flags_t> p = {
        {CHR_HASH(L, 'a','n','y'),                                         any},
        {CHR_HASH(L, 't','o','p','_','l','e','f','t'),                top_left},
        {CHR_HASH(L, 't','o','p'),                                         top},
        {CHR_HASH(L, 't','o','p','_','r','i','g','h','t'),           top_right},
        {CHR_HASH(L, 'l','e','f','t'),                             center_left},
        {CHR_HASH(L, 'c','e','n','t','e','r'),                          center},
        {CHR_HASH(L, 'r','i','g','h','t'),                        center_right},
        {CHR_HASH(L, 'b','o','t','t','o','m','_','l','e','f','t'), bottom_left},
        {CHR_HASH(L, 'b','o','t','t','o','m'),                          bottom},
        {CHR_HASH(L, 'b','o','t','t','o','m','_',
                     'r','i','g','h','t'),                        bottom_right},
        {CHR_HASH(L, 'a','n','y','-','n','o','t','_',
                     'c','e','n','t','e','r'),                      not_center},
    };
    auto line = local_make_tokenizer(str);
    name = process_string(line);
    bhv = process_string(line);
    right_image = process_string(line);
    left_image = process_string(line);
    duration = process_float(line, duration);
    repeat_delay = process_float(line, repeat_delay);
    placement_right = process_map(line, p, placement_right);
    centering_right = process_map(line, p, centering_right);
    placement_left = process_map(line, p, placement_left);
    centering_left = process_map(line, p, centering_left);
    follow = process_bool(line, follow);
    prevent_loop = process_bool(line, prevent_loop);
}

effect_t::effect_t(const input_t &in, string_view_t path)
: duration_{{(decltype(duration_.x))(1000.f * in.duration),
             (decltype(duration_.y))(1000.f * in.repeat_delay)}}
, follow_(in.follow)
, gravity_{init_gravity(in.placement_right, in.centering_right),
           init_gravity(in.placement_left, in.centering_left)} {
    if (path.size) {
        local_string_t *right
                = make_string(path, ILV_DSEP, get_view(in.right_image));
        local_string_t *left
                = make_string(path, ILV_DSEP, get_view(in.left_image));
        prepare_source(false, get_view(right));
        prepare_source(true, get_view(left));
    }
    set_loop(false, !in.prevent_loop);
    set_loop(true, !in.prevent_loop);
}

speech_t::input_t::input_t(string_view_t str) {
    auto line = local_make_tokenizer(str);
    name = process_string(line);
    text = process_string(line);
    auto sound_files = process_array(line);
    if (!sound_files.empty()) sound = make_string(sound_files[0]);
    skip = process_bool(line, skip);
    group = process_float(line, group);
}

effect_t::input_t speech_t::convert_input(const speech_t::input_t &in) {
    using gravity_flags_t = decltype(effect_t::input_t::placement_right);
    // in DP, character names are prepended to the speech text like this:
    // 'name: "text"' - which adds name.size() + 4 chars to the total length
    // that is used to compute the speech time, and the average name length
    // over the DP animation base is 11 characters, 11 + 4 = 15; the formula
    // budgets 1 s per 15 chars, the const below represents this extra time
    constexpr float extension = 1.f;

    size_t chars = 0;
    for (auto str = get_view(in.text); str.size; chars++)
        str = utf8_skip_char(str);
    effect_t::input_t retn;
    char group[16], skip[16];
    string_view_t g_view{(size_t)sprintf(group, "%d", in.group), group};
    string_view_t s_view{(size_t)sprintf(skip, "%d", in.skip), skip};
    retn.name = make_string(get_view(in.name));
    retn.bhv = make_string(get_imm_view("group = "), g_view,
                           get_imm_view(", skip = "), s_view,
                           get_imm_view(", sound = "), get_view(in.sound));
    retn.right_image = make_string(get_view(in.text));
    retn.left_image = make_string(get_view(in.text));
    retn.duration = extension + 0.5f + (1.f / 15.f) * chars;
    retn.repeat_delay = retn.duration + 10.f; // the end + 10 s, same as in DP
    retn.placement_right = retn.placement_left = gravity_flags_t::top;
    retn.centering_right = retn.centering_left = gravity_flags_t::bottom;
    retn.prevent_loop = true;
    retn.follow = true;
    return retn;
}

void speech_t::render(
        uint32_t fg, uint32_t bg, T2IV max_dims, const text_fn_t &text_fn) {
    if (!is_empty()) return;
    // text margins: 8 px on the sides, 4 on top, 12 at the bottom (arrow too)
    constexpr uint32_t arrow = 8, side = 8, top = 4, bottom = 12;
    AINF anim = {0, uint32_t(max_dims.x), uint32_t(max_dims.y), 0,
                 (uint32_t*)get_view(text_).str};
    const auto size = text_fn(MSG_WTGD, &anim);
    const uint32_t xdim = std_max(2 * side + uint16_t(size), 64u);
    const uint32_t ydim = top + bottom + 1 + uint16_t(size >> 16);
    auto draw = [&](uint32_t *bits) {
        // body, with a 2 px border
        for (uint32_t y = 0; y < ydim - arrow; y++)
            for (uint32_t x = 0; x < xdim; x++)
                bits[y * xdim + x] = ((y < 2) || (y >= ydim - arrow - 2)
                                   || (x < 2) || (x >= xdim - 2)) ? fg : bg;
        for (uint32_t y = ydim - arrow; y < ydim; y++)
            for (uint32_t x = 0; x < xdim; x++)
                bits[y * xdim + x] = 0;
        // rounded corners, drawn in 2x2 blocks
        for (uint32_t y = 0; y < 4; y++)
            for (uint32_t x = 0; x < 4; x++) {
                const auto c = ((x < 2) || (y < 2))
                             ? (x * y > 1) ? fg : 0 : bg;
                const uint32_t rows[2] = {2 * y, ydim - arrow - 2 - 2 * y};
                const uint32_t cols[2] = {2 * x, xdim - 2 - 2 * x};
                for (auto r : rows)
                    for (auto l : cols)
                        bits[r * xdim + l] = bits[r * xdim + l + 1] =
                        bits[(r + 1) * xdim + l] =
                        bits[(r + 1) * xdim + l + 1] = c;
            }
        // the arrow, at 2/3 of the width, overlapping the bottom border
        for (int32_t y = 0; y < 10; y++)
            for (int32_t x = -2; x < 10; x++)
                bits[(ydim - 1 - y) * xdim + xdim - xdim / 3 + x] =
                        ((x >= 0) && ((x & -2) != (y & -2)))
                      ? ((x & -2) > (y & -2)) ? 0 : bg : fg;
        // the text
        AINF text = {(intptr_t)bits, xdim | (side << 16) | (side << 24),
                     ydim | (top << 16) | (bottom << 24), fg,
                     (uint32_t*)get_view(text_).str};
        text_fn(MSG_WTDA, &text);
    };
    char colors[24];
    size_t clr_size = sprintf(
            colors, " %06X / %06X: ", fg & 0xFFFFFF, bg & 0xFFFFFF);
    local_string_t *source = make_string({clr_size, colors}, get_view(text_));
    for (bool left : {false, true})
        if (is_empty(left))
            prepare_source(left, get_view(source), xdim, ydim, draw);
}

behaviour_t::input_t::input_t(string_view_t str) {
    static const std::unordered_map<size_t, movement_flags_t> m = {
        {CHR_HASH(L, 'h','o','r','i','z','o','n','t','a','l','_',
                     'v','e','r','t','i','c','a','l'),         move_hv},
        {CHR_HASH(L, 'd','i','a','g','o','n','a','l','_',
                     'h','o','r','i','z','o','n','t','a','l'), move_dh},
        {CHR_HASH(L, 'd','i','a','g','o','n','a','l','_',
                     'v','e','r','t','i','c','a','l'),         move_dv},
        {CHR_HASH(L, 'h','o','r','i','z','o','n','t','a','l','_',
                     'o','n','l','y'),                       move_horz},
        {CHR_HASH(L, 'v','e','r','t','i','c','a','l','_',
                     'o','n','l','y'),                       move_vert},
        {CHR_HASH(L, 'd','i','a','g','o','n','a','l','_',
                     'o','n','l','y'),                       move_diag},
        {CHR_HASH(L, 'm','o','u','s','e','o','v','e','r'),  move_mouse},
        {CHR_HASH(L, 'd','r','a','g','g','e','d'),           move_drag},
        {CHR_HASH(L, 's','l','e','e','p'),                  move_sleep},
        {CHR_HASH(L, 'n','o','n','e'),                       move_none},
        {CHR_HASH(L, 'a','l','l'),                            move_all},
    };
    static const std::unordered_map<size_t, bool> f = {
        {CHR_HASH(L, 'f','a','l','s','e'),    false},
        {CHR_HASH(L, 't','r','u','e'),         true},
        {CHR_HASH(L, 'f','i','x','e','d'),    false},
        {CHR_HASH(L, 'm','i','r','r','o','r'), true},
    };
    auto line = local_make_tokenizer(str);
    name = process_string(line);
    chance = process_float(line, chance);
    max_duration = process_float(line, max_duration);
    min_duration = process_float(line, min_duration);
    speed = process_float(line, speed);
    right_image = process_string(line);
    left_image = process_string(line);
    movement = process_map(line, m, movement);
    linked_bhv = process_string(line);
    bgn_speech = process_string(line);
    end_speech = process_string(line);
    skip = process_bool(line, skip);
    target_xy.x = process_float(line, target_xy.x);
    target_xy.y = process_float(line, target_xy.y);
    follow_target = process_string(line);
    auto_follow_img = process_bool(line, auto_follow_img);
    follow_stop_bhv = process_string(line);
    follow_mov_bhv = process_string(line);
    right_img_center = process_quoted_int_pair(line, right_img_center);
    left_img_center = process_quoted_int_pair(line, left_img_center);
    prevent_loop = process_bool(line, prevent_loop);
    group = process_float(line, group);
    mirror_target_xy = process_map(line, f, mirror_target_xy);
}

static const T2IV process_duration(float min_d, float max_d) {
    if (min_d > max_d) std_swap(min_d, max_d);
    max_d -= min_d;
    return T2IV{{(decltype(T2IV::x))(1000.f * min_d),
                 (decltype(T2IV::y))(1000.f * max_d)}};
}

behaviour_t::behaviour_t(const input_t &in, bhv_id_t id,
        bhv_id_t linked_id, bhv_id_t follow_grp_id, const fx_vec_t &fx,
        const speech_t *bgn_speech, const speech_t *end_speech,
        lib_id_t follow_tgt, string_view_t path)
: id_(id)
, linked_id_(linked_id)
, follow_grp_id_(follow_grp_id)
, duration_(process_duration(in.min_duration, in.max_duration))
, movement_speed_(in.speed * FRM_WAIT / 30.f)
, movement_(in.movement)
, bgn_speech_(bgn_speech)
, end_speech_(end_speech)
, follow_tgt_(follow_tgt)
, follow_offset_{process_follow_offset(false, in, follow_tgt),
                 process_follow_offset(true, in, follow_tgt)}
, effects_(process_effects(path, fx))
, follow_(follow_tgt || in.target_xy.x || in.target_xy.y) {
    if (path.size) {
        local_string_t *right
                = make_string(path, ILV_DSEP, get_view(in.right_image));
        local_string_t *left
                = make_string(path, ILV_DSEP, get_view(in.left_image));
        prepare_source(false, get_view(right));
        prepare_source(true, get_view(left));
    }
    set_center(false, in.right_img_center);
    set_center(true, in.left_img_center);
    set_loop(false, !in.prevent_loop);
    set_loop(true, !in.prevent_loop);
}

static constexpr std::array<T2FV, 32> get_cos_sin_lut() {
    constexpr float DTR_CONV = M_PI / 180.f;
    std::array<T2FV, 32> retn = {};
    for (auto a = 14; a <= 45; a++)
        retn[a - 14] = T2FV{{std::cos(DTR_CONV * a), std::sin(DTR_CONV * a)}};
    return retn;
}

T2FV behaviour_t::choose_velocity(uint32_t *seed) const {
    static const auto cos_sin_lut = get_cos_sin_lut();
    auto move = movement_;
    T2FV retn{{0.f, 0.f}};
    if (move & move_all) {
        const auto prng = RNG_Load(seed);
        //          vvvv--- move_all selector; bit 5 also for move_{hv,dh,dv}
        //    ...BA9876543210 (bits of prng)
        // !Y ---^^^    ^^^^^--- cos_sin_lut index, [0; 32)
        //  !X ---''--- flip the quadrant for full range
        const auto hv = move & move_hv;
        switch (move & move_all) {
            case move_all:
                switch (prng & 0x1E0) {
                    case 0x000: case 0x020: case 0x040: case 0x060: case 0x080:
                        move = move_horz; break;
                    case 0x0A0: case 0x0C0: case 0x0E0: case 0x100: case 0x120:
                        move = move_vert; break;
                    default: move = move_diag; break; // 6 cases, not 5; is ok
                }
                break;
            case move_hv: move = (prng & 0x020) ? move_horz : move_vert; break;
            case move_dh: move = (prng & 0x020) ? move_diag : move_horz; break;
            case move_dv: move = (prng & 0x020) ? move_diag : move_vert; break;
            default: move &= move_all; break;
        }
        if (move != move_diag) {
            retn = (move == move_vert) ? T2FV{{0.f, 1.f}} : T2FV{{1.f, 0.f}};
        } else {
            // how the angles were meant to be computed, see DP tech doc:
            // - hv=move_none: pure diagonal movement, [15; 75]
            // - hv=move_hv:   used to be move_all so pure diag here, [15; 75]
            // - hv=move_horz: used to be move_dh, [15; 45]
            // - hv=move_vert: used to be move_dv, [45; 75]
            // how the angles are actually computed:
            // - the LUT contains 32 values: [14; 45], (prng % m) -> (prng & m)
            // - 14 degrees not that different from 15, no one will notice
            // - if the full range is needed, 1 extra bit flips (x,y) -> (y,x)
            // - that way, the full range is [14; 76] and 45 is 2x as probable
            retn = cos_sin_lut[prng & 0x1F];
            if (((prng & 0x200) && ((hv == move_none) || (hv == move_hv)))
                    || (hv == move_vert))
                std_swap(retn.x, retn.y);
        }
        retn.x *= (prng & 0x400) ? -movement_speed_ : movement_speed_;
        retn.y *= (prng & 0x800) ? -movement_speed_ : movement_speed_;
    }
    return retn;
}

int32_t behaviour_t::choose_duration(uint32_t *seed) const {
    return duration_.u + RNG_LoadBounded(seed, duration_.v);
}
