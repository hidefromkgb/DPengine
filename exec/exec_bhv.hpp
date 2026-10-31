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
private:
    // a 3x3 grid row by row from the top, each row padded to 4 entries, so
    // that X = ID & 3, Y = ID >> 2 (after an up-correction), see gravity();
    // X = {0, 1, 2}, Y = {0, 1, 2} so it's okay to use them as coefficients
    // (the padding, X = 3, holds the sets of directions: ANY and NOT_CENTER)
    enum gravity_flags_t : uint8_t {
           top_left = 0,    top,    top_right,        any,
        center_left = 4, center, center_right, not_center,
        bottom_left = 8, bottom, bottom_right,
    };
    const T2IV duration_; // u = duration, v = repeat_delay
    const bool follow_;
    // placement/centering point pairs to choose from, see gravity()
    const std::vector<gravity_t> gravity_[2];

    static std::vector<gravity_t> init_gravity(
            gravity_flags_t placement, gravity_flags_t centering) {
        static_assert(((1 <<    top_left) | (1 <<    top) | (1 <<    top_right)
                     | (1 << center_left) | (1 << center) | (1 << center_right)
                     | (1 << bottom_left) | (1 << bottom) | (1 << bottom_right))
                     == 0x777);
        static_assert(((1 <<    top_left) | (1 <<    top) | (1 <<    top_right)
                     | (1 << center_left) |                 (1 << center_right)
                     | (1 << bottom_left) | (1 << bottom) | (1 << bottom_right))
                     == 0x757);
        auto mask = [](gravity_flags_t flg) -> uint16_t {
            return ((flg & 3) == 3) ? (flg == any) ? 0x777 : 0x757 : (1 << flg);
        };
        // this precomputes Y = 2 - (ID >> 2) early, making it Y = ID >> 2
        auto up = [](int id) { return (uint8_t)((8 - (id & ~3)) | (id & 3)); };
        std::vector<gravity_t> retn;
        for (uint16_t pt = mask(placement); pt; pt &= pt - 1)
            for (uint16_t fx = mask(centering); fx; fx &= fx - 1)
                retn.emplace_back(gravity_t{up(__builtin_ctz(pt)),
                                            up(__builtin_ctz(fx))});
        return retn; // count trailing 0 bits ---^
    }

public:
    // offset from the parent's lower left corner, Y going up
    T2FV gravity(bool left, gravity_t pair, T2IV parent_dims) const {
        const T2IV pt{{pair.pt & 3, pair.pt >> 2}}; // parent
        const T2IV fx{{pair.fx & 3, pair.fx >> 2}}; // effect
        const auto g_x = (pt.x * parent_dims.x - fx.x * dims(left).x) >> 1;
        const auto g_y = (pt.y * parent_dims.y - fx.y * dims(left).y) >> 1;
        return {{(decltype(T2FV::x))g_x, (decltype(T2FV::y))g_y}};
    }
    gravity_t select_gravity(bool left, uint32_t *seed) const;

    class input_t {
    public:
        string_t *name = nullptr;
        string_t *bhv = nullptr;
        string_t *right_image = nullptr;
        string_t *left_image = nullptr;
        float duration = 5.f;
        float repeat_delay = 0.f;
        gravity_flags_t placement_right = gravity_flags_t::any;
        gravity_flags_t centering_right = gravity_flags_t::any;
        gravity_flags_t placement_left = gravity_flags_t::any;
        gravity_flags_t centering_left = gravity_flags_t::any;
        bool follow = false;
        bool prevent_loop = false;

        input_t(const input_t &) = delete;
        input_t &operator=(const input_t &) = delete;
        input_t(input_t &&rhs)
        : name(rhs.name)
        , bhv(rhs.bhv)
        , right_image(rhs.right_image)
        , left_image(rhs.left_image)
        , duration(rhs.duration)
        , repeat_delay(rhs.repeat_delay)
        , placement_right(rhs.placement_right)
        , centering_right(rhs.centering_right)
        , placement_left(rhs.placement_left)
        , centering_left(rhs.centering_left)
        , follow(rhs.follow)
        , prevent_loop(rhs.prevent_loop) {
            rhs.name = rhs.bhv = rhs.right_image = rhs.left_image = nullptr;
        }
        input_t &operator=(input_t &&) = delete;
        ~input_t() {
            free_string(name);
            free_string(bhv);
            free_string(right_image);
            free_string(left_image);
        }
        input_t() = default;
        input_t(string_view_t str);
        bool validate() const {
            bool okay = !!get_view(name).size;
            okay &= !!get_view(bhv).size;
            okay &= !!get_view(right_image).size;
            okay &= !!get_view(left_image).size;
            return okay;
        }
    };

    effect_t(const input_t &in, string_view_t path);

    T2IV duration() const { return duration_; } // u = duration, v = repeat
    bool follow() const { return follow_; }
};

using fx_vec_t = ref_vec_t<effect_t::input_t>;

