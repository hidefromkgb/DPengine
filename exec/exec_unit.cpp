#include <cassert>

#include "exec_parser.hpp"
#include "exec_unit.hpp"



unit_t::~unit_t() {
    const auto discard_r = maybe_discard(false);
    assert_and_discard(discard_r, discard_r);
    const auto discard_l = maybe_discard(true);
    assert_and_discard(discard_l, discard_l);
}

bool unit_t::maybe_discard(bool left) {
    if (is_being_uploaded(left)) return false;
    if (is_owner_discardable(left)) {
        assert(sides_[left].image_.time);
        release(sides_[left].image_.time);
    }
    sides_[left].flags_ &= ~(prepare | upload);
    sides_[left].image_ = {};
    return true;
}

bool unit_t::prepare_source(bool left, string_view_t name) {
    if (!maybe_discard(left)) return false;
    set_flag(left, true, prepare);
    set_flag(left, true, from_path);
    auto data = (char*)allocate(left, name.size + 1);
    strncpy(data, name.str, name.size);
    data[name.size] = 0;
    return true;
}

bool unit_t::prepare_source(bool left, string_view_t name, uint32_t xdim,
        uint32_t ydim, const std::function<void(uint32_t*)> &draw) {
    // the layout is as follows:
    //     0: AINF
    // +AINF: buf
    //  +buf: time
    // +time: name
    if (!maybe_discard(left)) return false;
    set_flag(left, true, prepare);
    set_flag(left, false, from_path);
    auto anim = (AINF*)allocate(left, sizeof(AINF) + name.size + 1
                                    + sizeof(uint32_t) * (xdim * ydim + 1));
    anim->uuid = (intptr_t)(anim + 1);
    anim->time = ((uint32_t*)anim->uuid) + xdim * ydim;
    anim->xdim = xdim;
    anim->ydim = ydim;
    anim->fcnt = 1;
    anim->time[0] = 0; // single frame only for this image type
    strncpy((char*)(anim->time + 1), name.str, name.size);
    ((char*)(anim->time + 1))[name.size] = 0;
    draw((uint32_t*)anim->uuid); // drawing something in the buffer
    return true;
}

bool unit_t::schedule_upload(bool left, ENGD *engd) {
    if (!is_owner_discardable(left)) return is_ready(left);
    set_flag(left, false, prepare);
    set_flag(left, true, upload);
    auto &side = sides_[left];
    char *data = (char*)side.image_.time;
    side.image_.time = nullptr;
    if (side.flags_ & from_path) {
        auto hash = data + strlen(data); // animation hash: last dir + gif
        for (auto dirs = 2;
            (hash > data) && ((hash[-1] != *DEF_DSEP) || --dirs); hash--) {}
        cEngineLoadAnimAsync(
                engd, &side.image_, (uint8_t*)hash, data, ELA_DISK, release);
    } else if (auto anim = (AINF*)data) {
        cEngineLoadAnimAsync(engd, &side.image_, (uint8_t*)(anim->time + 1),
                anim, ELA_AINF, release);
    }
    return true;
}
