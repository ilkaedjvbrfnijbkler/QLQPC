#pragma once

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace qlqpc::game
{
    namespace exe
    {
        inline constexpr uintptr_t preferred_base = 0x00400000;
        inline constexpr uint32_t expected_timestamp = 0x5751F20Bu;
        inline constexpr uint32_t expected_image_size = 0x013C7000u;

        // functions
        inline constexpr uintptr_t addr_ql_resource_interceptor_on_request = 0x00434620; // QLResourceInterceptor::OnRequest
        inline constexpr uintptr_t addr_fons_bounds_color_site = 0x00443DAA;             // color code RE_SetColor call site inside fonsTextBounds
        inline constexpr uintptr_t addr_fons_bounds_color_skip = 0x00443DBF;             // return target after RE_SetColor in fonsTextBounds
        inline constexpr uintptr_t addr_fons_draw_text = 0x00444360;                     // fonsDrawText
        inline constexpr uintptr_t addr_rb_begin_surface = 0x00451A80;                   // RB_BeginSurface
        inline constexpr uintptr_t addr_rb_iterate_post_colors = 0x00452A8C;             // RB_IterateStagesGeneric post-ComputeColors/ComputeTexCoords
        inline constexpr uintptr_t addr_rb_surface_lightning = 0x0045C540;               // RB_SurfaceLightningBolt
        inline constexpr uintptr_t addr_qgl_shutdown = 0x00470200;                       // QGL_Shutdown
        inline constexpr uintptr_t addr_qgl_init = 0x004728D0;                           // QGL_Init
        inline constexpr uintptr_t addr_cl_adjust_time_delta = 0x004B0660;               // CL_AdjustTimeDelta
        inline constexpr uintptr_t addr_cl_set_cgame_time = 0x004B07C0;                  // CL_SetCGameTime
        inline constexpr uintptr_t addr_cmd_unaliasall_f = 0x004B51C0;                   // Cmd_Unaliasall_f
        inline constexpr uintptr_t addr_cl_mouse_move_fn = 0x004B5800;                   // CL_MouseMove
        inline constexpr uintptr_t addr_cl_mouse_rate_site = 0x004B596E;                 // rate=[ebp-4] site in CL_MouseMove
        inline constexpr uintptr_t addr_cl_create_new_commands = 0x004B5DE0;             // CL_CreateNewCommands
        inline constexpr uintptr_t addr_cl_ready_to_send = 0x004B5E60;                   // CL_ReadyToSendPacket
        inline constexpr uintptr_t addr_cl_write_packet = 0x004B5F70;                    // CL_WritePacket
        inline constexpr uintptr_t addr_cl_send_command = 0x004B62A0;                    // CL_SendCommand
        inline constexpr uintptr_t addr_key_unbindall_f = 0x004B6D60;                    // Key_Unbindall_f
        inline constexpr uintptr_t addr_cl_frame = 0x004BC3E0;                           // CL_Frame
        inline constexpr uintptr_t addr_cl_parse_gamestate = 0x004BD790;                 // CL_ParseGamestate
        inline constexpr uintptr_t addr_cylinder_scale = 0x004C66E3;                     // CM_TraceThroughCylinder hitbox scale site
        inline constexpr uintptr_t addr_cbuf_insert_text = 0x004C7E50;                   // Cbuf_InsertText
        inline constexpr uintptr_t addr_cbuf_execute = 0x004C84E0;                       // Cbuf_Execute
        inline constexpr uintptr_t addr_cbuf_execute_before_pop = 0x004C851C;            // Cbuf_Execute text read site
        inline constexpr uintptr_t addr_cbuf_execute_after_pop = 0x004C859A;             // Cbuf_Execute command pop site
        inline constexpr uintptr_t addr_cmd_exec_f = 0x004C86D0;                         // Cmd_Exec_f
        inline constexpr uintptr_t addr_com_printf = 0x004C9860;                         // Com_Printf
        inline constexpr uintptr_t addr_com_error = 0x004C9B60;                          // Com_Error
        inline constexpr uintptr_t addr_z_free = 0x004CA1D0;                             // Z_Free
        inline constexpr uintptr_t addr_copy_string = 0x004CA470;                        // CopyString
        inline constexpr uintptr_t addr_com_init_small_zone = 0x004CA790;                // Com_InitSmallZoneMemory
        inline constexpr uintptr_t addr_com_modify_msec = 0x004CB710;                    // Com_ModifyMsec
        inline constexpr uintptr_t addr_com_event_loop = 0x004CBC90;                     // Com_EventLoop
        inline constexpr uintptr_t addr_com_frame_pacer = 0x004CC83E;                    // Sleep loop site in Com_Frame
        inline constexpr uintptr_t addr_cvar_find = 0x004CCD10;                          // Cvar_FindVar
        inline constexpr uintptr_t addr_cvar_set2 = 0x004CCE90;                          // Cvar_Set2
        inline constexpr uintptr_t addr_cvar_set = 0x004CD250;                           // Cvar_Set
        inline constexpr uintptr_t addr_cmd_seta_f = 0x004CD7F0;                         // Cmd_SetA_f
        inline constexpr uintptr_t addr_cvar_restart_f = 0x004CDB50;                     // Cvar_Restart_f
        inline constexpr uintptr_t addr_cvar_update = 0x004CDC80;                        // Cvar_Update
        inline constexpr uintptr_t addr_cvar_get = 0x004CDD30;                           // Cvar_Get (5-arg)
        inline constexpr uintptr_t addr_cvar_get3 = 0x004CE0D0;                          // Cvar_Get (3-arg)
        inline constexpr uintptr_t addr_cvar_register4 = 0x004CE400;                     // Cvar_Register (4-arg)
        inline constexpr uintptr_t addr_cvar_register6 = 0x004CE460;                     // Cvar_Register (6-arg)
        inline constexpr uintptr_t addr_cvar_init = 0x004CE4C0;                          // Cvar_Init
        inline constexpr uintptr_t addr_fs_free_file = 0x004D0510;                       // FS_FreeFile
        inline constexpr uintptr_t addr_fs_read_file = 0x004D2AF0;                       // FS_ReadFile
        inline constexpr uintptr_t addr_q_strncpyz = 0x004D8F40;                         // Q_strncpyz
        inline constexpr uintptr_t addr_com_default_extension = 0x004D9A60;              // COM_DefaultExtension
        inline constexpr uintptr_t addr_vm_create = 0x004E9FF0;                          // VM_Create
        inline constexpr uintptr_t addr_vm_free = 0x004EA0E0;                            // VM_Free
        inline constexpr uintptr_t addr_in_frame = 0x004EC4F0;                           // IN_Frame
        inline constexpr uintptr_t addr_sys_que_event = 0x004ED050;                      // Sys_QueEvent
        inline constexpr uintptr_t addr_sys_get_event_msg = 0x004ED191;                  // Sys_GetEvent Msg.time write site
        inline constexpr uintptr_t addr_sys_get_event_empty = 0x004ED3B0;                // Sys_GetEvent empty queue site
        inline constexpr uintptr_t addr_sys_sleep = 0x004ED7E0;                          // Sys_Sleep
        inline constexpr uintptr_t addr_win_main = 0x004ED830;                           // WinMain
        inline constexpr uintptr_t addr_sys_is_lan_address = 0x004EE570;                 // Sys_IsLANAddress
        inline constexpr uintptr_t addr_sys_milliseconds = 0x004EF510;                   // Sys_Milliseconds
        inline constexpr uintptr_t addr_sys_init_time = 0x004EF520;                      // Sys_InitTime

        // globals
        inline constexpr uintptr_t addr_world_numfogs = 0x005861BC;                      // s_worldData.numfogs
        inline constexpr uintptr_t addr_world_fogs_ptr = 0x005861C0;                     // s_worldData.fogs
        inline constexpr uintptr_t addr_cmd_argv = 0x011FA1B8;                           // const char* cmd_argv[]
        inline constexpr uintptr_t addr_cmd_argc = 0x011FF9B8;                           // int cmd_argc
        inline constexpr uintptr_t addr_cvar_sv_running_ptr = 0x01205E2C;                // cvar_t* com_sv_running
        inline constexpr uintptr_t addr_cvar_hash_table = 0x01208688;                    // cvar_t* hashTable[256]
        inline constexpr uintptr_t addr_cvar_iter_cursor = 0x01208A88;                   // cvar_t* iter cursor
        inline constexpr uintptr_t addr_hwnd_parent = 0x012D34B4;                        // HWND hWndParent (glw_state.hWnd)
        inline constexpr uintptr_t addr_sys_time_base = 0x012D39A8;                      // uint32_t sys_timeBase
        inline constexpr uintptr_t addr_cvar_vars = 0x01437900;                          // cvar_t* cvar_vars head
        inline constexpr uintptr_t addr_cvar_modified_flags = 0x01437908;                // uint32_t cvar_modifiedFlags
        inline constexpr uintptr_t addr_cvar_indexes = 0x01437920;                       // cvar_t cvar_indexes[2048]
        inline constexpr uintptr_t addr_cvar_num_indexes = 0x01459920;                   // int cvar_numIndexes
        inline constexpr uintptr_t addr_cvar_sv_paused_ptr = 0x0145B95C;                 // cvar_t* sv_paused
        inline constexpr uintptr_t addr_cvar_timescale_ptr = 0x0145B960;                 // cvar_t* com_timescale
        inline constexpr uintptr_t addr_cvar_cl_paused_ptr = 0x0145C9E4;                 // cvar_t* cl_paused
        inline constexpr uintptr_t addr_small_zone_block = 0x0145C9EC;                   // memzone_t* smallzone block
        inline constexpr uintptr_t addr_cvar_com_maxfps_ptr = 0x0145CA48;                // cvar_t* com_maxfps
        inline constexpr uintptr_t addr_com_frametime = 0x0145CA4C;                      // int com_frameTime
        inline constexpr uintptr_t addr_com_fully_initialized = 0x0145CA58;              // int com_fullyInitialized
        inline constexpr uintptr_t addr_cmd_text_maxsize = 0x0146CA84;                   // int cmd_text.maxsize
        inline constexpr uintptr_t addr_cmd_text_cursize = 0x0146CA88;                   // int cmd_text.cursize
        inline constexpr uintptr_t addr_cvar_showsend_ptr = 0x0146CC40;                  // cvar_t* cl_showSend
        inline constexpr uintptr_t addr_cl_snap_server_time = 0x0146CD2C;                // int cl.snap.serverTime
        inline constexpr uintptr_t addr_cl_server_time = 0x0146CFBC;                     // int cl.serverTime
        inline constexpr uintptr_t addr_cl_old_server_time = 0x0146CFC0;                 // int cl.oldServerTime
        inline constexpr uintptr_t addr_cl_server_time_delta = 0x0146CFC8;               // int cl.serverTimeDelta
        inline constexpr uintptr_t addr_cl_extrapolated_snap = 0x0146CFCC;               // int cl.extrapolatedSnapshot
        inline constexpr uintptr_t addr_cl_new_snapshots = 0x0146CFD0;                   // int cl.newSnapshots
        inline constexpr uintptr_t addr_cl_mouse_dx = 0x01471E9C;                        // int cl.mouseDx
        inline constexpr uintptr_t addr_cl_mouse_dy = 0x01471EA0;                        // int cl.mouseDy
        inline constexpr uintptr_t addr_cmd_ring = 0x01471ED0;                           // usercmd_t cl.cmds[64] (28 bytes each)
        inline constexpr uintptr_t addr_cmd_number = 0x014725D0;                         // int cl.cmdNumber
        inline constexpr uintptr_t addr_cl_viewangles = 0x01472754;                      // float cl.viewangles[3]
        inline constexpr uintptr_t addr_cls_state = 0x01528BA0;                          // connstate_t cls.state
        inline constexpr uintptr_t addr_cls_key_catchers = 0x01528BA4;                   // int cls.keyCatchers
        inline constexpr uintptr_t addr_cls_real_frametime = 0x01528CC4;                 // int cls.realFrametime
        inline constexpr uintptr_t addr_cls_realtime = 0x01528CC8;                       // int cls.realtime
        inline constexpr uintptr_t addr_cls_frametime = 0x01528CCC;                      // int cls.frametime
        inline constexpr uintptr_t addr_clc_demoplaying = 0x016177E0;                    // int clc.demoplaying
        inline constexpr uintptr_t addr_clc_netchan_type = 0x01617C04;                   // int clc.netchan.remoteAddress.type (2 = NA_LOOPBACK)
        inline constexpr uintptr_t addr_clc_netchan_ip = 0x01617C08;                     // int clc.netchan.remoteAddress.ip
        inline constexpr uintptr_t addr_cvar_maxpackets_ptr = 0x01627C3C;                // cvar_t* cl_maxpackets
        inline constexpr uintptr_t addr_cvar_showtimedelta_ptr = 0x01627C58;             // cvar_t* cl_showTimeDelta
        inline constexpr uintptr_t addr_cvar_m_filter_ptr = 0x01647F00;                  // cvar_t* m_filter
        inline constexpr uintptr_t addr_in_up_active = 0x0164AF30;                       // int in_up.active
        inline constexpr uintptr_t addr_in_forward_active = 0x0164AF50;                  // int in_forward.active
        inline constexpr uintptr_t addr_in_moveleft_active = 0x0164B150;                 // int in_moveleft.active
        inline constexpr uintptr_t addr_old_realtime = 0x0164B158;                       // int cl.oldRealtime
        inline constexpr uintptr_t addr_in_down_active = 0x0164B170;                     // int in_down.active
        inline constexpr uintptr_t addr_frame_msec = 0x0165D598;                         // int cl.frame_msec
        inline constexpr uintptr_t addr_in_speed_active = 0x0165D5B0;                    // int in_speed.active
        inline constexpr uintptr_t addr_cvar_cl_run_ptr = 0x0165D5D8;                    // cvar_t* cl_run
        inline constexpr uintptr_t addr_in_moveright_active = 0x0165D5F0;                // int in_moveright.active
        inline constexpr uintptr_t addr_in_back_active = 0x0165D610;                     // int in_back.active
        inline constexpr uintptr_t addr_hinst_opengl = 0x016E410C;                       // HMODULE glw_state.hinstOpenGL
        inline constexpr uintptr_t addr_tess_svars_colors = 0x0170B240;                  // uint8_t[SHADER_MAX_VERTEXES][4] tess.svars.colors
        inline constexpr uintptr_t addr_tess_num_vertexes = 0x01716DD4;                  // int tess.numVertexes
        inline constexpr uintptr_t addr_tr_world_ptr = 0x01716EC4;                       // world_t* tr.world
        inline constexpr uintptr_t addr_backend_current_entity = 0x01745F20;             // trRefEntity_t* backEnd.currentEntity
    }

    // cgamex86.dll addresses at preferred base 0x10000000
    namespace cgame
    {
        inline constexpr uintptr_t preferred_base = 0x10000000;
        inline constexpr uint32_t expected_timestamp = 0x5751F21Cu;
        inline constexpr uint32_t expected_image_size = 0x00B87000u;

        inline constexpr uintptr_t addr_hitreg_fire = 0x10006083;                  // CG_FireWeapon mid-hook site
        inline constexpr uintptr_t addr_pmove_single = 0x10006470;                 // PmoveSingle(pmove_t*)
        inline constexpr uintptr_t addr_pmove_post_angles = 0x100065FF;            // post-PM_UpdateViewAngles in PmoveSingle
        inline constexpr uintptr_t addr_text_measure = 0x100082B0;                 // font width/height measure helper
        inline constexpr uintptr_t addr_text_paint = 0x10008440;                   // font paint helper
        inline constexpr uintptr_t addr_draw_fps_site = 0x10009C83;                // post-idiv in CG_DrawFPS
        inline constexpr uintptr_t addr_hitreg_draw = 0x10009D3B;                  // post-FPS paint mid-hook site
        inline constexpr uintptr_t addr_frame_interp_site = 0x10018C2D;            // post-frameInterpolation in CG_AddPacketEntities
        inline constexpr uintptr_t addr_hitreg_feedback = 0x100435CB;              // CG_TransitionPlayerState feedback site
        inline constexpr uintptr_t addr_point_trace = 0x10044040;                  // CG_Trace
        inline constexpr uintptr_t addr_alt_point_trace = 0x10044100;              // CG_Trace alternate path
        inline constexpr uintptr_t addr_step_offset = 0x10044620;                  // CG_StepOffset()
        inline constexpr uintptr_t addr_predict_player_state = 0x100446E0;         // CG_PredictPlayerState()
        inline constexpr uintptr_t addr_calc_view_values = 0x1004DD00;             // CG_CalcViewValues()
        inline constexpr uintptr_t addr_lightning_bolt_ref_entity = 0x10051F65;    // trap_R_AddRefEntityToScene site in CG_LightningBolt
        inline constexpr uintptr_t addr_lightning_bolt_skip_target = 0x10051F7B;   // return target after trap_R_AddRefEntityToScene in CG_LightningBolt
        inline constexpr uintptr_t addr_add_weapon_with_powerups = 0x10052365;     // CG_AddWeaponWithPowerups entry after push ebx
        inline constexpr uintptr_t addr_add_weapon_skip_target = 0x1005240F;       // CG_AddWeaponWithPowerups return site
        inline constexpr uintptr_t addr_add_weapon_flash = 0x10052B75;             // trap_R_AddRefEntityToScene site for tag_flash in CG_AddPlayerWeapon
        inline constexpr uintptr_t addr_add_weapon_flash_skip_target = 0x10052B8E; // return target after trap_R_AddRefEntityToScene in CG_AddPlayerWeapon
        inline constexpr uintptr_t addr_angle_vectors = 0x10056E40;                // AngleVectors

        inline constexpr uintptr_t addr_c_pmove_counter = 0x10079798;              // int c_pmove
        inline constexpr uintptr_t addr_pml_frametime = 0x100797C4;                // float pml.frametime
        inline constexpr uintptr_t addr_pml_msec = 0x100797C8;                     // int pml.msec
        inline constexpr uintptr_t addr_cg_pmove = 0x100D1D50;                     // pmove_t cg_pmove
        inline constexpr uintptr_t addr_cg_pmove_cmd = 0x100D1D54;                 // usercmd_t cg_pmove.cmd
        inline constexpr uintptr_t addr_cg_pmove_fixed = 0x100D1D7C;               // int cg_pmove.pmove_fixed
        inline constexpr uintptr_t addr_cg_pmove_step_delta = 0x100D1E28;          // float cg_pmove.step_delta
        inline constexpr uintptr_t addr_cg_pmove_step_time = 0x100D1E2C;           // int cg_pmove.step_time
        inline constexpr uintptr_t addr_cgs_gametype = 0x10A3FF14;                 // int cgs.gametype
        inline constexpr uintptr_t addr_cgs_serverinfo_flags = 0x10A3FF28;         // int cgs serverinfo flags
        inline constexpr uintptr_t addr_cgs_clientinfo_team0 = 0x10A41DF8;         // int cgs.clientinfo[0].team
        inline constexpr uintptr_t addr_cgs_media_lightning = 0x10A5F490;          // qhandle_t cgs.media.lightningShader[5]
        inline constexpr uintptr_t addr_trace_mode = 0x10A5FD9C;                   // int trace mode flag
        inline constexpr uintptr_t addr_cg_nopredict_int = 0x10A6D26C;             // int cg_nopredict.integer
        inline constexpr uintptr_t addr_cg_error_decay_val = 0x10A6F0C8;           // float cg_errorDecay.value
        inline constexpr uintptr_t addr_cg_demo_playback = 0x10A6F8A8;             // int cg.demoPlayback
        inline constexpr uintptr_t addr_cg_snap = 0x10A6F8C4;                      // snapshot_t* cg.snap
        inline constexpr uintptr_t addr_cg_next_snap = 0x10A6F8C8;                 // snapshot_t* cg.nextSnap
        inline constexpr uintptr_t addr_cg_frame_interp = 0x10A9C1DC;              // float cg.frameInterpolation
        inline constexpr uintptr_t addr_cg_time = 0x10A9C1EC;                      // int cg.time
        inline constexpr uintptr_t addr_cg_third_person = 0x10A9C208;              // int cg.renderingThirdPerson
        inline constexpr uintptr_t addr_cg_hyperspace = 0x10A9C20C;                // int cg.hyperspace
        inline constexpr uintptr_t addr_cg_pps = 0x10A9C210;                       // playerState_t cg.predictedPlayerState (0x250 bytes)
        inline constexpr uintptr_t addr_cg_valid_pps = 0x10A9C730;                 // int cg.validPPS
        inline constexpr uintptr_t addr_cg_pred_err_time = 0x10A9C734;             // int cg.predictedErrorTime
        inline constexpr uintptr_t addr_cg_pred_err_vec = 0x10A9C738;              // float cg.predictedError[3]
        inline constexpr uintptr_t addr_cg_step_change = 0x10A9C788;               // float cg.stepChange
        inline constexpr uintptr_t addr_cg_step_time = 0x10A9C78C;                 // int cg.stepTime
        inline constexpr uintptr_t addr_cg_duck_change = 0x10A9C790;               // float cg.duckChange
        inline constexpr uintptr_t addr_cg_duck_time = 0x10A9C794;                 // int cg.duckTime
        inline constexpr uintptr_t addr_cg_refdef_vieworg = 0x10A9C838;            // float cg.refdef.vieworg[3]

        inline constexpr uintptr_t clientinfo_stride = 1848;                       // sizeof(clientInfo_t) (0x738)
        inline constexpr int max_clients = 64;                                     // MAX_CLIENTS
    }

    inline constexpr size_t player_state_bytes = 0x250;
    inline constexpr size_t usercmd_bytes = 28;
    inline constexpr int cvar_archive_flag = 1;

    namespace ps_off
    {
        inline constexpr size_t command_time = 0x00;
        inline constexpr size_t pm_type = 0x04;
        inline constexpr size_t pm_flags = 0x0C;
        inline constexpr size_t origin = 0x14;
        inline constexpr size_t delta_angles = 0x38;
        inline constexpr size_t client_num = 0x88;
        inline constexpr size_t weapon = 0x90;
        inline constexpr size_t viewangles = 0xA0;
        inline constexpr size_t viewheight = 0xAC;
        inline constexpr size_t health = 0xC0;
        inline constexpr size_t pers_hits = 0x104;
    }

    namespace snap_off
    {
        inline constexpr size_t server_time = 0x08;
        inline constexpr size_t pm_type = 0x30;
        inline constexpr size_t pm_flags = 0x38;
        inline constexpr size_t client_num = 0xB4;
        inline constexpr size_t health = 0xEC;
        inline constexpr size_t pers_team = 0x138;
    }

    namespace cent_off
    {
        inline constexpr size_t number = 0x00;
        inline constexpr size_t client_num = 0xB0;
    }

    namespace re_off
    {
        inline constexpr size_t custom_shader = 0x70;
        inline constexpr size_t shader_rgba = 0x74;
    }

    namespace stage_off
    {
        inline constexpr size_t state_bits = 304;
    }

    namespace shader_off
    {
        inline constexpr size_t sort = 0x4C;
        inline constexpr size_t fog_pass = 200;
    }

    namespace vm_off
    {
        inline constexpr size_t dll_handle = 0x40;
    }

    #pragma pack(push, 1)
    struct ql_fog
    {
        float bounds[2][3];     // 0..23
        uint32_t brush_side;    // 24..27
        uint32_t color_int;     // 28..31 (packed rgba)
        float tc_scale;         // 32..35 (1.0 / (depth_for_opaque * 8.0))
        float color[3];         // 36..47
        float depth_for_opaque; // 48..51
        int has_surface;        // 52..55 (0 for volumetric/camera-inside fog)
        float surface[4];       // 56..71
    };
    #pragma pack(pop)
    static_assert(sizeof(ql_fog) == 72, "ql_fog must remain 72 bytes");

    using cvar_get_t = void*(__cdecl*)(const char*, const char*, const char*, const char*, int);
    using cvar_get3_t = void*(__cdecl*)(const char*, const char*, int);
    using cvar_set_t = void*(__cdecl*)(const char*, const char*);
    using cvar_find_var_t = void*(__cdecl*)(const char*);
    using copy_string_t = char*(__cdecl*)(const char* str);
    using z_free_t = void(__cdecl*)(void* ptr);
    using com_printf_t = void(__cdecl*)(const char* fmt, ...);
    using com_error_t = void(__cdecl*)(int code, const char* fmt, ...);
    using fs_read_file_t = int(__cdecl*)(const char* qpath, void** buffer);
    using fs_free_file_t = void(__cdecl*)(void* buffer);
    using q_strncpyz_t = void(__cdecl*)(char* dest, const char* src, int dest_size);
    using com_default_ext_t = void(__cdecl*)(char* path, int max_size, const char* extension);
    using void_cmd_t = void(__cdecl*)();
    using in_frame_t = void(__cdecl*)();
    using com_event_loop_t = int(__cdecl*)();
    using cl_ready_to_send_t = int(__cdecl*)();
    using cl_write_packet_t = void(__cdecl*)();
    using cl_mouse_move_t = void*(__cdecl*)(void*);
    using pmove_single_t = int(__cdecl*)(void*);
    using cg_step_offset_t = void(__cdecl*)();
    using point_trace_t = void(__cdecl*)(uint32_t*, const float*, const float*, const float*, const float*, int, int);
    using text_paint_t = void(__cdecl*)(float, float, int, float, const float*, const char*, float, int, int);

    uintptr_t exe_base();

    inline uintptr_t exe_addr(uintptr_t preferred_va)
    {
        return exe_base() + (preferred_va - exe::preferred_base);
    }

    template <typename T>
    inline T* exe_ptr(uintptr_t preferred_va)
    {
        return reinterpret_cast<T*>(exe_addr(preferred_va));
    }

    template <typename Fn>
    inline Fn exe_fn(uintptr_t preferred_va)
    {
        return reinterpret_cast<Fn>(exe_addr(preferred_va));
    }

    inline uintptr_t cgame_addr(HMODULE h_cgame, uintptr_t preferred_va)
    {
        if (h_cgame == nullptr)
        {
            return 0;
        }
        return reinterpret_cast<uintptr_t>(h_cgame) + (preferred_va - cgame::preferred_base);
    }

    template <typename T>
    inline T* cgame_ptr(HMODULE h_cgame, uintptr_t preferred_va)
    {
        return reinterpret_cast<T*>(cgame_addr(h_cgame, preferred_va));
    }

    template <typename Fn>
    inline Fn cgame_fn(HMODULE h_cgame, uintptr_t preferred_va)
    {
        return reinterpret_cast<Fn>(cgame_addr(h_cgame, preferred_va));
    }

    bool verify_pe_module(HMODULE mod, uint32_t expected_timestamp, uint32_t expected_image_size);

    inline int read_cvar_int(const void* cvar_ptr, int fallback = 0)
    {
        if (cvar_ptr == nullptr)
        {
            return fallback;
        }
        return *reinterpret_cast<const int*>(reinterpret_cast<uintptr_t>(cvar_ptr) + 0x30);
    }

    inline float read_cvar_float(const void* cvar_ptr, float fallback = 0.0f)
    {
        if (cvar_ptr == nullptr)
        {
            return fallback;
        }
        return *reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(cvar_ptr) + 0x2C);
    }

    inline const char* read_cvar_string(const void* cvar_ptr, const char* fallback = "")
    {
        if (cvar_ptr == nullptr)
        {
            return fallback;
        }
        const char* s = *reinterpret_cast<const char* const*>(reinterpret_cast<uintptr_t>(cvar_ptr) + 0x04);
        return (s != nullptr) ? s : fallback;
    }

    int read_client_team(HMODULE h_cgame, int client_num, int fallback = 0);

    void query_hwnd_state_cached(
        HWND hwnd,
        bool& out_valid,
        bool& out_minimized,
        bool& out_focused);

    std::filesystem::path get_module_dir_dynamic(HMODULE mod);

    using cgame_loaded_t = void (*)(HMODULE h_cgame);
    using cgame_unloading_t = void (*)();

    bool install_vm_hooks(cgame_loaded_t on_loaded, cgame_unloading_t on_unloading);
    void release_vm_hooks();
    void attach_existing_cgame();
    void detach_active_cgame();
}
