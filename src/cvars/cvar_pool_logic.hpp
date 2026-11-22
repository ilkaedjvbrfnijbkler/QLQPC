#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace qlqpc::cvars
{
    inline constexpr int32_t native_cvar_slots = 2048;
    inline constexpr int32_t max_total_cvar_slots = 16384;
    inline constexpr int32_t overflow_cvar_slots = max_total_cvar_slots - native_cvar_slots;
    inline constexpr int32_t cvar_hash_buckets = 256;
    inline constexpr uint32_t small_zone_default_bytes = 0x100000u;  // 1 MB stock
    inline constexpr uint32_t small_zone_expanded_bytes = 0x800000u; // 8 MB expanded

    // cvar_t::flags bitmasks
    inline constexpr uint32_t cvar_archive = 0x0001u;
    inline constexpr uint32_t cvar_init = 0x0010u;
    inline constexpr uint32_t cvar_latch = 0x0020u;
    inline constexpr uint32_t cvar_rom = 0x0040u;
    inline constexpr uint32_t cvar_user_created = 0x0080u;
    inline constexpr uint32_t cvar_cheat = 0x0200u;
    inline constexpr uint32_t cvar_norestart = 0x0400u;
    inline constexpr uint32_t cvar_replicated = 0x0800u;
    inline constexpr uint32_t cvar_range = 0x1000u;
    inline constexpr uint32_t cvar_valid_options = 0x2000u;
    inline constexpr uint32_t cvar_client_config = 0x80000u;
    inline constexpr uint32_t cvar_restart_skip_mask = cvar_init | cvar_rom | cvar_norestart;

    // exact binary layout of Quake Live's 68-byte 32-bit cvar_t
    struct engine_cvar
    {
        char* name;                    // +0x00
        char* string;                  // +0x04
        char* reset_string;            // +0x08
        char* latched_string;          // +0x0C
        int32_t reserved10;            // +0x10
        char* min_string;              // +0x14
        char* max_string;              // +0x18
        uint32_t flags;                // +0x1C
        int32_t modified;              // +0x20
        int32_t validate;              // +0x24
        int32_t modification_count;    // +0x28
        float value;                   // +0x2C
        int32_t integer;               // +0x30
        float min_val;                 // +0x34
        float max_val;                 // +0x38
        engine_cvar* next;             // +0x3C
        engine_cvar* hash_next;        // +0x40
    };

    static_assert(sizeof(engine_cvar) == 68, "engine_cvar must match 68-byte 32-bit cvar_t");
    static_assert(offsetof(engine_cvar, flags) == 0x1C);
    static_assert(offsetof(engine_cvar, modification_count) == 0x28);
    static_assert(offsetof(engine_cvar, value) == 0x2C);
    static_assert(offsetof(engine_cvar, min_val) == 0x34);
    static_assert(offsetof(engine_cvar, next) == 0x3C);
    static_assert(offsetof(engine_cvar, hash_next) == 0x40);

    // 8-bit case-insensitive cvar name hash matching engine lookup
    inline uint8_t compute_cvar_hash(const char* name)
    {
        if (name == nullptr)
        {
            return 0;
        }
        uint8_t hash = 0;
        for (int i = 0; name[i] != '\0'; ++i)
        {
            const auto ch = static_cast<unsigned char>(name[i]);
            const auto lower = static_cast<uint8_t>(std::tolower(ch));
            const auto term = static_cast<uint8_t>((119 + i) * lower);
            hash = static_cast<uint8_t>(hash + term);
        }
        return hash;
    }

    inline bool cvar_name_has_illegal_chars(const char* name)
    {
        if (name == nullptr)
        {
            return true;
        }
        for (const char* p = name; *p != '\0'; ++p)
        {
            if (*p == '\\' || *p == '"' || *p == ';')
            {
                return true;
            }
        }
        return false;
    }

    inline const char* sanitize_cvar_name(const char* name)
    {
        return cvar_name_has_illegal_chars(name) ? "BADNAME" : name;
    }

    inline bool unlink_from_single_bucket(engine_cvar* target, engine_cvar** bucket_head)
    {
        if (target == nullptr || bucket_head == nullptr)
        {
            return false;
        }
        engine_cvar** link = bucket_head;
        while (*link != nullptr)
        {
            if (*link == target)
            {
                *link = target->hash_next;
                target->hash_next = nullptr;
                return true;
            }
            link = &((*link)->hash_next);
        }
        return false;
    }

    inline bool unlink_cvar_from_hash_table(
        engine_cvar* target,
        engine_cvar** hash_table,
        int32_t bucket_count = cvar_hash_buckets)
    {
        if (target == nullptr || target->name == nullptr || hash_table == nullptr || bucket_count <= 0)
        {
            return false;
        }
        const uint8_t bucket = compute_cvar_hash(target->name);
        if (bucket >= bucket_count)
        {
            return false;
        }
        return unlink_from_single_bucket(target, &hash_table[bucket]);
    }

    // strip replicated flag on newly created non-forced set variables so scratch cvars stay out of repconfig.cfg
    inline void strip_unwanted_replicated_on_new_set(
        uint32_t& flags,
        bool had_cvar_before,
        int force)
    {
        if (!had_cvar_before && force == 0 && (flags & cvar_user_created) != 0u)
        {
            flags &= ~cvar_replicated;
        }
    }

    enum class reset_action
    {
        preserve,
        free_and_reclaim,
        reset_to_default,
    };

    constexpr reset_action decide_cvar_restart_action(
        uint32_t flags,
        bool is_qlqpc_builtin,
        bool is_vm_pinned = false)
    {
        if ((flags & cvar_restart_skip_mask) != 0u)
        {
            return reset_action::preserve;
        }
        if ((flags & cvar_user_created) != 0u && !is_vm_pinned && !is_qlqpc_builtin)
        {
            return reset_action::free_and_reclaim;
        }
        return reset_action::reset_to_default;
    }

    constexpr bool can_insert_into_cmd_text(
        int32_t cursize,
        int32_t maxsize,
        size_t text_len)
    {
        if (cursize < 0 || maxsize <= 0 || cursize > maxsize)
        {
            return false;
        }
        const size_t available = static_cast<size_t>(maxsize - cursize);
        return available > 0u && text_len <= (available - 1u);
    }

    inline bool equals_ignore_case(const char* a, const char* b)
    {
        if (a == nullptr || b == nullptr)
        {
            return a == b;
        }
        while (*a != '\0' && *b != '\0')
        {
            const auto ca = static_cast<unsigned char>(*a++);
            const auto cb = static_cast<unsigned char>(*b++);
            if (std::tolower(ca) != std::tolower(cb))
            {
                return false;
            }
        }
        return *a == *b;
    }

    inline bool is_default_cfg_name(const char* resolved_name)
    {
        if (resolved_name == nullptr)
        {
            return false;
        }
        const char* base = resolved_name;
        for (const char* p = resolved_name; *p != '\0'; ++p)
        {
            if (*p == '/' || *p == '\\')
            {
                base = p + 1;
            }
        }
        return equals_ignore_case(base, "default.cfg");
    }

    // extends Quake Live's 2048-slot cvar array with a reclaimable overflow table up to 16384 slots
    class cvar_pool_allocator
    {
    public:
        struct slot_ref
        {
            int32_t index{-1};
            engine_cvar* ptr{nullptr};
        };

        void reset(engine_cvar* native, engine_cvar* overflow)
        {
            native_base = native;
            overflow_base = overflow;
            native_free.clear();
            overflow_free.clear();
            in_free_list.fill(false);
            vm_pinned.fill(false);
        }

        bool is_initialized() const
        {
            return native_base != nullptr;
        }

        slot_ref allocate_slot(int32_t& high_water_count)
        {
            if (native_base == nullptr || high_water_count < 0)
            {
                return {};
            }

            if (!native_free.empty())
            {
                const int32_t idx = native_free.back();
                native_free.pop_back();
                in_free_list[static_cast<size_t>(idx)] = false;
                return { idx, native_base + idx };
            }

            if (high_water_count < native_cvar_slots)
            {
                const int32_t idx = high_water_count++;
                return { idx, native_base + idx };
            }

            if (!overflow_free.empty() && overflow_base != nullptr)
            {
                const int32_t idx = overflow_free.back();
                overflow_free.pop_back();
                in_free_list[static_cast<size_t>(idx)] = false;
                return { idx, overflow_base + (idx - native_cvar_slots) };
            }

            if (overflow_base != nullptr && high_water_count < max_total_cvar_slots)
            {
                const int32_t idx = high_water_count++;
                return { idx, overflow_base + (idx - native_cvar_slots) };
            }

            return {};
        }

        int32_t index_of(const engine_cvar* ptr) const
        {
            if (ptr == nullptr)
            {
                return -1;
            }
            if (native_base != nullptr &&
                ptr >= native_base &&
                ptr < native_base + native_cvar_slots)
            {
                const ptrdiff_t byte_off =
                    reinterpret_cast<const uint8_t*>(ptr) - reinterpret_cast<const uint8_t*>(native_base);
                if ((byte_off % static_cast<ptrdiff_t>(sizeof(engine_cvar))) == 0)
                {
                    return static_cast<int32_t>(byte_off / static_cast<ptrdiff_t>(sizeof(engine_cvar)));
                }
                return -1;
            }
            if (overflow_base != nullptr &&
                ptr >= overflow_base &&
                ptr < overflow_base + overflow_cvar_slots)
            {
                const ptrdiff_t byte_off =
                    reinterpret_cast<const uint8_t*>(ptr) - reinterpret_cast<const uint8_t*>(overflow_base);
                if ((byte_off % static_cast<ptrdiff_t>(sizeof(engine_cvar))) == 0)
                {
                    return native_cvar_slots +
                        static_cast<int32_t>(byte_off / static_cast<ptrdiff_t>(sizeof(engine_cvar)));
                }
                return -1;
            }
            return -1;
        }

        engine_cvar* slot_ptr(int32_t index, int32_t high_water_count) const
        {
            if (index < 0 || index >= high_water_count || index >= max_total_cvar_slots)
            {
                return nullptr;
            }
            if (index < native_cvar_slots)
            {
                return (native_base != nullptr) ? (native_base + index) : nullptr;
            }
            return (overflow_base != nullptr)
                ? (overflow_base + (index - native_cvar_slots))
                : nullptr;
        }

        void pin_vm_slot(int32_t index)
        {
            if (index >= 0 && index < max_total_cvar_slots)
            {
                vm_pinned[static_cast<size_t>(index)] = true;
            }
        }

        bool is_vm_pinned(int32_t index) const
        {
            if (index < 0 || index >= max_total_cvar_slots)
            {
                return false;
            }
            return vm_pinned[static_cast<size_t>(index)];
        }

        bool reclaim_slot(const engine_cvar* ptr)
        {
            const int32_t idx = index_of(ptr);
            if (idx < 0 || idx >= max_total_cvar_slots)
            {
                return false;
            }
            const auto uidx = static_cast<size_t>(idx);
            if (vm_pinned[uidx] || in_free_list[uidx])
            {
                return false;
            }
            in_free_list[uidx] = true;
            if (idx < native_cvar_slots)
            {
                native_free.push_back(idx);
            }
            else
            {
                overflow_free.push_back(idx);
            }
            return true;
        }

        size_t free_slot_count() const
        {
            return native_free.size() + overflow_free.size();
        }

        size_t native_free_count() const
        {
            return native_free.size();
        }

        size_t overflow_free_count() const
        {
            return overflow_free.size();
        }

    private:
        engine_cvar* native_base{nullptr};
        engine_cvar* overflow_base{nullptr};
        std::vector<int32_t> native_free;
        std::vector<int32_t> overflow_free;
        std::array<bool, max_total_cvar_slots> in_free_list{};
        std::array<bool, max_total_cvar_slots> vm_pinned{};
    };

    // tracks queued .cfg bytes in cmd_text so top-level exec clean-switches without resetting nested exec/vstr
    class exec_nesting_tracker
    {
    public:
        void clear()
        {
            pre_cfg_bytes = 0;
            active_cfg_bytes_val = 0;
            line_from_cfg = false;
            exec_override_depth = 0;
        }

        void on_line_popped(int32_t popped_bytes)
        {
            if (popped_bytes <= 0)
            {
                line_from_cfg = false;
                return;
            }
            int32_t remaining = popped_bytes;
            bool had_pre_cfg = false;
            if (pre_cfg_bytes > 0)
            {
                had_pre_cfg = true;
                const int32_t take_pre = (std::min)(remaining, pre_cfg_bytes);
                pre_cfg_bytes -= take_pre;
                remaining -= take_pre;
            }
            if (remaining > 0 && active_cfg_bytes_val > 0)
            {
                active_cfg_bytes_val -= (std::min)(remaining, active_cfg_bytes_val);
                line_from_cfg = !had_pre_cfg;
            }
            else
            {
                line_from_cfg = false;
            }
        }

        void on_cbuf_execute_exit()
        {
            line_from_cfg = false;
        }

        void clamp_to_buffer_size(int32_t cmd_text_cursize)
        {
            if (cmd_text_cursize <= 0)
            {
                pre_cfg_bytes = 0;
                active_cfg_bytes_val = 0;
                line_from_cfg = false;
                return;
            }
            if (pre_cfg_bytes > cmd_text_cursize)
            {
                pre_cfg_bytes = cmd_text_cursize;
            }
            const int32_t max_active = cmd_text_cursize - pre_cfg_bytes;
            if (active_cfg_bytes_val > max_active)
            {
                active_cfg_bytes_val = max_active;
            }
        }

        bool should_clean_switch_on_exec(bool com_fully_initialized) const
        {
            return com_fully_initialized && !line_from_cfg;
        }

        void on_cfg_inserted(int32_t inserted_bytes)
        {
            if (inserted_bytes > 0)
            {
                active_cfg_bytes_val += inserted_bytes;
            }
        }

        void on_generic_insert_text(int32_t inserted_bytes)
        {
            if (inserted_bytes <= 0 || exec_override_depth != 0)
            {
                return;
            }
            if (line_from_cfg)
            {
                active_cfg_bytes_val += inserted_bytes;
            }
            else if (active_cfg_bytes_val > 0)
            {
                pre_cfg_bytes += inserted_bytes;
            }
        }

        void begin_explicit_exec_insert()
        {
            ++exec_override_depth;
        }

        void end_explicit_exec_insert()
        {
            if (exec_override_depth > 0)
            {
                --exec_override_depth;
            }
        }

        int32_t active_cfg_bytes() const
        {
            return active_cfg_bytes_val;
        }

        int32_t pre_cfg_insert_bytes() const
        {
            return pre_cfg_bytes;
        }

        bool is_current_line_from_cfg() const
        {
            return line_from_cfg;
        }

    private:
        int32_t pre_cfg_bytes{0};
        int32_t active_cfg_bytes_val{0};
        bool line_from_cfg{false};
        int32_t exec_override_depth{0};
    };
}
