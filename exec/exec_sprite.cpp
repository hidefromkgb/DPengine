#include <algorithm>
#include <cassert>

#include "exec_library.hpp"
#include "exec_sprite.hpp"



static int bound_crossed(float move, float offs, int32_t bmin, int32_t bmax) {
    if ((move < 0.f) && (offs + move <= bmin)) return -1;
    if ((move > 0.f) && (offs + move >= bmax)) return  1;
    return 0;
}

const effect_t *sprite_t::fx() const {
    return static_cast<const effect_t*>(unit_);
}

void sprite_t::advance(int64_t time, bool reset) {
    if (reset) curr_frame_ = UINT32_MAX;
    assert(unit_);
    if (auto uuid = unit_->advance(
            get_flag(left), time, next_frame_time_, curr_frame_))
        uuid_ = uuid;
}

bool sprite_t::next_behaviour(
        uint32_t *seed, int64_t time, T2IV area, const cast_t &cast) {
    // [TODO:] the player-controlled states belong here
    if ((time < next_bhv_time_) || get_flag(special)) return false;
    const auto next = lib_->next_behaviour(seed, bhv_);
    assert(next);
    if (next->follow_target()) {
        const auto target = pick_target(seed, *next, cast);
        // nobody to follow, keeping the current behaviour for another update:
        // unlike links or the very first pick, random ones can be redone later
        if (!target && bhv_ && !bhv_->linked_id()) return false;
        parent_ = target;
    } else {
        parent_ = nullptr;
    }
    apply_behaviour(seed, area, *next);
    next_bhv_time_ = time + bhv_->choose_duration(seed);
    return true;
}

void sprite_t::apply_behaviour(
        uint32_t *seed, T2IV area, const behaviour_t &next) {
    const auto prev_unit = unit_;
    const bool prev_left = get_flag(left);
    bhv_ = &next;
    unit_ = (has_target()) ? &lib_->follow_override(seed, next, false) : &next;
    set_flag(enroute, false);
    set_flag(wanteffects, true); // see sprite_bank_t::deploy_effects()
    velocity_ = (get_flag(special)) ? T2FV{} : bhv_->choose_velocity(seed);
    // a moving sprite faces where it goes, a static one keeps its current
    // side unless it has none yet - either way the side has to be settled
    // before the center is looked up, as the two sides differ
    if (velocity_.x || !prev_unit)
        set_flag(left, (velocity_.x) ? velocity_.x < 0.f
                                     : RNG_LoadBounded(seed, 2));
    const auto center = unit_->center(get_flag(left));
    if (prev_unit) { // sprite already on screen: aligning the new center
        const auto prev_center = prev_unit->center(prev_left);
        position_.x += prev_center.x - center.x;
        position_.y -= prev_center.y - center.y;
    } else if (unit_->dims(get_flag(left)).x) { // first anim: placing sprite
        auto x = area.x - 2 * center.x;
        x = (x <= 1) ? x >> 1 : (decltype(x))RNG_LoadBounded(seed, x);
        auto y = area.y - 2 * center.y;
        y = (y <= 1) ? y : (decltype(y))RNG_LoadBounded(seed, y);
        position_ = {{decltype(position_.x)(x),
                      decltype(position_.y)(y) + 2 * center.y}};
    }
}

const behaviour_t &sprite_t::base_bhv() const {
    return *((get_flag(special)) ? resume_ : bhv_);
}

void sprite_t::pick_special(uint32_t *seed, T2IV area, flags_t state,
        const behaviour_t &special_bhv) {
    assert(state && ((special & state) == state));
    const bool was_special = get_flag(special);
    if (set_flag(state, true)) return; // STATE was set already, exit early
    if (was_special && (&special_bhv == bhv_)) return; // already in place
    if (!was_special) resume_ = bhv_; // NEXT_BHV_TIME_ is of no use from now
    apply_behaviour(seed, area, special_bhv); // never expires, never moves
    curr_frame_ = UINT32_MAX; // restarted by the next update_behaviour()
}

