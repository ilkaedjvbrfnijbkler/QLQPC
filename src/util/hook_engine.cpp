#include "util/hook_engine.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <intrin.h>
#include <utility>

namespace qlqpc::util
{
    static_assert(sizeof(void*) == 4, "hook_engine is specialized for 32-bit x86");

    namespace
    {
        std::atomic<bool> hooks_process_terminating{false};

        constexpr uint32_t align_up_16(size_t n)
        {
            return static_cast<uint32_t>((n + 15u) & ~size_t{15u});
        }

        // 16-byte aligned executable slab allocator to keep trampolines off the CRT heap
        class exec_allocator
        {
        public:
            ~exec_allocator()
            {
                if (hooks_process_terminating.load(std::memory_order_acquire))
                {
                    return;
                }
                AcquireSRWLockExclusive(&lock);
                for (size_t s = 0; s < slab_count; ++s)
                {
                    if (slabs[s].base != nullptr)
                    {
                        VirtualFree(slabs[s].base, 0, MEM_RELEASE);
                        slabs[s].base = nullptr;
                    }
                }
                slab_count = 0;
                ReleaseSRWLockExclusive(&lock);
            }

            uint8_t* allocate(uint32_t raw_size, uint32_t& out_alloc_size)
            {
                const uint32_t size = align_up_16(raw_size);
                out_alloc_size = 0;
                if (size == 0 || size > slab_bytes)
                {
                    return nullptr;
                }

                AcquireSRWLockExclusive(&lock);
                uint8_t* result = allocate_locked(size);
                ReleaseSRWLockExclusive(&lock);

                if (result != nullptr)
                {
                    std::memset(result, 0xCC, size);
                    out_alloc_size = size;
                }
                return result;
            }

            void free(uint8_t* ptr, uint32_t alloc_size)
            {
                if (ptr == nullptr || alloc_size == 0)
                {
                    return;
                }

                AcquireSRWLockExclusive(&lock);
                free_locked(ptr, alloc_size);
                ReleaseSRWLockExclusive(&lock);
            }

        private:
            static constexpr uint32_t slab_bytes = 64u * 1024u;
            static constexpr size_t max_slabs = 16u;
            static constexpr size_t max_ranges = 256u;

            struct free_range
            {
                uint32_t offset{0};
                uint32_t length{0};
            };

            struct slab
            {
                uint8_t* base{nullptr};
                uint16_t range_count{0};
                uint32_t alloc_count{0};
                free_range ranges[max_ranges]{};
            };

