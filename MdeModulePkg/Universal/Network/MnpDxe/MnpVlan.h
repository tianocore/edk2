/** @file
  Header file to be included by MnpVlan.c.

Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The full
text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __MNP_VLAN_H__
#define __MNP_VLAN_H__

#include "MnpDriver.h"

extern EFI_VLAN_CONFIG_PROTOCOL mVlanConfigProtocolTemplate;


/**
  Create a child handle for the VLAN ID.

  @param[in]       ImageHandle        The driver image handle.
  @param[in]       ControllerHandle   Handle of device to bind driver to.
  @param[in]       VlanId             The VLAN ID.
  @param[out]      Devicepath         Pointer to returned device path for child handle.

  @return The handle of VLAN child or NULL if failed to create VLAN child.

**/
EFI_HANDLE
MnpCreateVlanChild (
  IN     EFI_HANDLE                  ImageHandle,
  IN     EFI_HANDLE                  ControllerHandle,
  IN     UINT16                      VlanId,
     OUT EFI_DEVICE_PATH_PROTOCOL    **Devicepath OPTIONAL
  );

/**
  Remove VLAN tag of a packet.

  @param[in, out]  MnpDeviceData      Pointer to the mnp device context data.
  @param[in, out]  Nbuf               Pointer to the NET_BUF to remove VLAN tag.
  @param[out]      VlanId             Pointer to the returned VLAN ID.

  @retval TRUE             VLAN tag is removed from this packet.
  @retval FALSE            There is no VLAN tag in this packet.

**/
BOOLEAN
MnpRemoveVlanTag (
  IN OUT MNP_DEVICE_DATA   *MnpDeviceData,
  IN OUT NET_BUF           *Nbuf,
     OUT UINT16            *VlanId
  );

/**
  Build the vlan packet to transmit from the TxData passed in.

  @param  MnpServiceData         Pointer to the mnp service context data.
  @param  TxData                 Pointer to the transmit data containing the
                                 information to build the packet.
  @param  ProtocolType           Pointer to the Ethernet protocol type.
  @param  Packet                 Pointer to record the address of the packet.
  @param  Length                 Pointer to a UINT32 variable used to record the
                                 packet's length.

**/
VOID
MnpInsertVlanTag (
  IN     MNP_SERVICE_DATA                    *MnpServiceData,
  IN     EFI_MANAGED_NETWORK_TRANSMIT_DATA   *TxData,
     OUT UINT16                              *ProtocolType,
  IN OUT UINT8                               **Packet,
  IN OUT UINT32                              *Length
  );

/**
  Get VLAN configuration variable.

  @param[in]       MnpDeviceData      Pointer to the MNP device context data.
  @param[out]      NumberOfVlan       Pointer to number of VLAN to be returned.
  @param[out]      VlanVariable       Pointer to the buffer to return requested
                                      array of VLAN_TCI.

  @retval EFI_SUCCESS            The array of VLAN_TCI was returned in VlanVariable
                                 and number of VLAN was returned in NumberOfVlan.
  @retval EFI_NOT_FOUND          VLAN configuration variable not found.
  @retval EFI_OUT_OF_RESOURCES   There is not enough pool memory to store the configuration.

**/
EFI_STATUS
MnpGetVlanVariable (
  IN     MNP_DEVICE_DATA   *MnpDeviceData,
     OUT UINTN             *NumberOfVlan,
     OUT VLAN_TCI          **VlanVariable
  );

/**
  Set VLAN configuration variable.

  @param[in] MnpDeviceData       Pointer to the MNP device context data.
  @param[in] NumberOfVlan        Number of VLAN in array VlanVariable.
  @param[in] VlanVariable        Pointer to array of VLAN_TCI.

  @retval EFI_SUCCESS            The VLAN variable is successfully set.
  @retval EFI_OUT_OF_RESOURCES   There is not enough resource to set the configuration.

**/
EFI_STATUS
MnpSetVlanVariable (
  IN MNP_DEVICE_DATA             *MnpDeviceData,
  IN UINTN                       NumberOfVlan,
  IN VLAN_TCI                    *VlanVariable
  );

