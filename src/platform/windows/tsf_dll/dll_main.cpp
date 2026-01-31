#include "tsf_text_service.h"
#include "logger.h"
#include <msctf.h>
#include <olectl.h>
#include <strsafe.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")

namespace suyan {
HINSTANCE g_hInstance = nullptr;
}

namespace {

constexpr wchar_t TSF_CLSID_KEY[] = L"CLSID\\{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}";
constexpr wchar_t TSF_INPROC_KEY[] = L"CLSID\\{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}\\InprocServer32";
constexpr wchar_t TSF_DISPLAY_NAME[] = L"素言输入法";
constexpr ULONG TSF_ICON_INDEX = 0;

HRESULT registerServer() {
    wchar_t modulePath[MAX_PATH];
    if (GetModuleFileNameW(suyan::g_hInstance, modulePath, MAX_PATH) == 0) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    HKEY hKey;
    LONG result = RegCreateKeyExW(HKEY_CLASSES_ROOT, TSF_CLSID_KEY, 0, nullptr,
                                   REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(result);
    }

    result = RegSetValueExW(hKey, nullptr, 0, REG_SZ,
                            reinterpret_cast<const BYTE*>(TSF_DISPLAY_NAME),
                            static_cast<DWORD>((wcslen(TSF_DISPLAY_NAME) + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);
    if (result != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(result);
    }

    result = RegCreateKeyExW(HKEY_CLASSES_ROOT, TSF_INPROC_KEY, 0, nullptr,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(result);
    }

    result = RegSetValueExW(hKey, nullptr, 0, REG_SZ,
                            reinterpret_cast<const BYTE*>(modulePath),
                            static_cast<DWORD>((wcslen(modulePath) + 1) * sizeof(wchar_t)));
    if (result == ERROR_SUCCESS) {
        const wchar_t* threadingModel = L"Apartment";
        result = RegSetValueExW(hKey, L"ThreadingModel", 0, REG_SZ,
                                reinterpret_cast<const BYTE*>(threadingModel),
                                static_cast<DWORD>((wcslen(threadingModel) + 1) * sizeof(wchar_t)));
    }
    RegCloseKey(hKey);
    if (result != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(result);
    }

    return S_OK;
}

HRESULT unregisterServer() {
    RegDeleteTreeW(HKEY_CLASSES_ROOT, TSF_CLSID_KEY);
    return S_OK;
}

HRESULT registerTextService() {
    ITfInputProcessorProfileMgr* profileMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_ITfInputProcessorProfileMgr, reinterpret_cast<void**>(&profileMgr));
    if (FAILED(hr)) {
        return hr;
    }

    profileMgr->UnregisterProfile(
        suyan::CLSID_SuYanTextService,
        MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
        suyan::GUID_SuYanProfile,
        0
    );

    wchar_t modulePath[MAX_PATH];
    DWORD pathLen = GetModuleFileNameW(suyan::g_hInstance, modulePath, MAX_PATH);

    hr = profileMgr->RegisterProfile(
        suyan::CLSID_SuYanTextService,
        MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
        suyan::GUID_SuYanProfile,
        TSF_DISPLAY_NAME,
        static_cast<ULONG>(wcslen(TSF_DISPLAY_NAME)),
        modulePath,
        pathLen,
        TSF_ICON_INDEX,
        nullptr,
        0,
        TRUE,
        0
    );

    profileMgr->Release();
    return hr;
}

HRESULT unregisterTextService() {
    ITfInputProcessorProfileMgr* profileMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_ITfInputProcessorProfileMgr, reinterpret_cast<void**>(&profileMgr));
    if (FAILED(hr)) {
        return hr;
    }

    hr = profileMgr->UnregisterProfile(
        suyan::CLSID_SuYanTextService,
        MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
        suyan::GUID_SuYanProfile,
        0
    );

    profileMgr->Release();
    return hr;
}

HRESULT registerCategories() {
    ITfCategoryMgr* categoryMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_ITfCategoryMgr, reinterpret_cast<void**>(&categoryMgr));
    if (FAILED(hr)) {
        suyan::log::error("registerCategories: CoCreateInstance failed, hr=0x%08lx", hr);
        return hr;
    }

    hr = categoryMgr->RegisterCategory(suyan::CLSID_SuYanTextService,
                                        GUID_TFCAT_TIP_KEYBOARD,
                                        suyan::CLSID_SuYanTextService);
    suyan::log::debug("registerCategories: GUID_TFCAT_TIP_KEYBOARD hr=0x%08lx", hr);

    HRESULT hr2 = categoryMgr->RegisterCategory(suyan::CLSID_SuYanTextService,
                                                 GUID_TFCAT_TIPCAP_UIELEMENTENABLED,
                                                 suyan::CLSID_SuYanTextService);
    suyan::log::debug("registerCategories: GUID_TFCAT_TIPCAP_UIELEMENTENABLED hr=0x%08lx", hr2);

    hr2 = categoryMgr->RegisterCategory(suyan::CLSID_SuYanTextService,
                                         GUID_TFCAT_TIPCAP_SECUREMODE,
                                         suyan::CLSID_SuYanTextService);
    suyan::log::debug("registerCategories: GUID_TFCAT_TIPCAP_SECUREMODE hr=0x%08lx", hr2);

