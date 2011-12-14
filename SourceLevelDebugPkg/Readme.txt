UDK based firmware on UEFI IA-32 and UEFI x64 platforms can be debugged with
SourceLevelDebugPkg in conjunction with Intel(R) UEFI Development Kit Debugger
Tool (Intel (R) UDK Debugger Tool).

The Intel(R) UDK Debugger Tool and its detailed user manual can be obtained
from: http://www.intel.com/technology/efi.

NOTE: In addition to the known issues listed in the user manual, the following
anomalies have been observed:

1) When using a USB debug cable, after the TARGET completes a reset during
   memory initialization, the connection between the HOST and the TARGET may be
   lost. A work around for this issue is to unplug the USB debug cable and then
   plug the cable back in. A new debug session can then be started.
