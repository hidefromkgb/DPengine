#ifndef EXEC_UNIT_HPP
#define EXEC_UNIT_HPP



#include <functional>

#include "exec_common.hpp"



class unit_t : public no_copy_t {
private:
    // !prepare && !upload = AINF is unprepared, no memory allocations
    //  prepare && !upload = AINF is prepared, ready to upload
    // !prepare &&  upload = AINF is being uploaded
    //  prepare &&  upload = AINF is ready
    enum flags_t : uint32_t {
        GEN_FLAGS(prepare, upload, from_path, repeat)
    };
    struct {
        flags_t flags_ = {};
        T2IV center_ = {};
        AINF image_ = {};
    } sides_[2];

    void *allocate(bool left, size_t size) {
        sides_[left].image_.time = (uint32_t*)realloc(nullptr, size);
        return (void*)sides_[left].image_.time;
    }
    // the engine calls this once it no longer needs the source buffer, which
    // is NOT the same as the AINF being filled in
    static void release(void *data) {
        if (data) data = realloc(data, 0);
    }
    void set_flag(bool left, bool value, flags_t flag) {
        auto &side = sides_[left];
        side.flags_ = (value) ? (side.flags_ | flag) : (side.flags_ & ~flag);
    }
    bool maybe_discard(bool left);

protected:
    unit_t(): sides_{} {}
    ~unit_t();

    void set_center(bool left, const T2IV &center) {
        sides_[left].center_ = center;
    }
    void set_loop(bool left, bool loop) { set_flag(left, loop, repeat); }

    // return true = AINF prepared successfully
    // return false = AINF busy, could not prepare
    bool prepare_source(bool left, const std::string &name);
    bool prepare_source(bool left, const std::string &name, uint32_t xdim,
            uint32_t ydim, const std::function<void(uint32_t*)> &draw);

public:
    bool is_owner_discardable(bool left) const {
        const auto flags = sides_[left].flags_;
        return (flags & prepare) && !(flags & upload);
    }
    bool is_owner_discardable() const {
        return is_owner_discardable(false) && is_owner_discardable(true);
    }

    bool is_being_uploaded(bool left) const {
        const auto flags = sides_[left].flags_;
        return !(flags & prepare) && (flags & upload);
    }
    bool is_being_uploaded() const {
        return is_being_uploaded(false) && is_being_uploaded(true);
    }

    bool is_empty(bool left) const {
        const auto flags = sides_[left].flags_;
        return !(flags & prepare) && !(flags & upload);
    }
    bool is_empty() const { return is_empty(false) && is_empty(true); }

    bool is_ready(bool left) const {
        const auto flags = sides_[left].flags_;
        return (flags & prepare) && (flags & upload);
    }
    bool is_ready() const { return is_ready(false) && is_ready(true); }

    bool schedule_upload(bool left, ENGD *engd);
    bool schedule_upload(ENGD *engd) {
        const bool r = schedule_upload(false, engd);
        const bool l = schedule_upload(true, engd);
        return r && l;
    }

    T2IV dims(bool left) const {
        T2IV retn{{(decltype(retn.x))sides_[left].image_.xdim,
                   (decltype(retn.y))sides_[left].image_.ydim}};
        return retn;
    }
    // the engine writes the AINF separately from releasing the source buffer,
    // and only at the end of ECB_LOAD with 0, until that barrier the AINF has
    // no frames, no times and no uuid, and if the engine has nothing to put
    // there (the load failed, or the request got turned down) it stays that
    // way for good. Hence this, to be called after the barrier and nowhere
    // else: it is what turns a side from `being uploaded` into `ready`, and
    // as discarding is refused until then, the engine never gets to write
    // into a side that has moved on; everything that depends on the AINF is
    // settled here too - a side only gets uploaded once, so this only ever
    // acts once per side, and is a no-op for sides that were never uploaded
    void finalize(bool left) {
        auto &side = sides_[left];
        if (!is_being_uploaded(left)) return;
        set_flag(left, true, prepare);
        // this saves processing time by reducing the number of IFs required in
        // the frame advancement routine: no animation means nothing to loop
        if (side.image_.fcnt < 2) set_loop(left, false);
        // per DP tech doc a center of (0, 0) means 'use natural image center',
        // and anything else is a point on the image measured from its top left
        // corner; either way it needs the AINF dimensions
        decltype(side.center_.x) xdim = side.image_.xdim;
        decltype(side.center_.y) ydim = side.image_.ydim;
        side.center_ = (side.center_.x || side.center_.y)
                     ? T2IV{{side.center_.x, ydim - side.center_.y}}
                     : T2IV{{xdim >> 1, ydim >> 1}};
    }
    void finalize() { finalize(false); finalize(true); }

    T2IV center(bool left) const { return sides_[left].center_; }

    // returns the uuid whenever there is a new frame to show, 0 otherwise.
    // TIME_NEXT is when the current frame ends.
    intptr_t advance(bool left, int64_t time,
            int64_t &time_next, uint32_t &frame) const {
        if (frame == UINT32_MAX) {
            frame = 0;
            auto &side = sides_[left];
            if (!side.image_.fcnt) return 0;
            time_next = time + side.image_.time[frame];
            return side.image_.uuid;
        }
        if (time <= time_next) return 0;
        auto &side = sides_[left];
        bool wrap = (frame + 1 >= side.image_.fcnt);
        if (!wrap || (side.flags_ & repeat)) {
            frame = (!wrap) ? frame + 1 : 0;
            if (time > (time_next += side.image_.time[frame]))
                time_next = time + side.image_.time[frame]; // >1 frame skipped
            return side.image_.uuid;
        }
        time_next = never;
        return 0;
    }
};

#endif // EXEC_UNIT_HPP
