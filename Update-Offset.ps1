Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$output = @"
* enable loading symbol from Microsoft symbol server
.sympath srv*
* set breakpoint on loading ncsi.dll
sxe ld:ncsi.dll
* run debuggee and wait for the module loading breakpoint
g
* resolve function offset from symbol
? ncsi!NCSI_INTERFACE_ATTRIBUTES::SetCapability - ncsi
* exit debugger
q
"@ | & "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe" "rundll32.exe" "ncsi.dll,NcsiPerformReprobe" | Select-String -Pattern "Evaluate expression: (\d+)"

$offset = [long]$output.Matches[0].Groups[1].Value

Write-Output ("Function offset: 0x{0:x}" -f $offset)

if ([System.Environment]::Is64BitOperatingSystem) {
  $type = [Microsoft.Win32.RegistryValueKind]::QWord
} else {
  $type = [Microsoft.Win32.RegistryValueKind]::Dword
}

[Microsoft.Win32.Registry]::SetValue("HKWY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\NlaSvc\Parameters\Internet\NCSIOverride\Offsets\NCSI_INTERFACE_ATTRIBUTES_SetCapability", "NCSI_INTERFACE_ATTRIBUTES_SetCapability", $offset, $type)
Write-Host "Successfully set the offset value in registry."
