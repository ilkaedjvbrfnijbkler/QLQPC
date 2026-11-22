#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace qlqpc::util
{
    union xmm_reg
    {
        uint8_t u8[16];
        uint16_t u16[8];
        uint32_t u32[4];
        uint64_t u64[2];
        float f32[4];
        double f64[2];
    };

    // 32-bit x86 register frame constructed by the mid-hook stub; mutating registers, flags, or eip updates restored state
    #pragma pack(push, 4)
    struct mid_context
    {
        xmm_reg xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
        uintptr_t eflags;
        uintptr_t edi, esi, edx, ecx, ebx, eax, ebp;
        uintptr_t esp;
        uintptr_t trampoline_esp;
        uintptr_t eip;
    };
    #pragma pack(pop)
    static_assert(sizeof(mid_context) == 172, "mid_context must remain 172 bytes");
    static_assert(offsetof(mid_context, esp) == 0xA0, "mid_context::esp must be at +0xA0");
    static_assert(offsetof(mid_context, eip) == 0xA8, "mid_context::eip must be at +0xA8");

    using mid_hook_t = void (*)(mid_context& ctx);

    class inline_hook;
    class mid_hook;

    inline_hook create_inline(void* target, void* destination);
    mid_hook create_mid(void* target, mid_hook_t destination);

    class inline_hook
    {
    public:
        inline_hook() = default;
        inline_hook(const inline_hook&) = delete;
        inline_hook& operator=(const inline_hook&) = delete;
        inline_hook(inline_hook&& other) noexcept;
        inline_hook& operator=(inline_hook&& other) noexcept;
        ~inline_hook();

        void reset();

        explicit operator bool() const
        {
            return trampoline != nullptr;
        }

        // must not be noexcept: hooked engine targets can call Com_Error (longjmp) which conflicts with noexcept under /EHsc
        template <typename ret_t = void, typename... args_t>
        ret_t call(args_t... args) const
        {
            return reinterpret_cast<ret_t (*)(args_t...)>(trampoline)(args...);
        }

        template <typename ret_t = void, typename... args_t>
        ret_t thiscall(args_t... args) const
        {
            return reinterpret_cast<ret_t(__thiscall*)(args_t...)>(trampoline)(args...);
        }

        template <typename ret_t = void, typename... args_t>
        ret_t fastcall(args_t... args) const
        {
            return reinterpret_cast<ret_t(__fastcall*)(args_t...)>(trampoline)(args...);
        }

        template <typename ret_t = void, typename... args_t>
        ret_t stdcall(args_t... args) const
        {
            return reinterpret_cast<ret_t(__stdcall*)(args_t...)>(trampoline)(args...);
        }

    private:
        friend class mid_hook;
        friend inline_hook create_inline(void* target, void* destination);
        friend mid_hook create_mid(void* target, mid_hook_t destination);

        static inline_hook create_from_trampoline(
            uint8_t* target_ptr,
            uint8_t* dest_ptr,
            uint8_t* tramp_ptr,
            uint32_t tramp_alloc,
            uint8_t stolen_len);

        uint8_t* target{nullptr};
        uint8_t* destination{nullptr};
        uint8_t* trampoline{nullptr};
        uint32_t trampoline_alloc_size{0};
        uint8_t original_len{0};
        uint8_t original_bytes[24]{};
    };

    class mid_hook
    {
    public:
        mid_hook() = default;
        mid_hook(const mid_hook&) = delete;
        mid_hook& operator=(const mid_hook&) = delete;
        mid_hook(mid_hook&& other) noexcept;
        mid_hook& operator=(mid_hook&& other) noexcept;
        ~mid_hook();

        void reset();

        explicit operator bool() const
        {
            return stub != nullptr && static_cast<bool>(hook);
        }

    private:
        friend mid_hook create_mid(void* target, mid_hook_t destination);

        inline_hook hook{};
        uint8_t* stub{nullptr};
        uint32_t stub_alloc_size{0};
        mid_hook_t destination{nullptr};
    };

    template <typename T, typename U>
    inline inline_hook create_inline(T target, U destination)
    {
        return create_inline(reinterpret_cast<void*>(target), reinterpret_cast<void*>(destination));
    }

    template <typename T>
    inline mid_hook create_mid(T target, mid_hook_t destination)
    {
        return create_mid(reinterpret_cast<void*>(target), destination);
    }

    // skip unpatching and VirtualFree under OS loader lock when ExitProcess tears down statics
    void set_process_terminating_hooks();

    bool patch_bytes(uint8_t* dst, const void* src, size_t len);

    inline bool patch_u32(uint8_t* dst, uint32_t val)
    {
        return patch_bytes(dst, &val, sizeof(val));
    }

    struct inline_hook_spec
    {
        inline_hook* slot;
        uintptr_t preferred_va;
        void* detour;
    };

    struct mid_hook_spec
    {
        mid_hook* slot;
        uintptr_t preferred_va;
        mid_hook_t detour;
    };

    template <size_t N>
    inline void release_hook_table(const std::array<inline_hook_spec, N>& specs)
    {
        for (size_t i = specs.size(); i > 0; --i)
        {
            specs[i - 1].slot->reset();
        }
    }

    template <size_t N>
    inline void release_hook_table(const std::array<mid_hook_spec, N>& specs)
    {
        for (size_t i = specs.size(); i > 0; --i)
        {
            specs[i - 1].slot->reset();
        }
    }

    template <typename resolver_t, size_t N>
    inline bool install_inline_table(
        resolver_t&& resolve_va,
        const std::array<inline_hook_spec, N>& specs)
    {
        for (size_t i = 0; i < specs.size(); ++i)
        {
            auto hook = create_inline(
                reinterpret_cast<void*>(resolve_va(specs[i].preferred_va)),
                specs[i].detour);
            if (!hook)
            {
                for (size_t j = i; j > 0; --j)
                {
                    specs[j - 1].slot->reset();
                }
                return false;
            }
            *specs[i].slot = std::move(hook);
        }
        return true;
    }

    template <typename resolver_t, size_t N>
    inline bool install_mid_table(
        resolver_t&& resolve_va,
        const std::array<mid_hook_spec, N>& specs)
    {
        for (size_t i = 0; i < specs.size(); ++i)
        {
            auto hook = create_mid(
                reinterpret_cast<void*>(resolve_va(specs[i].preferred_va)),
                specs[i].detour);
            if (!hook)
            {
                for (size_t j = i; j > 0; --j)
                {
                    specs[j - 1].slot->reset();
                }
                return false;
            }
            *specs[i].slot = std::move(hook);
        }
        return true;
    }

    enum class rel_kind : uint8_t
    {
        none = 0,
        rel32,
        jcc32,
        jmp8,
        jcc8,
        unsupported_rel
    };

    struct decoded_insn
    {
        uint8_t length{0};
        rel_kind kind{rel_kind::none};
        uint8_t cc{0};
    };

    bool decode_x86_32_insn(const uint8_t* ip, size_t max_bytes, decoded_insn& out);
}
