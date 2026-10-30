#ifndef EXEC_LIBRARY_HPP
#define EXEC_LIBRARY_HPP



#include "exec_bhv.hpp"
#include "exec_common.hpp"
#include "exec_prng.hpp"



struct bhv_id_desc_t {
    std::unordered_map<size_t, bhv_id_t> m;
    std::vector<bhv_id_t> v;
};
using bhv_id_map_t = std::unordered_map<size_t, bhv_id_desc_t>;



class interaction_t : public no_copy_t {
private:
    bool all_;
    uint16_t proximity_;
    uint64_t chance_;
    const T2IV duration_; // u = duration (INT_MAX), v = reactivation delay
    std::vector<bhv_id_t> initiator_;
    std::unordered_map<lib_id_t, std::vector<bhv_id_t>>
        targets_;

public:
    class input_t {
    public:
        string_t *name = nullptr;
        float chance = 0.f;
        float proximity = 125.f;
        std::vector<string_t*> targets = {};
        bool target_activation_all = false;
        std::vector<string_t*> bhv = {};
        float reactivation_delay = 60.f;

        input_t(const input_t &) = delete;
        input_t &operator=(const input_t &) = delete;
        input_t(input_t &&rhs)
        : name(rhs.name)
        , chance(rhs.chance)
        , proximity(rhs.proximity)
        , targets(std::move(rhs.targets))
        , target_activation_all(rhs.target_activation_all)
        , bhv(std::move(rhs.bhv))
        , reactivation_delay(rhs.reactivation_delay) {
            rhs.name = nullptr;
            rhs.targets.clear();
            rhs.bhv.clear();
        }
        input_t &operator=(input_t &&) = delete;
        ~input_t() {
            free_string(name);
            for (auto t : targets) free_string(t);
            for (auto b : bhv) free_string(b);
        }
        input_t() = default;
        input_t(string_view_t str);
        bool validate() const {
            bool okay = !targets.empty();
            okay &= !bhv.empty();
            for (auto t : targets) okay &= get_view(t).size;
            for (auto b : bhv) okay &= get_view(b).size;
            return okay;
        }
    };

    bool is_empty() const { return initiator_.empty() || targets_.empty(); }

    interaction_t(const input_t &in, string_view_t lib_name,
            const bhv_id_map_t &bhv_id_map);
};

class library_t : public no_copy_t {
public:
    class input_t {
    public:
        string_t *name = nullptr;
        std::vector<string_t*> categories = {};
        std::vector<speech_t::input_t> speeches = {};
        std::vector<effect_t::input_t> effects = {};
        std::vector<behaviour_t::input_t> behaviours = {};
        std::vector<interaction_t::input_t> interactions = {};

        input_t(const input_t &) = delete;
        input_t &operator=(const input_t &) = delete;
        input_t(input_t &&rhs)
        : name(rhs.name)
        , categories(std::move(rhs.categories))
        , speeches(std::move(rhs.speeches))
        , effects(std::move(rhs.effects))
        , behaviours(std::move(rhs.behaviours))
        , interactions(std::move(rhs.interactions)) {
            rhs.name = nullptr;
            rhs.categories.clear();
        }
        input_t &operator=(input_t &&) = delete;
        ~input_t() {
            free_string(name);
            for (auto c : categories) free_string(c);
        }
        input_t() = default;
        input_t(string_view_t base, string_view_t dir);
    };

private:
    static constexpr uint32_t def_speech_fg_ = 0xFF000000;
    static constexpr uint32_t def_speech_bg_ = 0xFFFFFFFF;

    enum bhv_type_t : uint32_t { nonzero_prob = 0,
        stationary, moving, mouseover, dragged, sleeping,
        max_ = sleeping, };
    // each behaviour within a library is uniquely identified by 3 numbers:
    // - its 'native' group (the one it got from the config file)
    // - its type (see library_t::group_t)
    // - its index within the type array
    union bhv_id_internal_t {
        bhv_id_t _;
        struct {
            uint32_t index:15;
            bhv_type_t type:3;
            int32_t group:14;
        };
    };
    struct group_t {
        weighted_rng_t nonzero_weights;
        std::array<ref_vec_t<behaviour_t>, bhv_type_t::max_ + 1> bhv;
        ref_vec_t<speech_t> speeches; // random ones, this group's and group 0's