            uint8_t* allocate_locked(uint32_t size)
            {
                for (size_t s = 0; s < slab_count; ++s)
                {
                    slab& cur = slabs[s];
                    for (uint16_t i = 0; i < cur.range_count; ++i)
                    {
                        if (cur.ranges[i].length >= size)
                        {
                            uint8_t* const addr = cur.base + cur.ranges[i].offset;
                            cur.ranges[i].offset += size;
                            cur.ranges[i].length -= size;
                            if (cur.ranges[i].length == 0)
                            {
                                for (uint16_t j = i; j + 1 < cur.range_count; ++j)
                                {
                                    cur.ranges[j] = cur.ranges[j + 1];
                                }
                                --cur.range_count;
                            }
                            ++cur.alloc_count;
                            return addr;
                        }
                    }
                }

                if (slab_count >= max_slabs)
                {
                    return nullptr;
                }

                auto* const mem = static_cast<uint8_t*>(
                    VirtualAlloc(nullptr, slab_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
                if (mem == nullptr)
                {
                    return nullptr;
                }
                std::memset(mem, 0xCC, slab_bytes);

                slab& cur = slabs[slab_count++];
                cur.base = mem;
                cur.alloc_count = 1;
                if (size < slab_bytes)
                {
                    cur.range_count = 1;
                    cur.ranges[0].offset = size;
                    cur.ranges[0].length = slab_bytes - size;
                }
                else
                {
                    cur.range_count = 0;
                }
                return mem;
            }

            void free_locked(uint8_t* ptr, uint32_t size)
            {
                for (size_t s = 0; s < slab_count; ++s)
                {
                    slab& cur = slabs[s];
                    if (ptr < cur.base || ptr >= cur.base + slab_bytes)
                    {
                        continue;
                    }
                    const auto offset = static_cast<uint32_t>(ptr - cur.base);
                    if (size > slab_bytes - offset)
                    {
                        return;
                    }
                    std::memset(ptr, 0xCC, size);

                    if (cur.alloc_count > 0)
                    {
                        --cur.alloc_count;
                    }
                    if (cur.alloc_count == 0)
                    {
                        VirtualFree(cur.base, 0, MEM_RELEASE);
                        for (size_t k = s; k + 1 < slab_count; ++k)
                        {
                            slabs[k] = slabs[k + 1];
                        }
                        slabs[--slab_count] = slab{};
                        return;
                    }

                    uint16_t insert_idx = 0;
                    while (insert_idx < cur.range_count && cur.ranges[insert_idx].offset < offset)
                    {
                        ++insert_idx;
                    }

                    const bool merge_left =
                        (insert_idx > 0 && cur.ranges[insert_idx - 1].offset + cur.ranges[insert_idx - 1].length == offset);
                    const bool merge_right =
                        (insert_idx < cur.range_count && offset + size == cur.ranges[insert_idx].offset);

                    if (merge_left && merge_right)
                    {
                        cur.ranges[insert_idx - 1].length += size + cur.ranges[insert_idx].length;
                        for (uint16_t j = insert_idx; j + 1 < cur.range_count; ++j)
                        {
                            cur.ranges[j] = cur.ranges[j + 1];
                        }
                        --cur.range_count;
                    }
                    else if (merge_left)
                    {
                        cur.ranges[insert_idx - 1].length += size;
                    }
                    else if (merge_right)
                    {
                        cur.ranges[insert_idx].offset = offset;
                        cur.ranges[insert_idx].length += size;
                    }
                    else if (cur.range_count < max_ranges)
                    {
                        for (uint16_t j = cur.range_count; j > insert_idx; --j)
                        {
                            cur.ranges[j] = cur.ranges[j - 1];
                        }
                        cur.ranges[insert_idx] = free_range{offset, size};
                        ++cur.range_count;
                    }
                    return;
                }
            }

            SRWLOCK lock = SRWLOCK_INIT;
            size_t slab_count{0};
            slab slabs[max_slabs]{};
        };

        exec_allocator slab_allocator{};

        void store_i32(uint8_t* dst, int32_t val)
        {
            std::memcpy(dst, &val, sizeof(int32_t));
        }

        void store_u32(uint8_t* dst, uint32_t val)
        {
            std::memcpy(dst, &val, sizeof(uint32_t));
        }

        int32_t load_i32(const uint8_t* src)
        {
            int32_t val = 0;
            std::memcpy(&val, src, sizeof(int32_t));
            return val;
        }

        void emit_jmp_e9(uint8_t* write_buf, const uint8_t* src_va, const uint8_t* dst_va)
        {
            write_buf[0] = 0xE9u;
            const auto rel = static_cast<int32_t>(dst_va - (src_va + 5));
            store_i32(write_buf + 1, rel);
        }

        // updates at least 8 bytes at target via 64-bit CAS so E9 jumps apply and revert atomically
        bool patch_code_atomic(uint8_t* target, const uint8_t* new_bytes, size_t len, bool installing)
        {
            if (target == nullptr || new_bytes == nullptr || len < 5)
            {
                return false;
            }

            const uintptr_t addr = reinterpret_cast<uintptr_t>(target);
            const uintptr_t page_end = (addr + 0x1000u) & ~uintptr_t{0xFFFu};
            const size_t cas_len = std::max<size_t>(len, 8u);
            const size_t page1_len = std::min<size_t>(cas_len, static_cast<size_t>(page_end - addr));
            const size_t page2_len = cas_len - page1_len;

            DWORD old_prot1 = 0;
            if (VirtualProtect(target, page1_len, PAGE_EXECUTE_READWRITE, &old_prot1) == FALSE)
            {
                return false;
            }

            DWORD old_prot2 = 0;
            if (page2_len > 0 &&
                VirtualProtect(reinterpret_cast<void*>(page_end), page2_len, PAGE_EXECUTE_READWRITE, &old_prot2) == FALSE)
            {
                DWORD dummy1 = 0;
                VirtualProtect(target, page1_len, old_prot1, &dummy1);
                return false;
            }

            if (!installing && len > 8)
            {
                std::memcpy(target + 8, new_bytes + 8, len - 8);
            }

            auto* const p64 = reinterpret_cast<volatile long long*>(target);
            long long expected = _InterlockedCompareExchange64(p64, 0, 0);
            for (;;)
            {
                uint8_t merged[8]{};
                std::memcpy(merged, &expected, 8);
                std::memcpy(merged, new_bytes, std::min<size_t>(len, 8u));
                long long desired = 0;
                std::memcpy(&desired, merged, 8);

                const long long prev = _InterlockedCompareExchange64(p64, desired, expected);
                if (prev == expected)
                {
                    break;
                }
                expected = prev;
            }

            if (installing && len > 8)
            {
                std::memcpy(target + 8, new_bytes + 8, len - 8);
            }

            DWORD dummy = 0;
            if (page2_len > 0)
            {
                VirtualProtect(reinterpret_cast<void*>(page_end), page2_len, old_prot2, &dummy);
            }
            VirtualProtect(target, page1_len, old_prot1, &dummy);
            FlushInstructionCache(GetCurrentProcess(), target, len);
            return true;
        }

        // 32-bit mid-hook stub (152 bytes): saves XMM0-7, flags, GPRs, invokes handler(&ctx), restores and jumps to ctx.eip
        constexpr std::array<uint8_t, 152> mid_hook_stub = {
            0x68, 0x00, 0x00, 0x00, 0x00,
            0x54, 0x54, 0x55, 0x50, 0x53, 0x51, 0x52, 0x56, 0x57, 0x9C,
            0x81, 0xEC, 0x80, 0x00, 0x00, 0x00,
            0xF3, 0x0F, 0x7F, 0x7C, 0x24, 0x70,
            0xF3, 0x0F, 0x7F, 0x74, 0x24, 0x60,
            0xF3, 0x0F, 0x7F, 0x6C, 0x24, 0x50,
            0xF3, 0x0F, 0x7F, 0x64, 0x24, 0x40,
            0xF3, 0x0F, 0x7F, 0x5C, 0x24, 0x30,
            0xF3, 0x0F, 0x7F, 0x54, 0x24, 0x20,
            0xF3, 0x0F, 0x7F, 0x4C, 0x24, 0x10,
            0xF3, 0x0F, 0x7F, 0x04, 0x24,
            0x83, 0x84, 0x24, 0xA0, 0x00, 0x00, 0x00, 0x08,
            0x54,
            0xE8, 0x00, 0x00, 0x00, 0x00,
            0x83, 0xC4, 0x04,
            0xF3, 0x0F, 0x6F, 0x04, 0x24,
            0xF3, 0x0F, 0x6F, 0x4C, 0x24, 0x10,
            0xF3, 0x0F, 0x6F, 0x54, 0x24, 0x20,
            0xF3, 0x0F, 0x6F, 0x5C, 0x24, 0x30,
            0xF3, 0x0F, 0x6F, 0x64, 0x24, 0x40,
            0xF3, 0x0F, 0x6F, 0x6C, 0x24, 0x50,
            0xF3, 0x0F, 0x6F, 0x74, 0x24, 0x60,
            0xF3, 0x0F, 0x6F, 0x7C, 0x24, 0x70,
            0x81, 0xC4, 0x80, 0x00, 0x00, 0x00,
            0x9D, 0x5F, 0x5E, 0x5A, 0x59, 0x5B, 0x58, 0x5D,
            0x8D, 0x64, 0x24, 0x04,
            0x5C, 0xC3
        };

        constexpr size_t stub_imm_tramp_offset = 0x01;
        constexpr size_t stub_rel_call_offset = 0x4E;
        constexpr size_t stub_call_next_ip = 0x52;

        struct stolen_insn
        {
            decoded_insn decoded{};
            uint8_t src_off{0};
            uint8_t tramp_off{0};
            uint8_t tramp_len{0};
        };

        uint8_t* build_relocated_trampoline(
            uint8_t* target,
            uint8_t& out_stolen_len,
            uint32_t& out_alloc_size)
        {
            out_stolen_len = 0;
            out_alloc_size = 0;

            stolen_insn insns[8]{};
            size_t insn_count = 0;
            uint8_t src_off = 0;
            uint8_t tramp_off = 0;

            while (src_off < 5u)
            {
                if (insn_count >= (sizeof(insns) / sizeof(insns[0])))
                {
                    return nullptr;
                }
                decoded_insn dec{};
                if (!decode_x86_32_insn(target + src_off, 15u, dec) || dec.length == 0)
                {
                    return nullptr;
                }
                if (src_off + dec.length > 24u)
                {
                    return nullptr;
                }

                uint8_t t_len = dec.length;
                if (dec.kind == rel_kind::jmp8)
                {
                    t_len = 5u;
                }
                else if (dec.kind == rel_kind::jcc8)
                {
                    t_len = 6u;
                }

                insns[insn_count] = stolen_insn{dec, src_off, tramp_off, t_len};
                ++insn_count;
                src_off = static_cast<uint8_t>(src_off + dec.length);
                tramp_off = static_cast<uint8_t>(tramp_off + t_len);
            }

            const uint8_t stolen_len = src_off;
            const uint32_t total_tramp_bytes = static_cast<uint32_t>(tramp_off) + 5u;

            uint32_t alloc_size = 0;
            uint8_t* const tramp = slab_allocator.allocate(total_tramp_bytes, alloc_size);
            if (tramp == nullptr)
            {
                return nullptr;
            }

            auto resolve_branch_target = [&](const uint8_t* orig_branch_dst, const uint8_t*& out_resolved) -> bool
            {
                if (orig_branch_dst >= target && orig_branch_dst < target + stolen_len)
                {
                    const auto target_off = static_cast<uint8_t>(orig_branch_dst - target);
                    for (size_t k = 0; k < insn_count; ++k)
                    {
                        if (insns[k].src_off == target_off)
                        {
                            out_resolved = tramp + insns[k].tramp_off;
                            return true;
                        }
                    }
                    return false;
                }
                out_resolved = orig_branch_dst;
                return true;
            };

            for (size_t idx = 0; idx < insn_count; ++idx)
            {
                const stolen_insn& si = insns[idx];
                const uint8_t* src_ip = target + si.src_off;
                uint8_t* const dst_ip = tramp + si.tramp_off;

                switch (si.decoded.kind)
                {
                case rel_kind::none:
                    std::memcpy(dst_ip, src_ip, si.decoded.length);
                    break;

                case rel_kind::rel32:
                case rel_kind::jcc32:
                {
                    std::memcpy(dst_ip, src_ip, si.decoded.length);
                    const int32_t orig_disp = load_i32(src_ip + si.decoded.length - 4u);
                    const uint8_t* orig_dst = src_ip + si.decoded.length + orig_disp;
                    const uint8_t* resolved_dst = nullptr;
                    if (!resolve_branch_target(orig_dst, resolved_dst))
                    {
                        slab_allocator.free(tramp, alloc_size);
                        return nullptr;
                    }
                    const auto new_disp = static_cast<int32_t>(resolved_dst - (dst_ip + si.tramp_len));
                    store_i32(dst_ip + si.tramp_len - 4u, new_disp);
                    break;
                }

                case rel_kind::jmp8:
                {
                    const int8_t orig_disp = static_cast<int8_t>(src_ip[si.decoded.length - 1u]);
                    const uint8_t* orig_dst = src_ip + si.decoded.length + orig_disp;
                    const uint8_t* resolved_dst = nullptr;
                    if (!resolve_branch_target(orig_dst, resolved_dst))
                    {
                        slab_allocator.free(tramp, alloc_size);
                        return nullptr;
                    }
                    emit_jmp_e9(dst_ip, dst_ip, resolved_dst);
                    break;
                }

                case rel_kind::jcc8:
                {
                    const int8_t orig_disp = static_cast<int8_t>(src_ip[si.decoded.length - 1u]);
                    const uint8_t* orig_dst = src_ip + si.decoded.length + orig_disp;
                    const uint8_t* resolved_dst = nullptr;
                    if (!resolve_branch_target(orig_dst, resolved_dst))
                    {
                        slab_allocator.free(tramp, alloc_size);
                        return nullptr;
                    }
                    dst_ip[0] = 0x0Fu;
                    dst_ip[1] = static_cast<uint8_t>(0x80u + si.decoded.cc);
                    const auto new_disp = static_cast<int32_t>(resolved_dst - (dst_ip + 6u));
                    store_i32(dst_ip + 2u, new_disp);
                    break;
                }

                case rel_kind::unsupported_rel:
                {
                    const int8_t orig_disp = static_cast<int8_t>(src_ip[si.decoded.length - 1u]);
                    const uint8_t* orig_dst = src_ip + si.decoded.length + orig_disp;
                    if (orig_dst < target || orig_dst >= target + stolen_len || tramp_off != stolen_len)
                    {
                        slab_allocator.free(tramp, alloc_size);
                        return nullptr;
                    }
                    std::memcpy(dst_ip, src_ip, si.decoded.length);
                    break;
                }
                }
            }

            emit_jmp_e9(tramp + tramp_off, tramp + tramp_off, target + stolen_len);
            FlushInstructionCache(GetCurrentProcess(), tramp, total_tramp_bytes);

            out_stolen_len = stolen_len;
            out_alloc_size = alloc_size;
            return tramp;
        }
    }

