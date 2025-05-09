##
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# optional hardware support components
##

!if $(PVSCSI_ENABLE) == TRUE
INF  OvmfPkg/PvScsiDxe/PvScsiDxe.inf
!endif
!if $(MPT_SCSI_ENABLE) == TRUE
INF  OvmfPkg/MptScsiDxe/MptScsiDxe.inf
!endif
!if $(LSI_SCSI_ENABLE) == TRUE
INF  OvmfPkg/LsiScsiDxe/LsiScsiDxe.inf
!endif