        void append(const group_t &rhs) {
            for (size_t i = 0; i <= bhv_type_t::max_; i++)
                bhv[i].insert(
                        bhv[i].end(), rhs.bhv[i].begin(), rhs.bhv[i].end());
        }
    };

    lib_id_t id_;
    string_t *library_path_;
    string_t *readable_name_;
    std::vector<std::unique_ptr<interaction_t>> interactions_;
    std::vector<std::unique_ptr<behaviour_t>> behaviours_;
    std::vector<std::unique_ptr<speech_t>> speeches_;
    // TODO: remap groups sequenitially and make this a vector?
    std::unordered_map<int, group_t> groups_;
    size_t preview_id_;
    uint32_t max_effects_;
    uint32_t speech_fg_;
    uint32_t speech_bg_;

    inline static bhv_id_t init_bhv_id(movement_flags_t move, int16_t group);

    static void extract_speech_colors_worker(intptr_t data, uint64_t unused);
    unit_t &get_preview_internal() { return *behaviours_[preview_id_]; }

    // struct and methods that compartmentalize the constructor:
    struct build_ctx_t;
    void init_interactions(const build_ctx_t &ctx);
    void map_effects(build_ctx_t &ctx);
    void map_speeches(build_ctx_t &ctx);
    void create_behaviours(build_ctx_t &ctx);
    void init_random_speeches(const build_ctx_t &ctx);
    void init_probabilities(const build_ctx_t &ctx);
    void init_follow_groups(const build_ctx_t &ctx);
    void init_special_behaviours(const build_ctx_t &ctx);
    const behaviour_t &special_bhv_internal(
            uint32_t *seed, const behaviour_t &curr, bhv_type_t type) const;
    void init_max_effects();
    void dump_stats() const;

public:
    const behaviour_t *get(bhv_id_t id) const;

    const behaviour_t *next_behaviour(
            uint32_t *seed, const behaviour_t *curr) const;
    const unit_t &follow_override(
            uint32_t *seed, const behaviour_t &bhv, bool move) const;
    // CURR's start, else PREV's end, else a CHANCE roll (if CHANCE > 0)
    const speech_t *transition_speech(uint32_t *seed, uint64_t chance,
            const behaviour_t &prev, const behaviour_t &curr) const;
    // a random line of CURR's group, nullptr only if the group has none
    const speech_t *force_random_speech(
            uint32_t *seed, const behaviour_t &curr) const;

    // random special behaviours of CURR's group, see init_special_behaviours()
    const behaviour_t &sleep_bhv(
            uint32_t *seed, const behaviour_t &curr) const {
        return special_bhv_internal(seed, curr, sleeping);
    }
    const behaviour_t &hover_bhv(
            uint32_t *seed, const behaviour_t &curr) const {
        return special_bhv_internal(seed, curr, mouseover);
    }
    const behaviour_t &drag_bhv(uint32_t *seed, const behaviour_t &curr) const {
        return special_bhv_internal(seed, curr, dragged);
    }

    static bhv_id_desc_t build_bhv_id_desc(const input_t &in);

    ~library_t();
    library_t(lib_id_t id, string_view_t path, const input_t &in,
            const bhv_id_map_t &bhv_id_map);

    // both *_preview() methods intentionally non-const:
    // their unit_t's should only be accessible during init
    const unit_t &get_preview() { return get_preview_internal(); }
    const unit_t &load_preview(ENGD *engd) {
        auto &preview = get_preview_internal();
        preview.schedule_upload(false, engd);
        return preview;
    }
    // CSPEECH: library colors instead of black on white
    void load(ENGD *engd, bool speech, bool cspeech,
            const speech_t::text_fn_t &text_fn);

    uint32_t max_effects() const { return max_effects_; }
    // any speech, nullptr if none; see sprite_t::preinit_speech()
    const speech_t *first_speech() const {
        return (speeches_.empty()) ? nullptr : speeches_.front().get();
    }

    // to be called after every ECB_LOAD with 0, see unit_t::finalize()
    void finalize();

    static void extract_speech_colors(
            ENGD *engd, const std::vector<library_t*> &libs);

    lib_id_t id() const { return id_; };
    string_view_t name() const { return get_view(readable_name_); }
};

#endif // EXEC_LIBRARY_HPP
