#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <vector>

// runtime spir-v 1.0 shader generator for fixed-function and post-processing pipelines

namespace qlqpc::vulkan
{
    // fits within vulkan 1.0's minimum guaranteed 128-byte push constant limit
    struct alignas(16) shader_push_constants
    {
        float mvp[16];
        float clip_plane[4];
        float uv_scale[4];
        float post_params[4];
        float mode_flags[4];
    };
    static_assert(sizeof(shader_push_constants) == 128, "shader_push_constants must be 128 bytes");

    enum class frag_combiner : uint8_t
    {
        single_modulate = 0,
        single_replace = 1,
        untextured = 2,
        multi_modulate = 3,
        multi_replace = 4,
        multi_decal = 5,
        multi_add = 6,
    };

    enum class alpha_test_mode : uint8_t
    {
        none = 0,
        gt0 = 1,
        lt05 = 2,
        ge05 = 3,
    };

    enum class post_process_kind : uint8_t
    {
        none = 0,
        color_correct = 1,
        bright_pass = 2,
        downsample = 3,
        blur = 4,
        combine = 5,
        gamma_present = 6,
    };

    struct frag_shader_desc
    {
        post_process_kind post{post_process_kind::none};
        frag_combiner combiner{frag_combiner::single_modulate};
        alpha_test_mode alpha_test{alpha_test_mode::none};
        bool clip_plane_enabled{false};

        bool operator==(const frag_shader_desc&) const = default;
    };

    inline constexpr uint32_t spv_magic = 0x07230203u;
    inline constexpr uint32_t spv_version = 0x00010000u;

    enum spv_op : uint16_t
    {
        op_ext_inst_import = 11,
        op_ext_inst = 12,
        op_memory_model = 14,
        op_entry_point = 15,
        op_execution_mode = 16,
        op_capability = 17,
        op_type_void = 19,
        op_type_bool = 20,
        op_type_int = 21,
        op_type_float = 22,
        op_type_vector = 23,
        op_type_matrix = 24,
        op_type_image = 25,
        op_type_sampled_image = 27,
        op_type_struct = 30,
        op_type_pointer = 32,
        op_type_function = 33,
        op_constant = 43,
        op_constant_composite = 44,
        op_function = 54,
        op_function_end = 56,
        op_variable = 59,
        op_load = 61,
        op_store = 62,
        op_access_chain = 65,
        op_decorate = 71,
        op_member_decorate = 72,
        op_vector_shuffle = 79,
        op_composite_construct = 80,
        op_composite_extract = 81,
        op_image_sample_implicit_lod = 87,
        op_fnegate = 127,
        op_fadd = 129,
        op_fsub = 131,
        op_fmul = 133,
        op_fdiv = 136,
        op_vector_times_scalar = 142,
        op_matrix_times_vector = 145,
        op_dot = 148,
        op_select = 169,
        op_ford_less_than = 184,
        op_ford_greater_than = 186,
        op_ford_less_than_equal = 188,
        op_ford_greater_than_equal = 190,
        op_selection_merge = 247,
        op_label = 248,
        op_branch = 249,
        op_branch_conditional = 250,
        op_kill = 252,
        op_return = 253,
    };

    enum spv_storage_class : uint32_t
    {
        storage_uniform_constant = 0,
        storage_input = 1,
        storage_output = 3,
        storage_push_constant = 9,
    };

    enum spv_decoration : uint32_t
    {
        dec_block = 2,
        dec_col_major = 5,
        dec_builtin = 11,
        dec_location = 30,
        dec_binding = 33,
        dec_descriptor_set = 34,
        dec_offset = 35,
        dec_matrix_stride = 37,
    };

    enum glsl_std450_op : uint32_t
    {
        glsl_pow = 26,
        glsl_fclamp = 43,
        glsl_fmix = 46,
    };

    class spirv_module_builder
    {
    public:
        spirv_module_builder()
        {
            words.reserve(512);
            words.push_back(spv_magic);
            words.push_back(spv_version);
            words.push_back(0u);
            words.push_back(1u);
            words.push_back(0u);
        }

        uint32_t alloc_id()
        {
            return next_id++;
        }

        void emit(spv_op op, std::initializer_list<uint32_t> args)
        {
            const auto count = static_cast<uint32_t>(1 + args.size());
            words.push_back((count << 16) | static_cast<uint32_t>(op));
            for (uint32_t a : args)
            {
                words.push_back(a);
            }
        }

        void emit_ext_import(uint32_t id, std::string_view name)
        {
            const auto str_words = static_cast<uint32_t>((name.size() + 4) / 4);
            const uint32_t count = 2 + str_words;
            words.push_back((count << 16) | static_cast<uint32_t>(op_ext_inst_import));
            words.push_back(id);
            push_string(name);
        }

        void emit_entry_point(
            uint32_t exec_model,
            uint32_t entry_id,
            std::string_view name,
            std::initializer_list<uint32_t> interface_ids)
        {
            const auto str_words = static_cast<uint32_t>((name.size() + 4) / 4);
            const uint32_t count = 3 + str_words + static_cast<uint32_t>(interface_ids.size());
            words.push_back((count << 16) | static_cast<uint32_t>(op_entry_point));
            words.push_back(exec_model);
            words.push_back(entry_id);
            push_string(name);
            for (uint32_t v : interface_ids)
            {
                words.push_back(v);
            }
        }

        uint32_t const_f32(uint32_t f32_type, float val)
        {
            const uint32_t id = alloc_id();
            emit(op_constant, {f32_type, id, std::bit_cast<uint32_t>(val)});
            return id;
        }

        uint32_t const_i32(uint32_t i32_type, int32_t val)
        {
            const uint32_t id = alloc_id();
            emit(op_constant, {i32_type, id, static_cast<uint32_t>(val)});
            return id;
        }