class speech_t : public effect_t {
public:
    class input_t {
    public:
        string_t *name = nullptr;
        string_t *text = nullptr;
        string_t *sound = nullptr;
        bool skip = false;
        int group = 0;

        input_t(const input_t &) = delete;
        input_t &operator=(const input_t &) = delete;
        input_t(input_t &&rhs)
        : name(rhs.name)
        , text(rhs.text)
        , sound(rhs.sound)
        , skip(rhs.skip)
        , group(rhs.group) {
            rhs.name = rhs.text = rhs.sound = nullptr;
        }
        input_t &operator=(input_t &&) = delete;
        ~input_t() {
            free_string(name);
            free_string(text);
            free_string(sound);
        }
        input_t() = default;
        input_t(string_view_t str);
        bool validate() const { return !!get_view(text).size; }
    };

    // sends MSG_WTGD / MSG_WTDA to a window that has a font
    using text_fn_t = std::function<intptr_t(uint32_t msg, AINF *anim)>;

private:
    string_t *text_;
    string_t *sound_;

    static effect_t::input_t convert_input(const speech_t::input_t &in);

public:
    ~speech_t() {
        free_string(text_);
        free_string(sound_);
    }
    speech_t(const input_t &in) : effect_t(convert_input(in), string_view_t{}) {
        text_ = make_string(get_view(in.text));
        sound_ = make_string(get_view(in.sound));
    }
    void render( // draws the speech bubble unless already done
            uint32_t fg, uint32_t bg, T2IV max_dims, const text_fn_t &text_fn);
};

class behaviour_t : public unit_t {
public:
    class input_t {
    public:
        string_t *name = nullptr;
        float chance = 0.f;
        float max_duration = 15.f;
        float min_duration = 5.f;
        float speed = 3.f;
        string_t *right_image = nullptr;
        string_t *left_image = nullptr;
        movement_flags_t movement = move_all;
        string_t *linked_bhv = nullptr;
        string_t *bgn_speech = nullptr;
        string_t *end_speech = nullptr;
        bool skip = false;
        T2IV target_xy = {{0, 0}};
        string_t *follow_target = nullptr;
        bool auto_follow_img = true;
        string_t *follow_stop_bhv = nullptr;
        string_t *follow_mov_bhv = nullptr;
        T2IV right_img_center = {{0, 0}};
        T2IV left_img_center = {{0, 0}};
        bool prevent_loop = false;
        int group = 0;
        bool mirror_target_xy = false;

        input_t(const input_t &) = delete;
        input_t &operator=(const input_t &) = delete;
        input_t(input_t &&rhs)
        : name(rhs.name)
        , chance(rhs.chance)
        , max_duration(rhs.max_duration)
        , min_duration(rhs.min_duration)
        , speed(rhs.speed)
        , right_image(rhs.right_image)
        , left_image(rhs.left_image)
        , movement(rhs.movement)
        , linked_bhv(rhs.linked_bhv)
        , bgn_speech(rhs.bgn_speech)
        , end_speech(rhs.end_speech)
        , skip(rhs.skip)
        , target_xy(rhs.target_xy)
        , follow_target(rhs.follow_target)
        , auto_follow_img(rhs.auto_follow_img)
        , follow_stop_bhv(rhs.follow_stop_bhv)
        , follow_mov_bhv(rhs.follow_mov_bhv)
        , right_img_center(rhs.right_img_center)
        , left_img_center(rhs.left_img_center)
        , prevent_loop(rhs.prevent_loop)
        , group(rhs.group)
        , mirror_target_xy(rhs.mirror_target_xy) {
            rhs.name = rhs.right_image = rhs.left_image
                     = rhs.linked_bhv = rhs.bgn_speech = rhs.end_speech
                     = rhs.follow_target = rhs.follow_stop_bhv
                     = rhs.follow_mov_bhv = nullptr;
        }
        input_t &operator=(input_t &&) = delete;
        ~input_t() {
            free_string(name);
            free_string(right_image);     free_string(left_image);
            free_string(linked_bhv);      free_string(follow_target);
            free_string(bgn_speech);      free_string(end_speech);
            free_string(follow_stop_bhv); free_string(follow_mov_bhv);
        }
        input_t() = default;
        input_t(string_view_t str);
        bool validate() const {
            bool okay = !!get_view(name).size;
            okay &= !!get_view(right_image).size;
            okay &= !!get_view(left_image).size;
            return okay;
        }
    };

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
            string_view_t path, const fx_vec_t &fx) {
        std::vector<std::unique_ptr<effect_t>> retn;
        for (auto &f : fx)
            retn.emplace_back(std::make_unique<effect_t>(f, path));
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
            bhv_id_t follow_grp_id, const fx_vec_t &fx,
            const speech_t *bgn_speech, const speech_t *end_speech,
            lib_id_t follow_tgt, string_view_t path);
};

#endif // EXEC_BHV_HPP
