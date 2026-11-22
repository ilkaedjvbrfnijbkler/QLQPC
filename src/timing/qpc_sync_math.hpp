#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace qlqpc::timing
{
    // Quake Live pitch limit from PM_UpdateViewAngles: 16000 * (360.0 / 65536.0)
    inline constexpr float ql_max_pitch_deg = 16000.0f * (360.0f / 65536.0f);
    inline constexpr float ql_angle_to_short_scale = 65536.0f / 360.0f;

    inline uint16_t ql_angle_to_short(float deg)
    {
        const float wrapped = std::remainder(deg, 360.0f);
        const long rounded = std::lrintf(wrapped * ql_angle_to_short_scale);
        return static_cast<uint16_t>(rounded & 0xFFFFL);
    }

    inline float ql_short_to_angle(int32_t short_val)
    {
        const auto s16 = static_cast<int16_t>(short_val & 0xFFFF);
        return static_cast<float>(static_cast<double>(s16) * (360.0 / 65536.0));
    }

    // computes float32 viewangles matching PM_UpdateViewAngles without 16-bit SHORT2ANGLE quantization
    inline void compute_precise_viewangles(
        const float cl_viewangles[3],
        const int32_t ps_delta_angles[3],
        float out_viewangles[3])
    {
        const float raw_pitch = std::remainder(
            cl_viewangles[0] + ql_short_to_angle(ps_delta_angles[0]), 360.0f);
        out_viewangles[0] = std::clamp(raw_pitch, -ql_max_pitch_deg, ql_max_pitch_deg);
        out_viewangles[1] = std::remainder(
            cl_viewangles[1] + ql_short_to_angle(ps_delta_angles[1]), 360.0f);
        out_viewangles[2] = std::remainder(
            cl_viewangles[2] + ql_short_to_angle(ps_delta_angles[2]), 360.0f);
    }

    // lossless QPC tick to integer ms and [0, 1) sub-ms fraction accumulator
    class tick_msec_accumulator
    {
    public:
        void reset()
        {
            last_ticks = 0;
            rem_ticks = 0;
            initialized = false;
        }

        // re-anchor after synchronous load stalls so the next frame does not lunge forward
        void reanchor(int64_t now_ticks)
        {
            last_ticks = now_ticks;
            rem_ticks = 0;
            initialized = true;
        }

        int step(int64_t now_ticks, int64_t freq)
        {
            if (!initialized)
            {
                last_ticks = now_ticks;
                rem_ticks = 0;
                initialized = true;
                return 1;
            }

            int64_t delta_ticks = now_ticks - last_ticks;
            last_ticks = now_ticks;

            const int64_t max_hitch_ticks = freq / 2;
            if (delta_ticks < 0)
            {
                delta_ticks = 0;
            }
            else if (delta_ticks > max_hitch_ticks)
            {
                delta_ticks = max_hitch_ticks;
            }

            const int64_t total_scaled = rem_ticks + (delta_ticks * 1000);
            const int whole_ms = static_cast<int>(total_scaled / freq);
            rem_ticks = total_scaled % freq;
            return whole_ms;
        }

        double sub_ms_fraction(int64_t freq) const
        {
            if (!initialized)
            {
                return 0.0;
            }
            const double frac = static_cast<double>(rem_ticks) / static_cast<double>(freq);
            return std::clamp(frac, 0.0, 0.999999);
        }

        int64_t remainder_ticks() const
        {
            return rem_ticks;
        }

    private:
        int64_t last_ticks{0};
        int64_t rem_ticks{0};
        bool initialized{false};
    };

    // monotonic render timestamp combining integer cg.time and sub-ms fraction
    class monotonic_render_clock
    {
    public:
        void reset()
        {
            last_exact_ms = 0.0;
            initialized = false;
        }

        double sample(int32_t cg_time, double sub_ms_frac)
        {
            const double clamped_frac = std::clamp(sub_ms_frac, 0.0, 0.999999);
            const double candidate = static_cast<double>(cg_time) + clamped_frac;
            if (!initialized || std::abs(candidate - last_exact_ms) > 50.0)
            {
                last_exact_ms = candidate;
                initialized = true;
                return candidate;
            }
            if (candidate < last_exact_ms)
            {
                return last_exact_ms;
            }
            last_exact_ms = candidate;
            return candidate;
        }

    private:
        double last_exact_ms{0.0};
        bool initialized{false};
    };

    // rational Bresenham pacer decoupling outbound usercmd cadence from render FPS
    class command_cadence_pacer
    {
    public:
        void reset()
        {
            last_ticks = 0;
            accum_scaled = 0;
            initialized = false;
        }

        static int clamp_cmd_rate(int requested_hz)
        {
            if (requested_hz <= 0)
            {
                return 250;
            }
            return std::clamp(requested_hz, 30, 250);
        }

        bool should_emit_command(int64_t now_ticks, int64_t freq, int requested_hz)
        {
            const int rate = clamp_cmd_rate(requested_hz);
            if (!initialized)
            {
                last_ticks = now_ticks;
                accum_scaled = 0;
                initialized = true;
                return true;
            }

            int64_t delta_ticks = now_ticks - last_ticks;
            last_ticks = now_ticks;
            if (delta_ticks < 0)
            {
                delta_ticks = 0;
            }
            else if (delta_ticks > freq / 4)
            {
                delta_ticks = freq / 4;
            }

            accum_scaled += delta_ticks * static_cast<int64_t>(rate);
            if (accum_scaled >= freq)
            {
                accum_scaled -= freq;
                if (accum_scaled >= freq)
                {
                    accum_scaled = freq - 1;
                }
                return true;
            }
            return false;
        }

    private:
        int64_t last_ticks{0};
        int64_t accum_scaled{0};
        bool initialized{false};
    };

    // tracks QPC delta between non-zero mouse samples so accel stays invariant to FPS and USB rate
    class mouse_rate_timer
    {
    public:
        void reset()
        {
            last_frame_ticks = 0;
            last_nonzero_ticks = 0;
            initialized = false;
        }

        double on_mouse_sample(int64_t now_ticks, int64_t freq, bool has_counts)
        {
            if (!initialized)
            {
                last_frame_ticks = now_ticks;
                last_nonzero_ticks = now_ticks;
                initialized = true;
                return 1.0;
            }

            const int64_t frame_delta = std::max<int64_t>(1, now_ticks - last_frame_ticks);
            last_frame_ticks = now_ticks;
            const double frame_dt_ms = (static_cast<double>(frame_delta) * 1000.0) / static_cast<double>(freq);

            if (!has_counts)
            {
                return std::clamp(frame_dt_ms, 0.05, 50.0);
            }

            const int64_t nonzero_delta = std::max<int64_t>(frame_delta, now_ticks - last_nonzero_ticks);
            last_nonzero_ticks = now_ticks;
            const double nonzero_dt_ms = (static_cast<double>(nonzero_delta) * 1000.0) / static_cast<double>(freq);

            const double max_window_ms = std::max(frame_dt_ms, 8.0);
            return std::clamp(nonzero_dt_ms, std::max(0.05, frame_dt_ms), max_window_ms);
        }

    private:
        int64_t last_frame_ticks{0};
        int64_t last_nonzero_ticks{0};
        bool initialized{false};
    };

    // deadbanded snapshot clock sync replacing stock +1/+1/-2 sawtooth oscillation
    class snapshot_clock_sync
    {
    public:
        enum class adjust_mode : uint8_t
        {
            hold = 0,
            step = 1,
            fast = 2,
            reset = 3
        };

        struct adjust_result
        {
            int32_t new_server_time_delta{0};
            adjust_mode mode{adjust_mode::hold};
        };

        static constexpr size_t baseline_window_size = 32;

        void reset()
        {
            raw_window.fill(0);
            window_count = 0;
            window_head = 0;
            baseline_raw_delta = 0.0;
            jitter_margin_ms = 6.0;
            snap_interval_ms = 25;
            last_snap_server_time = 0;
            pending_neg_adjust = 0;
            neg_adjust_residual = 0;
            snapshot_count = 0;
            initialized = false;
        }

        adjust_result on_new_snapshot(
            int32_t snap_server_time,
            int32_t cls_realtime,
            int32_t cur_server_time_delta)
        {
            const int32_t raw_delta = snap_server_time - cls_realtime;
            const bool clock_rewound = initialized && (snap_server_time < (last_snap_server_time - 100));
            const int64_t delta_diff = static_cast<int64_t>(raw_delta) - static_cast<int64_t>(cur_server_time_delta);
            const bool large_delta_jump = (delta_diff > 500 || delta_diff < -500);

            if (!initialized || clock_rewound || large_delta_jump)
            {
                const bool reset_now = initialized || large_delta_jump;
                raw_window.fill(0);
                raw_window[0] = raw_delta;
                window_count = 1;
                window_head = 1;
                baseline_raw_delta = static_cast<double>(raw_delta);
                jitter_margin_ms = 6.0;
                snap_interval_ms = 25;
                last_snap_server_time = snap_server_time;
                snapshot_count = 1;
                initialized = true;

                if (reset_now)
                {
                    pending_neg_adjust = 0;
                    neg_adjust_residual = 0;
                    return {raw_delta, adjust_mode::reset};
                }

                const int32_t target_delta = raw_delta - snap_interval_ms - 6;
                pending_neg_adjust = std::max<int32_t>(0, cur_server_time_delta - target_delta);
                neg_adjust_residual = 0;
                return {cur_server_time_delta, adjust_mode::hold};
            }

            ++snapshot_count;
            const int32_t snap_step = snap_server_time - last_snap_server_time;
            if (snap_step > 0)
            {
                last_snap_server_time = snap_server_time;
                if (snap_step >= 8 && snap_step <= 55)
                {
                    if (std::abs(snap_step - 25) <= 3)
                    {
                        snap_interval_ms = 25;
                    }
                    else if (std::abs(snap_step - 50) <= 4)
                    {
                        snap_interval_ms = 50;
                    }
                    else if (std::abs(snap_step - 33) <= 3)
                    {
                        snap_interval_ms = 33;
                    }
                    else
                    {
                        snap_interval_ms = std::clamp((snap_interval_ms * 3 + snap_step + 2) / 4, 8, 50);
                    }
                }
            }

            raw_window[window_head] = raw_delta;
            window_head = (window_head + 1) % baseline_window_size;
            if (window_count < baseline_window_size)
            {
                ++window_count;
            }

            // upper ~90th percentile ignores transient positive raw_delta spikes
            std::array<int32_t, baseline_window_size> sorted{};
            for (size_t i = 0; i < window_count; ++i)
            {
                sorted[i] = raw_window[i];
            }
            std::sort(sorted.begin(), sorted.begin() + static_cast<ptrdiff_t>(window_count));

            size_t upper_idx = 0;
            if (window_count == 2)
            {
                upper_idx = 1;
            }
            else if (window_count >= 3)
            {
                const size_t skip = std::max<size_t>(1, window_count / 10);
                upper_idx = window_count - 1 - skip;
            }
            const double upper_d = static_cast<double>(sorted[upper_idx]);

            if (snapshot_count <= 4)
            {
                baseline_raw_delta = upper_d;
            }
            else if (upper_d >= baseline_raw_delta)
            {
                const double deficit = upper_d - baseline_raw_delta;
                const double climb_cap = (window_count >= 8 && deficit > 20.0)
                    ? ((deficit > 60.0) ? 6.0 : 3.0)
                    : 0.5;
                baseline_raw_delta += std::min(climb_cap, deficit);
            }
            else
            {
                baseline_raw_delta -= std::min(2.0, baseline_raw_delta - upper_d);
            }

            const double delay_spike_ms = std::max(0.0, upper_d - static_cast<double>(raw_delta));
            if (delay_spike_ms <= 0.5)
            {
                jitter_margin_ms = std::max(5.5, jitter_margin_ms - 0.03 * (jitter_margin_ms - 5.5));
            }
            else
            {
                const double needed_margin = std::clamp(delay_spike_ms + 2.5, 5.5, 18.0);
                if (needed_margin > jitter_margin_ms)
                {
                    jitter_margin_ms += 0.40 * (needed_margin - jitter_margin_ms);
                }
                else
                {
                    jitter_margin_ms -= 0.03 * (jitter_margin_ms - 5.5);
                }
                jitter_margin_ms = std::clamp(jitter_margin_ms, 5.5, 18.0);
            }

            const int32_t max_margin = std::max<int32_t>(5, snap_interval_ms - 4);
            const double clamped_margin = std::min(jitter_margin_ms, static_cast<double>(max_margin));
            const int32_t target_delta = static_cast<int32_t>(std::llround(
                baseline_raw_delta - static_cast<double>(snap_interval_ms) - clamped_margin));

            if (snapshot_count <= 3 && cur_server_time_delta > target_delta)
            {
                pending_neg_adjust = cur_server_time_delta - target_delta;
                return {cur_server_time_delta, adjust_mode::hold};
            }

            // fast jumps only upward; downward jumps are drained smoothly so cl.serverTime never stalls
            if (target_delta - cur_server_time_delta > 100)
            {
                pending_neg_adjust = 0;
                neg_adjust_residual = 0;
                const int32_t mid = static_cast<int32_t>(
                    (static_cast<int64_t>(target_delta) + cur_server_time_delta) >> 1);
                return {mid, adjust_mode::fast};
            }

            const int32_t effective_cur = cur_server_time_delta - pending_neg_adjust;
            const int32_t diff = target_delta - effective_cur;

            if (std::abs(diff) <= 2)
            {
                return {cur_server_time_delta, adjust_mode::hold};
            }

            if (diff > 2)
            {
                int32_t rem_diff = diff;
                if (pending_neg_adjust > 0)
                {
                    const int32_t cancel = std::min(pending_neg_adjust, diff);
                    pending_neg_adjust -= cancel;
                    rem_diff -= cancel;
                    if (pending_neg_adjust == 0)
                    {
                        neg_adjust_residual = 0;
                    }
                    if (rem_diff <= 2)
                    {
                        return {cur_server_time_delta, adjust_mode::hold};
                    }
                }
                const int32_t step = (rem_diff > 25) ? 2 : 1;
                return {cur_server_time_delta + step, adjust_mode::step};
            }

            pending_neg_adjust = std::max(0, cur_server_time_delta - target_delta);
            return {cur_server_time_delta, adjust_mode::hold};
        }

        // drains downward delta adjustment at 25 percent of cls_frametime without freezing cl.serverTime
        int32_t consume_monotonic_pullback(int32_t cls_frametime)
        {
            if (pending_neg_adjust <= 0)
            {
                neg_adjust_residual = 0;
                return 0;
            }
            if (cls_frametime <= 0)
            {
                return 0;
            }
            neg_adjust_residual += cls_frametime;
            const int32_t avail = neg_adjust_residual / 4;
            if (avail <= 0)
            {
                return 0;
            }
            const int32_t absorb = std::min(std::min(pending_neg_adjust, avail), cls_frametime);
            pending_neg_adjust -= absorb;
            if (pending_neg_adjust <= 0)
            {
                neg_adjust_residual = 0;
            }
            else
            {
                neg_adjust_residual = std::max<int32_t>(0, neg_adjust_residual - absorb * 4);
            }
            return absorb;
        }

        int32_t pending_pullback() const
        {
            return pending_neg_adjust;
        }

    private:
        std::array<int32_t, baseline_window_size> raw_window{};
        size_t window_count{0};
        size_t window_head{0};
        double baseline_raw_delta{0.0};
        double jitter_margin_ms{6.0};
        int32_t snap_interval_ms{25};
        int32_t last_snap_server_time{0};
        int32_t pending_neg_adjust{0};
        int32_t neg_adjust_residual{0};
        uint32_t snapshot_count{0};
        bool initialized{false};
    };

    inline constexpr int32_t max_cmd_future_lead_ms = 4;

    inline int32_t clamp_command_server_time(int32_t cur_st, int32_t prev_st)
    {
        if (cur_st > prev_st)
        {
            return cur_st;
        }
        if ((prev_st - cur_st) < 50)
        {
            return std::min(prev_st + 1, cur_st + max_cmd_future_lead_ms);
        }
        return cur_st;
    }

    // caps listen-server packet writes to [15, 125] Hz so cl.outPackets never wraps
    class loopback_packet_limiter
    {
    public:
        void reset()
        {
            last_send_ticks = 0;
            initialized = false;
        }

        bool should_send(int64_t now_ticks, int64_t freq, int max_packets)
        {
            const int hz = std::clamp((max_packets > 0) ? max_packets : 125, 15, 125);
            const int64_t interval_ticks = freq / hz;
            if (interval_ticks <= 0)
            {
                return true;
            }
            if (!initialized || now_ticks < last_send_ticks)
            {
                last_send_ticks = now_ticks;
                initialized = true;
                return true;
            }
            const int64_t elapsed = now_ticks - last_send_ticks;
            if (elapsed >= interval_ticks)
            {
                if (elapsed < interval_ticks * 2)
                {
                    last_send_ticks += interval_ticks;
                }
                else
                {
                    last_send_ticks = now_ticks;
                }
                return true;
            }
            return false;
        }

    private:
        int64_t last_send_ticks{0};
        bool initialized{false};
    };

    inline float compute_continuous_frame_interpolation(
        double exact_render_ms,
        int32_t snap_server_time,
        int32_t next_snap_server_time)
    {
        const int32_t denom = next_snap_server_time - snap_server_time;
        if (denom <= 0)
        {
            return 0.0f;
        }
        const double numer = exact_render_ms - static_cast<double>(snap_server_time);
        const double f = numer / static_cast<double>(denom);
        return static_cast<float>(std::clamp(f, 0.0, 1.0));
    }

    inline float compute_subframe_pmove_dt_sec(
        double exact_render_ms,
        int32_t committed_command_time)
    {
        const double dt_ms = exact_render_ms - static_cast<double>(committed_command_time);
        if (dt_ms <= 0.0)
        {
            return 0.0f;
        }
        return static_cast<float>(std::min(dt_ms * 0.001, 0.025));
    }

    // restores committed commandTime, eFlags (EF_FIRING), torso, events, and weapon state after ephemeral pmove
    inline void restore_weapon_state(uint8_t* pps, const uint8_t* committed_pps)
    {
        if (pps == nullptr || committed_pps == nullptr)
        {
            return;
        }

        std::memcpy(pps + 0x00, committed_pps + 0x00, 4);

        constexpr uint32_t weapon_pm_flags = 0x200u | 0x400u | 0x8000u;
        uint32_t live_pm_flags = 0;
        uint32_t comm_pm_flags = 0;
        std::memcpy(&live_pm_flags, pps + 0x0C, 4);
        std::memcpy(&comm_pm_flags, committed_pps + 0x0C, 4);
        live_pm_flags = (live_pm_flags & ~weapon_pm_flags) | (comm_pm_flags & weapon_pm_flags);
        std::memcpy(pps + 0x0C, &live_pm_flags, 4);

        std::memcpy(pps + 0x2C, committed_pps + 0x2C, 4);
        std::memcpy(pps + 0x50, committed_pps + 0x50, 8);
        std::memcpy(pps + 0x68, committed_pps + 0x68, 24);
        std::memcpy(pps + 0x90, committed_pps + 0x90, 4);
        std::memcpy(pps + 0x98, committed_pps + 0x98, 4);
        std::memcpy(pps + 0xC4, committed_pps + 0xC4, 4);
        std::memcpy(pps + 0xE0, committed_pps + 0xE0, 4);
        std::memcpy(pps + 0x180, committed_pps + 0x180, 64);
    }

    class qpc_fps_estimator
    {
    public:
        void reset()
        {
            window_start_ticks = 0;
            frames_in_window = 0;
            displayed_fps = 0;
        }

        int on_frame(int64_t now_ticks, int64_t freq)
        {
            if (window_start_ticks == 0 || now_ticks < window_start_ticks)
            {
                window_start_ticks = now_ticks;
                frames_in_window = 0;
                return (displayed_fps > 0) ? displayed_fps : 250;
            }

            ++frames_in_window;
            const int64_t elapsed = now_ticks - window_start_ticks;
            if (elapsed >= (freq / 2) && elapsed > 0)
            {
                const double fps = (static_cast<double>(frames_in_window) * static_cast<double>(freq))
                                 / static_cast<double>(elapsed);
                displayed_fps = std::clamp(static_cast<int>(std::lround(fps)), 1, 99999);
                window_start_ticks = now_ticks;
                frames_in_window = 0;
            }
            return (displayed_fps > 0) ? displayed_fps : 250;
        }

    private:
        int64_t window_start_ticks{0};
        int32_t frames_in_window{0};
        int32_t displayed_fps{0};
    };

    inline constexpr int decoupling_off_hz = 0;
    inline constexpr int min_outbound_hz = 30;
    inline constexpr int max_outbound_hz = 250;
    inline constexpr int default_outbound_hz = 250;

    constexpr int clamp_outbound_hz(int hz)
    {
        if (hz <= decoupling_off_hz)
        {
            return decoupling_off_hz;
        }
        return std::clamp(hz, min_outbound_hz, max_outbound_hz);
    }

    constexpr bool is_decoupling_enabled(int outbound_hz)
    {
        return outbound_hz > decoupling_off_hz;
    }
}