void sprite_t::drop_special(
        uint32_t *seed, T2IV area, flags_t state, int64_t time) {
    assert(state && ((special & state) == state));
    if (!set_flag(state, false)) return; // STATE was reset already, exit early
    if (get_flag(special)) return; // still in another special state
    // PARENT_ is still RESUME_'s target, has_target() ignored it until now
    apply_behaviour(seed, area, *resume_);
    next_bhv_time_ = time + bhv_->choose_duration(seed); // overwrites RESUME_
    curr_frame_ = UINT32_MAX; // restarted by the next update_behaviour()
}

void sprite_t::set_sleep(uint32_t *seed, int64_t time, T2IV area, bool on) {
    if (on == get_flag(asleep)) return;
    if (on) { // sleep overrides hover, while drag keeps going
        pick_special(seed, area, asleep, lib_->sleep_bhv(seed, base_bhv()));
    } else if (get_flag(dragged)) { // woke up mid-drag
        drop_special(seed, area, dragged, time); // only to be re-picked
        pick_special(seed, area, dragged, lib_->drag_bhv(seed, base_bhv()));
    }
    drop_special(seed, area, (on) ? hovered : asleep, time);
}

void sprite_t::set_drag(
        uint32_t *seed, int64_t time, T2IV area, bool on, bool hover) {
    if (on == get_flag(dragged)) return;
    if (on) { // sleeping characters keep sleeping
        auto &d = (get_flag(asleep)) ? *bhv_ : lib_->drag_bhv(seed, base_bhv());
        pick_special(seed, area, dragged, d);
    } else if (!get_flag(asleep) && hover) { // as in DP
        pick_special(seed, area, hovered, lib_->hover_bhv(seed, base_bhv()));
    }
    drop_special(seed, area, (on) ? hovered : dragged, time);
}

const behaviour_t *sprite_t::hover(
        uint32_t *seed, T2IV cursor, ENGD *engd) const {
    if (get_flag(special)) return nullptr;
    const auto &hover_bhv = lib_->hover_bhv(seed, *bhv_);
    // HOVER_BHV has to be under the cursor too, or it flickers in and out
    const auto prev_center = unit_->center(get_flag(left));
    const auto center = hover_bhv.center(get_flag(left));
    int64_t time_next = 0;
    AINF test = {0, 0, 0, UINT32_MAX}; // HOVER_BHV starts anew, from frame #0
    test.uuid = hover_bhv.advance(get_flag(left), 0, time_next, test.fcnt);
    if (!test.uuid) return nullptr;
    test.xdim = cursor.x - prev_center.x + center.x - (int32_t)position_.x;
    test.ydim = cursor.y + prev_center.y - center.y - (int32_t)position_.y;
    cEngineCallback(engd, ECB_TEST, intptr_t(&test));
    return (test.fcnt) ? &hover_bhv : nullptr;
}

sprite_t *sprite_t::pick_target(
        uint32_t *seed, const behaviour_t &bhv, const cast_t &cast) const {
    auto ic = find_in_map(cast, bhv.follow_target());
    if (!ic) return nullptr;
    assert(lib_->id() != bhv.follow_target()); // illegal to follow own library
    // [TODO:] DP tech doc allows self-follows but the DP base has 0 such cases
    //         (might be worth considering to prohibit on import?)
    return (*ic)[RNG_LoadBounded(seed, ic->size())];
}

bool sprite_t::has_target() const {
    return !get_flag(special)
        && ((bhv_->follow_target()) ? !!parent_ : bhv_->follow());
}

