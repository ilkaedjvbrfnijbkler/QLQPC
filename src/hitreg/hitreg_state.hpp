// https://github.com/Shepherd-Projects/QL1K/blob/main/src/hitreg_state.hpp
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>

namespace qlqpc::hitreg
{
    enum class hitreg_display_kind : uint8_t
    {
        none,
        percent,
        infinity,
        unavailable,
    };

    enum class client_accuracy_display_kind : uint8_t
    {
        none,
        percent,
        unavailable,
    };

    enum class hitreg_unavailable_reason : uint8_t
    {
        none,
        inflight_hold_capacity,
        pending_tick_capacity,
        cross_hold_feedback,
        client_ray_unavailable,
    };

    enum class hitreg_trace_kind : uint8_t
    {
        player,
        world,
        none,
        other,
    };

    inline bool hitreg_is_opponent(int32_t local_team, int32_t candidate_team)
    {
        if (local_team == 0)
        {
            return candidate_team == 0;
        }
        if (local_team == 1 || local_team == 2)
        {
            return (candidate_team == 1 || candidate_team == 2) &&
                   candidate_team != local_team;
        }
        return false;
    }

    struct hitreg_display
    {
        hitreg_display_kind kind{hitreg_display_kind::none};
        client_accuracy_display_kind client_accuracy_kind{client_accuracy_display_kind::none};
        hitreg_unavailable_reason unavailable_reason{hitreg_unavailable_reason::none};
        uint64_t generation{};
        uint32_t percent{};
        uint32_t percent_hundredths{};
        uint32_t client_hits{};
        uint32_t server_hits{};
        uint32_t samples{};
        uint32_t fire_events{};
        uint32_t client_accuracy_percent_hundredths{};
        uint32_t client_accuracy_hits{};
        uint32_t client_accuracy_opportunities{};
        uint32_t trace_total{};
        uint32_t trace_player{};
        uint32_t trace_world{};
        uint32_t trace_none{};
        uint32_t trace_other{};
        uint32_t trace_client_player{};
        int32_t start_time{};
        int32_t end_time{};
        uint32_t session_client_hits{};
        uint32_t session_client_opportunities{};
        uint32_t session_server_hits{};
        uint32_t session_accuracy_percent_hundredths{};
    };

    class hitreg_state
    {
    public:
        static constexpr int32_t lightning_weapon = 6;

        void reset()
        {
            active_holds = {};
            active_hold_count = 0;
            pub_display = {};
            pub_display.generation = pub_generation;
            have_last_fire_event_time = false;
            last_fire_event_time = 0;
            have_last_feedback_state = false;
            last_feedback_command_time = 0;
            last_feedback_hits = 0;
            seen_transition_count = 0;

            sess_client_hits = 0;
            sess_client_opportunities = 0;
            sess_server_hits = 0;
        }

        bool wants_client_ray(int32_t weapon, int32_t command_time) const
        {
            return weapon == lightning_weapon &&
                   (!have_last_fire_event_time ||
                    time_before(last_fire_event_time, command_time));
        }

        void on_weapon_fire(
            int32_t weapon,
            int32_t command_time,
            hitreg_trace_kind trace_kind,
            bool client_player_contact,
            bool client_ray_available)
        {
            if (have_last_fire_event_time && !time_before(last_fire_event_time, command_time))
            {
                return;
            }
            have_last_fire_event_time = true;
            last_fire_event_time = command_time;

            if (weapon != lightning_weapon)
            {
                close_open_hold(command_time);
                return;
            }

            hold_record* hold = open_hold();
            if (hold == nullptr)
            {
                if (active_hold_count == active_holds.size())
                {
                    active_holds = {};
                    active_hold_count = 1;
                    hold = &active_holds[0];
                    hold->start_time = command_time;
                    hold->open = true;
                    mark_unavailable(*hold, hitreg_unavailable_reason::inflight_hold_capacity);
                }
                else
                {
                    hold = &active_holds[active_hold_count++];
                    *hold = {};
                    hold->start_time = command_time;
                    hold->open = true;
                }
            }

            increment_saturated(hold->fire_events);
            increment_saturated(hold->client_accuracy_opportunities);
            if (!client_ray_available)
            {
                mark_unavailable(*hold, hitreg_unavailable_reason::client_ray_unavailable);
            }
            else
            {
                increment_saturated(hold->trace_total);
                switch (trace_kind)
                {
                case hitreg_trace_kind::player:
                    increment_saturated(hold->trace_player);
                    break;
                case hitreg_trace_kind::world:
                    increment_saturated(hold->trace_world);
                    break;
                case hitreg_trace_kind::none:
                    increment_saturated(hold->trace_none);
                    break;
                case hitreg_trace_kind::other:
                    increment_saturated(hold->trace_other);
                    break;
                }
                if (client_player_contact)
                {
                    increment_saturated(hold->client_accuracy_hits);
                    increment_saturated(hold->trace_client_player);
                }
            }
            if (hold->pending_server_count == hold->pending_server_times.size())
            {
                mark_unavailable(*hold, hitreg_unavailable_reason::pending_tick_capacity);
            }
            else
            {
                pending_tick& tick = hold->pending_server_times[hold->pending_server_count++];
                tick = {};
                tick.command_time = command_time;
                tick.client_contact = client_player_contact;
            }
        }

