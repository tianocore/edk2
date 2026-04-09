##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(TPM2_ENABLE) == TRUE
!if $(TPM1_ENABLE) == TRUE
INF  SecurityPkg/Tcg/TcgDxe/TcgDxe.inf
!endif
INF  SecurityPkg/Tcg/Tcg2Dxe/Tcg2Dxe.inf
INF  SecurityPkg/Tcg/Tcg2PlatformDxe/Tcg2PlatformDxe.inf
INF  SecurityPkg/Tcg/Tcg2Config/Tcg2ConfigDxe.inf
!endif
