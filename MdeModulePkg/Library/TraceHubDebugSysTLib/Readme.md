## Introduction of TrcaceHubDebugSysTLib ##
TrcaceHubDebugSysTLib library is a top level library for dumping Trace Hub messages.
It provides Trace Hub related APIs to dump Trace Hub message via MIPI SYS-T submodule.
User need to properly configure following Trace Hub related PCDs and HOB.
  (See MdeModulePkg.dec to get detailed definition for PCDs below)
  - PcdTraceHubDebugLevel
  - PcdEnableTraceHubDebugMsg
  - PcdTraceHubDebugMmioAddress
  (See TraceHubDebugInfoHob.h to get detailed definition for HOB below)
  - gTraceHubDebugInfoHobGuid

## BaseTraceHubDebugSysTLib.inf ##
System prints Trace Hub message in SEC/PEI/DXE/SMM based on fixed PCDs.
Only support single Trace Hub debug instance.

## PeiTraceHubDebugSysTLib.inf ##
System prints Trace Hub message in PEI based on fixed PCDs and HOB.
System applies Trace Hub HOB once it detect gTraceHubDebugInfoHobGuid HOB.
Trace Hub PCDs will be applied if no HOB exist.

## DxeSmmTraceHubDebugSysTLib.inf ##
System prints Trace Hub message in DXE/SMM based on fixed PCDs and HOB.
Trace Hub PCDs will be applied if no HOB exist.

## Note ##
Trace Hub debug library not support DXE_RUNTIME_DRIVER type of module currently.
