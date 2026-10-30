#ifndef EXEC_SPRITE_HPP
#define EXEC_SPRITE_HPP

#include "exec_common.hpp"

class unit_t;
class library_t;
class behaviour_t;
class effect_t;
class speech_t;
class sprite_bank_t;
class sprite_t;

// a slot of sprite_bank_t: empty / behaviour / effect (speeches are effects)
class sprite_t : public no_copy_t {
    friend class sprite_bank_t;

private:
    // a stopgap solution to make behaviour field names fit the max length
    #define bhv_t behaviour_t

    // the sprites on display by library, see pick_target()
    using cast_t = std::unordered_map<lib_id_t, std::vector<sprite_t*>>;

    enum flags_t : uint32_t {
        GEN_FLAGS(left, wanteffects, effect, speech, enroute,
                  asleep, dragged, hovered)
        special = asleep | dragged | hovered,
    };

    const unit_t *unit_;        // what is being animated, nullptr = empty slot
    const bhv_t *bhv_;          // what the sprite belongs to; UNIT_ if empty
    sprite_t *parent_;          // what the sprite follows / effect belongs to
    T2FV position_;             // position, Y being the sprite's lower edge
    intptr_t uuid_;             // animation the frame belongs to, 0 = nothing
    int64_t next_frame_time_;   // start time of the next frame
    uint32_t curr_frame_;       // frame currently on display
    float depth_;               // sorting key, see depth_sort()
    struct {
        gravity_t gravity_;     // effects only, see effect_t::select_gravity()
        flags_t flags_:24;      // flags; see flags_t
    };
    union {
        struct { // behaviours only
            const library_t *lib_;      // the sprite's library
            T2FV velocity_;             // per-frame velocity
            int64_t next_move_time_;    // when the sprite should move
            union { // (this is possible because special states do not expire)
                int64_t next_bhv_time_; // when the behaviour is due to change
                const bhv_t *resume_;   // what to resume after special states
            };
        };
        struct { // effects only
            const unit_t *parent_unit_; // what OFFSET_ was computed for
            T2FV offset_;               // see effect_t::gravity()
            int64_t next_wait_time_;    // when the effect is due to end + wait
            int64_t next_rep_time_;     // when the effect is due to repeat
        };
    };

    #undef bhv_t

    bool get_flag(flags_t flag) const { return flags_ & flag; }
    bool set_flag(flags_t flag, bool value) {
        const auto prev_flag = get_flag(flag);
        flags_ = (value) ? (flags_ | flag) : (flags_ & ~flag);
        return prev_flag;
    }

    bool is_free() const { return !unit_; }
    void free() { unit_ = nullptr; }
    sprite_t() { free(); }

    static bool depth_sort(const sprite_t &a, const sprite_t &b) {
        return (a.depth_ != b.depth_) // followers go in front of their parent
             ? (a.depth_ > b.depth_)
             : (a.get_flag(effect) && !b.get_flag(effect));
    }
    void append(std::vector<T4FV> &display_list) const {
        display_list.emplace_back(T4FV{{position_.x, position_.y,
                                        float(curr_frame_), float(uuid_)}});
    }
    void advance(int64_t time, bool reset);
    const effect_t *fx() const;

    void init_behaviour(const library_t &lib, uint32_t *seed, int64_t time,
            T2IV area, const cast_t &cast); // frees the slot if not loaded
    bool next_behaviour(
            uint32_t *seed, int64_t time, T2IV area, const cast_t &cast);
    // all but NEXT_BHV_TIME_, which stays up to the caller (if not special)
    void apply_behaviour(uint32_t *seed, T2IV area, const behaviour_t &next);
    bool move_behaviour(uint32_t *seed, int64_t time, T2IV area);
    bool follow_behaviour(uint32_t *seed, T2IV area);
    void update_behaviour(
            uint32_t *seed, int64_t time, T2IV area, const cast_t &cast);

    // special states: no expiry, no movement, no following; on leaving the
    // last one, RESUME_ is re-applied with a fresh duration and PARENT_ kept
    const behaviour_t &base_bhv() const; // RESUME_ if special
    void pick_special(uint32_t *seed, T2IV area, flags_t state,
            const behaviour_t &special_bhv);
    void drop_special(uint32_t *seed, T2IV area, flags_t state, int64_t time);
    const behaviour_t *hover(uint32_t *seed, T2IV cursor, ENGD *engd) const;

    void set_drag(uint32_t *seed, int64_t time, T2IV area, bool on, bool hover);
    void set_sleep(uint32_t *seed, int64_t time, T2IV area, bool on);

