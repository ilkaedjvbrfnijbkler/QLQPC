#include "cvars/cvars.hpp"

#include "cvars/cvar_pool_logic.hpp"
#include "game/game.hpp"
#include "hitreg/accuracy_probe.hpp"
#include "timing/qpc_sync_math.hpp"
#include "util/hook_engine.hpp"
#include "visuals/lightning_logic.hpp"
#include "visuals/visuals.hpp"
#include "webui/webui_patch.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace qlqpc::cvars
{
    namespace
    {
        constexpr int cvar_hw_flags = static_cast<int>(cvar_archive | cvar_client_config);
        constexpr int cvar_rep_flags = static_cast<int>(cvar_archive | cvar_replicated | cvar_client_config);
        constexpr uint32_t sync_interval_ms = 25u;

        enum class sync_phase : uint8_t
        {
            awaiting_cvars,
            awaiting_initial_sync,
            live,
        };

        util::inline_hook hook_cvar_get{};
        util::inline_hook hook_cvar_get3{};
        util::inline_hook hook_cvar_register4{};
        util::inline_hook hook_cvar_register6{};
        util::inline_hook hook_cvar_update{};
        util::inline_hook hook_cvar_set2{};
        util::inline_hook hook_cmd_seta{};
        util::inline_hook hook_cvar_restart{};
        util::inline_hook hook_cvar_init{};
        util::inline_hook hook_cbuf_execute{};
        util::inline_hook hook_cbuf_insert_text{};
        util::inline_hook hook_cmd_exec{};
        util::mid_hook hook_cbuf_before_pop{};
        util::mid_hook hook_cbuf_after_pop{};

        cvar_pool_allocator cvar_pool{};
        exec_nesting_tracker exec_tracker{};
        engine_cvar* overflow_pool = nullptr;
        bool small_zone_patched = false;
        int32_t cbuf_line_start_cursize = 0;

        runtime_settings cur_settings{};
        sync_phase cur_sync_phase{sync_phase::awaiting_cvars};
        bool sync_gate_primed = false;
        uint32_t last_sync_tick_ms = 0;

        void* cvar_com_maxfps = nullptr;
        void* cvar_cl_cmdrate = nullptr;
        void* cvar_draw_accuracy = nullptr;
        void* cvar_draw_other_weapons = nullptr;
        void* cvar_team_lightning_style = nullptr;
        void* cvar_team_lightning_opacity = nullptr;
        void* cvar_enemy_lightning_style = nullptr;
        void* cvar_enemy_lightning_opacity = nullptr;
        void* cvar_force_white_names = nullptr;
        void* cvar_fog = nullptr;
        void* cvar_fog_distance = nullptr;
        void* cvar_fog_intensity = nullptr;
        void* cvar_fog_override_native = nullptr;
        void* cvar_fog_color = nullptr;
        void* cvar_vulkan = nullptr;

        int fog_r = 255;
        int fog_g = 255;
        int fog_b = 255;
        int last_cvar_cmdrate = timing::default_outbound_hz;
        int last_cvar_draw_acc = 0;
        int last_cvar_draw_other_weapons = 1;
        int last_cvar_force_white_names = 0;
        int last_cvar_fog = 0;
        std::string last_cvar_fog_color = "0xffffffff";
        int last_cvar_fog_dist = 768;
        int last_cvar_fog_int_pct = 100;
        int last_cvar_fog_ovr = 1;
        int last_cvar_vulkan = 0;

        struct bool_cvar_binding
        {
            const char* name;
            void** cvar_slot;
            int* last_value;
            int default_value;
            int reg_flags;
            bool* runtime_mirror;
            bool invert_mirror;
            bool affects_fog;
        };

        constexpr std::array<bool_cvar_binding, 6> bool_cvar_specs{{
            { "cg_drawAccuracy",     &cvar_draw_accuracy,      &last_cvar_draw_acc,           0, cvar_rep_flags, nullptr,                         false, false },
            { "cg_drawOtherWeapons", &cvar_draw_other_weapons, &last_cvar_draw_other_weapons, 1, cvar_rep_flags, &cur_settings.hide_other_weapons, true,  false },
            { "cg_forceWhiteNames",  &cvar_force_white_names,  &last_cvar_force_white_names,  0, cvar_rep_flags, &cur_settings.force_white_names,  false, false },
            { "r_fog",               &cvar_fog,                &last_cvar_fog,                0, cvar_rep_flags, nullptr,                         false, true  },
            { "r_fogOverrideNative", &cvar_fog_override_native,&last_cvar_fog_ovr,            1, cvar_rep_flags, nullptr,                         false, true  },
            { "r_vulkan",            &cvar_vulkan,             &last_cvar_vulkan,             0, cvar_hw_flags,  nullptr,                         false, false },
        }};

        bool should_run_periodic_sync(uint32_t now_ms)
        {
            if (!sync_gate_primed || static_cast<uint32_t>(now_ms - last_sync_tick_ms) >= sync_interval_ms)
            {
                last_sync_tick_ms = now_ms;
                sync_gate_primed = true;
                return true;
            }
            return false;
        }

        int32_t parse_cvar_integer(const char* str)
        {
            if (str == nullptr)
            {
                return 0;
            }
            if (std::strstr(str, "0x") != nullptr)
            {
                if (std::strncmp(str, "0x", 2) != 0)
                {
                    return 0;
                }
                uint32_t hex_val = 0;
                int digits = 0;
                for (const char* p = str + 2; *p != '\0' && digits < 8; ++p)
                {
                    uint32_t nibble = 0;
                    if (*p >= '0' && *p <= '9')
                    {
                        nibble = static_cast<uint32_t>(*p - '0');
                    }
                    else if (*p >= 'a' && *p <= 'f')
                    {
                        nibble = static_cast<uint32_t>(10 + (*p - 'a'));
                    }
                    else if (*p >= 'A' && *p <= 'F')
                    {
                        nibble = static_cast<uint32_t>(10 + (*p - 'A'));
                    }
                    else
                    {
                        break;
                    }
                    hex_val = (hex_val << 4) | nibble;
                    ++digits;
                }
                return static_cast<int32_t>(hex_val);
            }
            return std::atoi(str);
        }

        bool try_expand_small_zone_pre_init()
        {
            void* const* const block_ptr = game::exe_ptr<void*>(game::exe::addr_small_zone_block);
            if (block_ptr == nullptr)
            {
                return false;
            }
            if (*block_ptr != nullptr)
            {
                return true;
            }
            uint8_t* const fn = game::exe_ptr<uint8_t>(game::exe::addr_com_init_small_zone);
            if (fn == nullptr)
            {
                return false;
            }
            // match push 1; push 0x100000; mov smallzone_size, 0x100000 in Com_InitSmallZoneMemory
            if (fn[0] == 0x6Au && fn[1] == 0x01u && fn[2] == 0x68u && fn[7] == 0xC7u && fn[8] == 0x05u)
            {
                uint32_t push_imm = 0;
                uint32_t mov_imm = 0;
                std::memcpy(&push_imm, fn + 3, sizeof(uint32_t));
                std::memcpy(&mov_imm, fn + 13, sizeof(uint32_t));
                if (push_imm == small_zone_default_bytes && mov_imm == small_zone_default_bytes)
                {
                    util::patch_u32(fn + 3, small_zone_expanded_bytes);
                    util::patch_u32(fn + 13, small_zone_expanded_bytes);
                    small_zone_patched = true;
                    return true;
                }
            }
            return false;
        }

        void restore_small_zone_patch_if_unrun()
        {
            if (!small_zone_patched)
            {
                return;
            }
            void* const* const block_ptr = game::exe_ptr<void*>(game::exe::addr_small_zone_block);
            if (block_ptr != nullptr && *block_ptr == nullptr)
            {
                uint8_t* const fn = game::exe_ptr<uint8_t>(game::exe::addr_com_init_small_zone);
                if (fn != nullptr)
                {
                    uint32_t push_imm = 0;
                    uint32_t mov_imm = 0;
                    std::memcpy(&push_imm, fn + 3, sizeof(uint32_t));
                    std::memcpy(&mov_imm, fn + 13, sizeof(uint32_t));
                    if (push_imm == small_zone_expanded_bytes && mov_imm == small_zone_expanded_bytes)
                    {
                        util::patch_u32(fn + 3, small_zone_default_bytes);
                        util::patch_u32(fn + 13, small_zone_default_bytes);
                    }
                }
            }
            small_zone_patched = false;
        }

        void scrub_preexisting_tombstones(int32_t high_water_count)
        {
            auto** const hash_table = game::exe_ptr<engine_cvar*>(game::exe::addr_cvar_hash_table);
            auto* const native_base = game::exe_ptr<engine_cvar>(game::exe::addr_cvar_indexes);
            if (hash_table != nullptr)
            {
                for (int32_t b = 0; b < cvar_hash_buckets; ++b)
                {
                    engine_cvar** link = &hash_table[b];
                    while (*link != nullptr)
                    {
                        engine_cvar* const cur = *link;
                        if (cur->name == nullptr)
                        {
                            *link = cur->hash_next;
                            cur->hash_next = nullptr;
                        }
                        else
                        {
                            link = &cur->hash_next;
                        }
                    }
                }
            }
            if (native_base != nullptr && high_water_count > 0)
            {
                const int32_t limit = (std::min)(high_water_count, native_cvar_slots);
                for (int32_t i = 0; i < limit; ++i)
                {
                    if (native_base[i].name == nullptr)
                    {
                        std::memset(&native_base[i], 0, sizeof(engine_cvar));
                        cvar_pool.reclaim_slot(&native_base[i]);
                    }
                }
            }
        }

        bool init_pool_state()
        {
            if (overflow_pool == nullptr)
            {
                overflow_pool = static_cast<engine_cvar*>(VirtualAlloc(
                    nullptr,
                    static_cast<SIZE_T>(overflow_cvar_slots) * sizeof(engine_cvar),
                    MEM_COMMIT | MEM_RESERVE,
                    PAGE_READWRITE));
                if (overflow_pool == nullptr)
                {
                    return false;
                }
            }

            auto* const native_base = game::exe_ptr<engine_cvar>(game::exe::addr_cvar_indexes);
            auto* const num_indexes = game::exe_ptr<int32_t>(game::exe::addr_cvar_num_indexes);
            cvar_pool.reset(native_base, overflow_pool);
            exec_tracker.clear();

            if (num_indexes != nullptr && *num_indexes > 0)
            {
                scrub_preexisting_tombstones(*num_indexes);
            }
            return true;
        }

        void strip_user_created_flag(void* cvar)
        {
            if (cvar != nullptr)
            {
                *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(cvar) + 0x1C) &= ~cvar_user_created;
            }
        }

        bool all_managed_cvars_ready()
        {
            for (const auto& spec : bool_cvar_specs)
            {
                if (*spec.cvar_slot == nullptr)
                {
                    return false;
                }
            }
            return cvar_cl_cmdrate != nullptr &&
                   cvar_team_lightning_style != nullptr &&
                   cvar_team_lightning_opacity != nullptr &&
                   cvar_enemy_lightning_style != nullptr &&
                   cvar_enemy_lightning_opacity != nullptr &&
                   cvar_fog_color != nullptr &&
                   cvar_fog_distance != nullptr &&
                   cvar_fog_intensity != nullptr;
        }

        bool is_qlqpc_builtin_cvar(const void* cvar)
        {
            if (cvar == nullptr)
            {
                return false;
            }
            if (cvar == cvar_cl_cmdrate ||
                cvar == cvar_team_lightning_style ||
                cvar == cvar_team_lightning_opacity ||
                cvar == cvar_enemy_lightning_style ||
                cvar == cvar_enemy_lightning_opacity ||
                cvar == cvar_fog_distance ||
                cvar == cvar_fog_intensity ||
                cvar == cvar_fog_color)
            {
                return true;
            }
            for (const auto& spec : bool_cvar_specs)
            {
                if (cvar == *spec.cvar_slot)
                {
                    return true;
                }
            }

            const char* name = *reinterpret_cast<const char* const*>(cvar);
            if (name == nullptr)
            {
                return false;
            }
            for (const auto& spec : bool_cvar_specs)
            {
                if (equals_ignore_case(name, spec.name))
                {
                    return true;
                }
            }
            static constexpr const char* builtin_names[] = {
                "cl_cmdrate",
                "cg_teamLightningStyle",
                "cg_teamLightningOpacity",
                "cg_enemyLightningStyle",
                "cg_enemyLightningOpacity",
                "r_fogDistance",
                "r_fogIntensity",
                "r_fogColor",
            };
            for (const char* builtin_name : builtin_names)
            {
                if (equals_ignore_case(name, builtin_name))
                {
                    return true;
                }
            }
            return false;
        }

        void reset_registered_qlqpc_cvar_slots()
        {
            cvar_com_maxfps = nullptr;
            cvar_cl_cmdrate = nullptr;
            for (const auto& spec : bool_cvar_specs)
            {
                *spec.cvar_slot = nullptr;
            }
            cvar_team_lightning_style = nullptr;
            cvar_team_lightning_opacity = nullptr;
            cvar_enemy_lightning_style = nullptr;
            cvar_enemy_lightning_opacity = nullptr;
            cvar_fog_color = nullptr;
            cvar_fog_distance = nullptr;
            cvar_fog_intensity = nullptr;
            cur_sync_phase = sync_phase::awaiting_cvars;
        }

        void apply_cvar_int(const char* name, int value)
        {
            static const auto fn_cvar_set = game::exe_fn<game::cvar_set_t>(game::exe::addr_cvar_set);
            if (fn_cvar_set == nullptr || name == nullptr)
            {
                return;
            }
            char num_buf[16]{};
            std::snprintf(num_buf, sizeof(num_buf), "%d", value);
            fn_cvar_set(name, num_buf);
        }

        void apply_cvar_string(const char* name, const char* value)
        {
            static const auto fn_cvar_set = game::exe_fn<game::cvar_set_t>(game::exe::addr_cvar_set);
            if (fn_cvar_set == nullptr || name == nullptr)
            {
                return;
            }
            fn_cvar_set(name, (value != nullptr) ? value : "");
        }

        void populate_new_cvar_slot(
            engine_cvar* cvar,
            const char* name,
            const char* default_val,
            const char* min_val,
            const char* max_val,
            uint32_t flags,
            bool has_range)
        {
            const auto fn_copy_string = game::exe_fn<game::copy_string_t>(game::exe::addr_copy_string);
            const auto fn_com_printf = game::exe_fn<game::com_printf_t>(game::exe::addr_com_printf);
            auto** const cvar_vars = game::exe_ptr<engine_cvar*>(game::exe::addr_cvar_vars);
            auto** const hash_table = game::exe_ptr<engine_cvar*>(game::exe::addr_cvar_hash_table);

            std::memset(cvar, 0, sizeof(engine_cvar));
            cvar->name = fn_copy_string(name);
            cvar->string = fn_copy_string(default_val);
            cvar->modified = 1;
            cvar->validate = 0;
            cvar->modification_count = 1;
            cvar->value = static_cast<float>(std::atof(cvar->string));
            cvar->integer = parse_cvar_integer(cvar->string);
            cvar->reset_string = fn_copy_string(default_val);

            if (has_range)
            {
                const char* safe_min = (min_val != nullptr) ? min_val : "0";
                const char* safe_max = (max_val != nullptr) ? max_val : "0";
                cvar->min_string = fn_copy_string(safe_min);
                cvar->max_string = fn_copy_string(safe_max);
                cvar->min_val = static_cast<float>(std::atof(safe_min));
                cvar->max_val = static_cast<float>(std::atof(safe_max));
                cvar->flags = flags | cvar_range;
            }
            else
            {
                cvar->flags = flags;
            }

            if ((cvar->flags & cvar_cheat) != 0u)
            {
                if ((cvar->flags & cvar_archive) != 0u)
                {
                    fn_com_printf(
                        "Warning: cvar \"%s\" cannot be both CVAR_CHEAT and CVAR_ARCHIVE - dropping CVAR_ARCHIVE\n",
                        cvar->name);
                    cvar->flags &= ~cvar_archive;
                }
                if ((cvar->flags & cvar_replicated) != 0u)
                {
                    fn_com_printf(
                        "Warning: cvar \"%s\" cannot be both CVAR_CHEAT and CVAR_REPLICATED - dropping CVAR_REPLICATED\n",
                        cvar->name);
                    cvar->flags &= ~cvar_replicated;
                }
            }

            cvar->next = *cvar_vars;
            *cvar_vars = cvar;

            const uint8_t hash = compute_cvar_hash(name);
            cvar->hash_next = hash_table[hash];
            hash_table[hash] = cvar;
        }

        engine_cvar* create_cvar_miss(
            const char* sanitized_name,
            const char* default_val,
            const char* min_val,
            const char* max_val,
            uint32_t flags,
            bool has_range)
        {
            auto* const num_indexes = game::exe_ptr<int32_t>(game::exe::addr_cvar_num_indexes);
            const auto slot = cvar_pool.allocate_slot(*num_indexes);
            if (slot.ptr == nullptr)
            {
                const auto fn_com_error = game::exe_fn<game::com_error_t>(game::exe::addr_com_error);
                fn_com_error(0, "MAX_CVARS");
                return nullptr;
            }
            populate_new_cvar_slot(slot.ptr, sanitized_name, default_val, min_val, max_val, flags, has_range);
            return slot.ptr;
        }

        void* cvar_get5_pooled(
            const char* name,
            const char* default_val,
            const char* min_val,
            const char* max_val,
            int flags)
        {
            const char* safe_min = (min_val != nullptr) ? min_val : "0";
            const char* safe_max = (max_val != nullptr) ? max_val : "0";
            if (!cvar_pool.is_initialized())
            {
                game::exe_fn<game::com_error_t>(game::exe::addr_com_error)(1, "QLQPC: cvar pool not initialized");
                return nullptr;
            }
            if (name == nullptr || default_val == nullptr)
            {
                return hook_cvar_get.call<void*>(name, default_val, safe_min, safe_max, flags);
            }
            if (cvar_name_has_illegal_chars(name))
            {
                game::exe_fn<game::com_printf_t>(game::exe::addr_com_printf)("invalid cvar name string: %s\n", name);
                name = "BADNAME";
            }
            if (auto* const existing = static_cast<engine_cvar*>(
                    game::exe_fn<game::cvar_find_var_t>(game::exe::addr_cvar_find)(name));
                existing != nullptr)
            {
                // engine Cvar_Get overwrites range strings without freeing previous allocations when validate == 0
                if (existing->validate == 0)
                {
                    const auto fn_z_free = game::exe_fn<game::z_free_t>(game::exe::addr_z_free);
                    if (existing->min_string != nullptr)
                    {
                        fn_z_free(existing->min_string);
                        existing->min_string = nullptr;
                    }
                    if (existing->max_string != nullptr)
                    {
                        fn_z_free(existing->max_string);
                        existing->max_string = nullptr;
                    }
                }
                return hook_cvar_get.call<void*>(name, default_val, safe_min, safe_max, flags);
            }
            return create_cvar_miss(name, default_val, safe_min, safe_max, static_cast<uint32_t>(flags), true);
        }

        void free_cvar_strings_and_reclaim(engine_cvar* cvar)
        {
            const auto fn_z_free = game::exe_fn<game::z_free_t>(game::exe::addr_z_free);
            auto** const hash_table = game::exe_ptr<engine_cvar*>(game::exe::addr_cvar_hash_table);
            auto** const iter_cursor = game::exe_ptr<engine_cvar*>(game::exe::addr_cvar_iter_cursor);

            if (iter_cursor != nullptr && *iter_cursor == cvar)
            {
                *iter_cursor = nullptr;
            }
            unlink_cvar_from_hash_table(cvar, hash_table, cvar_hash_buckets);

            if (cvar->name != nullptr)
            {
                fn_z_free(cvar->name);
            }
            if (cvar->string != nullptr)
            {
                fn_z_free(cvar->string);
            }
            if (cvar->latched_string != nullptr)
            {
                fn_z_free(cvar->latched_string);
            }
            if (cvar->reset_string != nullptr)
            {
                fn_z_free(cvar->reset_string);
            }
            if (cvar->reserved10 != 0)
            {
                fn_z_free(reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(cvar->reserved10))));
            }
            if (cvar->min_string != nullptr)
            {
                fn_z_free(cvar->min_string);
            }
            if (cvar->max_string != nullptr)
            {
                fn_z_free(cvar->max_string);
            }

            std::memset(cvar, 0, sizeof(engine_cvar));
            cvar_pool.reclaim_slot(cvar);
        }

        void clean_restart_cvars()
        {
            auto** const cvar_vars = game::exe_ptr<engine_cvar*>(game::exe::addr_cvar_vars);
            auto* const mod_flags = game::exe_ptr<uint32_t>(game::exe::addr_cvar_modified_flags);
            auto** const iter_cursor = game::exe_ptr<engine_cvar*>(game::exe::addr_cvar_iter_cursor);
            if (iter_cursor != nullptr)
            {
                *iter_cursor = nullptr;
            }

            engine_cvar** link = cvar_vars;
            engine_cvar* cur = *link;
            while (cur != nullptr)
            {
                const bool qlqpc_builtin = is_qlqpc_builtin_cvar(cur);
                const bool is_vm_pinned = cvar_pool.is_vm_pinned(cvar_pool.index_of(cur));
                const reset_action action = decide_cvar_restart_action(cur->flags, qlqpc_builtin, is_vm_pinned);

                if (action == reset_action::free_and_reclaim)
                {
                    if ((cur->flags & cvar_archive) != 0u && mod_flags != nullptr)
                    {
                        *mod_flags |= cvar_archive;
                    }
                    *link = cur->next;
                    free_cvar_strings_and_reclaim(cur);
                    cur = *link;
                    continue;
                }

                if (action == reset_action::reset_to_default && cur->name != nullptr && cur->reset_string != nullptr)
                {
                    hook_cvar_set2.call<void*>(cur->name, cur->reset_string, 1);
                }

                link = &cur->next;
                cur = *link;
            }
        }

        void* __cdecl hk_cvar_get(
            const char* name,
            const char* default_val,
            const char* min_val,
            const char* max_val,
            int flags)
        {
            if (name != nullptr)
            {
                if (equals_ignore_case(name, "com_maxfps"))
                {
                    void* res = cvar_get5_pooled(
                        name,
                        (default_val != nullptr) ? default_val : "125",
                        "0",
                        "10000",
                        flags);
                    cvar_com_maxfps = res;
                    return res;
                }
                if (equals_ignore_case(name, "cl_cmdrate"))
                {
                    void* res = cvar_get5_pooled(
                        name,
                        (default_val != nullptr) ? default_val : "250",
                        "0",
                        "250",
                        flags | cvar_rep_flags);
                    cvar_cl_cmdrate = res;
                    return res;
                }
                if (equals_ignore_case(name, "cg_drawAccuracy"))
                {
                    void* res = cvar_get5_pooled(
                        name,
                        (default_val != nullptr) ? default_val : "0",
                        "0",
                        "1",
                        flags | cvar_rep_flags);
                    cvar_draw_accuracy = res;
                    return res;
                }
                if (equals_ignore_case(name, "cg_playerLean"))
                {
                    void* res = cvar_get5_pooled(
                        name,
                        (default_val != nullptr) ? default_val : "1",
                        "0",
                        "50",
                        flags | game::cvar_archive_flag);
                    return res;
                }
            }
            return cvar_get5_pooled(
                name,
                (default_val != nullptr) ? default_val : "",
                (min_val != nullptr) ? min_val : "0",
                (max_val != nullptr) ? max_val : "0",
                flags);
        }

        void* __cdecl hk_cvar_get3(const char* name, const char* default_val, int flags)
        {
            if (!cvar_pool.is_initialized())
            {
                game::exe_fn<game::com_error_t>(game::exe::addr_com_error)(1, "QLQPC: cvar pool not initialized");
                return nullptr;
            }
            if (name == nullptr || default_val == nullptr)
            {
                return hook_cvar_get3.call<void*>(name, default_val, flags);
            }
            if (cvar_name_has_illegal_chars(name))
            {
                game::exe_fn<game::com_printf_t>(game::exe::addr_com_printf)("invalid cvar name string: %s\n", name);
                name = "BADNAME";
            }
            if (game::exe_fn<game::cvar_find_var_t>(game::exe::addr_cvar_find)(name) != nullptr)
            {
                return hook_cvar_get3.call<void*>(name, default_val, flags);
            }
            return create_cvar_miss(name, default_val, nullptr, nullptr, static_cast<uint32_t>(flags), false);
        }

        int __cdecl hk_cvar_update(int32_t* vm_cvar)
        {
            if (!cvar_pool.is_initialized())
            {
                game::exe_fn<game::com_error_t>(game::exe::addr_com_error)(1, "QLQPC: cvar pool not initialized");
                return 0;
            }
            if (vm_cvar == nullptr)
            {
                return hook_cvar_update.call<int>(vm_cvar);
            }

            const int32_t handle = vm_cvar[0];
            const int32_t high_water = *game::exe_ptr<int32_t>(game::exe::addr_cvar_num_indexes);
            engine_cvar* const cvar = cvar_pool.slot_ptr(handle, high_water);
            if (cvar == nullptr)
            {
                game::exe_fn<game::com_error_t>(game::exe::addr_com_error)(1, "Cvar_Update: handle out of range");
                return 0;
            }

            int32_t result = cvar->modification_count;
            if (result != vm_cvar[1] && cvar->string != nullptr)
            {
                vm_cvar[1] = result;
                const size_t len = std::strlen(cvar->string);
                if (len + 1u > 256u)
                {
                    game::exe_fn<game::com_error_t>(game::exe::addr_com_error)(
                        1,
                        "Cvar_Update: src %s length %d exceeds MAX_CVAR_VALUE_STRING",
                        cvar->string,
                        static_cast<int>(len));
                }
                game::exe_fn<game::q_strncpyz_t>(game::exe::addr_q_strncpyz)(
                    reinterpret_cast<char*>(vm_cvar) + 20,
                    cvar->string,
                    256);
                *reinterpret_cast<float*>(vm_cvar + 2) = cvar->value;
                vm_cvar[3] = cvar->integer;
            }
            return result;
        }

        int __cdecl hk_cvar_register4(int32_t* vm_cvar, const char* name, const char* default_val, int flags)
        {
            engine_cvar* const cvar = static_cast<engine_cvar*>(hk_cvar_get3(name, default_val, flags));
            if (vm_cvar == nullptr || cvar == nullptr)
            {
                return static_cast<int>(reinterpret_cast<uintptr_t>(cvar));
            }
            const int32_t idx = cvar_pool.index_of(cvar);
            cvar_pool.pin_vm_slot(idx);
            vm_cvar[0] = idx;
            vm_cvar[1] = -1;
            vm_cvar[4] = static_cast<int32_t>(cvar->flags);
            return hk_cvar_update(vm_cvar);
        }

        int __cdecl hk_cvar_register6(
            int32_t* vm_cvar,
            const char* name,
            const char* default_val,
            const char* min_val,
            const char* max_val,
            int flags)
        {
            engine_cvar* const cvar = static_cast<engine_cvar*>(
                hk_cvar_get(name, default_val, min_val, max_val, flags | static_cast<int>(cvar_range)));
            if (vm_cvar == nullptr || cvar == nullptr)
            {
                return static_cast<int>(reinterpret_cast<uintptr_t>(cvar));
            }
            const int32_t idx = cvar_pool.index_of(cvar);
            cvar_pool.pin_vm_slot(idx);
            vm_cvar[0] = idx;
            vm_cvar[1] = -1;
            return hk_cvar_update(vm_cvar);
        }

        void* __cdecl hk_cvar_set2(const char* name, const char* value, int force)
        {
            const char* sanitized = sanitize_cvar_name(name);
            auto* const existing = static_cast<engine_cvar*>(
                game::exe_fn<game::cvar_find_var_t>(game::exe::addr_cvar_find)(sanitized));
            const bool had_cvar_before = (existing != nullptr);
            const std::string prev_str =
                (force == 0 && cur_sync_phase == sync_phase::live &&
                 existing != nullptr && existing == cvar_vulkan && existing->string != nullptr)
                    ? std::string(existing->string)
                    : std::string{};

            void* const res = hook_cvar_set2.call<void*>(name, value, force);
            if (res != nullptr)
            {
                auto* const cvar = static_cast<engine_cvar*>(res);
                strip_unwanted_replicated_on_new_set(cvar->flags, had_cvar_before, force);
                if (force == 0 && cur_sync_phase == sync_phase::live &&
                    cvar == cvar_vulkan && cvar->string != nullptr && prev_str != cvar->string)
                {
                    if (const auto fn_printf = game::exe_fn<game::com_printf_t>(game::exe::addr_com_printf))
                    {
                        fn_printf(
                            "%s will be changed to %s upon restarting.\n",
                            cvar->name,
                            cvar->string);
                    }
                }
            }
            return res;
        }

        int __cdecl hk_cmd_seta_f()
        {
            const int res = hook_cmd_seta.call<int>();
            const int argc = *game::exe_ptr<int>(game::exe::addr_cmd_argc);
            if (argc == 3)
            {
                const char* const* argv = game::exe_ptr<const char*>(game::exe::addr_cmd_argv);
                if (argv != nullptr && argv[1] != nullptr)
                {
                    if (game::exe_fn<game::cvar_find_var_t>(game::exe::addr_cvar_find)(sanitize_cvar_name(argv[1])) != nullptr)
                    {
                        auto* const mod_flags = game::exe_ptr<uint32_t>(game::exe::addr_cvar_modified_flags);
                        if (mod_flags != nullptr)
                        {
                            *mod_flags |= cvar_archive;
                        }
                    }
                }
            }
            return res;
        }

        void __cdecl hk_cvar_restart_f()
        {
            clean_restart_cvars();
        }

        int __cdecl hk_cvar_init()
        {
            if (overflow_pool != nullptr)
            {
                std::memset(overflow_pool, 0, static_cast<size_t>(overflow_cvar_slots) * sizeof(engine_cvar));
            }
            cvar_pool.reset(game::exe_ptr<engine_cvar>(game::exe::addr_cvar_indexes), overflow_pool);
            exec_tracker.clear();
            reset_registered_qlqpc_cvar_slots();
            const int res = hook_cvar_init.call<int>();
            register_qlqpc_cvars();
            return res;
        }

        int __cdecl hk_cbuf_execute()
        {
            exec_tracker.on_cbuf_execute_exit();
            auto* const cursize_ptr = game::exe_ptr<int32_t>(game::exe::addr_cmd_text_cursize);
            if (cursize_ptr != nullptr)
            {
                exec_tracker.clamp_to_buffer_size(*cursize_ptr);
            }
            const int res = hook_cbuf_execute.call<int>();
            exec_tracker.on_cbuf_execute_exit();
            return res;
        }

        void on_cbuf_before_pop(util::mid_context&)
        {
            auto* const cursize_ptr = game::exe_ptr<int32_t>(game::exe::addr_cmd_text_cursize);
            if (cursize_ptr != nullptr)
            {
                exec_tracker.clamp_to_buffer_size(*cursize_ptr);
                cbuf_line_start_cursize = *cursize_ptr;
            }
        }

        void on_cbuf_after_pop(util::mid_context&)
        {
            auto* const cursize_ptr = game::exe_ptr<int32_t>(game::exe::addr_cmd_text_cursize);
            if (cursize_ptr != nullptr)
            {
                const int32_t popped = cbuf_line_start_cursize - *cursize_ptr;
                exec_tracker.on_line_popped(popped);
            }
        }

        int __cdecl hk_cbuf_insert_text(const char* text)
        {
            auto* const cursize_ptr = game::exe_ptr<int32_t>(game::exe::addr_cmd_text_cursize);
            const int32_t before = (cursize_ptr != nullptr) ? *cursize_ptr : 0;
            const int res = hook_cbuf_insert_text.call<int>(text);
            const int32_t after = (cursize_ptr != nullptr) ? *cursize_ptr : 0;
            exec_tracker.on_generic_insert_text(after - before);
            return res;
        }

        void insert_cfg_tracked(const char* text)
        {
            auto* const cursize_ptr = game::exe_ptr<int32_t>(game::exe::addr_cmd_text_cursize);
            const int32_t before = (cursize_ptr != nullptr) ? *cursize_ptr : 0;
            hook_cbuf_insert_text.call<int>(text);
            const int32_t after = (cursize_ptr != nullptr) ? *cursize_ptr : 0;
            exec_tracker.on_cfg_inserted(after - before);
        }

        int __cdecl hk_cmd_exec_f()
        {
            const int argc = *game::exe_ptr<int>(game::exe::addr_cmd_argc);
            const char* const* argv = game::exe_ptr<const char*>(game::exe::addr_cmd_argv);
            const auto fn_com_printf = game::exe_fn<game::com_printf_t>(game::exe::addr_com_printf);
            if (argc != 2 || argv == nullptr || argv[1] == nullptr)
            {
                fn_com_printf("exec <filename> : execute a script file\n");
                return 0;
            }

            char resolved_name[64]{};
            game::exe_fn<game::q_strncpyz_t>(game::exe::addr_q_strncpyz)(
                resolved_name, argv[1], static_cast<int>(sizeof(resolved_name)));
            game::exe_fn<game::com_default_ext_t>(game::exe::addr_com_default_extension)(
                resolved_name, static_cast<int>(sizeof(resolved_name)), ".cfg");

            const auto fn_fs_read_file = game::exe_fn<game::fs_read_file_t>(game::exe::addr_fs_read_file);
            const auto fn_fs_free_file = game::exe_fn<game::fs_free_file_t>(game::exe::addr_fs_free_file);

            void* target_buf = nullptr;
            fn_fs_read_file(resolved_name, &target_buf);
            if (target_buf == nullptr)
            {
                fn_com_printf("couldn't exec %s\n", argv[1]);
                return 0;
            }

            const int* com_init_ptr = game::exe_ptr<int>(game::exe::addr_com_fully_initialized);
            const bool com_fully_initialized = (com_init_ptr != nullptr && *com_init_ptr != 0);
            const bool do_clean_switch = exec_tracker.should_clean_switch_on_exec(com_fully_initialized);

            const auto* cursize_ptr = game::exe_ptr<int32_t>(game::exe::addr_cmd_text_cursize);
            const auto* maxsize_ptr = game::exe_ptr<int32_t>(game::exe::addr_cmd_text_maxsize);
            const bool has_cbuf_bounds = (cursize_ptr != nullptr && maxsize_ptr != nullptr);
            const size_t target_len = std::strlen(static_cast<const char*>(target_buf));

            void* default_buf = nullptr;
            if (do_clean_switch && !is_default_cfg_name(resolved_name))
            {
                fn_fs_read_file("default.cfg", &default_buf);
                if (default_buf == nullptr)
                {
                    fn_com_printf("couldn't exec default.cfg\n");
                    fn_fs_free_file(target_buf);
                    return 0;
                }
            }

            const size_t def_len = (default_buf != nullptr)
                ? std::strlen(static_cast<const char*>(default_buf))
                : 0u;
            const size_t required_len = target_len + ((default_buf != nullptr) ? (def_len + 1u) : 0u);
            const bool fits = !has_cbuf_bounds || can_insert_into_cmd_text(*cursize_ptr, *maxsize_ptr, required_len);

            fn_com_printf("execing %s\n", argv[1]);

            if (!fits)
            {
                fn_com_printf("Cbuf_InsertText overflowed\n");
                if (default_buf != nullptr)
                {
                    fn_fs_free_file(default_buf);
                }
                fn_fs_free_file(target_buf);
                return 0;
            }

            if (do_clean_switch)
            {
                game::exe_fn<game::void_cmd_t>(game::exe::addr_key_unbindall_f)();
                game::exe_fn<game::void_cmd_t>(game::exe::addr_cmd_unaliasall_f)();
                clean_restart_cvars();

                if (auto* const mod_flags = game::exe_ptr<uint32_t>(game::exe::addr_cvar_modified_flags); mod_flags != nullptr)
                {
                    *mod_flags |= cvar_archive;
                }
            }

            exec_tracker.begin_explicit_exec_insert();
            // Cbuf_InsertText prepends LIFO: push target first so default.cfg executes ahead of it
            insert_cfg_tracked(static_cast<const char*>(target_buf));
            if (default_buf != nullptr)
            {
                insert_cfg_tracked(static_cast<const char*>(default_buf));
                fn_fs_free_file(default_buf);
            }
            exec_tracker.end_explicit_exec_insert();

            fn_fs_free_file(target_buf);
            return 0;
        }

        const std::array<util::inline_hook_spec, 12> cvar_inline_specs{{
            { &hook_cvar_get,         game::exe::addr_cvar_get,         reinterpret_cast<void*>(&hk_cvar_get) },
            { &hook_cvar_get3,        game::exe::addr_cvar_get3,        reinterpret_cast<void*>(&hk_cvar_get3) },
            { &hook_cvar_register4,   game::exe::addr_cvar_register4,   reinterpret_cast<void*>(&hk_cvar_register4) },
            { &hook_cvar_register6,   game::exe::addr_cvar_register6,   reinterpret_cast<void*>(&hk_cvar_register6) },
            { &hook_cvar_update,      game::exe::addr_cvar_update,      reinterpret_cast<void*>(&hk_cvar_update) },
            { &hook_cvar_set2,        game::exe::addr_cvar_set2,        reinterpret_cast<void*>(&hk_cvar_set2) },
            { &hook_cmd_seta,         game::exe::addr_cmd_seta_f,       reinterpret_cast<void*>(&hk_cmd_seta_f) },
            { &hook_cvar_restart,     game::exe::addr_cvar_restart_f,   reinterpret_cast<void*>(&hk_cvar_restart_f) },
            { &hook_cvar_init,        game::exe::addr_cvar_init,        reinterpret_cast<void*>(&hk_cvar_init) },
            { &hook_cbuf_execute,     game::exe::addr_cbuf_execute,     reinterpret_cast<void*>(&hk_cbuf_execute) },
            { &hook_cbuf_insert_text, game::exe::addr_cbuf_insert_text, reinterpret_cast<void*>(&hk_cbuf_insert_text) },
            { &hook_cmd_exec,         game::exe::addr_cmd_exec_f,       reinterpret_cast<void*>(&hk_cmd_exec_f) },
        }};

        const std::array<util::mid_hook_spec, 2> cvar_mid_specs{{
            { &hook_cbuf_before_pop, game::exe::addr_cbuf_execute_before_pop, &on_cbuf_before_pop },
            { &hook_cbuf_after_pop,  game::exe::addr_cbuf_execute_after_pop,  &on_cbuf_after_pop },
        }};
    }

    const runtime_settings& settings()
    {
        return cur_settings;
    }

    void reset_for_rearm()
    {
        cur_settings = runtime_settings{};
        if (cur_sync_phase == sync_phase::live)
        {
            cur_sync_phase = sync_phase::awaiting_initial_sync;
        }
    }

    bool install_hooks()
    {
        if (!try_expand_small_zone_pre_init() || !init_pool_state())
        {
            return false;
        }

        if (!util::install_inline_table(&game::exe_addr, cvar_inline_specs))
        {
            release_hooks();
            return false;
        }
        if (!util::install_mid_table(&game::exe_addr, cvar_mid_specs))
        {
            release_hooks();
            return false;
        }

        return true;
    }

    void release_hooks()
    {
        util::release_hook_table(cvar_mid_specs);
        util::release_hook_table(cvar_inline_specs);

        restore_small_zone_patch_if_unrun();
        exec_tracker.clear();

        // retain overflow memory if any engine handle still references index >= 2048
        const int32_t* num_indexes = game::exe_ptr<int32_t>(game::exe::addr_cvar_num_indexes);
        if (overflow_pool != nullptr && (num_indexes == nullptr || *num_indexes <= native_cvar_slots))
        {
            VirtualFree(overflow_pool, 0, MEM_RELEASE);
            overflow_pool = nullptr;
            cvar_pool.reset(nullptr, nullptr);
        }
    }

    void clear_qlqpc_user_created_flags()
    {
        strip_user_created_flag(cvar_cl_cmdrate);
        for (const auto& spec : bool_cvar_specs)
        {
            strip_user_created_flag(*spec.cvar_slot);
        }
        strip_user_created_flag(cvar_team_lightning_style);
        strip_user_created_flag(cvar_team_lightning_opacity);
        strip_user_created_flag(cvar_enemy_lightning_style);
        strip_user_created_flag(cvar_enemy_lightning_opacity);
        strip_user_created_flag(cvar_fog_color);
        strip_user_created_flag(cvar_fog_distance);
        strip_user_created_flag(cvar_fog_intensity);
    }

    void relax_engine_cvar_bounds()
    {
        auto* const cvar_com_maxfps_slot =
            game::exe_ptr<engine_cvar*>(game::exe::addr_cvar_com_maxfps_ptr);
        if (cvar_com_maxfps_slot != nullptr && *cvar_com_maxfps_slot != nullptr)
        {
            engine_cvar* const cvar = *cvar_com_maxfps_slot;
            cvar_com_maxfps = cvar;
            cvar->min_val = 0.0f;
            cvar->max_val = 10000.0f;
        }

        if (cvar_cl_cmdrate != nullptr)
        {
            auto* const cvar_cmd = reinterpret_cast<engine_cvar*>(cvar_cl_cmdrate);
            cvar_cmd->min_val = 0.0f;
            cvar_cmd->max_val = 250.0f;
        }

        auto** const hash_table = game::exe_ptr<engine_cvar*>(game::exe::addr_cvar_hash_table);
        if (hash_table != nullptr)
        {
            const uint8_t bucket = compute_cvar_hash("cg_playerLean");
            for (engine_cvar* cur = hash_table[bucket]; cur != nullptr; cur = cur->hash_next)
            {
                if (cur->name != nullptr && equals_ignore_case(cur->name, "cg_playerLean"))
                {
                    cur->min_val = 0.0f;
                    cur->max_val = 50.0f;
                    break;
                }
            }
        }
    }

    void register_qlqpc_cvars()
    {
        const auto fn_cvar_get = game::exe_fn<game::cvar_get_t>(game::exe::addr_cvar_get);
        const auto fn_cvar_get3 = game::exe_fn<game::cvar_get3_t>(game::exe::addr_cvar_get3);
        if (fn_cvar_get != nullptr)
        {
            if (cvar_cl_cmdrate == nullptr)
            {
                cvar_cl_cmdrate = fn_cvar_get("cl_cmdrate", "250", "0", "250", cvar_rep_flags);
            }

            for (const auto& spec : bool_cvar_specs)
            {
                if (*spec.cvar_slot == nullptr)
                {
                    *spec.cvar_slot = fn_cvar_get(
                        spec.name,
                        (spec.default_value != 0) ? "1" : "0",
                        "0",
                        "1",
                        spec.reg_flags);
                }
            }

            if (cvar_team_lightning_style == nullptr)
            {
                cvar_team_lightning_style = fn_cvar_get("cg_teamLightningStyle", "1", "0", "5", cvar_rep_flags);
            }
            if (cvar_team_lightning_opacity == nullptr)
            {
                cvar_team_lightning_opacity = fn_cvar_get("cg_teamLightningOpacity", "1.00", "0.00", "1.00", cvar_rep_flags);
            }
            if (cvar_enemy_lightning_style == nullptr)
            {
                cvar_enemy_lightning_style = fn_cvar_get("cg_enemyLightningStyle", "1", "0", "5", cvar_rep_flags);
            }
            if (cvar_enemy_lightning_opacity == nullptr)
            {
                cvar_enemy_lightning_opacity = fn_cvar_get("cg_enemyLightningOpacity", "1.00", "0.00", "1.00", cvar_rep_flags);
            }

            if (cvar_fog_distance == nullptr)
            {
                cvar_fog_distance = fn_cvar_get("r_fogDistance", "768", "10", "10000", cvar_rep_flags);
            }
            if (cvar_fog_intensity == nullptr)
            {
                cvar_fog_intensity = fn_cvar_get("r_fogIntensity", "1.00", "0.05", "10.0", cvar_rep_flags);
            }
        }

        if (fn_cvar_get3 != nullptr && cvar_fog_color == nullptr)
        {
            cvar_fog_color = fn_cvar_get3("r_fogColor", "0xffffffff", cvar_rep_flags);
        }

        clear_qlqpc_user_created_flags();

        if (all_managed_cvars_ready() && cur_sync_phase == sync_phase::awaiting_cvars)
        {
            cur_sync_phase = sync_phase::awaiting_initial_sync;
        }
    }

    void sync_runtime_from_engine_cvars(bool announce_restart_cvars)
    {
        if (cur_sync_phase == sync_phase::awaiting_cvars)
        {
            return;
        }

        clear_qlqpc_user_created_flags();

        const bool force_refresh = !announce_restart_cvars;
        bool fog_changed = false;

        const int raw_hz = game::read_cvar_int(cvar_cl_cmdrate, timing::default_outbound_hz);
        const int cur_hz = timing::clamp_outbound_hz(raw_hz);
        if (cur_hz != raw_hz)
        {
            apply_cvar_int("cl_cmdrate", cur_hz);
        }
        last_cvar_cmdrate = cur_hz;

        cur_settings.team_lightning_style = visuals::clamp_lightning_style(
            game::read_cvar_int(cvar_team_lightning_style, visuals::lightning_style_default));
        cur_settings.enemy_lightning_style = visuals::clamp_lightning_style(
            game::read_cvar_int(cvar_enemy_lightning_style, visuals::lightning_style_default));
        cur_settings.team_lightning_opacity_u8 = visuals::opacity_float_to_u8(
            game::read_cvar_float(cvar_team_lightning_opacity, 1.0f));
        cur_settings.enemy_lightning_opacity_u8 = visuals::opacity_float_to_u8(
            game::read_cvar_float(cvar_enemy_lightning_opacity, 1.0f));

        for (const auto& spec : bool_cvar_specs)
        {
            const int cur_val = (game::read_cvar_int(*spec.cvar_slot, spec.default_value) != 0) ? 1 : 0;
            const bool changed = (cur_val != *spec.last_value);
            if (force_refresh || changed)
            {
                if (spec.runtime_mirror != nullptr)
                {
                    *spec.runtime_mirror = spec.invert_mirror ? (cur_val == 0) : (cur_val != 0);
                }
                if (spec.cvar_slot == &cvar_draw_accuracy)
                {
                    hitreg::set_enabled(cur_val != 0);
                }
                if (spec.affects_fog)
                {
                    fog_changed = true;
                }
                *spec.last_value = cur_val;
            }
        }

        const std::string cur_hex_raw = game::read_cvar_string(cvar_fog_color, last_cvar_fog_color.c_str());
        if (force_refresh || cur_hex_raw != last_cvar_fog_color)
        {
            if (const auto parsed = webui::parse_ql_hex_color(cur_hex_raw))
            {
                fog_r = parsed->r;
                fog_g = parsed->g;
                fog_b = parsed->b;
                const std::string canon = webui::format_ql_hex_color(fog_r, fog_g, fog_b);
                if (cur_hex_raw != canon)
                {
                    apply_cvar_string("r_fogColor", canon.c_str());
                }
                last_cvar_fog_color = canon;
                fog_changed = true;
            }
            else
            {
                const std::string fallback = webui::format_ql_hex_color(fog_r, fog_g, fog_b);
                apply_cvar_string("r_fogColor", fallback.c_str());
                last_cvar_fog_color = fallback;
            }
        }

        const int cur_fog_dist = std::clamp(game::read_cvar_int(cvar_fog_distance, 768), 10, 10000);
        const float raw_fog_int_f = game::read_cvar_float(cvar_fog_intensity, 1.0f);
        const float cur_fog_int_f = std::isfinite(raw_fog_int_f) ? std::clamp(raw_fog_int_f, 0.05f, 10.0f) : 1.0f;
        const int cur_fog_int_pct = static_cast<int>(std::round(cur_fog_int_f * 100.0f));
        if (force_refresh ||
            cur_fog_dist != last_cvar_fog_dist ||
            cur_fog_int_pct != last_cvar_fog_int_pct)
        {
            last_cvar_fog_dist = cur_fog_dist;
            last_cvar_fog_int_pct = cur_fog_int_pct;
            fog_changed = true;
        }

        if (fog_changed || force_refresh)
        {
            visuals::update_fog(
                last_cvar_fog != 0,
                fog_r,
                fog_g,
                fog_b,
                static_cast<float>(last_cvar_fog_dist),
                static_cast<float>(last_cvar_fog_int_pct) / 100.0f,
                last_cvar_fog_ovr != 0);
        }
    }

    void ensure_main_thread_cvars()
    {
        if (cur_sync_phase == sync_phase::awaiting_cvars)
        {
            register_qlqpc_cvars();
            relax_engine_cvar_bounds();
        }
        if (cur_sync_phase == sync_phase::awaiting_initial_sync)
        {
            sync_runtime_from_engine_cvars(false);
            cur_sync_phase = sync_phase::live;
        }
    }

    void sync_controls_on_main_thread()
    {
        if (!should_run_periodic_sync(static_cast<uint32_t>(GetTickCount())))
        {
            return;
        }
        if (cur_sync_phase != sync_phase::live)
        {
            return;
        }

        sync_runtime_from_engine_cvars(true);
    }

    bool is_decoupling_active()
    {
        const int rate = (cvar_cl_cmdrate != nullptr)
            ? game::read_cvar_int(cvar_cl_cmdrate, last_cvar_cmdrate)
            : last_cvar_cmdrate;
        return timing::is_decoupling_enabled(rate);
    }

    int read_cmdrate()
    {
        return (cvar_cl_cmdrate != nullptr)
            ? game::read_cvar_int(cvar_cl_cmdrate, timing::default_outbound_hz)
            : timing::default_outbound_hz;
    }

    int read_maxfps()
    {
        return (cvar_com_maxfps != nullptr)
            ? game::read_cvar_int(cvar_com_maxfps, 0)
            : 0;
    }

    bool read_vulkan_enabled()
    {
        return game::read_cvar_int(cvar_vulkan, 0) != 0;
    }
}