    bool decode_x86_32_insn(const uint8_t* ip, size_t max_bytes, decoded_insn& out)
    {
        out = {};
        if (ip == nullptr || max_bytes == 0)
        {
            return false;
        }

        size_t i = 0;
        bool op_size_16 = false;
        bool addr_size_16 = false;

        while (i < max_bytes)
        {
            const uint8_t b = ip[i];
            if (b == 0x26u || b == 0x2Eu || b == 0x36u || b == 0x3Eu ||
                b == 0x64u || b == 0x65u || b == 0xF0u || b == 0xF2u || b == 0xF3u)
            {
                ++i;
            }
            else if (b == 0x66u)
            {
                op_size_16 = true;
                ++i;
            }
            else if (b == 0x67u)
            {
                addr_size_16 = true;
                ++i;
            }
            else
            {
                break;
            }
        }

        if (addr_size_16 || i >= max_bytes)
        {
            return false;
        }

        const uint8_t op = ip[i++];
        bool has_modrm = false;
        size_t imm_size = 0;
        const size_t imm_word = op_size_16 ? 2u : 4u;

        if (op == 0x0Fu)
        {
            if (i >= max_bytes)
            {
                return false;
            }
            const uint8_t op2 = ip[i++];
            if (op2 >= 0x80u && op2 <= 0x8Fu)
            {
                if (op_size_16)
                {
                    return false;
                }
                out.kind = rel_kind::jcc32;
                out.cc = static_cast<uint8_t>(op2 & 0x0Fu);
                imm_size = 4u;
            }
            else if (op2 == 0x05u || op2 == 0x06u || op2 == 0x07u || op2 == 0x08u || op2 == 0x09u ||
                     op2 == 0x0Bu || op2 == 0x0Eu || (op2 >= 0x30u && op2 <= 0x35u) || op2 == 0x37u ||
                     op2 == 0x77u || op2 == 0xA0u || op2 == 0xA1u || op2 == 0xA2u ||
                     op2 == 0xA8u || op2 == 0xA9u || op2 == 0xAAu || (op2 >= 0xC8u && op2 <= 0xCFu))
            {
            }
            else if (op2 == 0x38u)
            {
                if (i >= max_bytes)
                {
                    return false;
                }
                ++i;
                has_modrm = true;
            }
            else if (op2 == 0x3Au)
            {
                if (i >= max_bytes)
                {
                    return false;
                }
                ++i;
                has_modrm = true;
                imm_size = 1u;
            }
            else if (op2 == 0x70u || op2 == 0x71u || op2 == 0x72u || op2 == 0x73u ||
                     op2 == 0xA4u || op2 == 0xACu || op2 == 0xBAu || op2 == 0xC2u ||
                     op2 == 0xC4u || op2 == 0xC5u || op2 == 0xC6u)
            {
                has_modrm = true;
                imm_size = 1u;
            }
            else
            {
                has_modrm = true;
            }
        }
        else
        {
            if ((op >= 0x40u && op <= 0x5Fu) || (op >= 0x90u && op <= 0x99u) ||
                op == 0x06u || op == 0x07u || op == 0x0Eu || op == 0x16u || op == 0x17u ||
                op == 0x1Eu || op == 0x1Fu || op == 0x27u || op == 0x2Fu || op == 0x37u ||
                op == 0x3Fu || op == 0x60u || op == 0x61u || (op >= 0x6Cu && op <= 0x6Fu) ||
                (op >= 0x9Bu && op <= 0x9Fu) || (op >= 0xA4u && op <= 0xA7u) ||
                (op >= 0xAAu && op <= 0xAFu) || op == 0xC3u || op == 0xC9u || op == 0xCBu ||
                op == 0xCCu || op == 0xCEu || op == 0xCFu || op == 0xD6u || op == 0xD7u ||
                (op >= 0xECu && op <= 0xEFu) || op == 0xF1u || op == 0xF4u || op == 0xF5u ||
                (op >= 0xF8u && op <= 0xFDu))
            {
            }
            else if ((op >= 0xB0u && op <= 0xB7u) ||
                     op == 0x04u || op == 0x0Cu || op == 0x14u || op == 0x1Cu ||
                     op == 0x24u || op == 0x2Cu || op == 0x34u || op == 0x3Cu ||
                     op == 0x6Au || op == 0xA8u || op == 0xCDu || op == 0xD4u ||
                     op == 0xD5u || (op >= 0xE4u && op <= 0xE7u))
            {
                imm_size = 1u;
            }
            else if ((op >= 0xB8u && op <= 0xBFu) ||
                     op == 0x05u || op == 0x0Du || op == 0x15u || op == 0x1Du ||
                     op == 0x25u || op == 0x2Du || op == 0x35u || op == 0x3Du ||
                     op == 0x68u || op == 0xA9u)
            {
                imm_size = imm_word;
            }
            else if (op >= 0xA0u && op <= 0xA3u)
            {
                imm_size = 4u;
            }
            else if (op == 0xC2u || op == 0xCAu)
            {
                imm_size = 2u;
            }
            else if (op == 0xC8u)
            {
                imm_size = 3u;
            }
            else if (op >= 0x70u && op <= 0x7Fu)
            {
                out.kind = rel_kind::jcc8;
                out.cc = static_cast<uint8_t>(op & 0x0Fu);
                imm_size = 1u;
            }
            else if (op == 0xEBu)
            {
                out.kind = rel_kind::jmp8;
                imm_size = 1u;
            }
            else if (op == 0xE8u || op == 0xE9u)
            {
                if (op_size_16)
                {
                    return false;
                }
                out.kind = rel_kind::rel32;
                imm_size = 4u;
            }
            else if (op >= 0xE0u && op <= 0xE3u)
            {
                out.kind = rel_kind::unsupported_rel;
                imm_size = 1u;
            }
            else if (op == 0x9Au || op == 0xEAu)
            {
                imm_size = 2u + imm_word;
            }
            else if ((op & 0xC4u) == 0x00u || op == 0x62u || op == 0x63u ||
                     (op >= 0x84u && op <= 0x8Fu) || op == 0xC4u || op == 0xC5u ||
                     (op >= 0xD0u && op <= 0xDFu) || op == 0xFEu || op == 0xFFu)
            {
                has_modrm = true;
            }
            else if (op == 0x6Bu || op == 0x80u || op == 0x82u || op == 0x83u ||
                     op == 0xC0u || op == 0xC1u || op == 0xC6u)
            {
                has_modrm = true;
                imm_size = 1u;
            }
            else if (op == 0x69u || op == 0x81u || op == 0xC7u)
            {
                has_modrm = true;
                imm_size = imm_word;
            }
            else if (op == 0xF6u)
            {
                has_modrm = true;
                if (i < max_bytes && ((ip[i] >> 3) & 7u) <= 1u)
                {
                    imm_size = 1u;
                }
            }
            else if (op == 0xF7u)
            {
                has_modrm = true;
                if (i < max_bytes && ((ip[i] >> 3) & 7u) <= 1u)
                {
                    imm_size = imm_word;
                }
            }
            else
            {
                return false;
            }
        }

        if (has_modrm)
        {
            if (i >= max_bytes)
            {
                return false;
            }
            const uint8_t modrm = ip[i++];
            const uint8_t mod = (modrm >> 6) & 3u;
            const uint8_t rm = modrm & 7u;
            if (mod != 3u)
            {
                uint8_t sib_base = 0xFFu;
                if (rm == 4u)
                {
                    if (i >= max_bytes)
                    {
                        return false;
                    }
                    sib_base = ip[i++] & 7u;
                }
                if (mod == 0u)
                {
                    if (rm == 5u || (rm == 4u && sib_base == 5u))
                    {
                        i += 4u;
                    }
                }
                else if (mod == 1u)
                {
                    i += 1u;
                }
                else if (mod == 2u)
                {
                    i += 4u;
                }
            }
        }

        i += imm_size;
        if (i > max_bytes || i > 15u)
        {
            return false;
        }

        out.length = static_cast<uint8_t>(i);
        return true;
    }