        std::vector<uint32_t> finalize()
        {
            words[3] = next_id;
            return std::move(words);
        }

    private:
        void push_string(std::string_view s)
        {
            uint32_t word = 0;
            size_t byte_idx = 0;
            for (char c : s)
            {
                word |= (static_cast<uint32_t>(static_cast<unsigned char>(c)) << (byte_idx * 8));
                if (++byte_idx == 4)
                {
                    words.push_back(word);
                    word = 0;
                    byte_idx = 0;
                }
            }
            words.push_back(word);
        }

        std::vector<uint32_t> words;
        uint32_t next_id{1};
    };

    inline std::vector<uint32_t> build_vertex_shader_spirv()
    {
        spirv_module_builder b;

        const uint32_t ext_glsl = b.alloc_id();
        const uint32_t fn_main = b.alloc_id();

        const uint32_t var_gl_pos = b.alloc_id();
        const uint32_t var_in_pos = b.alloc_id();
        const uint32_t var_in_col = b.alloc_id();
        const uint32_t var_in_uv0 = b.alloc_id();
        const uint32_t var_in_uv1 = b.alloc_id();
        const uint32_t var_out_col = b.alloc_id();
        const uint32_t var_out_uv0 = b.alloc_id();
        const uint32_t var_out_uv1 = b.alloc_id();
        const uint32_t var_out_clip = b.alloc_id();
        const uint32_t var_pc = b.alloc_id();

        const uint32_t t_pc_struct = b.alloc_id();

        b.emit(op_capability, {1u});
        b.emit_ext_import(ext_glsl, "GLSL.std.450");
        b.emit(op_memory_model, {0u, 1u});
        b.emit_entry_point(
            0u,
            fn_main,
            "main",
            {
                var_gl_pos, var_in_pos, var_in_col, var_in_uv0, var_in_uv1,
                var_out_col, var_out_uv0, var_out_uv1, var_out_clip
            });

        b.emit(op_decorate, {var_gl_pos, dec_builtin, 0u});
        b.emit(op_decorate, {var_in_pos, dec_location, 0u});
        b.emit(op_decorate, {var_in_col, dec_location, 1u});
        b.emit(op_decorate, {var_in_uv0, dec_location, 2u});
        b.emit(op_decorate, {var_in_uv1, dec_location, 3u});
        b.emit(op_decorate, {var_out_col, dec_location, 0u});
        b.emit(op_decorate, {var_out_uv0, dec_location, 1u});
        b.emit(op_decorate, {var_out_uv1, dec_location, 2u});
        b.emit(op_decorate, {var_out_clip, dec_location, 3u});

        b.emit(op_decorate, {t_pc_struct, dec_block});
        b.emit(op_member_decorate, {t_pc_struct, 0u, dec_col_major});
        b.emit(op_member_decorate, {t_pc_struct, 0u, dec_offset, 0u});
        b.emit(op_member_decorate, {t_pc_struct, 0u, dec_matrix_stride, 16u});
        b.emit(op_member_decorate, {t_pc_struct, 1u, dec_offset, 64u});
        b.emit(op_member_decorate, {t_pc_struct, 2u, dec_offset, 80u});
        b.emit(op_member_decorate, {t_pc_struct, 3u, dec_offset, 96u});
        b.emit(op_member_decorate, {t_pc_struct, 4u, dec_offset, 112u});

        const uint32_t t_void = b.alloc_id();
        b.emit(op_type_void, {t_void});
        const uint32_t t_fn = b.alloc_id();
        b.emit(op_type_function, {t_fn, t_void});
        const uint32_t t_bool = b.alloc_id();
        b.emit(op_type_bool, {t_bool});
        const uint32_t t_i32 = b.alloc_id();
        b.emit(op_type_int, {t_i32, 32u, 1u});
        const uint32_t t_f32 = b.alloc_id();
        b.emit(op_type_float, {t_f32, 32u});
        const uint32_t t_vec2 = b.alloc_id();
        b.emit(op_type_vector, {t_vec2, t_f32, 2u});
        const uint32_t t_vec3 = b.alloc_id();
        b.emit(op_type_vector, {t_vec3, t_f32, 3u});
        const uint32_t t_vec4 = b.alloc_id();
        b.emit(op_type_vector, {t_vec4, t_f32, 4u});
        const uint32_t t_mat4 = b.alloc_id();
        b.emit(op_type_matrix, {t_mat4, t_vec4, 4u});

        b.emit(op_type_struct, {t_pc_struct, t_mat4, t_vec4, t_vec4, t_vec4, t_vec4});

        const uint32_t p_in_vec2 = b.alloc_id();
        b.emit(op_type_pointer, {p_in_vec2, storage_input, t_vec2});
        const uint32_t p_in_vec3 = b.alloc_id();
        b.emit(op_type_pointer, {p_in_vec3, storage_input, t_vec3});
        const uint32_t p_in_vec4 = b.alloc_id();
        b.emit(op_type_pointer, {p_in_vec4, storage_input, t_vec4});

        const uint32_t p_out_f32 = b.alloc_id();
        b.emit(op_type_pointer, {p_out_f32, storage_output, t_f32});
        const uint32_t p_out_vec2 = b.alloc_id();
        b.emit(op_type_pointer, {p_out_vec2, storage_output, t_vec2});
        const uint32_t p_out_vec4 = b.alloc_id();
        b.emit(op_type_pointer, {p_out_vec4, storage_output, t_vec4});

        const uint32_t p_pc_struct = b.alloc_id();
        b.emit(op_type_pointer, {p_pc_struct, storage_push_constant, t_pc_struct});
        const uint32_t p_pc_mat4 = b.alloc_id();
        b.emit(op_type_pointer, {p_pc_mat4, storage_push_constant, t_mat4});
        const uint32_t p_pc_vec4 = b.alloc_id();
        b.emit(op_type_pointer, {p_pc_vec4, storage_push_constant, t_vec4});

        const uint32_t c_i0 = b.const_i32(t_i32, 0);
        const uint32_t c_i1 = b.const_i32(t_i32, 1);
        const uint32_t c_i2 = b.const_i32(t_i32, 2);
        const uint32_t c_f0 = b.const_f32(t_f32, 0.0f);
        const uint32_t c_f1 = b.const_f32(t_f32, 1.0f);

        b.emit(op_variable, {p_out_vec4, var_gl_pos, storage_output});
        b.emit(op_variable, {p_in_vec3, var_in_pos, storage_input});
        b.emit(op_variable, {p_in_vec4, var_in_col, storage_input});
        b.emit(op_variable, {p_in_vec2, var_in_uv0, storage_input});
        b.emit(op_variable, {p_in_vec2, var_in_uv1, storage_input});
        b.emit(op_variable, {p_out_vec4, var_out_col, storage_output});
        b.emit(op_variable, {p_out_vec2, var_out_uv0, storage_output});
        b.emit(op_variable, {p_out_vec2, var_out_uv1, storage_output});
        b.emit(op_variable, {p_out_f32, var_out_clip, storage_output});
        b.emit(op_variable, {p_pc_struct, var_pc, storage_push_constant});

        b.emit(op_function, {t_void, fn_main, 0u, t_fn});
        const uint32_t lbl_entry = b.alloc_id();
        b.emit(op_label, {lbl_entry});

        const uint32_t pos3 = b.alloc_id();
        b.emit(op_load, {t_vec3, pos3, var_in_pos});
        const uint32_t px = b.alloc_id();
        b.emit(op_composite_extract, {t_f32, px, pos3, 0u});
        const uint32_t py = b.alloc_id();
        b.emit(op_composite_extract, {t_f32, py, pos3, 1u});
        const uint32_t pz = b.alloc_id();
        b.emit(op_composite_extract, {t_f32, pz, pos3, 2u});
        const uint32_t pos4 = b.alloc_id();
        b.emit(op_composite_construct, {t_vec4, pos4, px, py, pz, c_f1});

        const uint32_t ptr_mvp = b.alloc_id();
        b.emit(op_access_chain, {p_pc_mat4, ptr_mvp, var_pc, c_i0});
        const uint32_t mvp = b.alloc_id();
        b.emit(op_load, {t_mat4, mvp, ptr_mvp});
        const uint32_t clip_pos = b.alloc_id();
        b.emit(op_matrix_times_vector, {t_vec4, clip_pos, mvp, pos4});
        b.emit(op_store, {var_gl_pos, clip_pos});

        const uint32_t ptr_cplane = b.alloc_id();
        b.emit(op_access_chain, {p_pc_vec4, ptr_cplane, var_pc, c_i1});
        const uint32_t cplane = b.alloc_id();
        b.emit(op_load, {t_vec4, cplane, ptr_cplane});
        const uint32_t clip_dist = b.alloc_id();
        b.emit(op_dot, {t_f32, clip_dist, pos4, cplane});
        b.emit(op_store, {var_out_clip, clip_dist});

        const uint32_t col = b.alloc_id();
        b.emit(op_load, {t_vec4, col, var_in_col});
        b.emit(op_store, {var_out_col, col});

        const uint32_t ptr_uvs = b.alloc_id();
        b.emit(op_access_chain, {p_pc_vec4, ptr_uvs, var_pc, c_i2});
        const uint32_t uvs = b.alloc_id();
        b.emit(op_load, {t_vec4, uvs, ptr_uvs});
        const uint32_t su0 = b.alloc_id();
        b.emit(op_composite_extract, {t_f32, su0, uvs, 0u});
        const uint32_t sv0 = b.alloc_id();
        b.emit(op_composite_extract, {t_f32, sv0, uvs, 1u});
        const uint32_t su1 = b.alloc_id();
        b.emit(op_composite_extract, {t_f32, su1, uvs, 2u});
        const uint32_t sv1 = b.alloc_id();
        b.emit(op_composite_extract, {t_f32, sv1, uvs, 3u});

        // negative v scale marks rectangle textures whose v origin is inverted
        const uint32_t uv0_in = b.alloc_id();
        b.emit(op_load, {t_vec2, uv0_in, var_in_uv0});
        const uint32_t u0_in = b.alloc_id();
        b.emit(op_composite_extract, {t_f32, u0_in, uv0_in, 0u});
        const uint32_t v0_in = b.alloc_id();
        b.emit(op_composite_extract, {t_f32, v0_in, uv0_in, 1u});
        const uint32_t u0_out = b.alloc_id();
        b.emit(op_fmul, {t_f32, u0_out, u0_in, su0});
        const uint32_t v0_scaled = b.alloc_id();
        b.emit(op_fmul, {t_f32, v0_scaled, v0_in, sv0});
        const uint32_t sv0_neg = b.alloc_id();
        b.emit(op_ford_less_than, {t_bool, sv0_neg, sv0, c_f0});
        const uint32_t v0_off = b.alloc_id();
        b.emit(op_select, {t_f32, v0_off, sv0_neg, c_f1, c_f0});
        const uint32_t v0_out = b.alloc_id();
        b.emit(op_fadd, {t_f32, v0_out, v0_off, v0_scaled});
        const uint32_t uv0_out = b.alloc_id();
        b.emit(op_composite_construct, {t_vec2, uv0_out, u0_out, v0_out});
        b.emit(op_store, {var_out_uv0, uv0_out});

        const uint32_t uv1_in = b.alloc_id();
        b.emit(op_load, {t_vec2, uv1_in, var_in_uv1});
        const uint32_t u1_in = b.alloc_id();
        b.emit(op_composite_extract, {t_f32, u1_in, uv1_in, 0u});
        const uint32_t v1_in = b.alloc_id();
        b.emit(op_composite_extract, {t_f32, v1_in, uv1_in, 1u});
        const uint32_t u1_out = b.alloc_id();
        b.emit(op_fmul, {t_f32, u1_out, u1_in, su1});
        const uint32_t v1_scaled = b.alloc_id();
        b.emit(op_fmul, {t_f32, v1_scaled, v1_in, sv1});
        const uint32_t sv1_neg = b.alloc_id();
        b.emit(op_ford_less_than, {t_bool, sv1_neg, sv1, c_f0});
        const uint32_t v1_off = b.alloc_id();
        b.emit(op_select, {t_f32, v1_off, sv1_neg, c_f1, c_f0});
        const uint32_t v1_out = b.alloc_id();
        b.emit(op_fadd, {t_f32, v1_out, v1_off, v1_scaled});
        const uint32_t uv1_out = b.alloc_id();
        b.emit(op_composite_construct, {t_vec2, uv1_out, u1_out, v1_out});
        b.emit(op_store, {var_out_uv1, uv1_out});

        b.emit(op_return, {});
        b.emit(op_function_end, {});

        return b.finalize();
    }

