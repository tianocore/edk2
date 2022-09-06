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
!endif
