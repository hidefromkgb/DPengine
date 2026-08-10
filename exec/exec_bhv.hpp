#ifndef EXEC_BHV_HPP
#define EXEC_BHV_HPP



#include "exec_common.hpp"
#include "exec_unit.hpp"



enum movement_flags_t : uint32_t {
    GEN_FLAGS(move_none, move_drag, move_sleep, move_mouse,
              move_horz, move_vert, move_diag)
    move_hv = move_horz | move_vert, move_dh = move_diag | move_horz,
    move_dv = move_diag | move_vert, move_all= move_diag | move_horz | move_vert
};

class effect_t : public unit_t {
protected:
    enum gravity_flags_t : uint8_t { top_left = 0, top, top_right,
        center_left, center, center_right, bottom_left, bottom, bottom_right,
        any, not_center, };
    const struct {
        gravity_flags_t placement:4;
        gravity_flags_t centering:4;
    } gravity_flags_[2];
    const T2IV duration_; // u = duration, v = repeat_delay
    const bool follow_;
    std::vector<T2IV> gravity_[2]; // can't be made const: size unknown at init

    inline T2IV select_gravity(bool left, uint32_t *seed) const;

public:
    class input_t {
    public:
        std::string name;                                        // NOT USED
        std::string bhv;                                         // USED
        std::string right_image;                                 // USED
        std::string left_image;                                  // USED
        float duration = 5.f;                                    // USED
        float repeat_delay = 0.f;                                // USED
        gravity_flags_t placement_right = gravity_flags_t::any;  // USED
        gravity_flags_t centering_right = gravity_flags_t::any;  // USED
        gravity_flags_t placement_left = gravity_flags_t::any;   // USED
        gravity_flags_t centering_left = gravity_flags_t::any;   // USED
        bool follow = false;                                     // USED
        bool prevent_loop = false;                               // USED

        input_t() = default;
        input_t(const std::string_view &str);
        bool validate() const {
            bool okay = !name.empty();
            okay &= !bhv.empty();
            okay &= !right_image.empty();
            okay &= !left_image.empty();
            return okay;
        }
    };

#ifdef DEV_MODE
    input_t debug_;
#endif // DEV_MODE

    effect_t(const input_t &in, const std::string &path);

    // TODO: check if everything is correct with both horz and vert axes
    void finalize(T2IV bhv_right_size, T2IV bhv_left_size) {
        auto process_side = [](gravity_flags_t geff, gravity_flags_t gbhv,
                T2IV eff, T2IV bhv) {
            // depends on the order of elements in gravity_flags_t!
            static const std::array<std::vector<gravity_flags_t>, 9 + 2> i = {{
                {top_left    }, {top         }, {top_right   }, {center_left },
                {center      }, {center_right}, {bottom_left }, {bottom      },
                {bottom_right},
                // any
                {top_left    , top         , top_right   , center_left ,
                 center      , center_right, bottom_left , bottom      ,
                 bottom_right},
                // not_center
                {top_left    , top         , top_right   , center_left ,
                 center_right, bottom_left , bottom      , bottom_right},
            }};
            // depends on the order of elements in gravity_flags_t!
            static const std::array<T2IV, 9> g = {{
                {{0, 2}}, {{1, 2}}, {{2, 2}}, {{0, 1}}, {{1, 1}},
                {{2, 1}}, {{0, 0}}, {{1, 0}}, {{2, 0}},
            }};
            std::vector<T2IV> retn;
            for (auto &b : i[gbhv])
                for (auto &e : i[geff])
                    retn.emplace_back(T2IV{{
                                (g[b].x * bhv.x - g[e].x * eff.x) / 2,
                                (g[b].y * bhv.y - g[e].y * eff.y) / 2}});
            return retn;
        };
        gravity_[false] = process_side(gravity_flags_[false].centering,
                gravity_flags_[false].placement, dims(false), bhv_right_size);
        gravity_[true] = process_side(gravity_flags_[true].centering,
                gravity_flags_[true].placement, dims(true), bhv_left_size);
    }
};

using eff_vec_t = ref_vec_t<effect_t::input_t>;

class speech_t : public effect_t {
public:
    class input_t {
    public:
        std::string name;   // NOT USED
        std::string text;   // NOT USED
        std::string sound;  // NOT USED
        bool skip = false;  // USED
        int group = 0;      // USED

