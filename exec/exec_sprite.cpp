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
, next_move_time_(time)
, next_frame_time_(time)
, curr_frame_{}
, left_{} {
    advance(time, choose_behaviour(seed, time, area));
}

void sprite_t::advance(int64_t time, bool reset) {
    if (reset) curr_frame_ = UINT32_MAX;
    assert(bhv_);
    if (auto uuid = bhv_->advance(left_, time, next_frame_time_, curr_frame_))
        uuid_ = uuid;
}

bool sprite_t::choose_behaviour(uint32_t *seed, int64_t time, T2IV area) {
    // [TODO:] the special mouse-driven states (dragged, mouseover, sleeping)
    //         and the player-controlled ones belong here
    // [TODO:] once behaviours can carry a movement override, i.e. when they
    //         have a follow target or a player, that should be handled here
    if (time < next_bhv_time_) return false;
    const auto prev_bhv = bhv_;
    const bool prev_left = left_;
    bhv_ = lib_.next_behaviour(seed, prev_bhv);
    assert(bhv_);
    next_bhv_time_ = time + bhv_->choose_duration(seed);
    velocity_ = bhv_->choose_velocity(seed);
    // a moving sprite faces where it goes, a static one keeps its current
    // side unless it has none yet - either way the side has to be settled
    // before the center is looked up, as the two sides differ
    if (velocity_.x) left_ = (velocity_.x < 0.f);
    else if (!prev_bhv) left_ = RNG_Load(seed) & 1;
    const auto center = bhv_->center(left_);
    if (prev_bhv) { // sprite already on screen: aligning the new center
        const auto prev_center = prev_bhv->center(prev_left);
        position_.x += prev_center.x - center.x;
        position_.y -= prev_center.y - center.y;
    } else if (bhv_->dims(left_).x) { // first anim of the sprite: placing it
        position_.x = RNG_Load(seed) % std::max(area.x - 2 * center.x, 1);
        position_.y = RNG_Load(seed) % std::max(area.y - 2 * center.y, 1)
                    + 2 * center.y;
    } else { // nothing to show yet: staying put at -INFINITY, sorted last
        next_bhv_time_ = std::numeric_limits<int64_t>::max();
        position_ = T2FV{{0.f, -INFINITY}};
        velocity_ = T2FV{{}};
    }
    // [TODO:] spawn the behaviour effects here, if allowed
    return true;
}

bool sprite_t::move_one_step(int64_t time, T2IV area) {
    if (time < next_move_time_) return false;
    bool reset = false;
    next_move_time_ = ((time > next_move_time_ + FRM_WAIT)
                      ? time : next_move_time_) + FRM_WAIT;
    auto dims = bhv_->dims(left_);
    // [TODO:] player-controlled characters should stop at the area boundary
    //         instead of flipping the velocity
    if (bound_crossed(velocity_.x, position_.x + dims.x, dims.x, area.x)) {
        position_.x += bhv_->center(left_).x - bhv_->center(!left_).x;
        velocity_.x = -velocity_.x;
        left_ = !left_;
        reset = true;
    }
    if (bound_crossed(velocity_.y, position_.y, dims.y, area.y))
        velocity_.y = -velocity_.y;
    position_.x += velocity_.x;
    position_.y += velocity_.y;
    return reset;
}

void sprite_t::update(uint32_t *seed, int64_t time, T2IV area) {
    bool reset = choose_behaviour(seed, time, area);
    reset |= move_one_step(time, area);
    advance(time, reset);
}
