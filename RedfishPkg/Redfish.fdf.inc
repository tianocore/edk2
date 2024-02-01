## @file
# Redfish FDF include file for [FV*] section of all Architectures.
#
# This file can be included to the [FV*] section(s) of a platform FDF file
# by using "!include RedfishPkg/Redfish.fdf.inc" to specify the module instances
# to be built in the firmware volume.
#
# (C) Copyright 2020-2021 Hewlett Packard Enterprise Development LP<BR>
# Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
!if $(REDFISH_ENABLE) == TRUE
  INF RedfishPkg/RestJsonStructureDxe/RestJsonStructureDxe.inf
  INF RedfishPkg/RedfishHostInterfaceDxe/RedfishHostInterfaceDxe.inf
  INF RedfishPkg/RedfishRestExDxe/RedfishRestExDxe.inf
  INF RedfishPkg/RedfishCredentialDxe/RedfishCredentialDxe.inf
  INF RedfishPkg/RedfishDiscoverDxe/RedfishDiscoverDxe.inf
  INF RedfishPkg/RedfishConfigHandler/RedfishConfigHandlerDriver.inf
  INF RedfishPkg/RedfishPlatformConfigDxe/RedfishPlatformConfigDxe.inf
  INF MdeModulePkg/Universal/RegularExpressionDxe/RegularExpressionDxe.inf
  INF RedfishPkg/RedfishHttpDxe/RedfishHttpDxe.inf
!endif