        void on_usercmd(int32_t command_time, bool attack_held)
        {
            if (!attack_held)
            {
                close_open_hold(command_time);
            }
        }

        void on_server_feedback_transition(
            int32_t old_command_time,
            int32_t new_command_time,
            int32_t old_hits,
            int32_t new_hits)
        {
            int32_t baseline_hits = old_hits;
            if (have_last_feedback_state)
            {
                if (time_before(new_command_time, last_feedback_command_time))
                {
                    return;
                }
                baseline_hits = last_feedback_hits;
                if (new_command_time == last_feedback_command_time)
                {
                    for (size_t index = 0; index < seen_transition_count; ++index)
                    {
                        const feedback_transition_id& seen = seen_transitions[index];
                        if (seen.old_command_time == old_command_time &&
                            seen.old_hits == old_hits && seen.new_hits == new_hits)
                        {
                            return;
                        }
                    }
                    if (seen_transition_count == seen_transitions.size())
                    {
                        return;
                    }
                }
                else
                {
                    seen_transition_count = 0;
                }
            }

            have_last_feedback_state = true;
            last_feedback_command_time = new_command_time;
            last_feedback_hits = new_hits;
            seen_transitions[seen_transition_count++] =
                {old_command_time, old_hits, new_hits};

            const int64_t signed_hit_delta =
                static_cast<int64_t>(new_hits) - static_cast<int64_t>(baseline_hits);
            const uint32_t hit_delta =
                signed_hit_delta <= 0
                    ? 0U
                    : signed_hit_delta > std::numeric_limits<uint32_t>::max()
                          ? std::numeric_limits<uint32_t>::max()
                          : static_cast<uint32_t>(signed_hit_delta);
            on_server_feedback(old_command_time, new_command_time, hit_delta);
        }