bool sprite_t::move_behaviour(uint32_t *seed, int64_t time, T2IV area) {
    if (time < next_move_time_) return false;
    next_move_time_ = ((time > next_move_time_ + FRM_WAIT)
                      ? time : next_move_time_) + FRM_WAIT;
    if (has_target()) return follow_behaviour(seed, area);
    const auto dims = unit_->dims(get_flag(left));
    // [TODO:] player-controlled characters should stop at the area boundary
    //         instead of flipping the velocity
    // [TODO:] dodging the cursor within NCDR px when hover is on, see DP's
    //         ReboundToAvoidCursor(); the squared radius can be per frame
    const bool flipped
            = bound_crossed(velocity_.x, position_.x + dims.x, dims.x, area.x);
    if (flipped) {
        position_.x += unit_->center(get_flag(left)).x
                     - unit_->center(!get_flag(left)).x;
        velocity_.x = -velocity_.x;
        set_flag(left, !get_flag(left));
    }
    if (bound_crossed(velocity_.y, position_.y, dims.y, area.y))
        velocity_.y = -velocity_.y;
    position_.x += velocity_.x;
    position_.y += velocity_.y;
    return flipped;
}

static float fast_rsqrt(float x) { // I'm a John Carmack wannabe, mmkay?
    union { float f; uint32_t u; } y = {x};
    x *= -.5f;
    // technically, in C++ this is illegal - but a-okay for the future C port:
    y.u = (0xBE6EB3BFu - y.u) >> 1;
    y.f *= 1.5f + x * y.f * y.f; // repeat this line for better precision
    return y.f;
}

bool sprite_t::follow_behaviour(uint32_t *seed, T2IV area) {
    const T2FV curr{{position_.x + unit_->center(get_flag(left)).x,
                     position_.y - unit_->center(get_flag(left)).y}};
    if (parent_) { // target X/Y is an offset from the target's center
        const auto offset = bhv_->follow_offset(parent_->get_flag(left));
        const auto center = parent_->unit_->center(parent_->get_flag(left));
        velocity_ = {{parent_->position_.x + offset.x + center.x - curr.x,
                      parent_->position_.y + offset.y - center.y - curr.y}};
    } else { // target X/Y is area-based: 0 = min, 1 = max
        const auto offset = bhv_->follow_offset(false);
        velocity_ = {{offset.x * area.x - curr.x, offset.y * area.y - curr.y}};
    }
    const auto speed = bhv_->speed();
    const auto sq_dist = velocity_.x * velocity_.x + velocity_.y * velocity_.y;
    const bool movement = (speed > 0.f) && (sq_dist >= 1.f);
    if (!movement) {
        velocity_.x = velocity_.y = 0.f;
    } else if (sq_dist > speed * speed) {
        const auto step = speed * fast_rsqrt(sq_dist);
        velocity_.x *= step;
        velocity_.y *= step;
    } // otherwise, VELOCITY_ would bring the sprite to the exact ending spot
    const auto prev_unit = unit_;
    const bool prev_left = get_flag(left);
    if (movement && velocity_.x) set_flag(left, velocity_.x < 0.f);
    if (movement != get_flag(enroute)) { // images re-picked on stopping/moving
        set_flag(enroute, movement);
        unit_ = &lib_->follow_override(seed, *bhv_, movement);
    }
    if ((get_flag(left) == prev_left) && (unit_ == prev_unit)) {
        position_ = {{position_.x + velocity_.x, position_.y + velocity_.y}};
        return false;
    }
    position_ = {{curr.x + velocity_.x - unit_->center(get_flag(left)).x,
                  curr.y + velocity_.y + unit_->center(get_flag(left)).y}};
    return true;
}

void sprite_t::move_effect(bool reset) {
    if (!reset && !fx()->follow()) return;
    if (reset || (parent_->unit_ != parent_unit_)) { // a new image or side
        parent_unit_ = parent_->unit_;
        offset_ = fx()->gravity(get_flag(left), gravity_,
                parent_unit_->dims(parent_->get_flag(left)));
    }
    position_ = {{parent_->position_.x + offset_.x,
                  parent_->position_.y - offset_.y}};
    depth_ = (fx()->follow()) ? parent_->depth_ : position_.y;
}

void sprite_t::preinit_effect(sprite_t &parent, const effect_t &fx) {
    flags_ = effect;
    unit_ = &fx;
    parent_ = &parent;
    bhv_ = parent.bhv_; // the behaviour that deploys it
    next_rep_time_ = 0; // deploy ASAP
}

