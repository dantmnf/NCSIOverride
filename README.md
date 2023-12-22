# NCSIOverride

> [!CAUTION]
> NCSI has changed a lot since my last update and this hook no longer works with latest Windows releases, see details in https://github.com/dantmnf/NCSIOverride/issues/2#issuecomment-1362433801 (in Chinese)
> 
> Now that NCSI just works on my machine(TM) so I have no plan to update it. Feel free to do it yourself.

Does Windows block some functionality and keep saying you have no Internet, while you are searching for this?

JUST SAY FUCK OFF TO THAT BULLSHIT INDICATOR. THIS SHOULD NOT BE THE VEDAS TO KEEP USER FROM USING ANY FEATURE.

THE INDICATOR DOESN’T TEST YOUR SERVICE AT ALL. INDICATOR SAYS OK DOESN’T MEAN USER CAN CONNECT TO YOUR SERVICE.

## Notes

* Npcap (bundled with Wireshark/Nmap) is known to break NCSI.
* You can trigger a NCSI reprobe by disabling and enabling a random network adapter.

## Installation

Download from [Releases](https://github.com/dantmnf/NCSIOverride/releases) or GitHub Actions artifacts: [![build](https://github.com/dantmnf/NCSIOverride/workflows/build/badge.svg)](https://github.com/dantmnf/NCSIOverride/actions?query=is%3Asuccess+workflow%3Abuild)
1. Copy NlaSvc2.dll into system32
2. Import `install.reg`

## Configuration

Configuartion is stored in `HKLM\SYSTEM\CurrentControlSet\Services\NlaSvc\Parameters\Internet\NCSIOverride`, see install.reg for example

    NCSIOverride
    | # set default override for all interfaces here
    | DefaultOverrideV4 REG_DWORD
    | DefaultOverrideV6 REG_DWORD
    +-InterfaceOverride
    | +-{INTERFACE-GUID}
    |   | # set override for interface with specified GUID here
    |   | # this takes higher precedence than DefaultOverride
    |   | OverrideV4      REG_DWORD
    |   \ OverrideV6      REG_DWORD
    +-Offsets
      | # the function offset in ncsi.dll
      | # will stop working if the value is outdated
      \ NCSI_INTERFACE_ATTRIBUTES_SetCapability REG_QWORD


Values for DefaultOverrideV4, DefaultOverrideV6, OverrideV4, OverrideV6:
* 0: None
* 1: Local
* 2: Internet
* 3: Max


`{INTERFACE-GUID}` can be obtained by:

    PS C:\> Get-NetAdapter | select Name, InterfaceGuid

    Name       InterfaceGuid                         
    ----       -------------                         
    Ethernet 2 {640470cf-5b79-4df2-b462-5648463881d9}
    Wi-Fi      {4efa6faf-9a7c-47bc-8179-6dc85adc9a59}


With [Debugging Tools for Windows](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/debugger-download-tools) installed, the function offset can be updated by running [`Update-Offset.ps1`](Update-Offset.ps1) as administrator, or manually obtained using WinDbg:

    C:\Program Files (x86)\Windows Kits\10\Debuggers\x64>windbg rundll32.exe ncsi.dll,NcsiPerformReprobe
    # in WinDbg command window
    0:000> sxe ld:ncsi.dll
    0:000> g
    0:000> ? ncsi!NCSI_INTERFACE_ATTRIBUTES::SetCapability - ncsi
    Evaluate expression: 107868 = 00000000`0001a55c

## Building

You need MSVC or Clang with MSVC ABI to build this (thanks Detours).

## Notes

* Current implementation sets the override passively by changing parameter in function call. You need to trigger a reprobe to reflect changed override value.
* This can’t override physically disconnected interface. 
