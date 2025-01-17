##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(NETWORK_ENABLE) == TRUE
  !if $(NETWORK_PXE_BOOT_ENABLE) == TRUE
    NetworkPkg/UefiPxeBcDxe/UefiPxeBcDxe.inf {
      <LibraryClasses>
        NULL|OvmfPkg/Library/PxeBcPcdProducerLib/PxeBcPcdProducerLib.inf
    }
  !endif

  !if $(NETWORK_TLS_ENABLE) == TRUE
    NetworkPkg/TlsAuthConfigDxe/TlsAuthConfigDxe.inf {
      <LibraryClasses>
        NULL|OvmfPkg/Library/TlsAuthConfigLib/TlsAuthConfigLib.inf
    }
  !endif

  !if $(NETWORK_IP4_ENABLE) == TRUE
    NetworkPkg/Ip4Dxe/Ip4Dxe.inf {
      <LibraryClasses>
        UefiDriverEntryPoint|OvmfPkg/Library/UefiDriverEntryPointFwCfgOverrideLib/UefiDriverEntryPointFwCfgOverrideLib.inf
      <PcdsFixedAtBuild>
        gUefiOvmfPkgTokenSpaceGuid.PcdEntryPointOverrideFwCfgVarName|"opt/org.tianocore/IPv4Support"
    }
  !endif

  !if $(NETWORK_IP6_ENABLE) == TRUE
    NetworkPkg/Ip6Dxe/Ip6Dxe.inf {
      <LibraryClasses>
        UefiDriverEntryPoint|OvmfPkg/Library/UefiDriverEntryPointFwCfgOverrideLib/UefiDriverEntryPointFwCfgOverrideLib.inf
      <PcdsFixedAtBuild>
        gUefiOvmfPkgTokenSpaceGuid.PcdEntryPointOverrideFwCfgVarName|"opt/org.tianocore/IPv6Support"
    }
  !endif

  OvmfPkg/VirtioNetDxe/VirtioNet.inf {
    <LibraryClasses>
      UefiDriverEntryPoint|OvmfPkg/Library/UefiDriverEntryPointFwCfgOverrideLib/UefiDriverEntryPointFwCfgOverrideLib.inf
    <PcdsFixedAtBuild>
      gUefiOvmfPkgTokenSpaceGuid.PcdEntryPointOverrideFwCfgVarName|"opt/org.tianocore/VirtioNetSupport"
  }

  !if $(NETWORK_ISCSI_ENABLE) == TRUE
    NetworkPkg/IScsiDxe/IScsiDxe.inf {
      <LibraryClasses>
        UefiDriverEntryPoint|OvmfPkg/Library/UefiDriverEntryPointFwCfgOverrideLib/UefiDriverEntryPointFwCfgOverrideLib.inf
      <PcdsFixedAtBuild>
        gUefiOvmfPkgTokenSpaceGuid.PcdEntryPointOverrideFwCfgVarName|"opt/org.tianocore/ISCSISupport"
        gUefiOvmfPkgTokenSpaceGuid.PcdEntryPointOverrideDefaultValue|"no"
    }
  !endif

!endif