void sprite_t::init_effect(uint32_t *seed, int64_t time) {
    assert(get_flag(effect));
    uuid_ = 0;
    set_flag(left, parent_->get_flag(left));
    gravity_ = fx()->select_gravity(get_flag(left), seed);
    const auto span = fx()->duration(); // u = duration, v = repeat
    next_wait_time_ = (span.u) ? time + span.u : never;
    next_rep_time_ = (span.v) ? time + span.v : never;
    move_effect(true);
    advance(time, true);
    if (!uuid_) free(); // failed to load
}

void sprite_t::init_behaviour(const library_t &lib, uint32_t *seed,
        int64_t time, T2IV area, const cast_t &cast) {
    flags_ = {};
    parent_ = nullptr;
    unit_ = bhv_ = nullptr; // no previous behaviour
    uuid_ = 0;
    lib_ = &lib;
    velocity_ = {};
    next_bhv_time_ = next_move_time_ = next_frame_time_ = time;
    const bool changed = next_behaviour(seed, time, area, cast);
    depth_ = position_.y;
    advance(time, changed);
    if (!uuid_) free(); // failed to load, never placed
}

void sprite_t::update_behaviour(
        uint32_t *seed, int64_t time, T2IV area, const cast_t &cast) {
    if (get_flag(effect)) return;
    const bool changed = next_behaviour(seed, time, area, cast);
    const bool flipped = move_behaviour(seed, time, area);
    depth_ = position_.y;
    advance(time, changed || flipped);
}

