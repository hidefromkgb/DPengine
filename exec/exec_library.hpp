#ifndef EXEC_LIBRARY_HPP
#define EXEC_LIBRARY_HPP



#include "exec_bhv.hpp"
#include "exec_common.hpp"
#include "exec_prng.hpp"



struct bhv_id_desc_t {
    std::unordered_map<std::string, bhv_id_t> m;
    std::vector<bhv_id_t> v;
};
using bhv_id_map_t = std::unordered_map<std::string, bhv_id_desc_t>;



class interaction_t : public no_copy_t {
private:
    bool all_;
    uint16_t proximity_;
    uint32_t chance_;
    const T2IV duration_; // u = duration (INT_MAX), v = reactivation delay
    std::vector<bhv_id_t> initiator_;
    std::unordered_map<lib_id_t, std::vector<bhv_id_t>>
        targets_;

public:
    class input_t {
    public:
        std::string name;                    // NOT USED
        float chance = 0.f;                  // NOT USED
        float proximity = 125.f;             // NOT USED
        std::vector<std::string> targets;    // NOT USED
        bool target_activation_all = false;  // NOT USED
        std::vector<std::string> bhv;        // NOT USED
        float reactivation_delay = 60.f;     // NOT USED

        input_t() = default;
        input_t(const std::string_view &str);
        bool validate() const {
            bool okay = !targets.empty();
            okay &= !bhv.empty();
            for (auto &t : targets) okay &= !t.empty();
            for (auto &b : bhv) okay &= !b.empty();
            return okay;
        }
    };

#ifdef DEV_MODE
    input_t debug_;
#endif // DEV_MODE

    bool is_empty() const { return initiator_.empty() || targets_.empty(); }

    interaction_t(const input_t &in, const std::string &lib_name,
            const bhv_id_map_t &bhv_id_map);
};

class library_t : public no_copy_t {
public:
    class input_t {
    public:
        std::string name;
        std::vector<std::string> categories;
        std::vector<speech_t::input_t> speeches;
        std::vector<effect_t::input_t> effects;
        std::vector<behaviour_t::input_t> behaviours;
        std::vector<interaction_t::input_t> interactions;

        input_t(const std::string &base, const std::string &dir);
    };

private:
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

        void append(const group_t &rhs) {
            for (size_t i = 0; i <= bhv_type_t::max_; i++)
                bhv[i].insert(
                        bhv[i].end(), rhs.bhv[i].begin(), rhs.bhv[i].end());
        }
    };

    std::string library_path_;
    std::string readable_name_;
    std::vector<std::unique_ptr<interaction_t>> interactions_;
    std::vector<std::unique_ptr<behaviour_t>> behaviours_;
    std::vector<std::unique_ptr<speech_t>> speeches_;
    // TODO: remap groups sequenitially and make this a vector?
    std::unordered_map<int, group_t> groups_;
    size_t preview_id_;
    uint32_t speech_fg_;
    uint32_t speech_bg_;

    inline static bhv_id_t init_bhv_id(movement_flags_t move, int16_t group);

    const speech_t *select_speech(uint32_t *seed, uint32_t chance,
            bhv_id_t prev, bhv_id_t curr) const;

    static void extract_speech_colors_worker(intptr_t data, uint64_t unused);
    unit_t &get_preview_internal() { return *behaviours_[preview_id_]; }

    // struct and methods that compartmentalize the constructor:
    struct build_ctx_t;
    void init_interactions(const build_ctx_t &ctx);
    void map_effects(build_ctx_t &ctx);
    void map_speeches(build_ctx_t &ctx);
    void create_behaviours(build_ctx_t &ctx);
    void init_probabilities(const build_ctx_t &ctx);
    void init_follow_groups(const build_ctx_t &ctx);
    void dump_stats() const;

public:
    inline const behaviour_t *get(bhv_id_t id) const;

    static bhv_id_desc_t build_bhv_id_desc(const input_t &in);

    library_t(std::string path, const input_t &in,
            const bhv_id_map_t &bhv_id_map);

    // both *_preview() methods intentionally non-const:
    // their unit_t's should only be accessible during init
    const unit_t &get_preview() { return get_preview_internal(); }
    const unit_t &load_preview(ENGD *engd) {
        auto &preview = get_preview_internal();
        preview.schedule_upload(false, engd);
        return preview;
    }
    static void extract_speech_colors(
            ENGD *engd, const std::vector<library_t*> &libs);

    const std::string &name() const { return readable_name_; }
};

#endif // EXEC_LIBRARY_HPP
