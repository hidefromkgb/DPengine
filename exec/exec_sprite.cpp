#include <algorithm>
#include <cassert>

#include "exec_library.hpp"
#include "exec_sprite.hpp"



static int bound_crossed(float move, float offs, int32_t bmin, int32_t bmax) {
    if ((move < 0.f) && (offs + move <= bmin)) return -1;
    if ((move > 0.f) && (offs + move >= bmax)) return  1;
    return 0;
}

const behaviour_t *sprite_t::bhv() const {
    return static_cast<const behaviour_t*>(unit_);
}

const effect_t *sprite_t::eff() const {
    return static_cast<const effect_t*>(unit_);
}

void sprite_t::advance(int64_t time, bool reset) {
    if (reset) curr_frame_ = UINT32_MAX;
    assert(unit_);
    if (auto uuid = unit_->advance(left_, time, next_frame_time_, curr_frame_))
        uuid_ = uuid;
}

bool sprite_t::next_behaviour(uint32_t *seed, int64_t time, T2IV area) {
    // [TODO:] the special mouse-driven states (dragged, mouseover, sleeping)
    //         and the player-controlled ones belong here
    // [TODO:] once behaviours can carry a movement override, i.e. when they
    //         have a follow target or a player, that should be handled here
    if (time < next_bhv_time_) return false;
    const auto prev_bhv = bhv();
    const bool prev_left = left_;
    unit_ = lib_->next_behaviour(seed, prev_bhv);
    assert(unit_);
    next_bhv_time_ = time + bhv()->choose_duration(seed);
    velocity_ = bhv()->choose_velocity(seed);
    // a moving sprite faces where it goes, a static one keeps its current
    // side unless it has none yet - either way the side has to be settled
    // before the center is looked up, as the two sides differ
    if (velocity_.x) left_ = (velocity_.x < 0.f);
    else if (!prev_bhv) left_ = RNG_Load(seed) & 1;
    const auto center = bhv()->center(left_);
    if (prev_bhv) { // sprite already on screen: aligning the new center
        const auto prev_center = prev_bhv->center(prev_left);
        position_.x += prev_center.x - center.x;
        position_.y -= prev_center.y - center.y;
    } else if (bhv()->dims(left_).x) { // first anim of the sprite: placing it
        position_.x = RNG_Load(seed) % std::max(area.x - 2 * center.x, 1);
        position_.y = RNG_Load(seed) % std::max(area.y - 2 * center.y, 1)
                    + 2 * center.y;
    }
    return true;
}

