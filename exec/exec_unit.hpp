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
        GEN_FLAGS(prepare, upload, from_path, fake_center, repeat)
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
        set_flag(left, (center.x != 0) || (center.y != 0), fake_center);
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

    T2IV dims(bool left) const {
        T2IV retn{{(decltype(retn.x))sides_[left].image_.xdim,
                   (decltype(retn.y))sides_[left].image_.ydim}};
        return retn;
    }
    intptr_t advance(
            bool left, int64_t time, int64_t &old, uint32_t &frame) const {
        if (time <= old) return 0;
        auto &side = sides_[left];
        bool wrap = frame >= (side.image_.fcnt - 1);
        if (!wrap || (side.flags_ & repeat)) {
            if (time > (old += side.image_.time[frame]))
                old = time; // > 1 frame skipped, resetting
            frame = (!wrap) ? frame + 1 : 0;
            return side.image_.uuid;
        } else {
            old = std::numeric_limits<int64_t>::max(); // never expires
            wrap = frame >= side.image_.fcnt;
            frame = side.image_.fcnt - 1;
            return (wrap) ? side.image_.uuid : 0;
        }
    }
};

#endif // EXEC_UNIT_HPP