    inline_hook::inline_hook(inline_hook&& other) noexcept
    {
        *this = std::move(other);
    }

    inline_hook& inline_hook::operator=(inline_hook&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            target = std::exchange(other.target, nullptr);
            destination = std::exchange(other.destination, nullptr);
            trampoline = std::exchange(other.trampoline, nullptr);
            trampoline_alloc_size = std::exchange(other.trampoline_alloc_size, uint32_t{0});
            original_len = std::exchange(other.original_len, uint8_t{0});
            std::memcpy(original_bytes, other.original_bytes, sizeof(original_bytes));
        }
        return *this;
    }

    inline_hook::~inline_hook()
    {
        reset();
    }

    void inline_hook::reset()
    {
        if (trampoline == nullptr)
        {
            return;
        }

        if (!hooks_process_terminating.load(std::memory_order_acquire))
        {
            bool restored = true;
            if (target != nullptr && original_len >= 5)
            {
                restored = patch_code_atomic(target, original_bytes, original_len, false);
            }
            if (restored)
            {
                slab_allocator.free(trampoline, trampoline_alloc_size);
            }
        }
        target = nullptr;
        destination = nullptr;
        trampoline = nullptr;
        trampoline_alloc_size = 0;
        original_len = 0;
    }

    inline_hook inline_hook::create_from_trampoline(
        uint8_t* target_ptr,
        uint8_t* dest_ptr,
        uint8_t* tramp_ptr,
        uint32_t tramp_alloc,
        uint8_t stolen_len)
    {
        inline_hook hook{};
        if (target_ptr == nullptr || dest_ptr == nullptr || tramp_ptr == nullptr || stolen_len < 5u)
        {
            return hook;
        }

        std::memcpy(hook.original_bytes, target_ptr, stolen_len);
        hook.target = target_ptr;
        hook.destination = dest_ptr;
        hook.trampoline = tramp_ptr;
        hook.trampoline_alloc_size = tramp_alloc;
        hook.original_len = stolen_len;

        uint8_t patch[24]{};
        std::memset(patch, 0x90, stolen_len);
        emit_jmp_e9(patch, target_ptr, dest_ptr);

        if (!patch_code_atomic(target_ptr, patch, stolen_len, true))
        {
            hook.target = nullptr;
            hook.destination = nullptr;
            hook.trampoline = nullptr;
            hook.trampoline_alloc_size = 0;
            hook.original_len = 0;
        }

        return hook;
    }

