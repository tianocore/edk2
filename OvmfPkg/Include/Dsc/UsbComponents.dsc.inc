##
# USB Support
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf {
    <LibraryClasses>
      UefiDriverEntryPoint|OvmfPkg/Library/UefiDriverEntryPointFwCfgOverrideLib/UefiDriverEntryPointFwCfgOverrideLib.inf
    <PcdsFixedAtBuild>
      gUefiOvmfPkgTokenSpaceGuid.PcdEntryPointOverrideFwCfgVarName|"opt/org.tianocore/UsbStorageSupport"
  }
