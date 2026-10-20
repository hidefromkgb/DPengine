#include "exec_bhv.hpp"
#include "exec_parser.hpp"
#include "exec_prng.hpp"



gravity_t effect_t::select_gravity(bool left, uint32_t *seed) const {
    return random_selection<false>(gravity_[left], seed);
}

effect_t::input_t::input_t(const std::string_view &str) {
    static const std::unordered_map<std::string, gravity_flags_t> p = {
        {"any",         any        }, {"any-not_center", not_center  },
        {"top_left",    top_left   }, {"top_right",      top_right   },
        {"top",         top        }, {"bottom",         bottom      },
        {"bottom_left", bottom_left}, {"bottom_right",   bottom_right},
        {"left",        center_left}, {"right",          center_right},
        {"center",      center     },
    };
    token_t line({}, str);
    name = process_string(line);
    bhv = ascii_to_lower(process_string(line));
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

effect_t::effect_t(const input_t &in, const std::string &path)
: duration_{{(decltype(duration_.x))(1000.f * in.duration),
             (decltype(duration_.y))(1000.f * in.repeat_delay)}}
, follow_(in.follow)
, gravity_{init_gravity(in.placement_right, in.centering_right),
           init_gravity(in.placement_left, in.centering_left)} {
#ifdef DEV_MODE
    debug_ = in;
#endif // DEV_MODE
    if (!path.empty()) {
        prepare_source(false, concat_path({path, in.right_image}));
        prepare_source(true, concat_path({path, in.left_image}));
    }
    set_loop(false, !in.prevent_loop);
    set_loop(true, !in.prevent_loop);
}

speech_t::input_t::input_t(const std::string_view &str) {
    token_t line({}, str);
    name = ascii_to_lower(process_string(line));
    text = process_string(line);
    auto sound_files = process_array(line);
    if (!sound_files.empty()) sound = sound_files[0];
    skip = process_bool(line, skip);
    group = process_float(line, group);
}

behaviour_t::input_t::input_t(const std::string_view &str) {
    static const std::unordered_map<std::string, movement_flags_t> m = {
        {"horizontal_vertical", move_hv  }, {"mouseover", move_mouse},
        {"diagonal_horizontal", move_dh  }, {"dragged",   move_drag },
        {"diagonal_vertical",   move_dv  }, {"sleep",     move_sleep},
        {"horizontal_only",     move_horz}, {"none",      move_none },
        {"vertical_only",       move_vert}, {"all",       move_all  },
        {"diagonal_only",       move_diag},
    };
    static const std::unordered_map<std::string, bool> f = {
        {"false", false}, {"true",   true},
        {"fixed", false}, {"mirror", true},
    };
    token_t line({}, str);
    name = ascii_to_lower(process_string(line));
    chance = process_float(line, chance);
    max_duration = process_float(line, max_duration);
    min_duration = process_float(line, min_duration);
    speed = process_float(line, speed);
    right_image = process_string(line);
    left_image = process_string(line);
    movement = process_map(line, m, movement);
    linked_bhv = ascii_to_lower(process_string(line));
    bgn_speech = ascii_to_lower(process_string(line));
    end_speech = ascii_to_lower(process_string(line));
    skip = process_bool(line, skip);
    target_xy.x = process_float(line, target_xy.x);
    target_xy.y = process_float(line, target_xy.y);
    follow_target = ascii_to_lower(process_string(line));
    auto_follow_img = process_bool(line, auto_follow_img);
    follow_stop_bhv = ascii_to_lower(process_string(line));
    follow_mov_bhv = ascii_to_lower(process_string(line));
    right_img_center = process_quoted_int_pair(line, right_img_center);
    left_img_center = process_quoted_int_pair(line, left_img_center);
    prevent_loop = process_bool(line, prevent_loop);
    group = process_float(line, group);
    mirror_target_xy = process_map(line, f, mirror_target_xy);
}

static const T2IV process_duration(float min_d, float max_d) {
    if (min_d > max_d) std::swap(min_d, max_d);
    max_d -= min_d;
    return T2IV{{(decltype(T2IV::x))(1000.f * min_d),
                 (decltype(T2IV::y))(1000.f * max_d)}};
}

behaviour_t::behaviour_t(const input_t &in, bhv_id_t id, bhv_id_t linked_id,
        bhv_id_t follow_grp_id, lib_id_t follow_tgt, const std::string &path,
        std::vector<int16_t> bgn_speech, std::vector<int16_t> end_speech,
        const eff_vec_t &eff)
: id_(id)
, linked_id_(linked_id)
, follow_grp_id_(follow_grp_id)
, duration_(process_duration(in.min_duration, in.max_duration))
, movement_speed_(in.speed * FRM_WAIT / 30.f)
, movement_(in.movement)
, bgn_speech_idx_(std::move(bgn_speech))
, end_speech_idx_(std::move(end_speech))
, follow_tgt_(follow_tgt)
, follow_offset_{process_follow_offset(false, in, follow_tgt),
                 process_follow_offset(true, in, follow_tgt)}
, effects_(process_effects(path, eff))
, follow_(follow_tgt || in.target_xy.x || in.target_xy.y) {
#ifdef DEV_MODE
    debug_ = in;
#endif // DEV_MODE
    if (!path.empty()) {
        prepare_source(false, concat_path({path, in.right_image}));
        prepare_source(true, concat_path({path, in.left_image}));
    }
    set_center(false, in.right_img_center);
    set_center(true, in.left_img_center);
    set_loop(false, !in.prevent_loop);
    set_loop(true, !in.prevent_loop);
}

// this is the whole speech switching logic from Desktop Ponies, trust me.
// 0 means no speech, negatives are behaviour-specific speeches, positives
// are random speeches. END can only yield negatives (end speech from bhv,
// if present) or 0; BGN might yield 0, negatives (start speech from bhv),
// or positives (pre-filled random speeches taken from library that match
// the BGN group, if start speech is absent)
int16_t behaviour_t::select_speech(uint32_t *seed, uint32_t chance,
        const behaviour_t &prev, const behaviour_t &curr) {
    constexpr uint64_t max_chance = 100;
    constexpr uint64_t mul_chance // [0;max_chance]->[0;~0+5], type matters
            = (double(uint32_t(~0 - 1)) + max_chance) / max_chance;
    auto bgn = random_selection<true>(curr.bgn_speech_idx_, seed);
    auto end = random_selection<true>(prev.end_speech_idx_, seed);
    // priority in DP: 1. start speech; 2. end speech; 3. random speech;
    end = (bgn >= 0) ? (end >= 0) ? bgn : end : bgn;
    // TODO: fix the case when (end > 0) gets replaced with (bgn = 0);
    //       at the time this is hypothetical, but can become relevant
    return ((end < 0) || (RNG_Load(seed) < mul_chance * chance)) ? end : 0;
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
                std::swap(retn.x, retn.y);
        }
        retn.x *= (prng & 0x400) ? -movement_speed_ : movement_speed_;
        retn.y *= (prng & 0x800) ? -movement_speed_ : movement_speed_;
    }
    return retn;
}

int32_t behaviour_t::choose_duration(uint32_t *seed) const {
    return duration_.u + ((duration_.v) ? RNG_Load(seed) % duration_.v : 0);
}
