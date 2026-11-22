#include <windows.h>
#include <unknwn.h>

#include <filesystem>
#include <string>

namespace qlqpc::proxy
{
    namespace
    {
        using direct_input8_create_t = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
        using dll_can_unload_now_t = HRESULT(WINAPI*)();
        using dll_get_class_object_t = HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*);
        using dll_register_server_t = HRESULT(WINAPI*)();
        using dll_unregister_server_t = HRESULT(WINAPI*)();
        using get_df_di_joystick_t = const void*(WINAPI*)();

        HMODULE real_dinput8 = nullptr;
        INIT_ONCE dinput8_once = INIT_ONCE_STATIC_INIT;

        direct_input8_create_t real_DirectInput8Create = nullptr;
        dll_can_unload_now_t real_DllCanUnloadNow = nullptr;
        dll_get_class_object_t real_DllGetClassObject = nullptr;
        dll_register_server_t real_DllRegisterServer = nullptr;
        dll_unregister_server_t real_DllUnregisterServer = nullptr;
        get_df_di_joystick_t real_GetdfDIJoystick = nullptr;

        BOOL CALLBACK init_real_dinput8_once(PINIT_ONCE, PVOID, PVOID*)
        {
            std::wstring sys_dir(MAX_PATH, L'\0');
            for (;;)
            {
                const UINT len = GetSystemDirectoryW(sys_dir.data(), static_cast<UINT>(sys_dir.size()));
                if (len == 0)
                {
                    return TRUE;
                }
                if (len < sys_dir.size())
                {
                    sys_dir.resize(len);
                    break;
                }
                sys_dir.resize(len + 1u);
            }

            const std::filesystem::path real_path = std::filesystem::path(sys_dir) / L"dinput8.dll";
            real_dinput8 = LoadLibraryW(real_path.c_str());
            if (real_dinput8 != nullptr)
            {
                real_DirectInput8Create = reinterpret_cast<direct_input8_create_t>(
                    GetProcAddress(real_dinput8, "DirectInput8Create"));
                real_DllCanUnloadNow = reinterpret_cast<dll_can_unload_now_t>(
                    GetProcAddress(real_dinput8, "DllCanUnloadNow"));
                real_DllGetClassObject = reinterpret_cast<dll_get_class_object_t>(
                    GetProcAddress(real_dinput8, "DllGetClassObject"));
                real_DllRegisterServer = reinterpret_cast<dll_register_server_t>(
                    GetProcAddress(real_dinput8, "DllRegisterServer"));
                real_DllUnregisterServer = reinterpret_cast<dll_unregister_server_t>(
                    GetProcAddress(real_dinput8, "DllUnregisterServer"));
                real_GetdfDIJoystick = reinterpret_cast<get_df_di_joystick_t>(
                    GetProcAddress(real_dinput8, "GetdfDIJoystick"));
            }
            return TRUE;
        }

        void ensure_real_dinput8()
        {
            InitOnceExecuteOnce(&dinput8_once, &init_real_dinput8_once, nullptr, nullptr);
        }
    }
}

extern "C"
{
    HRESULT WINAPI proxy_DirectInput8Create(
        HINSTANCE hinst,
        DWORD dw_version,
        REFIID riidltf,
        LPVOID* ppv_out,
        LPUNKNOWN punk_outer)
    {
        if (ppv_out != nullptr)
        {
            *ppv_out = nullptr;
        }
        qlqpc::proxy::ensure_real_dinput8();
        if (qlqpc::proxy::real_DirectInput8Create == nullptr)
        {
            return E_FAIL;
        }
        return qlqpc::proxy::real_DirectInput8Create(hinst, dw_version, riidltf, ppv_out, punk_outer);
    }

    HRESULT WINAPI proxy_DllCanUnloadNow()
    {
        qlqpc::proxy::ensure_real_dinput8();
        if (qlqpc::proxy::real_DllCanUnloadNow == nullptr)
        {
            return S_FALSE;
        }
        return qlqpc::proxy::real_DllCanUnloadNow();
    }

    HRESULT WINAPI proxy_DllGetClassObject(
        REFCLSID rclsid,
        REFIID riid,
        LPVOID* ppv)
    {
        if (ppv != nullptr)
        {
            *ppv = nullptr;
        }
        qlqpc::proxy::ensure_real_dinput8();
        if (qlqpc::proxy::real_DllGetClassObject == nullptr)
        {
            return CLASS_E_CLASSNOTAVAILABLE;
        }
        return qlqpc::proxy::real_DllGetClassObject(rclsid, riid, ppv);
    }

    HRESULT WINAPI proxy_DllRegisterServer()
    {
        qlqpc::proxy::ensure_real_dinput8();
        if (qlqpc::proxy::real_DllRegisterServer == nullptr)
        {
            return E_FAIL;
        }
        return qlqpc::proxy::real_DllRegisterServer();
    }

    HRESULT WINAPI proxy_DllUnregisterServer()
    {
        qlqpc::proxy::ensure_real_dinput8();
        if (qlqpc::proxy::real_DllUnregisterServer == nullptr)
        {
            return E_FAIL;
        }
        return qlqpc::proxy::real_DllUnregisterServer();
    }

    const void* WINAPI proxy_GetdfDIJoystick()
    {
        qlqpc::proxy::ensure_real_dinput8();
        if (qlqpc::proxy::real_GetdfDIJoystick == nullptr)
        {
            return nullptr;
        }
        return qlqpc::proxy::real_GetdfDIJoystick();
    }
}