/**
  Create a VLAN device or modify the configuration parameter of an
  already-configured VLAN.

  The Set() function is used to create a new VLAN device or change the VLAN
  configuration parameters. If the VlanId hasn't been configured in the
  physical Ethernet device, a new VLAN device will be created. If a VLAN with
  this VlanId is already configured, then related configuration will be updated
  as the input parameters.

  If VlanId is zero, the VLAN device will send and receive untagged frames.
  Otherwise, the VLAN device will send and receive VLAN-tagged frames containing the VlanId.
  If VlanId is out of scope of (0-4094), EFI_INVALID_PARAMETER is returned.
  If Priority is out of the scope of (0-7), then EFI_INVALID_PARAMETER is returned.
  If there is not enough system memory to perform the registration, then
  EFI_OUT_OF_RESOURCES is returned.

  @param[in] This                Points to the EFI_VLAN_CONFIG_PROTOCOL.
  @param[in] VlanId              A unique identifier (1-4094) of the VLAN which is being created
                                 or modified, or zero (0).
  @param[in] Priority            3 bit priority in VLAN header. Priority 0 is default value. If
                                 VlanId is zero (0), Priority is ignored.

  @retval EFI_SUCCESS            The VLAN is successfully configured.
  @retval EFI_INVALID_PARAMETER  One or more of following conditions is TRUE:
                                 - This is NULL.
                                 - VlanId is an invalid VLAN Identifier.
                                 - Priority is invalid.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to perform the registration.

**/
EFI_STATUS
EFIAPI
VlanConfigSet (
  IN EFI_VLAN_CONFIG_PROTOCOL    *This,
  IN UINT16                      VlanId,
  IN UINT8                       Priority
  );

/**
  Find configuration information for specified VLAN or all configured VLANs.

  The Find() function is used to find the configuration information for matching
  VLAN and allocate a buffer into which those entries are copied.

  @param[in]  This               Points to the EFI_VLAN_CONFIG_PROTOCOL.
  @param[in]  VlanId             Pointer to VLAN identifier. Set to NULL to find all
                                 configured VLANs.
  @param[out] NumberOfVlan       The number of VLANs which is found by the specified criteria.
  @param[out] Entries            The buffer which receive the VLAN configuration.

  @retval EFI_SUCCESS            The VLAN is successfully found.
  @retval EFI_INVALID_PARAMETER  One or more of following conditions is TRUE:
                                 - This is NULL.
                                 - Specified VlanId is invalid.
  @retval EFI_NOT_FOUND          No matching VLAN is found.

**/
EFI_STATUS
EFIAPI
VlanConfigFind (
  IN     EFI_VLAN_CONFIG_PROTOCOL    *This,
  IN     UINT16                      *VlanId OPTIONAL,
     OUT UINT16                      *NumberOfVlan,
     OUT EFI_VLAN_FIND_DATA          **Entries
  );

/**
  Remove the configured VLAN device.

  The Remove() function is used to remove the specified VLAN device.
  If the VlanId is out of the scope of (0-4094), EFI_INVALID_PARAMETER is returned.
  If specified VLAN hasn't been previously configured, EFI_NOT_FOUND is returned.

  @param[in] This                Points to the EFI_VLAN_CONFIG_PROTOCOL.
  @param[in] VlanId              Identifier (0-4094) of the VLAN to be removed.

  @retval EFI_SUCCESS            The VLAN is successfully removed.
  @retval EFI_INVALID_PARAMETER  One or more of following conditions is TRUE:
                                 - This is NULL.
                                 - VlanId  is an invalid parameter.
  @retval EFI_NOT_FOUND          The to-be-removed VLAN does not exist.

**/
EFI_STATUS
EFIAPI
VlanConfigRemove (
  IN EFI_VLAN_CONFIG_PROTOCOL    *This,
  IN UINT16                      VlanId
  );

#endif