bool sprite_t::move_behaviour(int64_t time, T2IV area) {
    if (time < next_move_time_) return false;
    bool reset = false;
    next_move_time_ = ((time > next_move_time_ + FRM_WAIT)
                      ? time : next_move_time_) + FRM_WAIT;
    auto dims = bhv()->dims(left_);
    // [TODO:] player-controlled characters should stop at the area boundary
    //         instead of flipping the velocity
    if (bound_crossed(velocity_.x, position_.x + dims.x, dims.x, area.x)) {
        position_.x += bhv()->center(left_).x - bhv()->center(!left_).x;
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

void sprite_t::move_effect(bool reset) {
    if (!reset && !eff()->follow()) return;
    if (reset || parent_moved_on()) { // a new image or side
        parent_bhv_ = parent_->bhv()->id();
        offset_ = eff()->gravity(
                left_, gravity_, parent_->bhv()->dims(parent_->left_));
    }
    position_ = T2FV{{parent_->position_.x + offset_.x,
                      parent_->position_.y - offset_.y}};
    depth_ = (eff()->follow()) ? parent_->depth_ : position_.y;
}

void sprite_t::preinit_effect(const sprite_t &parent, const effect_t &eff) {
    unit_ = &eff;
    parent_ = &parent;
    parent_bhv_ = parent.bhv()->id();
    next_rep_time_ = 0; // deploy ASAP
}

void sprite_t::init_effect(uint32_t *seed, int64_t time) {
    assert(is_effect());
    uuid_ = 0;
    left_ = parent_->left_;
    gravity_ = eff()->select_gravity(left_, seed);
    const auto span = eff()->duration(); // u = duration, v = repeat
    next_wait_time_ = (span.u) ? time + span.u : never;
    next_rep_time_ = (span.v) ? time + span.v : never;
    move_effect(true);
    advance(time, true);
    if (!uuid_) free(); // failed to load
}

void sprite_t::init_behaviour(
        const library_t &lib, uint32_t *seed, int64_t time, T2IV area) {
    parent_bhv_ = {}; // no parent behaviour (= this is not an effect)
    unit_ = nullptr; // no previous behaviour
    uuid_ = 0;
    lib_ = &lib;
    velocity_ = T2FV{};
    next_bhv_time_ = next_move_time_ = next_frame_time_ = time;
    const bool changed = next_behaviour(seed, time, area);
    depth_ = position_.y;
    advance(time, changed);
    if (!uuid_) free(); // failed to load, never placed
}

bool sprite_t::update_behaviour(uint32_t *seed, int64_t time, T2IV area) {
    if (is_effect()) return false;
    const auto prev_bhv = bhv();
    const bool changed = next_behaviour(seed, time, area);
    const bool flipped = move_behaviour(time, area);
    depth_ = position_.y;
    advance(time, changed || flipped);
    return bhv() != prev_bhv;
}

bool sprite_t::update_effect(uint32_t *seed, int64_t time) {
    const bool moved_on = parent_moved_on();
    if (moved_on) next_rep_time_ = never;
    if ((next_wait_time_ != never) ? (time >= next_wait_time_) : moved_on) {
        if (next_rep_time_ == never)
            free();
        else
            uuid_ = 0; // waiting
        return false;
    }
    // a follower turning with its parent is redeployed on the other side
    const bool flipped = eff()->follow() && (left_ != parent_->left_);
    if (flipped) {
        left_ = parent_->left_;
        gravity_ = eff()->select_gravity(left_, seed);
    }
    move_effect(flipped);
    advance(time, flipped);
    return true;
}

bool sprite_t::repeat_effect(int64_t time) {
    if (time < next_rep_time_) return false;
    next_rep_time_ = never;
    return !parent_moved_on();
}

bool sprite_t::wake_effect(uint32_t *seed, int64_t time) {
    if (repeat_effect(time)) {
        init_effect(seed, time);
        return !is_free();
    }
    if (parent_moved_on()) free();
    return false;
}

const std::string &sprite_t::name() const {
    assert(!owner().is_effect());
    return owner().lib_->name();
}

bool sprite_t::parent_moved_on() const {
    return parent_->bhv()->id() != parent_bhv_;
}

sprite_bank_t::sprite_bank_t(const selection_t &sel, uint32_t *seed,
        int64_t time, T2IV area, bool effects)
: shown_(0), waiting_(0), area_(area), effects_(effects) {
    // [TODO:] budget for the speeches, once there are any
    if (effects_) {
        for (auto &s : sel)
            waiting_ += s.second * (1 + s.first.get().max_effects());
    } else {
        for (auto &s : sel)
            waiting_ += s.second;
    }
    sprites_.reserve(waiting_);
    for (uint32_t iter = 0; iter < waiting_; iter++)
        sprites_.emplace_back(std::make_unique<slot_t>());
    display_.reserve(waiting_);
    for (auto &s : sel)
        for (uint32_t iter = 0; iter < s.second; iter++)
            add_behaviour(s.first, seed, time);
    for (uint32_t iter = (effects_) ? 0 : shown_; iter < shown_; iter++)
        deploy_effects(*sprites_[iter]);
}

void sprite_bank_t::add_behaviour(
        const library_t &lib, uint32_t *seed, int64_t time) {
    assert(shown_ < waiting_);
    auto slot = sprites_[shown_].get();
    slot->init_behaviour(lib, seed, time, area_);
    if (!slot->is_free()) shown_++;
}

void sprite_bank_t::add_effect(const sprite_t &parent, const effect_t &eff) {
    if (shown_ >= waiting_) return; // too bad, no more space in the pool
    // initialize the bare minimum and put the effect on the wait list:
    sprites_[--waiting_]->preinit_effect(parent, eff);
}

void sprite_bank_t::deploy_effects(const sprite_t &chr) {
    for (auto &e : chr.bhv()->effects()) add_effect(chr, *e);
}

void sprite_bank_t::update(uint32_t *seed, int64_t time) {
    // behaviours first, as followers need updated positions
    for (uint32_t iter = 0; iter < shown_; iter++)
        if (sprites_[iter]->update_behaviour(seed, time, area_) && effects_)
            deploy_effects(*sprites_[iter]);
    // backwards, so that swapped-in slots are already updated
    for (uint32_t iter = (effects_) ? shown_ : 0; iter--; ) {
        auto &fx = *sprites_[iter];
        if (!fx.is_effect()) continue;
        // fires when duration >= repeat: active effect spawns more of itself
        if (fx.repeat_effect(time)) add_effect(fx.owner(), *fx.eff());
        if (fx.update_effect(seed, time)) continue;
        // reaching this point means the sprite is inactive, remove it
        std::swap(sprites_[iter], sprites_[--shown_]); // ...from shown sprites
        if (!fx.is_free()) // ...maybe it's alive, then it belongs in wait list
            std::swap(sprites_[shown_], sprites_[--waiting_]);
    }
    // forwards, for the same reason; this deploys new effects too
    for (uint32_t iter = waiting_; iter < sprites_.size(); iter++) {
        auto &fx = *sprites_[iter];
        assert(fx.is_effect());
        // wakes due effects: those just added and those with duration < repeat
        if (!fx.wake_effect(seed, time) && !fx.is_free()) continue;
        // at this point the sprite must be taken out of the wait list
        std::swap(sprites_[iter], sprites_[waiting_++]); // ...to the dumpster
        if (!fx.is_free()) // ...or, if it's alive, to the shown sprites
            std::swap(sprites_[waiting_ - 1], sprites_[shown_++]);
    }
    using sp_t = const std::unique_ptr<slot_t>; // sorting the sprites by Y
    std::stable_sort(sprites_.begin(), sprites_.begin() + shown_,
            [](sp_t &a, sp_t &b) { return sprite_t::depth_sort(*a, *b); });
    display_.clear();
    for (uint32_t iter = 0; iter < shown_; iter++)
        sprites_[iter]->append(display_); // building the display list
}

const sprite_t *sprite_bank_t::pick(int32_t isel) const {
    return (isel >= 0) ? &sprites_[isel]->owner() : nullptr;
}
