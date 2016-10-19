/** @file
  Define the PPI to abstract the functions that enable IDE and SATA channels, and to retrieve
  the base I/O port address for each of the enabled IDE and SATA channels.
  
Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_ATA_CONTROLLER_PPI_H_
#define _PEI_ATA_CONTROLLER_PPI_H_

///
/// Global ID for the PEI_ATA_CONTROLLER_PPI. 
///
#define PEI_ATA_CONTROLLER_PPI_GUID \
  { \
    0xa45e60d1, 0xc719, 0x44aa, {0xb0, 0x7a, 0xaa, 0x77, 0x7f, 0x85, 0x90, 0x6d } \
  }

///
/// Forward declaration for the PEI_ATA_CONTROLLER_PPI.
///
typedef struct _PEI_ATA_CONTROLLER_PPI PEI_ATA_CONTROLLER_PPI;

///
/// This bit is used in the ChannelMask parameter of EnableAtaChannel() to 
/// disable the IDE channels. 
/// This is designed for old generation chipset with PATA/SATA controllers. 
/// It may be ignored in PPI implementation for new generation chipset without PATA controller. 
///
#define PEI_ICH_IDE_NONE        0x00

///
/// This bit is used in the ChannelMask parameter of EnableAtaChannel() to 
/// enable the Primary IDE channel.
/// This is designed for old generation chipset with PATA/SATA controllers. 
/// It may be ignored in PPI implementation for new generation chipset without PATA controller. 
///
#define PEI_ICH_IDE_PRIMARY     0x01

///
/// This bit is used in the ChannelMask parameter of EnableAtaChannel() to 
/// enable the Secondary IDE channel.
/// This is designed for old generation chipset with PATA/SATA controllers. 
/// It may be ignored in PPI implementation for new generation chipset without PATA controller. 
///
#define PEI_ICH_IDE_SECONDARY   0x02

///
/// This bit is used in the ChannelMask parameter of EnableAtaChannel() to 
/// disable the SATA channel.
/// This is designed for old generation chipset with PATA/SATA controllers. 
/// It may be ignored in PPI implementation for new generation chipset without PATA controller. 
///
#define PEI_ICH_SATA_NONE       0x04

///
/// This bit is used in the ChannelMask parameter of EnableAtaChannel() to 
/// enable the Primary SATA channel.
/// This is designed for old generation chipset with PATA/SATA controllers. 
/// It may be ignored in PPI implementation for new generation chipset without PATA controller. 
///
#define PEI_ICH_SATA_PRIMARY    0x08

///
/// This bit is used in the ChannelMask parameter of EnableAtaChannel() to 
/// enable the Secondary SATA channel.
/// This is designed for old generation chipset with PATA/SATA controllers. 
/// It may be ignored in PPI implementation for new generation chipset without PATA controller. 
///
#define PEI_ICH_SATA_SECONDARY  0x010

///
/// Structure that contains the base addresses for the IDE registers
///
typedef struct {
  ///
  /// Base I/O port address of the IDE controller's command block
  ///
  UINT16  CommandBlockBaseAddr;
  ///
  /// Base I/O port address of the IDE controller's control block
  ///
  UINT16  ControlBlockBaseAddr;
} IDE_REGS_BASE_ADDR;

/**
  Sets IDE and SATA channels to an enabled or disabled state.

  This service enables or disables the IDE and SATA channels specified by ChannelMask.
  It may ignore ChannelMask setting to enable or disable IDE and SATA channels based on the platform policy. 
  The number of the enabled channels will be returned by GET_IDE_REGS_BASE_ADDR() function. 

  If the new state is set, then EFI_SUCCESS is returned.  If the new state can
  not be set, then EFI_DEVICE_ERROR is returned.

  @param[in] PeiServices   The pointer to the PEI Services Table.
  @param[in] This          The pointer to this instance of the PEI_ATA_CONTROLLER_PPI.
  @param[in] ChannelMask   The bitmask that identifies the IDE and SATA channels to 
                           enable or disable. This parameter is optional.

  @retval EFI_SUCCESS        The IDE or SATA channels were enabled or disabled successfully.
  @retval EFI_DEVICE_ERROR   The IDE or SATA channels could not be enabled or disabled.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_ENABLE_ATA)(
  IN EFI_PEI_SERVICES        **PeiServices,
  IN PEI_ATA_CONTROLLER_PPI  *This,
  IN UINT8                   ChannelMask
  );

/**
  Retrieves the I/O port base addresses for command and control registers of the 
  enabled IDE/SATA channels.

  This service fills in the structure poionted to by IdeRegsBaseAddr with the I/O
  port base addresses for the command and control registers of the IDE and SATA
  channels that were previously enabled in EnableAtaChannel().  The number of 
  enabled IDE and SATA channels is returned.

  @param[in]  PeiServices       The pointer to the PEI Services Table.
  @param[in]  This              The pointer to this instance of the PEI_ATA_CONTROLLER_PPI.
  @param[out] IdeRegsBaseAddr   The pointer to caller allocated space to return the 
                                I/O port base addresses of the IDE and SATA channels 
                                that were previosuly enabled with EnableAtaChannel().

  @return   The number of enabled IDE and SATA channels in the platform.

**/
typedef
UINT32
(EFIAPI *GET_IDE_REGS_BASE_ADDR)(
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  PEI_ATA_CONTROLLER_PPI  *This,
  OUT IDE_REGS_BASE_ADDR      *IdeRegsBaseAddr 
  );

///
/// This PPI contains services to enable and disable IDE and SATA channels and
/// retrieves the base I/O port addresses to the enabled IDE and SATA channels.
///
struct _PEI_ATA_CONTROLLER_PPI {
  PEI_ENABLE_ATA          EnableAtaChannel;
  GET_IDE_REGS_BASE_ADDR  GetIdeRegsBaseAddr;
};

extern EFI_GUID gPeiAtaControllerPpiGuid;

#endif


