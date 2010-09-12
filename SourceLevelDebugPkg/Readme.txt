UDK based firmware on UEFI IA-32 and UEFI x64 platforms may be debugged using Microsoft(R) Debugging Tools for Windows(R) (WinDbg). Debug capability is enabled with SourceLevelDebugPkg in conjunction with the Intel(R) UEFI Development Kit Debugger Tool (Intel(R) UDK Debugger Tool).

The Intel(R) UDK Debugger Tool and its detailed user manual may be obtained from:
http://www.intel.com/technology/efi. 

NOTE: In addition to the known issues listed in the user manual, the following anomalies have been observed:

1) When using a USB debug cable, after the TARGET completes a reset during memory initialization, the connection between the HOST and the TARGET may be lost (e.g. WinDbg reports busy status and does not respond to a break request). A work around for this issue is to unplug the USB debug cable and then plug the cable back in. A new debug session may then be started.