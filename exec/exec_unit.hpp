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
        GEN_FLAGS(prepare, upload, from_path, centered, repeat)
    };
    struct {
        flags_t flags_ = {};
        T2IV center_ = {};
        AINF image_ = {};
    } sides_[2];

    void *allocate(bool left, size_t size) {
        auto data = (uint8_t*)realloc(nullptr, size + sizeof(intptr_t));
        *((intptr_t*)data) = (intptr_t)(&sides_[left].flags_);
        sides_[left].image_.time = (uint32_t*)(data + sizeof(intptr_t));
        return (void*)sides_[left].image_.time;
    }
    static void finalize(void *data) {
        if (!data) return;
        data = ((uint8_t*)data) - sizeof(intptr_t);
        *((flags_t*)(*(intptr_t*)data)) |= prepare | upload;
        data = realloc(data, 0);
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
    void resolve_center(bool left) {
        // per DP tech doc a center of (0, 0) means 'use natural image center',
        // and anything else is a point on the image measured from its top left
        // corner; either way it needs the AINF dimensions and those only exist
        // once the loader is done, hence a separate pass to be run right after
        // the ECB_LOAD barrier
        auto &side = sides_[left];
        if ((side.flags_ & centered) || !is_ready(left)) return;
        set_flag(left, true, centered);
        decltype(side.center_.x) xdim = side.image_.xdim;
        decltype(side.center_.y) ydim = side.image_.ydim;
        side.center_ = (side.center_.x || side.center_.y)
                     ? T2IV{{side.center_.x, ydim - side.center_.y}}
                     : T2IV{{xdim >> 1, ydim >> 1}};
    }
    void resolve_center() { resolve_center(false); resolve_center(true); }

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
        bool wrap = frame >= (side.image_.fcnt - 1);
        if (!wrap || (side.flags_ & repeat)) {
            frame = (!wrap) ? frame + 1 : 0;
            if (time > (time_next += side.image_.time[frame]))
                time_next = time + side.image_.time[frame]; // >1 frame skipped
            return side.image_.uuid;
        }
        time_next = std::numeric_limits<int64_t>::max(); // never expires
        return 0;
    }
};

#endif // EXEC_UNIT_HPP
