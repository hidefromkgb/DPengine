#ifndef EXEC_SPRITE_HPP
#define EXEC_SPRITE_HPP

#include "exec_common.hpp"

class library_t;
class behaviour_t;

// a single on-screen sprite: one character of one library, running one
// behaviour at a time. Effects, speeches, follow targets and the special
// mouse-driven states are not here yet, see the [TODO:]'s in update()
class sprite_t : public no_copy_t {
private:
    const library_t &lib_;      // parent library
    const behaviour_t *bhv_;    // current behaviour, or nullptr if none
    T2FV position_;             // position, y being the sprite's lower edge
    T2FV velocity_;             // per-frame velocity
    intptr_t uuid_;             // animation the frame belongs to, 0 = nothing
    int64_t next_bhv_time_;     // when the current behaviour is due to change
    int64_t next_move_time_;    // when the sprite should move
    int64_t next_frame_time_;   // start time of the next frame
    uint32_t curr_frame_;       // frame currently on display
    bool left_;                 // the sprite faces left

    void advance(int64_t time, bool reset);
    bool choose_behaviour(uint32_t *seed, int64_t time, T2IV area);
    bool move_one_step(int64_t time, T2IV area);

public:
    sprite_t(const library_t &lib, uint32_t *seed, int64_t time, T2IV area);

    void update(uint32_t *seed, int64_t time, T2IV area);

    const library_t &library() const { return lib_; }

    // returns whether the entry is to be shown at all; the ones that are not
    // never had anything to show, and are sorted last, see choose_behaviour()
    bool append(std::vector<T4FV> &display_list) const {
        display_list.emplace_back(T4FV{{position_.x, position_.y,
                                        float(curr_frame_), float(uuid_)}});
        return uuid_;
    }
    static bool depth_sort(const sprite_t &a, const sprite_t &b) {
        return a.position_.y > b.position_.y;
    }
};

#endif // EXEC_SPRITE_HPP
