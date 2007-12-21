AtapiPassThru:
  For now, AtapiPassThru driver in this package is to test Scsi Bus support:
  ScsiBus driver should support both/either ScsiPassThru and ExtScsiPassThru
  installed on a controller handle.
   
  AtapiPassThru driver in this package can selectively produce ScsiPassThru
  and/or ExtScsiPassThru protocol based on feature flags of PcdSupportScsiPassThru
  and PcdSupportExtScsiPassThru.

CirrusLogic5430:
  Sample implementation of UGA Draw or Graphics Output Protocol for the Cirrus
  Logic 5430 family of PCI video card. It provides reference code for the GOP/UGA,
  Component Name (2), EFI driver supported Verison protocol.

Build Validation:
MYTOOLS(VS2005) IA32 X64 IPF EBC
ICC             IA32 X64 IPF
CYGWINGCC       IA32 X64

