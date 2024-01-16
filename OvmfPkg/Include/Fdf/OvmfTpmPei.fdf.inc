##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(TPM2_ENABLE) == TRUE
INF  OvmfPkg/Tcg/TpmMmioSevDecryptPei/TpmMmioSevDecryptPei.inf
INF  OvmfPkg/Tcg/Tcg2Config/Tcg2ConfigPei.inf
INF  SecurityPkg/Tcg/Tcg2Pei/Tcg2Pei.inf
INF  SecurityPkg/Tcg/Tcg2PlatformPei/Tcg2PlatformPei.inf
!endif
