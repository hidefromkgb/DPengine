#include <algorithm>
#include <cassert>
#include <cmath>

#include "exec_library.hpp"
#include "exec_sprite.hpp"



static int bound_crossed(float move, float offs, int32_t bmin, int32_t bmax) {
    if ((move < 0.f) && (offs + move <= bmin)) return -1;
    if ((move > 0.f) && (offs + move >= bmax)) return  1;
    return 0;
}

sprite_t::sprite_t(
        const library_t &lib, uint32_t *seed, int64_t time, T2IV area)
: lib_(lib)
, bhv_{}
, position_{}
, velocity_{}
, uuid_{}
, next_bhv_time_(time)
, curr_moved_time_(time)
, curr_frame_time_(time)
, curr_frame_{}
, left_{} {
    choose_behaviour(seed, time, area);
}

void sprite_t::choose_behaviour(uint32_t *seed, int64_t time, T2IV area) {
    const auto prev_bhv = bhv_;
    const bool prev_left = left_;
    bhv_ = lib_.next_behaviour(seed, prev_bhv);
    assert(bhv_);
    curr_frame_ = 0;
    curr_frame_time_ = time;
    next_bhv_time_ = time + bhv_->choose_duration(seed);
    velocity_ = bhv_->choose_velocity(seed);
    // a moving sprite faces where it goes, a static one keeps its current
    // side unless it has none yet - either way the side has to be settled
    // before the center is looked up, as the two sides differ
    if (velocity_.x) left_ = (velocity_.x < 0.f);
    else if (!prev_bhv) left_ = RNG_Load(seed) & 1;
    const auto center = bhv_->center(left_);
    if (prev_bhv) { // existing sprite: aligning the new center with the old
        const auto prev_center = prev_bhv->center(prev_left);
        position_.x += prev_center.x - center.x;
        position_.y -= prev_center.y - center.y;
    } else { // newly created sprite: performing the initial placement
        position_.x = RNG_Load(seed) % std::max(area.x - 2 * center.x, 1);
        position_.y = RNG_Load(seed) % std::max(area.y - 2 * center.y, 1)
                    + 2 * center.y;
    }
    int64_t null_time = 0;
    uint32_t null_frame = 0;
    uuid_ = bhv_->advance(left_, 1, null_time, null_frame);
    // [TODO:] spawn the behaviour effects here, if allowed
}

void sprite_t::update(uint32_t *seed, int64_t time, T2IV area) {
    // [TODO:] the special mouse-driven states (dragged, mouseover, sleeping)
    //         and the player-controlled ones belong here, before the rest
    if (time >= next_bhv_time_) choose_behaviour(seed, time, area);
    // [TODO:] correct CURR here once behaviours can carry a movement
    //         override, i.e. when they have a follow target or a player

    if (auto uuid = bhv_->advance(left_, time, curr_frame_time_, curr_frame_))
        uuid_ = uuid;

    if (time - curr_moved_time_ < FRM_WAIT) return;
    auto dims = bhv_->dims(left_);
    if (bound_crossed(velocity_.x, position_.x + dims.x, dims.x, area.x)) {
        position_.x += bhv_->center(left_).x - bhv_->center(!left_).x;
        velocity_.x = -velocity_.x;
        left_ = !left_;
    }
    if (bound_crossed(velocity_.y, position_.y, dims.y, area.y))
        velocity_.y = -velocity_.y;
    position_.x += velocity_.x;
    position_.y += velocity_.y;
    curr_moved_time_ = time;
}
