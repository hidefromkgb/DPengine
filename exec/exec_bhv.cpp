#include "exec_bhv.hpp"
#include "exec_parser.hpp"
#include "exec_prng.hpp"



inline T2IV effect_t::select_gravity(bool left, uint32_t *seed) const {
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
: gravity_flags_{{in.placement_right, in.centering_right},
                 {in.placement_left, in.centering_left}}
, duration_{{(decltype(duration_.x))(1000.f * in.duration),
             (decltype(duration_.y))(1000.f * in.repeat_delay)}}
, follow_(in.follow) {
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

behaviour_t::behaviour_t(const input_t &in, bhv_id_t id, bhv_id_t linked_id,
        bhv_id_t follow_grp_id, lib_id_t follow_tgt, const std::string &path,
        std::vector<int16_t> bgn_speech, std::vector<int16_t> end_speech,
        const eff_vec_t &eff)
: id_(id)
, linked_id_(linked_id)
, follow_grp_id_(follow_grp_id)
, duration_{{(decltype(duration_.x))(1000.f *
                            std::min(in.min_duration, in.max_duration)),
             (decltype(duration_.y))(1000.f *
                            std::max(in.min_duration, in.max_duration))}}
, movement_speed_(in.speed * FRM_WAIT / 30.f)
, movement_(in.movement)
, bgn_speech_idx_(std::move(bgn_speech))
, end_speech_idx_(std::move(end_speech))
, follow_tgt_(follow_tgt)
, follow_offset_{in.target_xy, (in.mirror_target_xy)
                                   ? T2IV{{-in.target_xy.x, in.target_xy.y}}
                                   : in.target_xy}
, effects_(process_effects(path, eff)) {
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
