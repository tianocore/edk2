## Introduction of TrcaceHubDebugSysTLib ##
TrcaceHubDebugSysTLib library is a upper level library consuming MipiSysTLib library.
It provides Intel Trace Hub related API to do Trace Hub message dump via mipi sys-T interface.
User need to configure following PCDs and HOB properly so that Trace Hub message can be ouput.
  (See MdeModulePkg.dec to get detailed definition for PCDs below)
  - PcdTraceHubDebugLevel
  - PcdEnableTraceHubDebugMsg
  - PcdTraceHubDebugAddress
  (See TraceHubDebugInfoHob.h to get detailed definition for HOB below)
  - gTraceHubDebugInfoHobGuid

## BaseTraceHubDebugSysTLib.inf ##
Consumed by modules in SEC/PEI/DXE/SMM phase based on fixed PCDs.
Only support single debug instance to do Trace Hub message output.

## PeiTraceHubDebugSysTLib.inf ##
Consumed by modules in PEI phase based on fixed PCDs and HOB.
System will apply parameters from Trace Hub HOB once it detect
gTraceHubDebugInfoHobGuid HOB installed.
Multiple Trace Hub HOBs are allowed to output message with multiple
debug channels.
Parameters from PCDs will be applied if no HOB exist.

## DxeSmmTraceHubDebugSysTLib.inf ##
Consumed by modules in DXE/SMM phase based on fixed PCDs and HOB.
This library migrate HOB's data to boot time memory.
Multiple Trace Hub HOBs are allowed to output message with multiple
debug channels.
Parameters from PCDs will be applied if no HOB exist.

## DxeRuntimeTraceHubDebugSysTLib.inf ##
Consumed by modules in Runtime phase based on fixed PCDs and HOB.
This library migrate HOB's data to runtime memory.
Multiple Trace Hub HOBs are allowed to output message with multiple
debug channels.
Parameters from PCDs will be applied if no HOB exist.
