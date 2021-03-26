## @file
# Redfish DSC include file for [Components*] section of all Architectures.
#
# This file can be included to the [Components*] section(s) of a platform DSC file
# by using "!include RedfishPkg/RedfishComponents.dsc.inc" to specify the INF files
# of EDKII Redfish drivers according to the value of flags described in
# "RedfishDefines.dsc.inc".
#
# (C) Copyright 2020-2021 Hewlett Packard Enterprise Development LP<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

!if $(REDFISH_ENABLE) == TRUE
  RedfishPkg/RestJsonStructureDxe/RestJsonStructureDxe.inf
  RedfishPkg/RedfishHostInterfaceDxe/RedfishHostInterfaceDxe.inf
  RedfishPkg/RedfishRestExDxe/RedfishRestExDxe.inf
  RedfishPkg/RedfishCredentialDxe/RedfishCredentialDxe.inf
  RedfishPkg/RedfishDiscoverDxe/RedfishDiscoverDxe.inf
  RedfishPkg/RedfishConfigHandler/RedfishConfigHandlerDriver.inf
!endif