        void on_server_feedback(
            int32_t old_command_time,
            int32_t new_command_time,
            uint32_t hit_damage_delta)
        {
            std::array<size_t, max_inflight_holds> interval_ticks_by_hold{};
            std::array<bool, max_inflight_holds> interval_client_contact_by_hold{};
            std::array<bool, max_inflight_holds> acknowledged_before_interval{};
            for (size_t hold_index = 0; hold_index < active_hold_count; ++hold_index)
            {
                const hold_record& hold = active_holds[hold_index];
                acknowledged_before_interval[hold_index] = hold.have_acknowledged_tick;
                for (size_t tick_index = 0; tick_index < hold.pending_server_count; ++tick_index)
                {
                    const pending_tick& tick = hold.pending_server_times[tick_index];
                    const int32_t tick_time = tick.command_time;
                    if (time_before(old_command_time, tick_time) &&
                        time_reached(new_command_time, tick_time))
                    {
                        ++interval_ticks_by_hold[hold_index];
                        interval_client_contact_by_hold[hold_index] =
                            interval_client_contact_by_hold[hold_index] || tick.client_contact;
                    }
                }
            }

            for (size_t hold_index = 0; hold_index < active_hold_count; ++hold_index)
            {
                if (interval_ticks_by_hold[hold_index] == 0)
                {
                    continue;
                }
                hold_record& hold = active_holds[hold_index];
                hold.have_acknowledged_tick = true;
                record_client_window(hold, interval_client_contact_by_hold[hold_index]);
            }

            if (hit_damage_delta != 0)
            {
                size_t owner_count = 0;
                size_t owner_index = 0;
                for (size_t hold_index = 0; hold_index < active_hold_count; ++hold_index)
                {
                    if (interval_ticks_by_hold[hold_index] != 0 ||
                        (acknowledged_before_interval[hold_index] &&
                         !active_holds[hold_index].open))
                    {
                        ++owner_count;
                        owner_index = hold_index;
                    }
                }

                if (owner_count == 0)
                {
                    for (size_t hold_index = 0; hold_index < active_hold_count; ++hold_index)
                    {
                        const hold_record& hold = active_holds[hold_index];
                        if (hold.fire_events != 0 &&
                            (hold.open || hold.pending_server_count != 0 ||
                             hold.have_acknowledged_tick))
                        {
                            ++owner_count;
                            owner_index = hold_index;
                        }
                    }
                }

                if (owner_count > 1)
                {
                    for (size_t hold_index = 0; hold_index < active_hold_count; ++hold_index)
                    {
                        hold_record& hold = active_holds[hold_index];
                        if (interval_ticks_by_hold[hold_index] != 0 ||
                            (interval_ticks_by_hold[hold_index] == 0 &&
                             hold.fire_events != 0 &&
                             (hold.open || hold.pending_server_count != 0 ||
                              hold.have_acknowledged_tick)))
                        {
                            mark_unavailable(hold, hitreg_unavailable_reason::cross_hold_feedback);
                        }
                    }
                }
                else if (owner_count == 1)
                {
                    hold_record& hold = active_holds[owner_index];
                    increment_saturated(hold.server_hits);
                }
            }

            for (size_t hold_index = 0; hold_index < active_hold_count; ++hold_index)
            {
                hold_record& hold = active_holds[hold_index];
                size_t retained = 0;
                for (size_t tick_index = 0; tick_index < hold.pending_server_count; ++tick_index)
                {
                    const pending_tick& tick = hold.pending_server_times[tick_index];
                    const int32_t tick_time = tick.command_time;
                    if (!time_reached(new_command_time, tick_time))
                    {
                        hold.pending_server_times[retained++] = tick;
                    }
                }
                hold.pending_server_count = retained;
            }

            size_t finalized = 0;
            while (finalized < active_hold_count)
            {
                const hold_record& hold = active_holds[finalized];
                if (hold.open || !time_reached(new_command_time, hold.end_time))
                {
                    break;
                }
                publish(hold);
                ++finalized;
            }
            if (finalized != 0)
            {
                for (size_t index = finalized; index < active_hold_count; ++index)
                {
                    active_holds[index - finalized] = active_holds[index];
                }
                for (size_t index = active_hold_count - finalized; index < active_hold_count; ++index)
                {
                    active_holds[index] = {};
                }
                active_hold_count -= finalized;
            }
        }

        const hitreg_display& published() const
        {
            return pub_display;
        }

        bool format_display_text(char* text, size_t capacity) const
        {
            if (text == nullptr || capacity == 0)
            {
                return false;
            }

            if (active_hold_count != 0)
            {
                const hold_record& current = active_holds[active_hold_count - 1];
                const bool show_live = current.open && current.client_accuracy_opportunities > 0;
                const bool show_pending = !current.open && current.client_accuracy_opportunities > 0 &&
                                          pub_display.client_accuracy_opportunities == 0;
                if (show_live || show_pending)
                {
                    if (client_accuracy_unavailable(current.unavailable_reason))
                    {
                        std::snprintf(text, capacity, "client accuracy n/a");
                        return true;
                    }
                    const uint64_t num = static_cast<uint64_t>(current.client_accuracy_hits) * 10000ULL +
                                         static_cast<uint64_t>(current.client_accuracy_opportunities / 2U);
                    const uint32_t ratio = static_cast<uint32_t>(num / current.client_accuracy_opportunities);
                    const uint32_t whole = ratio / 100U;
                    const uint32_t frac = ratio % 100U;
                    std::snprintf(text, capacity, "client acc %u.%02u%% (%u/%u)",
                                  whole, frac, current.client_accuracy_hits, current.client_accuracy_opportunities);
                    return true;
                }
            }

            if (pub_display.client_accuracy_kind == client_accuracy_display_kind::unavailable)
            {
                std::snprintf(text, capacity, "client accuracy n/a");
                return true;
            }
            if (pub_display.client_accuracy_kind == client_accuracy_display_kind::percent)
            {
                const uint32_t whole = pub_display.client_accuracy_percent_hundredths / 100U;
                const uint32_t frac = pub_display.client_accuracy_percent_hundredths % 100U;

                if (pub_display.client_hits > 0 && pub_display.kind == hitreg_display_kind::percent)
                {
                    std::snprintf(text, capacity, "client acc %u.%02u%% (%u/%u) | hitreg %u%%",
                                  whole, frac, pub_display.client_accuracy_hits,
                                  pub_display.client_accuracy_opportunities, pub_display.percent);
                }
                else
                {
                    std::snprintf(text, capacity, "client acc %u.%02u%% (%u/%u)",
                                  whole, frac, pub_display.client_accuracy_hits,
                                  pub_display.client_accuracy_opportunities);
                }
                return true;
            }

            text[0] = '\0';
            return false;
        }

