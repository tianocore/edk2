## @file
# Common libraries for Manageabilty Package
#
# Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
# Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
[LibraryClasses]
  ManageabilityTransportHelperLib|ManageabilityPkg/Library/BaseManageabilityTransportHelperLib/BaseManageabilityTransportHelper.inf
  IpmiCommandLib|ManageabilityPkg/Library/IpmiCommandLib/IpmiCommandLib.inf

[LibraryClasses.common.DXE_DRIVER]
  PldmProtocolLib|ManageabilityPkg/Library/PldmProtocolLibrary/Dxe/PldmProtocolLib.inf

[LibraryClasses.AARCH64]
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf

[LibraryClasses.AARCH64.PEIM]
  PeiServicesTablePointerLib|ArmPkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf

[Components.IA32, Components.AARCH64]
!if gManageabilityPkgTokenSpaceGuid.PcdManageabilityPeiIpmiEnable == TRUE
  ManageabilityPkg/Universal/IpmiProtocol/Pei/IpmiPpiPei.inf
!endif

!if gManageabilityPkgTokenSpaceGuid.PcdManageabilityPeiIpmiFrb == TRUE
  ManageabilityPkg/Universal/IpmiFrb/FrbPei.inf
!endif

[Components.X64, Components.AARCH64]
!if gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiEnable == TRUE
  ManageabilityPkg/Universal/IpmiProtocol/Dxe/IpmiProtocolDxe.inf
  ManageabilityPkg/Universal/IpmiBlobTransferDxe/IpmiBlobTransferDxe.inf
!endif

!if gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiSmbiosTransferEnable == TRUE
  ManageabilityPkg/Universal/IpmiSmbiosTransferDxe/IpmiSmbiosTransferDxe.inf
!endif

[Components.X64]
!if gManageabilityPkgTokenSpaceGuid.PcdManageabilitySmmIpmiEnable == TRUE
  ManageabilityPkg/Universal/IpmiProtocol/Smm/IpmiProtocolSmm.inf
!endif

!if gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxePldmEnable == TRUE
  ManageabilityPkg/Universal/PldmProtocol/Dxe/PldmProtocolDxe.inf
!endif

!if gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxePldmSmbiosTransferEnable == TRUE
  ManageabilityPkg/Universal/PldmSmbiosTransferDxe/PldmSmbiosTransferDxe.inf
!endif

!if gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeMctpEnable == TRUE
  ManageabilityPkg/Universal/MctpProtocol/Dxe/MctpProtocolDxe.inf
!endif

!if gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiFru == TRUE
  ManageabilityPkg/Universal/IpmiFru/IpmiFru.inf
!endif

!if gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiOsWdt == TRUE
  ManageabilityPkg/Universal/IpmiOsWdt/OsWdt.inf
!endif

!if gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiSolStatus == TRUE
  ManageabilityPkg/Universal/IpmiSolStatus/SolStatus.inf
!endif

!if gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiBmcElog == TRUE
  ManageabilityPkg/Universal/IpmiBmcElog/BmcElog.inf
!endif

!if gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiFrb == TRUE
  ManageabilityPkg/Universal/IpmiFrb/FrbDxe.inf
!endif

!if gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiBmcAcpi == TRUE
  ManageabilityPkg/Universal/IpmiBmcAcpi/BmcAcpi.inf
!endif
