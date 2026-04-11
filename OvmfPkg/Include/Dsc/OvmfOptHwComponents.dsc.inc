##
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# optional hardware support components
##

!if $(PVSCSI_ENABLE) == TRUE
  OvmfPkg/PvScsiDxe/PvScsiDxe.inf
!endif
!if $(MPT_SCSI_ENABLE) == TRUE
  OvmfPkg/MptScsiDxe/MptScsiDxe.inf
!endif
!if $(LSI_SCSI_ENABLE) == TRUE
  OvmfPkg/LsiScsiDxe/LsiScsiDxe.inf
!endif
!if $(SDCARD_ENABLE) == TRUE
  MdeModulePkg/Bus/Pci/SdMmcPciHcDxe/SdMmcPciHcDxe.inf {
    <LibraryClasses>
      UefiDriverEntryPoint|OvmfPkg/Library/UefiDriverEntryPointFwCfgOverrideLib/UefiDriverEntryPointFwCfgOverrideLib.inf
    <PcdsFixedAtBuild>
      gUefiOvmfPkgTokenSpaceGuid.PcdEntryPointOverrideFwCfgVarName|"opt/org.tianocore/SDCardSupport"
  }
  MdeModulePkg/Bus/Sd/SdDxe/SdDxe.inf
  MdeModulePkg/Bus/Sd/EmmcDxe/EmmcDxe.inf
!endif