    private:
        static constexpr size_t max_unacknowledged_ticks = 64;
        static constexpr size_t max_feedback_transitions = 64;
        static constexpr size_t max_inflight_holds = 16;

        struct feedback_transition_id
        {
            int32_t old_command_time{};
            int32_t old_hits{};
            int32_t new_hits{};
        };

        struct pending_tick
        {
            int32_t command_time{};
            bool client_contact{};
        };

        struct hold_record
        {
            int32_t start_time{};
            int32_t end_time{};
            uint32_t client_hits{};
            uint32_t server_hits{};
            uint32_t samples{};
            uint32_t fire_events{};
            uint32_t client_accuracy_hits{};
            uint32_t client_accuracy_opportunities{};
            uint32_t trace_total{};
            uint32_t trace_player{};
            uint32_t trace_world{};
            uint32_t trace_none{};
            uint32_t trace_other{};
            uint32_t trace_client_player{};
            std::array<pending_tick, max_unacknowledged_ticks> pending_server_times{};
            size_t pending_server_count{};
            bool open{};
            hitreg_unavailable_reason unavailable_reason{hitreg_unavailable_reason::none};
            bool have_acknowledged_tick{};
        };

        static bool time_reached(int32_t value, int32_t target)
        {
            return static_cast<int32_t>(static_cast<uint32_t>(value) -
                                        static_cast<uint32_t>(target)) >= 0;
        }

        static bool time_before(int32_t value, int32_t target)
        {
            return !time_reached(value, target);
        }

        static void increment_saturated(uint32_t& value)
        {
            if (value != std::numeric_limits<uint32_t>::max())
            {
                ++value;
            }
        }

        static void record_client_window(hold_record& hold, bool player_contact)
        {
            increment_saturated(hold.samples);
            if (player_contact)
            {
                increment_saturated(hold.client_hits);
            }
        }

        static bool client_accuracy_unavailable(hitreg_unavailable_reason reason)
        {
            return reason != hitreg_unavailable_reason::none &&
                   reason != hitreg_unavailable_reason::cross_hold_feedback;
        }

        static void mark_unavailable(hold_record& hold, hitreg_unavailable_reason reason)
        {
            if (hold.unavailable_reason == hitreg_unavailable_reason::none)
            {
                hold.unavailable_reason = reason;
            }
        }

        hold_record* open_hold()
        {
            if (active_hold_count == 0 || !active_holds[active_hold_count - 1].open)
            {
                return nullptr;
            }
            return &active_holds[active_hold_count - 1];
        }

        void close_open_hold(int32_t command_time)
        {
            hold_record* hold = open_hold();
            if (hold == nullptr)
            {
                return;
            }
            hold->open = false;
            hold->end_time = command_time;
        }