    inline_hook create_inline(void* target_ptr, void* destination_ptr)
    {
        if (target_ptr == nullptr || destination_ptr == nullptr)
        {
            return {};
        }

        auto* const target = static_cast<uint8_t*>(target_ptr);
        auto* const dest = static_cast<uint8_t*>(destination_ptr);

        uint8_t stolen_len = 0;
        uint32_t alloc_size = 0;
        uint8_t* const tramp = build_relocated_trampoline(target, stolen_len, alloc_size);
        if (tramp == nullptr)
        {
            return {};
        }

        inline_hook hook = inline_hook::create_from_trampoline(target, dest, tramp, alloc_size, stolen_len);
        if (!hook)
        {
            slab_allocator.free(tramp, alloc_size);
        }
        return hook;
    }

    mid_hook::mid_hook(mid_hook&& other) noexcept
    {
        *this = std::move(other);
    }

    mid_hook& mid_hook::operator=(mid_hook&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            hook = std::move(other.hook);
            stub = std::exchange(other.stub, nullptr);
            stub_alloc_size = std::exchange(other.stub_alloc_size, uint32_t{0});
            destination = std::exchange(other.destination, nullptr);
        }
        return *this;
    }

    mid_hook::~mid_hook()
    {
        reset();
    }

    void mid_hook::reset()
    {
        hook.reset();
        if (stub != nullptr)
        {
            if (!hooks_process_terminating.load(std::memory_order_acquire))
            {
                slab_allocator.free(stub, stub_alloc_size);
            }
            stub = nullptr;
            stub_alloc_size = 0;
        }
        destination = nullptr;
    }

