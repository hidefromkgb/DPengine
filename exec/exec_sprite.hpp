#ifndef EXEC_SPRITE_HPP
#define EXEC_SPRITE_HPP

#include "exec_common.hpp"

class unit_t;
class library_t;
class behaviour_t;
class effect_t;
class sprite_bank_t;
class sprite_t;

// the characters on display by library, see sprite_t::pick_target()
using cast_t = std::unordered_map<lib_id_t, std::vector<const sprite_t*>>;

// a slot of sprite_bank_t: empty, a behaviour, or an effect
class sprite_t : public no_copy_t {
    friend class sprite_bank_t;

private:
    enum flags_t : uint32_t { GEN_FLAGS(left, effect, enroute) };

    const unit_t *unit_;        // what is being animated, nullptr = empty slot
    const behaviour_t *bhv_;    // what the sprite belongs to; UNIT_ if empty
    const sprite_t *parent_;    // what the sprite follows / effect belongs to
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
            int64_t next_bhv_time_;     // when the behaviour is due to change
            int64_t next_move_time_;    // when the sprite should move
            T2FV velocity_;             // per-frame velocity
        };
        struct { // effects only
            const unit_t *parent_unit_; // what OFFSET_ was computed for
            int64_t next_wait_time_;    // when the effect is due to end + wait
            int64_t next_rep_time_;     // when the effect is due to repeat
            T2FV offset_;               // see effect_t::gravity()
        };
    };

    bool is_left() const { return flags_ & left; }
    bool is_effect() const { return flags_ & effect; }
    bool is_enroute() const { return flags_ & enroute; }
    void set_flag(flags_t flag, bool value) {
        flags_ = (value) ? (flags_ | flag) : (flags_ & ~flag);
    }

    bool is_free() const { return !unit_; }
    void free() { unit_ = nullptr; }
    sprite_t() { free(); }

    static bool depth_sort(const sprite_t &a, const sprite_t &b) {
        // followers go in front of their parent
        return (a.depth_ != b.depth_) ? (a.depth_ > b.depth_)
                                      : (a.is_effect() && !b.is_effect());
    }
    void append(std::vector<T4FV> &display_list) const {
        display_list.emplace_back(T4FV{{position_.x, position_.y,
                                        float(curr_frame_), float(uuid_)}});
    }
    void advance(int64_t time, bool reset);
    const effect_t *eff() const;
    // the actual sprite to pick when this sprite is clicked
    const sprite_t &owner() const { return (is_effect()) ? *parent_ : *this; }

    void init_behaviour(const library_t &lib, uint32_t *seed, int64_t time,
            T2IV area, const cast_t &cast); // frees the slot if not loaded
    bool next_behaviour(
            uint32_t *seed, int64_t time, T2IV area, const cast_t &cast);
    bool move_behaviour(uint32_t *seed, int64_t time, T2IV area);
    bool follow_behaviour(uint32_t *seed, T2IV area);
    // returns whether a different behaviour started
    bool update_behaviour(
            uint32_t *seed, int64_t time, T2IV area, const cast_t &cast);

    const sprite_t *pick_target(
            uint32_t *seed, const behaviour_t &bhv, const cast_t &cast) const;
    bool has_target() const; // a follow target is set, or an area point
    bool parent_moved_on() const;

    void preinit_effect(const sprite_t &parent, const effect_t &eff);
    void init_effect(uint32_t *seed, int64_t time); // frees slot if not loaded
    void move_effect(bool reset); // moves effects to their parent sprite
    bool repeat_effect(int64_t time); // returns whether to add a repeat effect
    bool wake_effect(uint32_t *seed, int64_t time); // returns whether deployed
    bool update_effect(uint32_t *seed, int64_t time); // false if freed/waiting

public:
    const std::string &name() const;
};

// the sprites of a run: [0, SHOWN_) on display, [SHOWN_, WAITING_) free,
// [WAITING_, end) effects waiting to be deployed
class sprite_bank_t : public no_copy_t {
private:
    // lets std::make_unique() reach the private constructor
    struct slot_t : sprite_t { slot_t() = default; };

    // by pointer: sorted on each update, while cross-pointers stay valid
    std::vector<std::unique_ptr<slot_t>> sprites_;
    std::vector<T4FV> display_; // display list, refilled on each update
    cast_t cast_;
    uint32_t shown_;
    uint32_t waiting_;
    T2IV area_;
    bool effects_;

    void add_behaviour(const library_t &lib, uint32_t *seed, int64_t time);
    void add_effect(const sprite_t &parent, const effect_t &eff);
    void deploy_effects(const sprite_t &chr);

public:
    sprite_bank_t(const selection_t &sel, uint32_t *seed, int64_t time,
            T2IV area, bool effects);

    uint32_t capacity() const { return sprites_.size(); }
    uint32_t shown() const { return shown_; }
    const sprite_t *pick(int32_t isel) const;
    void update(uint32_t *seed, int64_t time);
    T4FV *display() { return display_.data(); }
};

#endif // EXEC_SPRITE_HPP
