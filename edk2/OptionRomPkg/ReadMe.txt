For now, AtapiPassThru driver in this package is to test Scsi Bus support:
ScsiBus driver should support both/either ScsiPassThru and ExtScsiPassThru
installed on a controller handle.
 
AtapiPassThru driver in this package can selectively produce ScsiPassThru
and/or ExtScsiPassThru protocol based on feature flags of PcdSupportScsiPassThru
and PcdExtScsiPassThru.

Please note that AtapiPassThru driver has not been tuned to size optimal.
Neither is it feature complete: several driver model protocols required
by option ROM driver, e.g. EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL Protocol. 

Build Validation:
MYTOOLS(VS2005) IA32 X64 IPF EBC
ICC             IA32 X64 IPF
CYGWINGCC       IA32 X64