    mid_hook create_mid(void* target_ptr, mid_hook_t destination_fn)
    {
        mid_hook mid{};
        if (target_ptr == nullptr || destination_fn == nullptr)
        {
            return mid;
        }

        auto* const target = static_cast<uint8_t*>(target_ptr);

        uint32_t stub_alloc = 0;
        uint8_t* const stub = slab_allocator.allocate(static_cast<uint32_t>(mid_hook_stub.size()), stub_alloc);
        if (stub == nullptr)
        {
            return mid;
        }

        uint8_t stolen_len = 0;
        uint32_t tramp_alloc = 0;
        uint8_t* const tramp = build_relocated_trampoline(target, stolen_len, tramp_alloc);
        if (tramp == nullptr)
        {
            slab_allocator.free(stub, stub_alloc);
            return mid;
        }

        // fill stub fields before patching target so threads cannot jump into half-written stub
        std::memcpy(stub, mid_hook_stub.data(), mid_hook_stub.size());
        store_u32(stub + stub_imm_tramp_offset, reinterpret_cast<uint32_t>(tramp));
        const auto dest_addr = reinterpret_cast<const uint8_t*>(destination_fn);
        const auto call_rel = static_cast<int32_t>(dest_addr - (stub + stub_call_next_ip));
        store_i32(stub + stub_rel_call_offset, call_rel);
        FlushInstructionCache(GetCurrentProcess(), stub, mid_hook_stub.size());

        inline_hook attached = inline_hook::create_from_trampoline(target, stub, tramp, tramp_alloc, stolen_len);
        if (!attached)
        {
            slab_allocator.free(tramp, tramp_alloc);
            slab_allocator.free(stub, stub_alloc);
            return mid;
        }

        mid.hook = std::move(attached);
        mid.stub = stub;
        mid.stub_alloc_size = stub_alloc;
        mid.destination = destination_fn;
        return mid;
    }

    void set_process_terminating_hooks()
    {
        hooks_process_terminating.store(true, std::memory_order_release);
    }

    bool patch_bytes(uint8_t* dst, const void* src, size_t len)
    {
        if (dst == nullptr || src == nullptr || len == 0)
        {
            return false;
        }
        DWORD old_prot = 0;
        if (VirtualProtect(dst, len, PAGE_EXECUTE_READWRITE, &old_prot) == FALSE)
        {
            return false;
        }
        std::memcpy(dst, src, len);
        DWORD dummy = 0;
        VirtualProtect(dst, len, old_prot, &dummy);
        FlushInstructionCache(GetCurrentProcess(), dst, len);
        return true;
    }
}
