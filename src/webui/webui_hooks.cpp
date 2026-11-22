#include "webui/webui_hooks.hpp"

#include "game/game.hpp"
#include "util/hook_engine.hpp"
#include "webui/webui_embedded_assets.hpp"
#include "webui/webui_patch.hpp"

#include <windows.h>

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace qlqpc::webui
{
    namespace
    {
        util::inline_hook hook_ql_resource_interceptor_on_request{};

        struct web_string_raw
        {
            void* impl{nullptr};
        };
        static_assert(sizeof(web_string_raw) == 4, "Awesomium::WebString must be 4 bytes");

        struct web_url_raw
        {
            void* impl{nullptr};
        };
        static_assert(sizeof(web_url_raw) == 4, "Awesomium::WebURL must be 4 bytes");

        struct awesomium_api
        {
            using create_from_utf8_t = web_string_raw* (__cdecl*)(
                web_string_raw* out_sret,
                const char* str,
                unsigned int len);
            using to_utf8_t = unsigned int (__thiscall*)(
                const web_string_raw* self,
                char* dst,
                unsigned int max_len);
            using destroy_string_t = void (__thiscall*)(web_string_raw* self);
            using url_part_t = web_string_raw* (__thiscall*)(
                const web_url_raw* self,
                web_string_raw* out_sret);
            using destroy_url_t = void (__thiscall*)(web_url_raw* self);
            using create_response_t = void* (__cdecl*)(
                unsigned int num_bytes,
                unsigned char* data,
                const web_string_raw* mime_type);
            using request_url_t = web_url_raw* (__thiscall*)(
                const void* request,
                web_url_raw* out_sret);

            create_from_utf8_t create_from_utf8{nullptr};
            to_utf8_t to_utf8{nullptr};
            destroy_string_t destroy_string{nullptr};
            url_part_t url_host{nullptr};
            url_part_t url_path{nullptr};
            destroy_url_t destroy_url{nullptr};
            create_response_t create_response{nullptr};

            bool is_ready() const
            {
                return create_from_utf8 != nullptr &&
                       to_utf8 != nullptr &&
                       destroy_string != nullptr &&
                       url_host != nullptr &&
                       url_path != nullptr &&
                       destroy_url != nullptr &&
                       create_response != nullptr;
            }
        };

        struct scoped_web_string
        {
            const awesomium_api& api;
            web_string_raw raw{};
            bool active{false};

            explicit scoped_web_string(const awesomium_api& a)
                : api(a)
            {
            }

            ~scoped_web_string()
            {
                if (active && api.destroy_string != nullptr)
                {
                    api.destroy_string(&raw);
                }
            }

            scoped_web_string(const scoped_web_string&) = delete;
            scoped_web_string& operator=(const scoped_web_string&) = delete;
        };

        struct scoped_web_url
        {
            const awesomium_api& api;
            web_url_raw raw{};
            bool active{false};

            explicit scoped_web_url(const awesomium_api& a)
                : api(a)
            {
            }

            ~scoped_web_url()
            {
                if (active && api.destroy_url != nullptr)
                {
                    api.destroy_url(&raw);
                }
            }

            scoped_web_url(const scoped_web_url&) = delete;
            scoped_web_url& operator=(const scoped_web_url&) = delete;
        };

        const awesomium_api* get_awesomium_api()
        {
            static std::mutex api_mtx;
            static awesomium_api api{};
            static bool loaded = false;

            std::lock_guard<std::mutex> lock(api_mtx);
            if (loaded)
            {
                return api.is_ready() ? &api : nullptr;
            }

            const HMODULE h_mod = GetModuleHandleW(L"awesomium.dll");
            if (h_mod == nullptr)
            {
                return nullptr;
            }

            api.create_from_utf8 = reinterpret_cast<awesomium_api::create_from_utf8_t>(
                GetProcAddress(h_mod, "?CreateFromUTF8@WebString@Awesomium@@SA?AV12@PBDI@Z"));
            api.to_utf8 = reinterpret_cast<awesomium_api::to_utf8_t>(
                GetProcAddress(h_mod, "?ToUTF8@WebString@Awesomium@@QBEIPADI@Z"));
            api.destroy_string = reinterpret_cast<awesomium_api::destroy_string_t>(
                GetProcAddress(h_mod, "??1WebString@Awesomium@@QAE@XZ"));
            api.url_host = reinterpret_cast<awesomium_api::url_part_t>(
                GetProcAddress(h_mod, "?host@WebURL@Awesomium@@QBE?AVWebString@2@XZ"));
            api.url_path = reinterpret_cast<awesomium_api::url_part_t>(
                GetProcAddress(h_mod, "?path@WebURL@Awesomium@@QBE?AVWebString@2@XZ"));
            api.destroy_url = reinterpret_cast<awesomium_api::destroy_url_t>(
                GetProcAddress(h_mod, "??1WebURL@Awesomium@@QAE@XZ"));
            api.create_response = reinterpret_cast<awesomium_api::create_response_t>(
                GetProcAddress(h_mod, "?Create@ResourceResponse@Awesomium@@SAPAV12@IPAEABVWebString@2@@Z"));

            loaded = true;
            return api.is_ready() ? &api : nullptr;
        }

        std::string web_string_to_std(
            const awesomium_api& api,
            const web_string_raw& ws)
        {
            const unsigned int len = api.to_utf8(&ws, nullptr, 0);
            if (len == 0)
            {
                return {};
            }
            std::string out(len, '\0');
            api.to_utf8(&ws, out.data(), len);
            return out;
        }

        bool extract_request_host_and_path(
            const awesomium_api& api,
            const void* request,
            std::string& out_host,
            std::string& out_path)
        {
            if (request == nullptr)
            {
                return false;
            }
            const auto* vtable = *reinterpret_cast<void* const* const*>(request);
            if (vtable == nullptr || vtable[3] == nullptr)
            {
                return false;
            }

            const auto fn_req_url = reinterpret_cast<awesomium_api::request_url_t>(vtable[3]);
            scoped_web_url url(api);
            fn_req_url(request, &url.raw);
            url.active = true;

            scoped_web_string host_ws(api);
            api.url_host(&url.raw, &host_ws.raw);
            host_ws.active = true;
            out_host = web_string_to_std(api, host_ws.raw);

            scoped_web_string path_ws(api);
            api.url_path(&url.raw, &path_ws.raw);
            path_ws.active = true;
            out_path = web_string_to_std(api, path_ws.raw);
            return true;
        }

        std::string read_fs_webpath_cvar()
        {
            static const auto fn_find = game::exe_fn<game::cvar_find_var_t>(game::exe::addr_cvar_find);
            if (fn_find == nullptr)
            {
                return {};
            }
            void* const cvar = fn_find("fs_webpath");
            if (cvar == nullptr)
            {
                return {};
            }
            return game::read_cvar_string(cvar, "");
        }

        std::filesystem::path decode_engine_path(const std::string& raw)
        {
            if (raw.empty())
            {
                return {};
            }
            auto try_cp = [&](UINT cp, DWORD flags) -> std::wstring
            {
                const int needed = MultiByteToWideChar(
                    cp, flags, raw.data(), static_cast<int>(raw.size()), nullptr, 0);
                if (needed <= 0)
                {
                    return {};
                }
                std::wstring wide(static_cast<size_t>(needed), L'\0');
                if (MultiByteToWideChar(
                        cp, flags, raw.data(), static_cast<int>(raw.size()), wide.data(), needed) <= 0)
                {
                    return {};
                }
                return wide;
            };
            std::wstring wide = try_cp(CP_UTF8, MB_ERR_INVALID_CHARS);
            if (wide.empty())
            {
                wide = try_cp(CP_ACP, 0);
            }
            if (wide.empty())
            {
                return {};
            }
            return std::filesystem::path(wide);
        }

        std::optional<std::string> get_patched_index_html()
        {
            static std::mutex cache_mtx;
            static std::string cached_webpath{};
            static std::optional<std::string> cached_html{};
            static bool cache_initialized = false;

            const std::string cur_webpath = read_fs_webpath_cvar();

            std::lock_guard<std::mutex> lock(cache_mtx);
            if (cache_initialized && cached_webpath == cur_webpath)
            {
                return cached_html;
            }

            std::optional<std::string> raw_html;
            bool has_custom_index = false;
            const std::filesystem::path decoded_webpath = decode_engine_path(cur_webpath);
            if (!decoded_webpath.empty())
            {
                std::error_code ec;
                const std::filesystem::path custom_index = decoded_webpath / L"index.html";
                if (std::filesystem::is_regular_file(custom_index, ec))
                {
                    has_custom_index = true;
                    raw_html = load_text_file(custom_index);
                }
            }

            if (!has_custom_index)
            {
                const std::filesystem::path exe_dir = game::get_module_dir_dynamic(nullptr);
                if (!exe_dir.empty())
                {
                    raw_html = extract_web_pak_text_resource_from_file(
                        exe_dir / L"web.pak",
                        "index.html");
                }
            }

            cached_webpath = cur_webpath;
            if (raw_html)
            {
                cached_html = inject_overlay_into_index_html(*raw_html);
            }
            else
            {
                cached_html = std::nullopt;
            }
            cache_initialized = true;
            return cached_html;
        }

        void fatal_webui_error(const wchar_t* msg)
        {
            MessageBoxW(nullptr, msg, L"QLQPC", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
            ExitProcess(1);
        }

        void* create_awesomium_response(
            const awesomium_api& api,
            std::string_view payload,
            std::string_view mime_type)
        {
            scoped_web_string mime_ws(api);
            api.create_from_utf8(
                &mime_ws.raw,
                mime_type.data(),
                static_cast<unsigned int>(mime_type.size()));
            mime_ws.active = true;

            // ResourceResponse::Create copies payload into Awesomium-owned storage immediately
            return api.create_response(
                static_cast<unsigned int>(payload.size()),
                const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(payload.data())),
                &mime_ws.raw);
        }

        void* __fastcall hk_ql_resource_interceptor_on_request(
            void* self,
            void*,
            void* request)
        {
            const awesomium_api* api = get_awesomium_api();
            if (api == nullptr)
            {
                fatal_webui_error(L"QLQPC failed to resolve required Awesomium exports.");
            }

            std::string host;
            std::string path;
            if (!extract_request_host_and_path(*api, request, host, path))
            {
                fatal_webui_error(L"QLQPC failed to read WebUI request URL.");
            }

            switch (classify_asset_request(host, path))
            {
            case overlay_asset_kind::index_html:
            {
                const auto patched = get_patched_index_html();
                if (!patched)
                {
                    fatal_webui_error(L"QLQPC failed to patch WebUI index.html.");
                }
                void* resp = create_awesomium_response(*api, *patched, "text/html");
                if (resp == nullptr)
                {
                    fatal_webui_error(L"QLQPC failed to build Awesomium index.html response.");
                }
                return resp;
            }

            case overlay_asset_kind::overlay_js:
            {
                void* resp = create_awesomium_response(
                    *api,
                    overlay_js,
                    "application/javascript");
                if (resp == nullptr)
                {
                    fatal_webui_error(L"QLQPC failed to build Awesomium qlqpc_settings.js response.");
                }
                return resp;
            }

            case overlay_asset_kind::none:
                break;
            }

            return hook_ql_resource_interceptor_on_request.thiscall<void*>(self, request);
        }
    }

    bool install_hooks()
    {
        auto hook = util::create_inline(
            reinterpret_cast<void*>(game::exe_addr(game::exe::addr_ql_resource_interceptor_on_request)),
            reinterpret_cast<void*>(&hk_ql_resource_interceptor_on_request));
        if (!hook)
        {
            return false;
        }
        hook_ql_resource_interceptor_on_request = std::move(hook);
        return true;
    }

    void release_hooks()
    {
        hook_ql_resource_interceptor_on_request.reset();
    }
}