    // specialize fragment variants so opaque draws skip discard ops and keep early-z active
    inline std::vector<uint32_t> build_fragment_shader_spirv(const frag_shader_desc& desc)
    {
        spirv_module_builder b;

        const uint32_t ext_glsl = b.alloc_id();
        const uint32_t fn_main = b.alloc_id();

        const uint32_t var_in_col = b.alloc_id();
        const uint32_t var_in_uv0 = b.alloc_id();
        const uint32_t var_in_uv1 = b.alloc_id();
        const uint32_t var_in_clip = b.alloc_id();
        const uint32_t var_out_col = b.alloc_id();
        const uint32_t var_tex0 = b.alloc_id();
        const uint32_t var_tex1 = b.alloc_id();
        const uint32_t var_pc = b.alloc_id();
        const uint32_t t_pc_struct = b.alloc_id();

        b.emit(op_capability, {1u});
        b.emit_ext_import(ext_glsl, "GLSL.std.450");
        b.emit(op_memory_model, {0u, 1u});
        b.emit_entry_point(
            4u,
            fn_main,
            "main",
            {var_in_col, var_in_uv0, var_in_uv1, var_in_clip, var_out_col});
        b.emit(op_execution_mode, {fn_main, 7u});

        b.emit(op_decorate, {var_in_col, dec_location, 0u});
        b.emit(op_decorate, {var_in_uv0, dec_location, 1u});
        b.emit(op_decorate, {var_in_uv1, dec_location, 2u});
        b.emit(op_decorate, {var_in_clip, dec_location, 3u});
        b.emit(op_decorate, {var_out_col, dec_location, 0u});

        b.emit(op_decorate, {var_tex0, dec_descriptor_set, 0u});
        b.emit(op_decorate, {var_tex0, dec_binding, 0u});
        b.emit(op_decorate, {var_tex1, dec_descriptor_set, 0u});
        b.emit(op_decorate, {var_tex1, dec_binding, 1u});

        b.emit(op_decorate, {t_pc_struct, dec_block});
        b.emit(op_member_decorate, {t_pc_struct, 0u, dec_col_major});
        b.emit(op_member_decorate, {t_pc_struct, 0u, dec_offset, 0u});
        b.emit(op_member_decorate, {t_pc_struct, 0u, dec_matrix_stride, 16u});
        b.emit(op_member_decorate, {t_pc_struct, 1u, dec_offset, 64u});
        b.emit(op_member_decorate, {t_pc_struct, 2u, dec_offset, 80u});
        b.emit(op_member_decorate, {t_pc_struct, 3u, dec_offset, 96u});
        b.emit(op_member_decorate, {t_pc_struct, 4u, dec_offset, 112u});

        const uint32_t t_void = b.alloc_id();
        b.emit(op_type_void, {t_void});
        const uint32_t t_fn = b.alloc_id();
        b.emit(op_type_function, {t_fn, t_void});
        const uint32_t t_bool = b.alloc_id();
        b.emit(op_type_bool, {t_bool});
        const uint32_t t_i32 = b.alloc_id();
        b.emit(op_type_int, {t_i32, 32u, 1u});
        const uint32_t t_f32 = b.alloc_id();
        b.emit(op_type_float, {t_f32, 32u});
        const uint32_t t_vec2 = b.alloc_id();
        b.emit(op_type_vector, {t_vec2, t_f32, 2u});
        const uint32_t t_vec3 = b.alloc_id();
        b.emit(op_type_vector, {t_vec3, t_f32, 3u});
        const uint32_t t_vec4 = b.alloc_id();
        b.emit(op_type_vector, {t_vec4, t_f32, 4u});
        const uint32_t t_mat4 = b.alloc_id();
        b.emit(op_type_matrix, {t_mat4, t_vec4, 4u});

        const uint32_t t_image = b.alloc_id();
        b.emit(op_type_image, {t_image, t_f32, 1u, 0u, 0u, 0u, 1u, 0u});
        const uint32_t t_samp_img = b.alloc_id();
        b.emit(op_type_sampled_image, {t_samp_img, t_image});

        b.emit(op_type_struct, {t_pc_struct, t_mat4, t_vec4, t_vec4, t_vec4, t_vec4});

        const uint32_t p_in_f32 = b.alloc_id();
        b.emit(op_type_pointer, {p_in_f32, storage_input, t_f32});
        const uint32_t p_in_vec2 = b.alloc_id();
        b.emit(op_type_pointer, {p_in_vec2, storage_input, t_vec2});
        const uint32_t p_in_vec4 = b.alloc_id();
        b.emit(op_type_pointer, {p_in_vec4, storage_input, t_vec4});
        const uint32_t p_out_vec4 = b.alloc_id();
        b.emit(op_type_pointer, {p_out_vec4, storage_output, t_vec4});
        const uint32_t p_samp_img = b.alloc_id();
        b.emit(op_type_pointer, {p_samp_img, storage_uniform_constant, t_samp_img});
        const uint32_t p_pc_struct = b.alloc_id();
        b.emit(op_type_pointer, {p_pc_struct, storage_push_constant, t_pc_struct});
        const uint32_t p_pc_vec4 = b.alloc_id();
        b.emit(op_type_pointer, {p_pc_vec4, storage_push_constant, t_vec4});

        const uint32_t c_i3 = b.const_i32(t_i32, 3);
        const uint32_t c_f0 = b.const_f32(t_f32, 0.0f);
        const uint32_t c_feps = b.const_f32(t_f32, 1.0e-6f);
        const uint32_t c_f05 = b.const_f32(t_f32, 0.5f);
        const uint32_t c_f1 = b.const_f32(t_f32, 1.0f);
        const uint32_t c_flarge = b.const_f32(t_f32, 1.0e6f);
        const uint32_t c_lut_scale = b.const_f32(t_f32, 255.0f / 256.0f);
        const uint32_t c_lut_bias = b.const_f32(t_f32, 0.5f / 256.0f);

        const uint32_t c_vec3_zero = b.alloc_id();
        b.emit(op_constant_composite, {t_vec3, c_vec3_zero, c_f0, c_f0, c_f0});
        const uint32_t c_vec3_eps = b.alloc_id();
        b.emit(op_constant_composite, {t_vec3, c_vec3_eps, c_feps, c_feps, c_feps});
        const uint32_t c_vec3_half = b.alloc_id();
        b.emit(op_constant_composite, {t_vec3, c_vec3_half, c_f05, c_f05, c_f05});
        const uint32_t c_vec3_one = b.alloc_id();
        b.emit(op_constant_composite, {t_vec3, c_vec3_one, c_f1, c_f1, c_f1});
        const uint32_t c_vec3_large = b.alloc_id();
        b.emit(op_constant_composite, {t_vec3, c_vec3_large, c_flarge, c_flarge, c_flarge});

        const uint32_t c_lum_r = b.const_f32(t_f32, 0.30f);
        const uint32_t c_lum_g = b.const_f32(t_f32, 0.59f);
        const uint32_t c_lum_b = b.const_f32(t_f32, 0.11f);
        const uint32_t c_vec3_lum = b.alloc_id();
        b.emit(op_constant_composite, {t_vec3, c_vec3_lum, c_lum_r, c_lum_g, c_lum_b});

        static constexpr float blur_off_vals[11] =
        {
            -5.0f, -4.0f, -3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f
        };
        static constexpr float blur_wt_vals[11] =
        {
            0.042557f, 0.056743f, 0.075657f, 0.100876f, 0.134501f,
            0.179335f,
            0.134501f, 0.100876f, 0.075657f, 0.056743f, 0.042557f
        };
        uint32_t c_blur_off[11]{};
        uint32_t c_blur_wt[11]{};
        if (desc.post == post_process_kind::blur)
        {
            for (int i = 0; i < 11; ++i)
            {
                c_blur_off[i] = b.const_f32(t_f32, blur_off_vals[i]);
                c_blur_wt[i] = b.const_f32(t_f32, blur_wt_vals[i]);
            }
        }

        b.emit(op_variable, {p_in_vec4, var_in_col, storage_input});
        b.emit(op_variable, {p_in_vec2, var_in_uv0, storage_input});
        b.emit(op_variable, {p_in_vec2, var_in_uv1, storage_input});
        b.emit(op_variable, {p_in_f32, var_in_clip, storage_input});
        b.emit(op_variable, {p_out_vec4, var_out_col, storage_output});
        b.emit(op_variable, {p_samp_img, var_tex0, storage_uniform_constant});
        b.emit(op_variable, {p_samp_img, var_tex1, storage_uniform_constant});
        b.emit(op_variable, {p_pc_struct, var_pc, storage_push_constant});

        b.emit(op_function, {t_void, fn_main, 0u, t_fn});
        const uint32_t lbl_entry = b.alloc_id();
        b.emit(op_label, {lbl_entry});

        auto extract_rgb = [&](uint32_t vec4_id) -> uint32_t
        {
            const uint32_t rgb = b.alloc_id();
            b.emit(op_vector_shuffle, {t_vec3, rgb, vec4_id, vec4_id, 0u, 1u, 2u});
            return rgb;
        };

        auto make_vec4_from_rgb_a = [&](uint32_t rgb_id, uint32_t a_id) -> uint32_t
        {
            const uint32_t r = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, r, rgb_id, 0u});
            const uint32_t g = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, g, rgb_id, 1u});
            const uint32_t bl = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, bl, rgb_id, 2u});
            const uint32_t res = b.alloc_id();
            b.emit(op_composite_construct, {t_vec4, res, r, g, bl, a_id});
            return res;
        };

        uint32_t final_color = 0;

        if (desc.post == post_process_kind::none)
        {
            const uint32_t v_col = b.alloc_id();
            b.emit(op_load, {t_vec4, v_col, var_in_col});

            if (desc.combiner == frag_combiner::untextured)
            {
                final_color = v_col;
            }
            else
            {
                const uint32_t uv0 = b.alloc_id();
                b.emit(op_load, {t_vec2, uv0, var_in_uv0});
                const uint32_t s0 = b.alloc_id();
                b.emit(op_load, {t_samp_img, s0, var_tex0});
                const uint32_t tex0 = b.alloc_id();
                b.emit(op_image_sample_implicit_lod, {t_vec4, tex0, s0, uv0});

                if (desc.combiner == frag_combiner::single_replace)
                {
                    final_color = tex0;
                }
                else
                {
                    const uint32_t base = b.alloc_id();
                    b.emit(op_fmul, {t_vec4, base, tex0, v_col});

                    if (desc.combiner == frag_combiner::single_modulate)
                    {
                        final_color = base;
                    }
                    else
                    {
                        const uint32_t uv1 = b.alloc_id();
                        b.emit(op_load, {t_vec2, uv1, var_in_uv1});
                        const uint32_t s1 = b.alloc_id();
                        b.emit(op_load, {t_samp_img, s1, var_tex1});
                        const uint32_t tex1 = b.alloc_id();
                        b.emit(op_image_sample_implicit_lod, {t_vec4, tex1, s1, uv1});

                        if (desc.combiner == frag_combiner::multi_modulate)
                        {
                            final_color = b.alloc_id();
                            b.emit(op_fmul, {t_vec4, final_color, base, tex1});
                        }
                        else if (desc.combiner == frag_combiner::multi_replace)
                        {
                            final_color = tex1;
                        }
                        else if (desc.combiner == frag_combiner::multi_decal)
                        {
                            const uint32_t base_rgb = extract_rgb(base);
                            const uint32_t tex1_rgb = extract_rgb(tex1);
                            const uint32_t tex1_a = b.alloc_id();
                            b.emit(op_composite_extract, {t_f32, tex1_a, tex1, 3u});
                            const uint32_t base_a = b.alloc_id();
                            b.emit(op_composite_extract, {t_f32, base_a, base, 3u});
                            const uint32_t t1a_vec3 = b.alloc_id();
                            b.emit(op_composite_construct, {t_vec3, t1a_vec3, tex1_a, tex1_a, tex1_a});
                            const uint32_t mix_rgb = b.alloc_id();
                            b.emit(op_ext_inst, {t_vec3, mix_rgb, ext_glsl, glsl_fmix, base_rgb, tex1_rgb, t1a_vec3});
                            final_color = make_vec4_from_rgb_a(mix_rgb, base_a);
                        }
                        else
                        {
                            const uint32_t base_rgb = extract_rgb(base);
                            const uint32_t tex1_rgb = extract_rgb(tex1);
                            const uint32_t sum_rgb = b.alloc_id();
                            b.emit(op_fadd, {t_vec3, sum_rgb, base_rgb, tex1_rgb});
                            const uint32_t base_a = b.alloc_id();
                            b.emit(op_composite_extract, {t_f32, base_a, base, 3u});
                            const uint32_t tex1_a = b.alloc_id();
                            b.emit(op_composite_extract, {t_f32, tex1_a, tex1, 3u});
                            const uint32_t prod_a = b.alloc_id();
                            b.emit(op_fmul, {t_f32, prod_a, base_a, tex1_a});
                            final_color = make_vec4_from_rgb_a(sum_rgb, prod_a);
                        }
                    }
                }
            }

            if (desc.alpha_test != alpha_test_mode::none)
            {
                const uint32_t alpha = b.alloc_id();
                b.emit(op_composite_extract, {t_f32, alpha, final_color, 3u});
                const uint32_t kill_cond = b.alloc_id();
                if (desc.alpha_test == alpha_test_mode::gt0)
                {
                    b.emit(op_ford_less_than_equal, {t_bool, kill_cond, alpha, c_f0});
                }
                else if (desc.alpha_test == alpha_test_mode::lt05)
                {
                    b.emit(op_ford_greater_than_equal, {t_bool, kill_cond, alpha, c_f05});
                }
                else
                {
                    b.emit(op_ford_less_than, {t_bool, kill_cond, alpha, c_f05});
                }
                const uint32_t lbl_kill = b.alloc_id();
                const uint32_t lbl_pass = b.alloc_id();
                b.emit(op_selection_merge, {lbl_pass, 0u});
                b.emit(op_branch_conditional, {kill_cond, lbl_kill, lbl_pass});
                b.emit(op_label, {lbl_kill});
                b.emit(op_kill, {});
                b.emit(op_label, {lbl_pass});
            }
        }
        else if (desc.post == post_process_kind::color_correct)
        {
            const uint32_t uv0 = b.alloc_id();
            b.emit(op_load, {t_vec2, uv0, var_in_uv0});
            const uint32_t s0 = b.alloc_id();
            b.emit(op_load, {t_samp_img, s0, var_tex0});
            const uint32_t samp = b.alloc_id();
            b.emit(op_image_sample_implicit_lod, {t_vec4, samp, s0, uv0});
            const uint32_t rgb = extract_rgb(samp);
            const uint32_t safe_rgb = b.alloc_id();
            b.emit(op_ext_inst, {t_vec3, safe_rgb, ext_glsl, glsl_fclamp, rgb, c_vec3_eps, c_vec3_one});

            const uint32_t ptr_pp = b.alloc_id();
            b.emit(op_access_chain, {p_pc_vec4, ptr_pp, var_pc, c_i3});
            const uint32_t pp = b.alloc_id();
            b.emit(op_load, {t_vec4, pp, ptr_pp});
            const uint32_t gamma_recip = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, gamma_recip, pp, 0u});
            const uint32_t overbright = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, overbright, pp, 1u});
            const uint32_t contrast = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, contrast, pp, 2u});

            const uint32_t exp_vec = b.alloc_id();
            b.emit(op_composite_construct, {t_vec3, exp_vec, gamma_recip, gamma_recip, gamma_recip});
            const uint32_t pow_rgb = b.alloc_id();
            b.emit(op_ext_inst, {t_vec3, pow_rgb, ext_glsl, glsl_pow, safe_rgb, exp_vec});
            const uint32_t gam_rgb = b.alloc_id();
            b.emit(op_vector_times_scalar, {t_vec3, gam_rgb, pow_rgb, overbright});

            const uint32_t con_vec = b.alloc_id();
            b.emit(op_composite_construct, {t_vec3, con_vec, contrast, contrast, contrast});
            const uint32_t mix_rgb = b.alloc_id();
            b.emit(op_ext_inst, {t_vec3, mix_rgb, ext_glsl, glsl_fmix, c_vec3_half, gam_rgb, con_vec});
            const uint32_t out_rgb = b.alloc_id();
            b.emit(op_ext_inst, {t_vec3, out_rgb, ext_glsl, glsl_fclamp, mix_rgb, c_vec3_zero, c_vec3_one});
            final_color = make_vec4_from_rgb_a(out_rgb, c_f1);
        }
        else if (desc.post == post_process_kind::bright_pass)
        {
            const uint32_t uv0 = b.alloc_id();
            b.emit(op_load, {t_vec2, uv0, var_in_uv0});
            const uint32_t s0 = b.alloc_id();
            b.emit(op_load, {t_samp_img, s0, var_tex0});
            const uint32_t samp = b.alloc_id();
            b.emit(op_image_sample_implicit_lod, {t_vec4, samp, s0, uv0});
            const uint32_t rgb = extract_rgb(samp);

            const uint32_t ptr_pp = b.alloc_id();
            b.emit(op_access_chain, {p_pc_vec4, ptr_pp, var_pc, c_i3});
            const uint32_t pp = b.alloc_id();
            b.emit(op_load, {t_vec4, pp, ptr_pp});
            const uint32_t thresh = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, thresh, pp, 0u});
            const uint32_t thresh_v = b.alloc_id();
            b.emit(op_composite_construct, {t_vec3, thresh_v, thresh, thresh, thresh});
            const uint32_t diff = b.alloc_id();
            b.emit(op_fsub, {t_vec3, diff, rgb, thresh_v});
            const uint32_t denom_raw = b.alloc_id();
            b.emit(op_fsub, {t_vec3, denom_raw, c_vec3_one, thresh_v});
            const uint32_t denom = b.alloc_id();
            b.emit(op_ext_inst, {t_vec3, denom, ext_glsl, glsl_fclamp, denom_raw, c_vec3_eps, c_vec3_large});
            const uint32_t quot = b.alloc_id();
            b.emit(op_fdiv, {t_vec3, quot, diff, denom});
            const uint32_t clamped = b.alloc_id();
            b.emit(op_ext_inst, {t_vec3, clamped, ext_glsl, glsl_fclamp, quot, c_vec3_zero, c_vec3_one});
            final_color = make_vec4_from_rgb_a(clamped, c_f1);
        }
        else if (desc.post == post_process_kind::downsample)
        {
            const uint32_t uv1 = b.alloc_id();
            b.emit(op_load, {t_vec2, uv1, var_in_uv1});
            const uint32_t s0 = b.alloc_id();
            b.emit(op_load, {t_samp_img, s0, var_tex0});
            final_color = b.alloc_id();
            b.emit(op_image_sample_implicit_lod, {t_vec4, final_color, s0, uv1});
        }
        else if (desc.post == post_process_kind::blur)
        {
            const uint32_t uv0 = b.alloc_id();
            b.emit(op_load, {t_vec2, uv0, var_in_uv0});
            const uint32_t s0 = b.alloc_id();
            b.emit(op_load, {t_samp_img, s0, var_tex0});

            const uint32_t ptr_pp = b.alloc_id();
            b.emit(op_access_chain, {p_pc_vec4, ptr_pp, var_pc, c_i3});
            const uint32_t pp = b.alloc_id();
            b.emit(op_load, {t_vec4, pp, ptr_pp});
            const uint32_t step_uv = b.alloc_id();
            b.emit(op_vector_shuffle, {t_vec2, step_uv, pp, pp, 0u, 1u});

            uint32_t accum = 0;
            for (int i = 0; i < 11; ++i)
            {
                const uint32_t delta = b.alloc_id();
                b.emit(op_vector_times_scalar, {t_vec2, delta, step_uv, c_blur_off[i]});
                const uint32_t tap_uv = b.alloc_id();
                b.emit(op_fadd, {t_vec2, tap_uv, uv0, delta});
                const uint32_t samp = b.alloc_id();
                b.emit(op_image_sample_implicit_lod, {t_vec4, samp, s0, tap_uv});
                const uint32_t weighted = b.alloc_id();
                b.emit(op_vector_times_scalar, {t_vec4, weighted, samp, c_blur_wt[i]});
                if (i == 0)
                {
                    accum = weighted;
                }
                else
                {
                    const uint32_t next_accum = b.alloc_id();
                    b.emit(op_fadd, {t_vec4, next_accum, accum, weighted});
                    accum = next_accum;
                }
            }
            final_color = accum;
        }
        else if (desc.post == post_process_kind::combine)
        {
            const uint32_t uv0 = b.alloc_id();
            b.emit(op_load, {t_vec2, uv0, var_in_uv0});
            const uint32_t uv1 = b.alloc_id();
            b.emit(op_load, {t_vec2, uv1, var_in_uv1});
            const uint32_t s0 = b.alloc_id();
            b.emit(op_load, {t_samp_img, s0, var_tex0});
            const uint32_t s1 = b.alloc_id();
            b.emit(op_load, {t_samp_img, s1, var_tex1});

            const uint32_t samp0 = b.alloc_id();
            b.emit(op_image_sample_implicit_lod, {t_vec4, samp0, s0, uv0});
            const uint32_t samp1 = b.alloc_id();
            b.emit(op_image_sample_implicit_lod, {t_vec4, samp1, s1, uv1});
            const uint32_t base_in = extract_rgb(samp0);
            const uint32_t bloom_in = extract_rgb(samp1);

            const uint32_t ptr_pp = b.alloc_id();
            b.emit(op_access_chain, {p_pc_vec4, ptr_pp, var_pc, c_i3});
            const uint32_t pp = b.alloc_id();
            b.emit(op_load, {t_vec4, pp, ptr_pp});
            const uint32_t bloom_sat = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, bloom_sat, pp, 0u});
            const uint32_t scene_sat = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, scene_sat, pp, 1u});
            const uint32_t bloom_int = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, bloom_int, pp, 2u});
            const uint32_t scene_int = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, scene_int, pp, 3u});

            auto adjust_sat = [&](uint32_t rgb, uint32_t sat, uint32_t intensity)
            {
                const uint32_t grey = b.alloc_id();
                b.emit(op_dot, {t_f32, grey, rgb, c_vec3_lum});
                const uint32_t grey_v = b.alloc_id();
                b.emit(op_composite_construct, {t_vec3, grey_v, grey, grey, grey});
                const uint32_t sat_v = b.alloc_id();
                b.emit(op_composite_construct, {t_vec3, sat_v, sat, sat, sat});
                const uint32_t mixed = b.alloc_id();
                b.emit(op_ext_inst, {t_vec3, mixed, ext_glsl, glsl_fmix, grey_v, rgb, sat_v});
                const uint32_t scaled = b.alloc_id();
                b.emit(op_vector_times_scalar, {t_vec3, scaled, mixed, intensity});
                return scaled;
            };

            const uint32_t bloom_col = adjust_sat(bloom_in, bloom_sat, bloom_int);
            const uint32_t bloom_clamped = b.alloc_id();
            b.emit(op_ext_inst, {t_vec3, bloom_clamped, ext_glsl, glsl_fclamp, bloom_col, c_vec3_zero, c_vec3_one});
            const uint32_t inv_bloom = b.alloc_id();
            b.emit(op_fsub, {t_vec3, inv_bloom, c_vec3_one, bloom_clamped});
            const uint32_t base_raw = adjust_sat(base_in, scene_sat, scene_int);
            const uint32_t base_col = b.alloc_id();
            b.emit(op_fmul, {t_vec3, base_col, base_raw, inv_bloom});
            const uint32_t sum_rgb = b.alloc_id();
            b.emit(op_fadd, {t_vec3, sum_rgb, base_col, bloom_col});
            final_color = make_vec4_from_rgb_a(sum_rgb, c_f1);
        }
        else
        {
            // apply 256x1 gamma lut texture on tex1 to scene color on tex0
            const uint32_t uv0 = b.alloc_id();
            b.emit(op_load, {t_vec2, uv0, var_in_uv0});
            const uint32_t s0 = b.alloc_id();
            b.emit(op_load, {t_samp_img, s0, var_tex0});
            const uint32_t s1 = b.alloc_id();
            b.emit(op_load, {t_samp_img, s1, var_tex1});
            const uint32_t c = b.alloc_id();
            b.emit(op_image_sample_implicit_lod, {t_vec4, c, s0, uv0});

            const uint32_t cr = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, cr, c, 0u});
            const uint32_t cg = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, cg, c, 1u});
            const uint32_t cb = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, cb, c, 2u});

            auto remap_lut_u = [&](uint32_t ch) -> uint32_t
            {
                const uint32_t scaled = b.alloc_id();
                b.emit(op_fmul, {t_f32, scaled, ch, c_lut_scale});
                const uint32_t mapped = b.alloc_id();
                b.emit(op_fadd, {t_f32, mapped, scaled, c_lut_bias});
                return mapped;
            };

            const uint32_t uv_r = b.alloc_id();
            b.emit(op_composite_construct, {t_vec2, uv_r, remap_lut_u(cr), c_f05});
            const uint32_t uv_g = b.alloc_id();
            b.emit(op_composite_construct, {t_vec2, uv_g, remap_lut_u(cg), c_f05});
            const uint32_t uv_b = b.alloc_id();
            b.emit(op_composite_construct, {t_vec2, uv_b, remap_lut_u(cb), c_f05});

            const uint32_t lut_r = b.alloc_id();
            b.emit(op_image_sample_implicit_lod, {t_vec4, lut_r, s1, uv_r});
            const uint32_t lut_g = b.alloc_id();
            b.emit(op_image_sample_implicit_lod, {t_vec4, lut_g, s1, uv_g});
            const uint32_t lut_b = b.alloc_id();
            b.emit(op_image_sample_implicit_lod, {t_vec4, lut_b, s1, uv_b});

            const uint32_t out_r = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, out_r, lut_r, 0u});
            const uint32_t out_g = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, out_g, lut_g, 1u});
            const uint32_t out_b = b.alloc_id();
            b.emit(op_composite_extract, {t_f32, out_b, lut_b, 2u});

            final_color = b.alloc_id();
            b.emit(op_composite_construct, {t_vec4, final_color, out_r, out_g, out_b, c_f1});
        }

        if (desc.clip_plane_enabled)
        {
            const uint32_t clip_val = b.alloc_id();
            b.emit(op_load, {t_f32, clip_val, var_in_clip});
            const uint32_t clip_neg = b.alloc_id();
            b.emit(op_ford_less_than, {t_bool, clip_neg, clip_val, c_f0});
            const uint32_t lbl_kill = b.alloc_id();
            const uint32_t lbl_cont = b.alloc_id();
            b.emit(op_selection_merge, {lbl_cont, 0u});
            b.emit(op_branch_conditional, {clip_neg, lbl_kill, lbl_cont});
            b.emit(op_label, {lbl_kill});
            b.emit(op_kill, {});
            b.emit(op_label, {lbl_cont});
        }

        b.emit(op_store, {var_out_col, final_color});
        b.emit(op_return, {});
        b.emit(op_function_end, {});

        return b.finalize();
    }
}
