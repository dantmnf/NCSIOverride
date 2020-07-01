
#include <string>
#include <string_view>
#include <optional>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <detours.h>
#include <strsafe.h>

#include "fuckncsi.h"

using namespace std::literals;

BOOL WINAPI DllMain(HINSTANCE hMod, DWORD fdwReason, LPVOID lpvReserved) {
    if (DetourIsHelperProcess()) return TRUE;
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


#ifdef _WIN64
#define REG_TYPE_PTR REG_QWORD
#else
#define REG_TYPE_PTR REG_DWORD
#endif

static std::optional<uintptr_t> GetSymbolOffset(std::string_view name) {
    HKEY hk;
    auto ls = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet\\NCSIOverride\\Offsets", 0, KEY_READ, &hk);
    if (ls != 0) return {};
    DWORD type;
    uintptr_t offset;
    DWORD cb = sizeof(offset);
    ls = RegQueryValueExW(hk, L"NCSI_INTERFACE_ATTRIBUTES_SetCapability", nullptr, &type, reinterpret_cast<LPBYTE>(&offset), &cb);
    RegCloseKey(hk);
    if (ls != 0) return {};
    if (type != REG_TYPE_PTR) return {};
    return offset;
}

static constexpr auto NCSI_INTERFACE_ATTRIBUTES_SetCapability = "?SetCapability@NCSI_INTERFACE_ATTRIBUTES@@AEAAXW4_NLA_CONNECTIVITY_FAMILY@@W4_CONNECTIVITY_CAPABILITY@@W4_NLA_CAPABILITY_CHANGE_REASON@@@Z"sv;

static std::optional<void*> GetSymbolAddress(HMODULE base, std::string_view name) {
    auto offset = GetSymbolOffset(NCSI_INTERFACE_ATTRIBUTES_SetCapability);
    if (!offset) {
        OutputDebugStringA("GetSymbolOffset failed");
        return {};
    }
    auto ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(base) + offset.value());
    return ptr;  // 0x0001A55C
}

std::wstring GetInterfaceGUIDString(const NCSI_INTERFACE_ATTRIBUTES *attributes) {
    return std::wstring(reinterpret_cast<const wchar_t*>(reinterpret_cast<const uint8_t*>(attributes) + 72), 38);
}

GUID GetInterfaceGUID(const NCSI_INTERFACE_ATTRIBUTES *attributes) {
    auto guidstr = GetInterfaceGUIDString(attributes);
    GUID result{};
    if (SUCCEEDED(CLSIDFromString(guidstr.c_str(), &result))) {
        return result;
    } else {
        return GUID{};
    }
}

static void MySetCapability(NCSI_INTERFACE_ATTRIBUTES *attributes, int family, int cap, int reason);
static decltype(MySetCapability) *realSetCapability;

#define FORMAT_GUID "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"
#define GUID_ARG(guid) guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]

static std::optional<int> GetDefaultCapabilityOverride(int family) {
    HKEY hk;
    auto ls = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet\\NCSIOverride", 0, KEY_READ, &hk);
    if (ls != 0) {
        return {};
    }
    DWORD regtype;
    int result;
    DWORD buflen = 4;
    ls = RegQueryValueExW(hk, family == 0 ? L"DefaultOverrideV4" : L"DefaultOverrideV6", nullptr, &regtype, reinterpret_cast<LPBYTE>(&result), &buflen);
    RegCloseKey(hk);
    if (ls == 0 && regtype == REG_DWORD) {
        return result;
    }
    return {};
}

static std::optional<int> GetCapabilityForInterface(const GUID& interfaceGuid, int family) {
    wchar_t subkey[256];
    StringCchPrintfW(subkey, 256, L"SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet\\NCSIOverride\\InterfaceOverride\\" FORMAT_GUID, GUID_ARG(interfaceGuid));
    HKEY hk;
    auto ls = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ, &hk);
    if (ls != 0) {
        return {};
    }
    DWORD regtype;
    int result;
    DWORD buflen = 4;
    ls = RegQueryValueExW(hk, family == 0 ? L"OverrideV4" : L"OverrideV6", nullptr, &regtype, reinterpret_cast<LPBYTE>(&result), &buflen);
    RegCloseKey(hk);
    if (ls == 0 && regtype == REG_DWORD) {
        return result;
    }
    return {};
}

static std::optional<int> GetResultantCapabilityOverride(const GUID &interfaceGuid, int family) {
    auto ifcap = GetCapabilityForInterface(interfaceGuid, family);
    if (ifcap) return ifcap;
    auto defaultcap = GetDefaultCapabilityOverride(family);
    if (defaultcap) return defaultcap;
    return {};
}

static void MySetCapability(NCSI_INTERFACE_ATTRIBUTES *attributes, int family, int cap, int reason) {
    char buf[256];
    auto ifguid = GetInterfaceGUID(attributes);
    auto origcap = cap;
    auto optcap = GetResultantCapabilityOverride(ifguid, family);
    if (optcap)
    {
        cap = *optcap;
        StringCbPrintfA(buf, 512, "NCSI_INTERFACE_ATTRIBUTES::SetCapability(%p = " FORMAT_GUID ", %d, %d -> %d, %d)", attributes, GUID_ARG(ifguid), family, origcap, cap, reason);
    }
    else {
        StringCbPrintfA(buf, 512, "NCSI_INTERFACE_ATTRIBUTES::SetCapability(%p = " FORMAT_GUID ", %d, %d, %d)", attributes, GUID_ARG(ifguid), family, cap, reason);
    }
    OutputDebugStringA(buf);
    realSetCapability(attributes, family, cap, reason);
}

extern "C" void NCSIOverrideAttach(HMODULE hDll) {
    auto optSetCapability = GetSymbolAddress(hDll, NCSI_INTERFACE_ATTRIBUTES_SetCapability);
    if (!optSetCapability) return;
    realSetCapability = reinterpret_cast<decltype(realSetCapability)>(optSetCapability.value());
    if (*reinterpret_cast<uint16_t*>(realSetCapability) != 0x5540) {  // check for "push rbp" prolog
        OutputDebugStringA("NCSI_INTERFACE_ATTRIBUTES::SetCapability prolog check failed.");
        return;
    }
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(reinterpret_cast<void**>(&realSetCapability), reinterpret_cast<void*>(MySetCapability));
    DetourTransactionCommit();
}