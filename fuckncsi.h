#pragma once
#include <windows.h>
#ifdef __cplusplus
extern "C" {
struct NCSI_INTERFACE_ATTRIBUTES;
#else
typedef void* NCSI_INTERFACE_ATTRIBUTES;
#endif // __cplusplus

void NCSIOverrideAttach(HMODULE hDll);

#ifdef __cplusplus
}
#endif // __cplusplus
