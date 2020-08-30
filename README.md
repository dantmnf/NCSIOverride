# NCSIOverride

Does Windows block some functionality and keep saying you have no Internet, while you are searching for this?

JUST SAY FUCK OFF TO THAT BULLSHIT INDICATOR. THIS SHOULD NOT BE THE VEDAS TO KEEP USER FROM USING ANY FEATURE.

THE INDICATOR DOESN’T TEST YOUR SERVICE AT ALL. INDICATOR SAYS OK DOESN’T MEAN USER CAN CONNECT TO YOUR SERVICE.

## Installation

Download from GitHub Actions artifacts: [![build](https://github.com/dantmnf/NCSIOverride/workflows/build/badge.svg)](https://github.com/dantmnf/NCSIOverride/actions?query=is%3Asuccess+workflow%3Abuild)[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fdantmnf%2FNCSIOverride.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Fdantmnf%2FNCSIOverride?ref=badge_shield)

1. Copy NlaSvc2.dll into system32
2. Import install.reg

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


The function offset can be obtained using WinDbg:

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


## License
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fdantmnf%2FNCSIOverride.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Fdantmnf%2FNCSIOverride?ref=badge_large)