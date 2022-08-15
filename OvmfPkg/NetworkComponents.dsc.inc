##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(NETWORK_ENABLE) == TRUE
  NetworkPkg/UefiPxeBcDxe/UefiPxeBcDxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Library/PxeBcPcdProducerLib/PxeBcPcdProducerLib.inf
  }

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
!endif