void sprite_t::track_parent(uint32_t *seed, int64_t time) {
    // a follower turning with its parent is redeployed on the other side
    const bool flipped = fx()->follow()
            && (get_flag(left) != parent_->get_flag(left));
    if (flipped) {
        set_flag(left, parent_->get_flag(left));
        gravity_ = fx()->select_gravity(get_flag(left), seed);
    }
    move_effect(flipped);
    advance(time, flipped);
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
    track_parent(seed, time);
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

void sprite_t::preinit_speech(sprite_t &parent, const speech_t &initial) {
    flags_ = effect | speech;
    unit_ = &initial;
    parent_ = &parent;
    bhv_ = parent.bhv_; // no speech at spawn, as parent_moved_on() == false
    uuid_ = 0;
    next_wait_time_ = next_rep_time_ = 0; // random speeches allowed at once
}

bool sprite_t::do_speech(uint32_t *seed, int64_t time, uint64_t chance) {
    if (!parent_moved_on()) return false; // v-- screw the type: it's an effect
    const bool was_special = set_flag(special, parent_->get_flag(special));
    // random lines need the cooldown to be over and no target to follow;
    // in special states the target is kept but ignored, see has_target()
    const bool random = (time >= next_rep_time_)
                     && (get_flag(special) || !parent_->parent_);
    const speech_t *speech = nullptr; // silent when resuming, as in DP
    if (parent_->get_flag(hovered)) { // random line on enter but not from drag
        if (!was_special && random)
            speech = parent_->lib_->force_random_speech(seed, *parent_->bhv_);
    } else if (get_flag(special)) { // sleep or drag: the start line only
        speech = parent_->bhv_->bgn_speech();
    } else if (!was_special) {
        speech = parent_->lib_->transition_speech(
                seed, (random) ? chance : 0, *bhv_, *parent_->bhv_);
    }
    bhv_ = parent_->bhv_;
    if (!speech) return false;
    const auto cooldown = next_rep_time_;
    unit_ = speech;
    init_effect(seed, time);
    if (!is_free()) return true; // loaded, reporting success
    unit_ = speech;
    next_wait_time_ = 0;
    next_rep_time_ = cooldown;
    return false; // failed to load, going idle
}

bool sprite_t::update_speech(uint32_t *seed, int64_t time, uint64_t chance) {
    if (do_speech(seed, time, chance)) return true; // replaced in-situ
    if (time >= next_wait_time_) {
        uuid_ = 0; // idle
        return false;
    }
    track_parent(seed, time);
    return true;
}

string_view_t sprite_t::name() const {
    assert(!get_flag(effect));
    return lib_->name();
}

bool sprite_t::parent_moved_on() const {
    return parent_->bhv_ != bhv_;
}

sprite_bank_t::sprite_bank_t(const selection_t &sel, uint32_t *seed,
        int64_t time, T2IV area, bool effects, bool speech,
        const conf_t::spin_t &speech_chance)
: shown_(0)
, waiting_(0)
, area_(area)
, effects_(effects)
, speech_(speech)
, speech_chance_(speech_chance)
, grab_{}
, active_(nullptr)
, menu_target_(nullptr)
, menu_pending_(false)
, buttons_(0) {
    for (auto &s : sel) {
        auto &lib = s.first.get();
        waiting_ += s.second * (1 + ((effects_) ? lib.max_effects() : 0)
                                  + ((speech_ && lib.first_speech()) ? 1 : 0));
    }
    sprites_.reserve(waiting_);
    for (uint32_t iter = 0; iter < waiting_; iter++)
        sprites_.emplace_back(std::make_unique<slot_t>());
    display_.reserve(waiting_);
    for (auto &s : sel)
        for (uint32_t iter = 0; iter < s.second; iter++)
            add_behaviour(s.first, seed, time);
    // empty until here, so the first behaviours follow no one
    for (uint32_t iter = 0; iter < shown_; iter++)
        cast_[sprites_[iter]->lib_->id()].emplace_back(sprites_[iter].get());
}

void sprite_bank_t::add_behaviour(
        const library_t &lib, uint32_t *seed, int64_t time) {
    assert(shown_ < waiting_);
    auto slot = sprites_[shown_].get();
    slot->init_behaviour(lib, seed, time, area_, cast_);
    if (slot->is_free()) return;
    shown_++;
    const auto placeholder = (speech_) ? lib.first_speech() : nullptr;
    if (placeholder && (shown_ < waiting_)) // the character's speech slot
        sprites_[--waiting_]->preinit_speech(*slot, *placeholder);
}

void sprite_bank_t::add_effect(sprite_t &parent, const effect_t &fx) {
    if (shown_ >= waiting_) return; // too bad, no more space in the pool
    // initialize the bare minimum and put the effect on the wait list
    sprites_[--waiting_]->preinit_effect(parent, fx);
}

void sprite_bank_t::deploy_effects(sprite_t &chr) {
    // the flag first: cleared even when EFFECTS_ is off, so that turning it
    // on mid-run doesn't deploy the effects of an earlier behaviour change
    if (chr.set_flag(sprite_t::wanteffects, false) && effects_)
        for (auto &fx : chr.bhv_->effects()) add_effect(chr, *fx);
}

void sprite_bank_t::handle_input(
        uint32_t *seed, int64_t time, const input_t &in) {
    const auto held = (in.attr & UFR_MOUS) ? in.attr : 0u;
    const auto pressed = held & ~buttons_;
    const auto released = buttons_ & ~held;
    buttons_ = held;
    sprite_t *under = ((held & UFR_MOUS) && (in.isel < shown_))
                    ? sprites_[in.isel].get() : nullptr;
    if (under && under->get_flag(sprite_t::effect)) // effects pick the owner
        under = under->parent_;
    if (under && (released & UFR_RBTN)) { // see menu_request()
        menu_target_ = under;
        menu_pending_ = true;
    }
    if (active_ && active_->get_flag(sprite_t::dragged)) {
        if (held & UFR_LBTN) { // still dragged, nothing else to do
            active_->position_ = {{in.cursor.x + grab_.x,
                                   in.cursor.y + grab_.y}};
            return;
        }
        active_->set_drag(seed, time, area_, false, in.hover); // dropped
    }
    if (active_ && (!in.hover || (under != active_)
            || !active_->get_flag(sprite_t::hovered))) {
        active_->drop_special(seed, area_, sprite_t::hovered, time);
        active_ = nullptr;
    }
    if (under && (pressed & UFR_LBTN)) { // grabbed, which ends its hover
        (active_ = under)->set_drag(seed, time, area_, true, in.hover);
        grab_ = {{under->position_.x - in.cursor.x,
                  under->position_.y - in.cursor.y}};
    }
    if (!active_ && in.hover && under
            && !(held & (UFR_LBTN | UFR_MBTN | UFR_RBTN))) // no buttons held
        if (auto hover_bhv = under->hover(seed, in.cursor, in.engd))
            (active_ = under)->pick_special(
                    seed, area_, sprite_t::hovered, *hover_bhv);
}

void sprite_bank_t::toggle_sleep(
        uint32_t *seed, int64_t time, sprite_t &chr, bool all) {
    const bool on = !chr.get_flag(sprite_t::asleep);
    const auto ic = find_in_map(cast_, chr.lib_->id());
    assert(!all || ic);
    sprite_t::cast_t::mapped_type one{&chr};
    for (auto c : (all) ? *ic : one) c->set_sleep(seed, time, area_, on);
}

void sprite_bank_t::menu_action(
        uint32_t *seed, int64_t time, action_t action) {
    if (!menu_target_) return;
    auto &chr = *menu_target_;
    switch (action) {
        case remove_one: break;
        case remove_all: break;
        case sleep_one: toggle_sleep(seed, time, chr, false); break;
        case sleep_all: toggle_sleep(seed, time, chr, true); break;
        case control_p1: break;
        case control_p2: break;
    }
}

bool sprite_bank_t::try_recycle(uint32_t iter, uint32_t if_free,
        uint32_t if_not, std::vector<std::unique_ptr<slot_t>> &sprites) {
    auto temp = std::move(sprites[iter]);
    sprites[iter] = std::move(sprites[if_free]);
    if (!temp->is_free()) {
        sprites[if_free] = std::move(sprites[if_not]);
    } else {
        if_not = if_free;
    }
    return !(sprites[if_not] = std::move(temp))->is_free();
}

void sprite_bank_t::update(uint32_t *seed, int64_t time, const input_t &in) {
    const auto speech_chance
            = (uint64_t(speech_chance_.get()) << 32) / speech_chance_.max();
    handle_input(seed, time, in);
    // behaviours first, as followers need updated positions
    for (uint32_t iter = 0; iter < shown_; iter++) {
        auto &chr = *sprites_[iter];
        chr.update_behaviour(seed, time, area_, cast_);
        deploy_effects(chr);
    }
    // backwards, so that swapped-in slots are already updated
    for (uint32_t iter = (effects_ || speech_) ? shown_ : 0; iter--; ) {
        auto &fx = *sprites_[iter];
        if (!fx.get_flag(sprite_t::effect)) continue;
        if (fx.get_flag(sprite_t::speech)) {
            if (fx.update_speech(seed, time, speech_chance)) continue;
        } else {
            // fires when duration >= repeat: effect spawns more of itself
            if (fx.repeat_effect(time)) add_effect(*fx.parent_, *fx.fx());
            if (fx.update_effect(seed, time)) continue;
        }
        if (!try_recycle(iter, --shown_, --waiting_, sprites_)) waiting_++;
    }
    // forwards, for the same reason; this deploys new effects too
    for (uint32_t iter = waiting_; iter < sprites_.size(); iter++) {
        auto &fx = *sprites_[iter];
        assert(fx.get_flag(sprite_t::effect));
        // wakes due effects: those just added and those with duration < repeat
        bool awake = (fx.get_flag(sprite_t::speech))
                   ? fx.do_speech(seed, time, speech_chance)
                   : fx.wake_effect(seed, time);
        if ((awake || fx.is_free())
                && try_recycle(iter, waiting_++, shown_, sprites_)) shown_++;
    }
    using sp_t = const std::unique_ptr<slot_t>; // sorting the sprites by Y
    std::stable_sort(sprites_.begin(), sprites_.begin() + shown_,
            [](sp_t &a, sp_t &b) { return sprite_t::depth_sort(*a, *b); });
    display_.clear();
    for (uint32_t iter = 0; iter < shown_; iter++)
        sprites_[iter]->append(display_); // building the display list
}
