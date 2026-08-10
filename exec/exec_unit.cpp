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
        finalize(sides_[left].image_.time);
    }
    sides_[left].flags_ &= ~(prepare | upload);
    sides_[left].image_ = {};
    return true;
}

bool unit_t::prepare_source(bool left, const std::string &name) {
    if (!maybe_discard(left)) return false;
    set_flag(left, true, prepare);
    set_flag(left, true, from_path);
    auto data = (char*)allocate(left, name.size() + 1);
    strncpy(data, name.c_str(), name.size() + 1);
    return true;
}

bool unit_t::prepare_source(bool left, const std::string &name, uint32_t xdim,
        uint32_t ydim, const std::function<void(uint32_t*)> &draw) {
    // the layout is as follows:
    //  -ptr: &flags
    //     0: AINF
    // +AINF: buf
    //  +buf: time
    // +time: name
    if (!maybe_discard(left)) return false;
/*
    set_flag(left, true, prepare);
    set_flag(left, false, from_path);
    auto anim = (AINF*)allocate(left, sizeof(AINF) + name.size() + 1
                                    + sizeof(uint32_t) * (xdim * ydim + 1));
    anim->uuid = (intptr_t)(anim + 1);
    anim->time = ((uint32_t*)anim->uuid) + xdim * ydim;
    anim->xdim = xdim;
    anim->ydim = ydim;
    anim->time[0] = 0; // single frame only for this image type
    strncpy((char*)(anim->time + 1), name.c_str(), name.size() + 1);
    draw((uint32_t*)anim->uuid); // drawing something in the buffer
//*/
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
        std::string_view temp(data);
        auto pos = temp.find_last_of(DEF_DSEP);
        if ((pos != std::string_view::npos) && (pos > 0))
            pos = temp.find_last_of(DEF_DSEP, pos - 1);
        pos = (pos != std::string_view::npos) ? pos + 1 : 0;
        temp.remove_prefix(pos); // animation hash: last dir + gif name
        cEngineLoadAnimAsync(engd, &side.image_, (uint8_t*)temp.data(),
                data, ELA_DISK, finalize);
    } else if (auto anim = (AINF*)data) {
        cEngineLoadAnimAsync(engd, &side.image_, (uint8_t*)(anim->time + 1),
                anim, ELA_AINF, finalize);
    }
    return true;
}