    hr2 = categoryMgr->RegisterCategory(suyan::CLSID_SuYanTextService,
                                         GUID_TFCAT_TIPCAP_COMLESS,
                                         suyan::CLSID_SuYanTextService);
    suyan::log::debug("registerCategories: GUID_TFCAT_TIPCAP_COMLESS hr=0x%08lx", hr2);

    hr2 = categoryMgr->RegisterCategory(suyan::CLSID_SuYanTextService,
                                         GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT,
                                         suyan::CLSID_SuYanTextService);
    suyan::log::debug("registerCategories: GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT hr=0x%08lx", hr2);

    hr2 = categoryMgr->RegisterCategory(suyan::CLSID_SuYanTextService,
                                         GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
                                         suyan::CLSID_SuYanTextService);
    suyan::log::debug("registerCategories: GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT hr=0x%08lx", hr2);

    hr2 = categoryMgr->RegisterCategory(suyan::CLSID_SuYanTextService,
                                         GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
                                         suyan::CLSID_SuYanTextService);
    suyan::log::debug("registerCategories: GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT hr=0x%08lx", hr2);

    categoryMgr->Release();
    return hr;
}

HRESULT unregisterCategories() {
    ITfCategoryMgr* categoryMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_ITfCategoryMgr, reinterpret_cast<void**>(&categoryMgr));
    if (FAILED(hr)) {
        return hr;
    }

    categoryMgr->UnregisterCategory(suyan::CLSID_SuYanTextService,
                                     GUID_TFCAT_TIP_KEYBOARD,
                                     suyan::CLSID_SuYanTextService);
    categoryMgr->UnregisterCategory(suyan::CLSID_SuYanTextService,
                                     GUID_TFCAT_TIPCAP_UIELEMENTENABLED,
                                     suyan::CLSID_SuYanTextService);
    categoryMgr->UnregisterCategory(suyan::CLSID_SuYanTextService,
                                     GUID_TFCAT_TIPCAP_SECUREMODE,
                                     suyan::CLSID_SuYanTextService);
    categoryMgr->UnregisterCategory(suyan::CLSID_SuYanTextService,
                                     GUID_TFCAT_TIPCAP_COMLESS,
                                     suyan::CLSID_SuYanTextService);
    categoryMgr->UnregisterCategory(suyan::CLSID_SuYanTextService,
                                     GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT,
                                     suyan::CLSID_SuYanTextService);
    categoryMgr->UnregisterCategory(suyan::CLSID_SuYanTextService,
                                     GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
                                     suyan::CLSID_SuYanTextService);
    categoryMgr->UnregisterCategory(suyan::CLSID_SuYanTextService,
                                     GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
                                     suyan::CLSID_SuYanTextService);

    categoryMgr->Release();
    return S_OK;
}

}  // namespace

extern "C" {

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
    (void)lpReserved;

    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            suyan::g_hInstance = hInstance;
            suyan::log::initialize(L"tsf_dll");
            suyan::log::info("DllMain: DLL_PROCESS_ATTACH");
            DisableThreadLibraryCalls(hInstance);
            break;

        case DLL_PROCESS_DETACH:
            suyan::log::info("DllMain: DLL_PROCESS_DETACH");
            suyan::log::shutdown();
            break;
    }

    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
    if (!ppv) {
        return E_INVALIDARG;
    }

    *ppv = nullptr;

    if (!IsEqualCLSID(rclsid, suyan::CLSID_SuYanTextService)) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    return suyan::g_classFactory.QueryInterface(riid, ppv);
}

STDAPI DllCanUnloadNow() {
    return (suyan::g_dllRefCount == 0) ? S_OK : S_FALSE;
}

STDAPI DllRegisterServer() {
    suyan::log::info("DllRegisterServer: Starting registration");

    HRESULT hr = registerServer();
    if (FAILED(hr)) {
        suyan::log::error("DllRegisterServer: registerServer failed, hr=0x%08lx", hr);
        return hr;
    }

    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool comInitialized = SUCCEEDED(hr);

    hr = registerTextService();
    if (FAILED(hr)) {
        suyan::log::error("DllRegisterServer: registerTextService failed, hr=0x%08lx", hr);
        if (comInitialized) CoUninitialize();
        return hr;
    }

    hr = registerCategories();
    if (FAILED(hr)) {
        suyan::log::error("DllRegisterServer: registerCategories failed, hr=0x%08lx", hr);
    }

    if (comInitialized) CoUninitialize();

    suyan::log::info("DllRegisterServer: Registration complete");
    return S_OK;
}

STDAPI DllUnregisterServer() {
    suyan::log::info("DllUnregisterServer: Starting unregistration");

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool comInitialized = SUCCEEDED(hr);

    unregisterCategories();
    unregisterTextService();
    unregisterServer();

    if (comInitialized) CoUninitialize();

    suyan::log::info("DllUnregisterServer: Unregistration complete");
    return S_OK;
}

}  // extern "C"