    sprite_t *pick_target(
            uint32_t *seed, const behaviour_t &bhv, const cast_t &cast) const;
    bool has_target() const; // a follow target is set, or an area point
    bool parent_moved_on() const;

    void preinit_effect(sprite_t &parent, const effect_t &fx);
    void init_effect(uint32_t *seed, int64_t time); // frees slot if not loaded
    void move_effect(bool reset); // moves effects to their parent sprite
    bool repeat_effect(int64_t time); // returns whether to add a repeat effect
    bool wake_effect(uint32_t *seed, int64_t time); // returns whether deployed
    bool update_effect(uint32_t *seed, int64_t time); // false if freed/waiting
    void track_parent(uint32_t *seed, int64_t time);

    // a speech slot: one per character, idle in the wait list between speeches;
    // BHV_ is the parent's last seen behaviour, SPECIAL flags whether it
    // was in a special state then, NEXT_REP_TIME_ is when random speeches
    // are allowed again
    void preinit_speech(sprite_t &parent, const speech_t &initial);
    // starts a new speech in place if the parent's behaviour changed
    bool do_speech(uint32_t *seed, int64_t time, uint64_t chance);
    // returns false if idle
    bool update_speech(uint32_t *seed, int64_t time, uint64_t chance);

public:
    string_view_t name() const;
};

// the sprites of a run: [0, SHOWN_) on display, [SHOWN_, WAITING_) free,
// [WAITING_, end) effects waiting to be deployed
class sprite_bank_t : public no_copy_t {
public:
    struct input_t {
        T2IV cursor;
        uint32_t attr; // UFR_ flags
        uint32_t isel; // the sprite under the cursor, > INT32_MAX if none
        bool hover; // conf_t::hover, may change mid-run
        ENGD *engd;
    };
    enum action_t { // see menu_action()
        remove_one, remove_all, sleep_one, sleep_all, control_p1, control_p2,
    };

private:
    // lets std::make_unique() reach the private constructor
    struct slot_t : public sprite_t { slot_t() = default; };

    // by pointer: sorted on each update, while cross-pointers stay valid
    std::vector<std::unique_ptr<slot_t>> sprites_;
    std::vector<T4FV> display_; // display list, refilled on each update
    sprite_t::cast_t cast_;
    uint32_t shown_;
    uint32_t waiting_;
    T2IV area_;
    bool effects_;
    bool speech_;
    const conf_t::spin_t &speech_chance_;
    T2FV grab_; // ACTIVE_'s position relative to the cursor, if dragged
    sprite_t *active_; // the dragged or hovered character, see its flags
    sprite_t *menu_target_; // the character the local menu is for
    bool menu_pending_; // see menu_request()
    uint32_t buttons_; // the mouse buttons held during the previous update

    void add_behaviour(const library_t &lib, uint32_t *seed, int64_t time);
    void add_effect(sprite_t &parent, const effect_t &fx);
    // CHR's behaviour effects, if WANTEFFECTS (see apply_behaviour()) is set;
    // unlike DP, entering special states deploys their effects too
    void deploy_effects(sprite_t &chr);
    void handle_input(uint32_t *seed, int64_t time, const input_t &in);
    // ALL: every character of CHR's library, all set to the same state
    void toggle_sleep(uint32_t *seed, int64_t time, sprite_t &chr, bool all);

    // moves sprites between regions: shown / free / waiting
    static bool try_recycle(uint32_t iter, uint32_t if_free, uint32_t if_not,
                        std::vector<std::unique_ptr<slot_t>> &sprites);
public:
    sprite_bank_t(const selection_t &sel, uint32_t *seed, int64_t time,
            T2IV area, bool effects, bool speech,
            const conf_t::spin_t &speech_chance);

    uint32_t capacity() const { return sprites_.size(); }
    uint32_t shown() const { return shown_; }
    void update(uint32_t *seed, int64_t time, const input_t &in);
    bool is_asleep(const sprite_t &chr) const {
        return chr.get_flag(sprite_t::asleep);
    }
    // the player controlling CHR, 0 if none
    uint32_t controller(const sprite_t &chr) const { return 0; }
    // the character right-clicked during the last update(), once per click
    const sprite_t *menu_request() {
        if (!menu_pending_) return nullptr;
        menu_pending_ = false;
        return menu_target_;
    }
    // the target's state is read when the action comes, not when the menu
    // opens, as frames keep running in between
    void menu_action(uint32_t *seed, int64_t time, action_t action);
    T4FV *display() { return display_.data(); }
};

#endif // EXEC_SPRITE_HPP
