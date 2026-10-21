#ifndef EXEC_BHV_HPP
#define EXEC_BHV_HPP



#include "exec_common.hpp"
#include "exec_unit.hpp"



enum movement_flags_t : uint32_t {
    GEN_FLAGS(move_none, move_horz, move_vert, move_diag,
              move_drag, move_sleep, move_mouse)
    move_hv = move_horz | move_vert,
    move_dh = move_diag | move_horz,
    move_dv = move_diag | move_vert,
    move_all = move_diag | move_horz | move_vert,
    move_ctl = move_drag | move_sleep | move_mouse,
};

class effect_t : public unit_t {
protected:
    // a 3x3 grid row by row from the top, each row padded to 4 entries, so
    // that X = ID & 3, Y = ID >> 2 (after an up-correction), see gravity();
    // X = {0, 1, 2}, Y = {0, 1, 2} so it's okay to use them as coefficients
    enum gravity_flags_t : uint8_t {
           top_left =  0,    top,    top_right,    _top_spacer_gravity,
        center_left =  4, center, center_right, _center_spacer_gravity,
        bottom_left =  8, bottom, bottom_right, _bottom_spacer_gravity,
        any = 12, not_center,
        _max_gravity = not_center,
    };
    const T2IV duration_; // u = duration, v = repeat_delay
    const bool follow_;
    // placement/centering point pairs to choose from, see gravity()
    const std::vector<gravity_t> gravity_[2];

    static std::vector<gravity_t> init_gravity(
            gravity_flags_t placement, gravity_flags_t centering) {
        using gravity_table_t = std::vector<gravity_flags_t>;
        static const std::array<gravity_table_t, _max_gravity + 1> gravity = {{
            {   top_left}, {   top}, {   top_right}, {   _top_spacer_gravity},
            {center_left}, {center}, {center_right}, {_center_spacer_gravity},
            {bottom_left}, {bottom}, {bottom_right}, {_bottom_spacer_gravity},
            {   top_left,      top,      top_right,
             center_left,   center,   center_right,
             bottom_left,   bottom,   bottom_right}, // any
            {   top_left,      top,      top_right,
             center_left,             center_right,
             bottom_left,   bottom,   bottom_right}, // not_center
        }};
        auto up = [](gravity_flags_t flags) {
            return uint8_t((8 - (flags & ~3)) | (flags & 3));
        }; // this precomputes Y = 2 - (ID >> 2) early, making it Y = ID >> 2
        std::vector<gravity_t> retn;
        for (auto bhv : gravity[placement])
            for (auto eff : gravity[centering])
                retn.emplace_back(gravity_t{up(bhv), up(eff)});
        return retn;
    }

public:
    // offset from the parent's lower left corner, Y going up
    T2FV gravity(bool left, gravity_t pair, T2IV parent_dims) const {
        const T2IV bhv{{pair.bhv & 3, pair.bhv >> 2}};
        const T2IV eff{{pair.eff & 3, pair.eff >> 2}};
        const auto g_x = (bhv.x * parent_dims.x - eff.x * dims(left).x) >> 1;
        const auto g_y = (bhv.y * parent_dims.y - eff.y * dims(left).y) >> 1;
        return {{(decltype(T2FV::x))g_x, (decltype(T2FV::y))g_y}};
    }
    gravity_t select_gravity(bool left, uint32_t *seed) const;

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

    T2IV duration() const { return duration_; } // u = duration, v = repeat
    bool follow() const { return follow_; }
};

using eff_vec_t = ref_vec_t<effect_t::input_t>;

class speech_t : public effect_t {
public:
    class input_t {
    public:
        std::string name;   // USED
        std::string text;   // USED
        std::string sound;  // NOT USED
        bool skip = false;  // USED
        int group = 0;      // USED

        input_t() = default;
        input_t(const std::string_view &str);
        bool validate() const { return !text.empty(); }
    };

    // sends MSG_WTGD / MSG_WTDA to a window that has a font
    using text_fn_t = std::function<intptr_t(uint32_t msg, AINF *anim)>;

#ifdef DEV_MODE
    input_t debug_;
#endif // DEV_MODE

private:
    const std::string text_;
    const std::string sound_;

    static effect_t::input_t convert_input(const speech_t::input_t &in);

public:
    speech_t(const input_t &in)
    : effect_t(convert_input(in), std::string())
    , text_(in.text)
    , sound_(in.sound) {
#ifdef DEV_MODE
        debug_ = in;
#endif // DEV_MODE
    }

    void render( // draws the speech bubble unless already done
            uint32_t fg, uint32_t bg, T2IV max_dims, const text_fn_t &text_fn);
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

#ifdef DEV_MODE
    input_t debug_;
#endif // DEV_MODE

private:
    const bhv_id_t id_;
    const bhv_id_t linked_id_;
    // follow_grp_id_ is only needed for its group info: have to
    // pick moving/stationary follow images from this very group
    const bhv_id_t follow_grp_id_;
    const T2IV duration_; // u = min, v = possible extension above min
    const float movement_speed_;
    const movement_flags_t movement_;
    const speech_t *const bgn_speech_; // nullptr = none
    const speech_t *const end_speech_; // nullptr = none
    const lib_id_t follow_tgt_;
    const T2FV follow_offset_[2];
    const std::vector<std::unique_ptr<effect_t>> effects_;
    const bool follow_;

    std::vector<std::unique_ptr<effect_t>> process_effects(
            const std::string &path, const eff_vec_t &eff) {
        std::vector<std::unique_ptr<effect_t>> retn;
        for (auto &e : eff)
            retn.emplace_back(std::make_unique<effect_t>(e, path));
        return retn;
    }
    static T2FV process_follow_offset(bool left, const input_t &in, bool tgt) {
        // no target: if the offset is non-0 this follow is area-based, given
        // as 0..100% of the drawing area, so let's translate to 0..1 - or if
        // the offset is 0 then multiplying it does no harm, cannot get zeroer
        if (!tgt) return {{.01f * in.target_xy.x, .01f * in.target_xy.y}};
        left &= !!in.mirror_target_xy;
        return {{decltype(T2FV::x)((left) ? -in.target_xy.x : in.target_xy.x),
                 decltype(T2FV::y)(in.target_xy.y)}};
    }

public:
    bhv_id_t id() const { return id_; }
    bhv_id_t linked_id() const { return linked_id_; }
    bhv_id_t follow_grp_id() const { return follow_grp_id_; }
    T2IV duration() const { return duration_; } // u = min, v = extension
    float speed() const { return movement_speed_; }
    bool follow() const { return follow_; }
    lib_id_t follow_target() const { return follow_tgt_; } // 0 = none
    T2FV follow_offset(bool target_left) const {
        return follow_offset_[target_left];
    }
    const std::vector<std::unique_ptr<effect_t>> &effects() const {
        return effects_;
    }

    T2FV choose_velocity(uint32_t *seed) const;
    int32_t choose_duration(uint32_t *seed) const;

    const speech_t *bgn_speech() const { return bgn_speech_; }
    const speech_t *end_speech() const { return end_speech_; }

    void schedule_upload_full(ENGD *engd) {
        schedule_upload(engd);
        for (auto &e : effects_) e->schedule_upload(engd);
    }
    void finalize_full() {
        finalize();
        for (auto &e : effects_) e->finalize();
    }

    behaviour_t(const input_t &in, bhv_id_t id, bhv_id_t linked_id,
            bhv_id_t follow_grp_id, lib_id_t follow_tgt,
            const std::string &path, const speech_t *bgn_speech,
            const speech_t *end_speech, const eff_vec_t &eff);
};

#endif // EXEC_BHV_HPP