        input_t() = default;
        input_t(const std::string_view &str);
        bool validate() const { return !text.empty(); }
    };

#ifdef DEV_MODE
    input_t debug_;
#endif // DEV_MODE

private:
    const std::string sound_;

    static effect_t::input_t convert_input(const speech_t::input_t &in) {
        effect_t::input_t retn;
        retn.name = in.name;
        retn.bhv = "group = " + std::to_string(in.group) + ", skip = "
                 + std::to_string(in.skip) + ", sound = " + in.sound;
        retn.right_image = retn.left_image = in.text;
        retn.duration = 0.5f + 0.065f * in.text.size();
        // fixed value taken from DP codebase - only applies to random speech:
        retn.repeat_delay = 10.f;
        retn.placement_right = retn.placement_left = gravity_flags_t::top;
        retn.centering_right = retn.centering_left = gravity_flags_t::bottom;
        retn.follow = true;
        retn.prevent_loop = true;
        return retn;
    }

    static void render_text(uint32_t *data) {
    }

public:
    speech_t(const input_t &in)
    : effect_t(convert_input(in), std::string())
    , sound_(in.sound) {
#ifdef DEV_MODE
        debug_ = in;
#endif // DEV_MODE
        // TODO: call this one properly!
        prepare_source(false, in.text, 1, 1, render_text);
    }
};

class behaviour_t : public unit_t {
public:
    class input_t {
    public:
        std::string name;                      // USED
        float chance = 0.f;                    // USED
        float max_duration = 15.f;             // USED
        float min_duration = 5.f;              // USED
        float speed = 3.f;                     // USED
        std::string right_image;               // USED
        std::string left_image;                // USED
        movement_flags_t movement = move_all;  // USED
        std::string linked_bhv;                // USED
        std::string bgn_speech;                // USED
        std::string end_speech;                // USED
        bool skip = false;                     // USED
        T2IV target_xy = {{0, 0}};             // USED
        std::string follow_target;             // USED
        bool auto_follow_img = true;           // USED
        std::string follow_stop_bhv;           // USED
        std::string follow_mov_bhv;            // USED
        T2IV right_img_center = {{0, 0}};      // USED
        T2IV left_img_center = {{0, 0}};       // USED
        bool prevent_loop = false;             // USED
        int group = 0;                         // USED
        bool mirror_target_xy = false;         // USED

        input_t() = default;
        input_t(const std::string_view &str);
        bool validate() const {
            bool okay = !name.empty();
            okay &= !right_image.empty();
            okay &= !left_image.empty();
            return okay;
        }
    };

    static int16_t select_speech(uint32_t *seed, uint32_t chance,
            const behaviour_t &prev, const behaviour_t &curr);

#ifdef DEV_MODE
    input_t debug_;
#endif // DEV_MODE

private:
    const bhv_id_t id_;
    const bhv_id_t linked_id_;
    // follow_grp_id_ is only needed for its group info: have to
    // pick moving/stationary follow images from this very group
    const bhv_id_t follow_grp_id_;
    const T2IV duration_; // u = min, v = max
    const float movement_speed_;
    const movement_flags_t movement_;
    const std::vector<int16_t> bgn_speech_idx_; // speech indices from library
    const std::vector<int16_t> end_speech_idx_;
    const lib_id_t follow_tgt_;
    const T2IV follow_offset_[2];
    const std::vector<std::unique_ptr<effect_t>> effects_;

    std::vector<std::unique_ptr<effect_t>> process_effects(
            const std::string &path, const eff_vec_t &eff) {
        std::vector<std::unique_ptr<effect_t>> retn;
        for (auto &e : eff)
            retn.emplace_back(std::make_unique<effect_t>(e, path));
        return retn;
    }

public:
    bhv_id_t id() const { return id_; }
    bhv_id_t linked_id() const { return linked_id_; }
    bhv_id_t follow_grp_id() const { return follow_grp_id_; }

    behaviour_t(const input_t &in, bhv_id_t id, bhv_id_t linked_id,
            bhv_id_t follow_grp_id, lib_id_t follow_tgt,
            const std::string &path, std::vector<int16_t> bgn_speech,
            std::vector<int16_t> end_speech, const eff_vec_t &eff);
};

#endif // EXEC_BHV_HPP