        void publish(const hold_record& hold)
        {
            if (pub_generation != std::numeric_limits<uint64_t>::max())
            {
                ++pub_generation;
            }
            pub_display.generation = pub_generation;
            pub_display.client_hits = hold.client_hits;
            pub_display.server_hits = hold.server_hits;
            pub_display.samples = hold.samples;
            pub_display.fire_events = hold.fire_events;
            pub_display.client_accuracy_hits = hold.client_accuracy_hits;
            pub_display.client_accuracy_opportunities = hold.client_accuracy_opportunities;
            pub_display.trace_total = hold.trace_total;
            pub_display.trace_player = hold.trace_player;
            pub_display.trace_world = hold.trace_world;
            pub_display.trace_none = hold.trace_none;
            pub_display.trace_other = hold.trace_other;
            pub_display.trace_client_player = hold.trace_client_player;
            pub_display.start_time = hold.start_time;
            pub_display.end_time = hold.end_time;
            pub_display.unavailable_reason = hold.unavailable_reason;

            sess_client_hits += hold.client_accuracy_hits;
            sess_client_opportunities += hold.client_accuracy_opportunities;
            sess_server_hits += hold.server_hits;

            pub_display.session_client_hits = sess_client_hits;
            pub_display.session_client_opportunities = sess_client_opportunities;
            pub_display.session_server_hits = sess_server_hits;
            if (sess_client_opportunities > 0)
            {
                const uint64_t sess_num = static_cast<uint64_t>(sess_client_hits) * 10000ULL +
                                          static_cast<uint64_t>(sess_client_opportunities / 2U);
                pub_display.session_accuracy_percent_hundredths =
                    static_cast<uint32_t>(sess_num / sess_client_opportunities);
            }

            if (client_accuracy_unavailable(hold.unavailable_reason) ||
                hold.client_accuracy_opportunities == 0)
            {
                pub_display.client_accuracy_kind = client_accuracy_display_kind::unavailable;
                pub_display.client_accuracy_percent_hundredths = 0;
            }
            else
            {
                const uint64_t client_numerator =
                    static_cast<uint64_t>(hold.client_accuracy_hits) * 10000ULL +
                    static_cast<uint64_t>(hold.client_accuracy_opportunities / 2U);
                const uint64_t client_ratio =
                    client_numerator / hold.client_accuracy_opportunities;
                pub_display.client_accuracy_kind = client_accuracy_display_kind::percent;
                pub_display.client_accuracy_percent_hundredths =
                    static_cast<uint32_t>(client_ratio);
            }

            if (hold.unavailable_reason != hitreg_unavailable_reason::none)
            {
                pub_display.kind = hitreg_display_kind::unavailable;
                pub_display.percent = 0;
                pub_display.percent_hundredths = 0;
                return;
            }
            if (hold.client_hits == 0)
            {
                pub_display.kind = hold.server_hits == 0 ? hitreg_display_kind::percent
                                                         : hitreg_display_kind::infinity;
                pub_display.percent = hold.server_hits == 0 ? 100U : 0U;
                pub_display.percent_hundredths = hold.server_hits == 0 ? 10000U : 0U;
                return;
            }

            const uint64_t numerator = static_cast<uint64_t>(hold.server_hits) * 10000ULL +
                                       static_cast<uint64_t>(hold.client_hits / 2U);
            const uint64_t ratio = numerator / hold.client_hits;
            pub_display.kind = hitreg_display_kind::percent;
            pub_display.percent_hundredths = ratio > std::numeric_limits<uint32_t>::max()
                                                 ? std::numeric_limits<uint32_t>::max()
                                                 : static_cast<uint32_t>(ratio);
            const uint64_t rounded_whole = (ratio + 50ULL) / 100ULL;
            pub_display.percent = rounded_whole > std::numeric_limits<uint32_t>::max()
                                      ? std::numeric_limits<uint32_t>::max()
                                      : static_cast<uint32_t>(rounded_whole);
        }

        std::array<hold_record, max_inflight_holds> active_holds{};
        size_t active_hold_count{};
        hitreg_display pub_display{};
        uint64_t pub_generation{};
        int32_t last_fire_event_time{};
        bool have_last_fire_event_time{};
        int32_t last_feedback_command_time{};
        int32_t last_feedback_hits{};
        bool have_last_feedback_state{};
        std::array<feedback_transition_id, max_feedback_transitions> seen_transitions{};
        size_t seen_transition_count{};
        uint32_t sess_client_hits{};
        uint32_t sess_client_opportunities{};
        uint32_t sess_server_hits{};
    };
}
