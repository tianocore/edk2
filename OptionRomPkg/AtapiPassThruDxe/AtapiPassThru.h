/** @file
  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  Module Name:  AtapiPassThru.h

**/

#ifndef _APT_H
#define _APT_H



#include <Uefi.h>

#include <Protocol/ScsiPassThru.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Protocol/PciIo.h>
#include <Protocol/DriverSupportedEfiVersion.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>

#include <IndustryStandard/Pci.h>

#define MAX_TARGET_ID 4

//
// IDE Registers
//
typedef union {
  UINT16  Command;        /* when write */
  UINT16  Status;         /* when read */
} IDE_CMD_OR_STATUS;

typedef union {
  UINT16  Error;          /* when read */
  UINT16  Feature;        /* when write */
} IDE_ERROR_OR_FEATURE;

typedef union {
  UINT16  AltStatus;      /* when read */
  UINT16  DeviceControl;  /* when write */
} IDE_AltStatus_OR_DeviceControl;


typedef enum {
  IdePrimary    = 0,
  IdeSecondary  = 1,
  IdeMaxChannel = 2
} EFI_IDE_CHANNEL;

///


//
// Bit definitions in Programming Interface byte of the Class Code field
// in PCI IDE controller's Configuration Space
//
#define IDE_PRIMARY_OPERATING_MODE            BIT0
#define IDE_PRIMARY_PROGRAMMABLE_INDICATOR    BIT1
#define IDE_SECONDARY_OPERATING_MODE          BIT2
#define IDE_SECONDARY_PROGRAMMABLE_INDICATOR  BIT3


#define ATAPI_MAX_CHANNEL 2

///
/// IDE registers set
///
typedef struct {
  UINT16                          Data;
  IDE_ERROR_OR_FEATURE            Reg1;
  UINT16                          SectorCount;
  UINT16                          SectorNumber;
  UINT16                          CylinderLsb;
  UINT16                          CylinderMsb;
  UINT16                          Head;
  IDE_CMD_OR_STATUS               Reg;
  IDE_AltStatus_OR_DeviceControl  Alt;
  UINT16                          DriveAddress;
} IDE_BASE_REGISTERS;

#define ATAPI_SCSI_PASS_THRU_DEV_SIGNATURE  SIGNATURE_32 ('a', 's', 'p', 't')

typedef struct {
  UINTN                            Signature;
  EFI_HANDLE                       Handle;
  EFI_SCSI_PASS_THRU_PROTOCOL      ScsiPassThru;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  ExtScsiPassThru;
  EFI_PCI_IO_PROTOCOL              *PciIo;
  UINT64                           OriginalPciAttributes;
  //
  // Local Data goes here
  //
  IDE_BASE_REGISTERS               *IoPort;
  IDE_BASE_REGISTERS               AtapiIoPortRegisters[2];
  UINT32                           LatestTargetId;
  UINT64                           LatestLun;
} ATAPI_SCSI_PASS_THRU_DEV;

//
// IDE registers' base addresses
//
typedef struct {
  UINT16  CommandBlockBaseAddr;
  UINT16  ControlBlockBaseAddr;
} IDE_REGISTERS_BASE_ADDR;

#define ATAPI_SCSI_PASS_THRU_DEV_FROM_THIS(a) \
  CR (a, \
      ATAPI_SCSI_PASS_THRU_DEV, \
      ScsiPassThru, \
      ATAPI_SCSI_PASS_THRU_DEV_SIGNATURE \
      )

#define ATAPI_EXT_SCSI_PASS_THRU_DEV_FROM_THIS(a) \
  CR (a, \
      ATAPI_SCSI_PASS_THRU_DEV, \
      ExtScsiPassThru, \
      ATAPI_SCSI_PASS_THRU_DEV_SIGNATURE \
      )

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL                gAtapiScsiPassThruDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL                gAtapiScsiPassThruComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL               gAtapiScsiPassThruComponentName2;
extern EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL  gAtapiScsiPassThruDriverSupportedEfiVersion;

//
// ATAPI Command op code
//
#define OP_INQUIRY                      0x12
#define OP_LOAD_UNLOAD_CD               0xa6
#define OP_MECHANISM_STATUS             0xbd
#define OP_MODE_SELECT_10               0x55
#define OP_MODE_SENSE_10                0x5a
#define OP_PAUSE_RESUME                 0x4b
#define OP_PLAY_AUDIO_10                0x45
#define OP_PLAY_AUDIO_MSF               0x47
#define OP_PLAY_CD                      0xbc
#define OP_PLAY_CD_MSF                  0xb4
#define OP_PREVENT_ALLOW_MEDIUM_REMOVAL 0x1e
#define OP_READ_10                      0x28
#define OP_READ_12                      0xa8
#define OP_READ_CAPACITY                0x25
#define OP_READ_CD                      0xbe
#define OP_READ_CD_MSF                  0xb9
#define OP_READ_HEADER                  0x44
#define OP_READ_SUB_CHANNEL             0x42
#define OP_READ_TOC                     0x43
#define OP_REQUEST_SENSE                0x03
#define OP_SCAN                         0xba
#define OP_SEEK_10                      0x2b
#define OP_SET_CD_SPEED                 0xbb
#define OP_STOPPLAY_SCAN                0x4e
#define OP_START_STOP_UNIT              0x1b
#define OP_TEST_UNIT_READY              0x00

#define OP_FORMAT_UNIT                  0x04
#define OP_READ_FORMAT_CAPACITIES       0x23
#define OP_VERIFY                       0x2f
#define OP_WRITE_10                     0x2a
#define OP_WRITE_12                     0xaa
#define OP_WRITE_AND_VERIFY             0x2e

//
// ATA Command
//
#define ATAPI_SOFT_RESET_CMD  0x08

typedef enum {
  DataIn  = 0,
  DataOut = 1,
  DataBi  = 2,
  NoData  = 3,
  End     = 0xff
} DATA_DIRECTION;

typedef struct {
  UINT8           OpCode;
  DATA_DIRECTION  Direction;
} SCSI_COMMAND_SET;

#define MAX_CHANNEL         2

#define ValidCdbLength(Len) ((Len) == 6 || (Len) == 10 || (Len) == 12) ? 1 : 0

//
// IDE registers bit definitions
//
// ATA Err Reg bitmap
//
#define BBK_ERR   BIT7 ///< Bad block detected
#define UNC_ERR   BIT6 ///< Uncorrectable Data
#define MC_ERR    BIT5 ///< Media Change
#define IDNF_ERR  BIT4 ///< ID Not Found
#define MCR_ERR   BIT3 ///< Media Change Requested
#define ABRT_ERR  BIT2 ///< Aborted Command
#define TK0NF_ERR BIT1 ///< Track 0 Not Found
#define AMNF_ERR  BIT0 ///< Address Mark Not Found

//
// ATAPI Err Reg bitmap
//
#define SENSE_KEY_ERR (BIT7 | BIT6 | BIT5 | BIT4)
#define EOM_ERR BIT1 ///< End of Media Detected
#define ILI_ERR BIT0 ///< Illegal Length Indication

//
// Device/Head Reg
//
#define LBA_MODE  BIT6
#define DEV       BIT4
#define HS3       BIT3
#define HS2       BIT2
#define HS1       BIT1
#define HS0       BIT0
#define CHS_MODE  (0)
#define DRV0      (0)
#define DRV1      (1)
#define MST_DRV   DRV0
#define SLV_DRV   DRV1

//
// Status Reg
//
#define BSY   BIT7 ///< Controller Busy
#define DRDY  BIT6 ///< Drive Ready
#define DWF   BIT5 ///< Drive Write Fault
#define DSC   BIT4 ///< Disk Seek Complete
#define DRQ   BIT3 ///< Data Request
#define CORR  BIT2 ///< Corrected Data
#define IDX   BIT1 ///< Index
#define ERR   BIT0 ///< Error
#define CHECK BIT0 ///< Check bit for ATAPI Status Reg

//
// Device Control Reg
//
#define SRST  BIT2 ///< Software Reset
#define IEN_L BIT1 ///< Interrupt Enable

//
// ATAPI Feature Register
//
#define OVERLAP BIT1
#define DMA     BIT0

//
// ATAPI Interrupt Reason Reson Reg (ATA Sector Count Register)
//
#define RELEASE     BIT2
#define IO          BIT1
#define CoD         BIT0

#define PACKET_CMD  0xA0

#define DEFAULT_CMD (0xa0)
//
// default content of device control register, disable INT
//
#define DEFAULT_CTL           (0x0a)
#define MAX_ATAPI_BYTE_COUNT  (0xfffe)

//
// function prototype
//

EFI_STATUS
EFIAPI
AtapiScsiPassThruDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
AtapiScsiPassThruDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
AtapiScsiPassThruDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  );

//
// EFI Component Name Functions
//
/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
AtapiScsiPassThruComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );


/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
AtapiScsiPassThruComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );


EFI_STATUS
EFIAPI
AtapiScsiPassThruDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
 /*++

Routine Description:

  Entry point for EFI drivers.

Arguments:

  ImageHandle - EFI_HANDLE
  SystemTable - EFI_SYSTEM_TABLE

Returns:

  EFI_SUCCESS
  Others 

--*/
;

EFI_STATUS
RegisterAtapiScsiPassThru (
  IN  EFI_DRIVER_BINDING_PROTOCOL *This,
  IN  EFI_HANDLE                  Controller,
  IN  EFI_PCI_IO_PROTOCOL         *PciIo,
  IN  UINT64                      OriginalPciAttributes
  )
/*++

Routine Description:
  Attaches SCSI Pass Thru Protocol for specified IDE channel.
    
Arguments:
  This              - Protocol instance pointer.
  Controller        - Parent device handle to the IDE channel.    
  PciIo             - PCI I/O protocol attached on the "Controller".                        
  
Returns:
  Always return EFI_SUCCESS unless installing SCSI Pass Thru Protocol failed.

--*/
;

EFI_STATUS
EFIAPI
AtapiScsiPassThruFunction (
  IN EFI_SCSI_PASS_THRU_PROTOCOL                        *This,
  IN UINT32                                             Target,
  IN UINT64                                             Lun,
  IN OUT EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET         *Packet,
  IN EFI_EVENT                                          Event OPTIONAL
  )
/*++

Routine Description:

  Implements EFI_SCSI_PASS_THRU_PROTOCOL.PassThru() function.

Arguments:

  This:     The EFI_SCSI_PASS_THRU_PROTOCOL instance.
  Target:   The Target ID of the ATAPI device to send the SCSI 
            Request Packet. To ATAPI devices attached on an IDE
            Channel, Target ID 0 indicates Master device;Target
            ID 1 indicates Slave device.
  Lun:      The LUN of the ATAPI device to send the SCSI Request
            Packet. To the ATAPI device, Lun is always 0.
  Packet:   The SCSI Request Packet to send to the ATAPI device 
            specified by Target and Lun.
  Event:    If non-blocking I/O is not supported then Event is ignored, 
            and blocking I/O is performed.
            If Event is NULL, then blocking I/O is performed.
            If Event is not NULL and non blocking I/O is supported, 
            then non-blocking I/O is performed, and Event will be signaled 
            when the SCSI Request Packet completes.      

Returns:  

   EFI_STATUS

--*/
;

EFI_STATUS
EFIAPI
AtapiScsiPassThruGetNextDevice (
  IN  EFI_SCSI_PASS_THRU_PROTOCOL    *This,
  IN OUT UINT32                      *Target,
  IN OUT UINT64                      *Lun
  )
/*++

Routine Description:

  Used to retrieve the list of legal Target IDs for SCSI devices 
  on a SCSI channel.

Arguments:

  This                  - Protocol instance pointer.
  Target                - On input, a pointer to the Target ID of a SCSI 
                          device present on the SCSI channel.  On output, 
                          a pointer to the Target ID of the next SCSI device
                          present on a SCSI channel.  An input value of 
                          0xFFFFFFFF retrieves the Target ID of the first 
                          SCSI device present on a SCSI channel.
  Lun                   - On input, a pointer to the LUN of a SCSI device
                          present on the SCSI channel. On output, a pointer
                          to the LUN of the next SCSI device present on 
                          a SCSI channel.
Returns:

  EFI_SUCCESS           - The Target ID and Lun of the next SCSI device 
                          on the SCSI channel was returned in Target and Lun.
  EFI_NOT_FOUND         - There are no more SCSI devices on this SCSI channel.
  EFI_INVALID_PARAMETER - Target is not 0xFFFFFFFF,and Target and Lun were not
                           returned on a previous call to GetNextDevice().

--*/
;

EFI_STATUS
EFIAPI
AtapiScsiPassThruBuildDevicePath (
  IN     EFI_SCSI_PASS_THRU_PROTOCOL    *This,
  IN     UINT32                         Target,
  IN     UINT64                         Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL       **DevicePath
  )
/*++

Routine Description:

  Used to allocate and build a device path node for a SCSI device 
  on a SCSI channel. Would not build device path for a SCSI Host Controller.

Arguments:

  This                  - Protocol instance pointer.
  Target                - The Target ID of the SCSI device for which
                          a device path node is to be allocated and built.
  Lun                   - The LUN of the SCSI device for which a device 
                          path node is to be allocated and built.
  DevicePath            - A pointer to a single device path node that 
                          describes the SCSI device specified by 
                          Target and Lun. This function is responsible 
                          for allocating the buffer DevicePath with the boot
                          service AllocatePool().  It is the caller's 
                          responsibility to free DevicePath when the caller
                          is finished with DevicePath.    
  Returns:
  EFI_SUCCESS           - The device path node that describes the SCSI device
                          specified by Target and Lun was allocated and 
                          returned in DevicePath.
  EFI_NOT_FOUND         - The SCSI devices specified by Target and Lun does
                          not exist on the SCSI channel.
  EFI_INVALID_PARAMETER - DevicePath is NULL.
  EFI_OUT_OF_RESOURCES  - There are not enough resources to allocate 
                          DevicePath.

--*/
;

EFI_STATUS
EFIAPI
AtapiScsiPassThruGetTargetLun (
  IN  EFI_SCSI_PASS_THRU_PROTOCOL    *This,
  IN  EFI_DEVICE_PATH_PROTOCOL       *DevicePath,
  OUT UINT32                         *Target,
  OUT UINT64                         *Lun
  )
/*++

Routine Description:

  Used to translate a device path node to a Target ID and LUN.

Arguments:

  This                  - Protocol instance pointer.
  DevicePath            - A pointer to the device path node that 
                          describes a SCSI device on the SCSI channel.
  Target                - A pointer to the Target ID of a SCSI device 
                          on the SCSI channel. 
  Lun                   - A pointer to the LUN of a SCSI device on 
                          the SCSI channel.    
Returns:

  EFI_SUCCESS           - DevicePath was successfully translated to a 
                          Target ID and LUN, and they were returned 
                          in Target and Lun.
  EFI_INVALID_PARAMETER - DevicePath/Target/Lun is NULL.
  EFI_UNSUPPORTED       - This driver does not support the device path 
                          node type in DevicePath.
  EFI_NOT_FOUND         - A valid translation from DevicePath to a 
                          Target ID and LUN does not exist.

--*/
;

EFI_STATUS
EFIAPI
AtapiScsiPassThruResetChannel (
  IN  EFI_SCSI_PASS_THRU_PROTOCOL   *This
  )
/*++

Routine Description:

  Resets a SCSI channel.This operation resets all the 
  SCSI devices connected to the SCSI channel.

Arguments:

  This                  - Protocol instance pointer.

Returns:

  EFI_SUCCESS           - The SCSI channel was reset.
  EFI_UNSUPPORTED       - The SCSI channel does not support 
                          a channel reset operation.
  EFI_DEVICE_ERROR      - A device error occurred while 
                          attempting to reset the SCSI channel.
  EFI_TIMEOUT           - A timeout occurred while attempting 
                          to reset the SCSI channel.

--*/
;

EFI_STATUS
EFIAPI
AtapiScsiPassThruResetTarget (
  IN EFI_SCSI_PASS_THRU_PROTOCOL    *This,
  IN UINT32                         Target,
  IN UINT64                         Lun
  )
/*++

Routine Description:

  Resets a SCSI device that is connected to a SCSI channel.

Arguments:

  This                  - Protocol instance pointer.
  Target                - The Target ID of the SCSI device to reset. 
  Lun                   - The LUN of the SCSI device to reset.
    
Returns:

  EFI_SUCCESS           - The SCSI device specified by Target and 
                          Lun was reset.
  EFI_UNSUPPORTED       - The SCSI channel does not support a target
                          reset operation.
  EFI_INVALID_PARAMETER - Target or Lun are invalid.
  EFI_DEVICE_ERROR      - A device error occurred while attempting 
                          to reset the SCSI device specified by Target 
                          and Lun.
  EFI_TIMEOUT           - A timeout occurred while attempting to reset 
                          the SCSI device specified by Target and Lun.

--*/
;

EFI_STATUS
EFIAPI
AtapiExtScsiPassThruFunction (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                    *This,
  IN UINT8                                              *Target,
  IN UINT64                                             Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET     *Packet,
  IN EFI_EVENT                                          Event OPTIONAL
  )
/*++

Routine Description:

  Implements EFI_EXT_SCSI_PASS_THRU_PROTOCOL.PassThru() function.

Arguments:

  This:     The EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  Target:   The Target ID of the ATAPI device to send the SCSI 
            Request Packet. To ATAPI devices attached on an IDE
            Channel, Target ID 0 indicates Master device;Target
            ID 1 indicates Slave device.
  Lun:      The LUN of the ATAPI device to send the SCSI Request
            Packet. To the ATAPI device, Lun is always 0.
  Packet:   The SCSI Request Packet to send to the ATAPI device 
            specified by Target and Lun.
  Event:    If non-blocking I/O is not supported then Event is ignored, 
            and blocking I/O is performed.
            If Event is NULL, then blocking I/O is performed.
            If Event is not NULL and non blocking I/O is supported, 
            then non-blocking I/O is performed, and Event will be signaled 
            when the SCSI Request Packet completes.      

Returns:  

  EFI_STATUS

--*/
;

EFI_STATUS
EFIAPI
AtapiExtScsiPassThruGetNextTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL    *This,
  IN OUT UINT8                           **Target,
  IN OUT UINT64                          *Lun
  )
/*++

Routine Description:

  Used to retrieve the list of legal Target IDs for SCSI devices 
  on a SCSI channel.

Arguments:

  This                  - Protocol instance pointer.
  Target                - On input, a pointer to the Target ID of a SCSI 
                          device present on the SCSI channel.  On output, 
                          a pointer to the Target ID of the next SCSI device
                          present on a SCSI channel.  An input value of 
                          0xFFFFFFFF retrieves the Target ID of the first 
                          SCSI device present on a SCSI channel.
  Lun                   - On input, a pointer to the LUN of a SCSI device
                          present on the SCSI channel. On output, a pointer
                          to the LUN of the next SCSI device present on 
                          a SCSI channel.
Returns:

  EFI_SUCCESS           - The Target ID and Lun of the next SCSI device 
                          on the SCSI channel was returned in Target and Lun.
  EFI_NOT_FOUND         - There are no more SCSI devices on this SCSI channel.
  EFI_INVALID_PARAMETER - Target is not 0xFFFFFFFF,and Target and Lun were not
                           returned on a previous call to GetNextDevice().

--*/
;

EFI_STATUS
EFIAPI
AtapiExtScsiPassThruBuildDevicePath (
  IN     EFI_EXT_SCSI_PASS_THRU_PROTOCOL    *This,
  IN     UINT8                              *Target,
  IN     UINT64                             Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL           **DevicePath
  )
/*++

Routine Description:

  Used to allocate and build a device path node for a SCSI device 
  on a SCSI channel. Would not build device path for a SCSI Host Controller.

Arguments:

  This                  - Protocol instance pointer.
  Target                - The Target ID of the SCSI device for which
                          a device path node is to be allocated and built.
  Lun                   - The LUN of the SCSI device for which a device 
                          path node is to be allocated and built.
  DevicePath            - A pointer to a single device path node that 
                          describes the SCSI device specified by 
                          Target and Lun. This function is responsible 
                          for allocating the buffer DevicePath with the boot
                          service AllocatePool().  It is the caller's 
                          responsibility to free DevicePath when the caller
                          is finished with DevicePath.    
  Returns:
  EFI_SUCCESS           - The device path node that describes the SCSI device
                          specified by Target and Lun was allocated and 
                          returned in DevicePath.
  EFI_NOT_FOUND         - The SCSI devices specified by Target and Lun does
                          not exist on the SCSI channel.
  EFI_INVALID_PARAMETER - DevicePath is NULL.
  EFI_OUT_OF_RESOURCES  - There are not enough resources to allocate 
                          DevicePath.

--*/
;

EFI_STATUS
EFIAPI
AtapiExtScsiPassThruGetTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL    *This,
  IN  EFI_DEVICE_PATH_PROTOCOL       *DevicePath,
  OUT UINT8                          **Target,
  OUT UINT64                         *Lun
  )
/*++

Routine Description:

  Used to translate a device path node to a Target ID and LUN.

Arguments:

  This                  - Protocol instance pointer.
  DevicePath            - A pointer to the device path node that 
                          describes a SCSI device on the SCSI channel.
  Target                - A pointer to the Target ID of a SCSI device 
                          on the SCSI channel. 
  Lun                   - A pointer to the LUN of a SCSI device on 
                          the SCSI channel.    
Returns:

  EFI_SUCCESS           - DevicePath was successfully translated to a 
                          Target ID and LUN, and they were returned 
                          in Target and Lun.
  EFI_INVALID_PARAMETER - DevicePath/Target/Lun is NULL.
  EFI_UNSUPPORTED       - This driver does not support the device path 
                          node type in DevicePath.
  EFI_NOT_FOUND         - A valid translation from DevicePath to a 
                          Target ID and LUN does not exist.

--*/
;

EFI_STATUS
EFIAPI
AtapiExtScsiPassThruResetChannel (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL   *This
  )
/*++

Routine Description:

  Resets a SCSI channel.This operation resets all the 
  SCSI devices connected to the SCSI channel.

Arguments:

  This                  - Protocol instance pointer.

Returns:

  EFI_SUCCESS           - The SCSI channel was reset.
  EFI_UNSUPPORTED       - The SCSI channel does not support 
                          a channel reset operation.
  EFI_DEVICE_ERROR      - A device error occurred while 
                          attempting to reset the SCSI channel.
  EFI_TIMEOUT           - A timeout occurred while attempting 
                          to reset the SCSI channel.

--*/
;

EFI_STATUS
EFIAPI
AtapiExtScsiPassThruResetTarget (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL    *This,
  IN UINT8                              *Target,
  IN UINT64                             Lun
  )
/*++

Routine Description:

  Resets a SCSI device that is connected to a SCSI channel.

Arguments:

  This                  - Protocol instance pointer.
  Target                - The Target ID of the SCSI device to reset. 
  Lun                   - The LUN of the SCSI device to reset.
    
Returns:

  EFI_SUCCESS           - The SCSI device specified by Target and 
                          Lun was reset.
  EFI_UNSUPPORTED       - The SCSI channel does not support a target
                          reset operation.
  EFI_INVALID_PARAMETER - Target or Lun are invalid.
  EFI_DEVICE_ERROR      - A device error occurred while attempting 
                          to reset the SCSI device specified by Target 
                          and Lun.
  EFI_TIMEOUT           - A timeout occurred while attempting to reset 
                          the SCSI device specified by Target and Lun.

--*/
;

EFI_STATUS
EFIAPI
AtapiExtScsiPassThruGetNextTarget (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL    *This,
  IN OUT UINT8                           **Target
  )
/*++

Routine Description:
  Used to retrieve the list of legal Target IDs for SCSI devices 
  on a SCSI channel.

Arguments:
  This                  - Protocol instance pointer.
  Target                - On input, a pointer to the Target ID of a SCSI 
                          device present on the SCSI channel.  On output, 
                          a pointer to the Target ID of the next SCSI device
                           present on a SCSI channel.  An input value of 
                           0xFFFFFFFF retrieves the Target ID of the first 
                           SCSI device present on a SCSI channel.
  Lun                   - On input, a pointer to the LUN of a SCSI device
                          present on the SCSI channel. On output, a pointer
                          to the LUN of the next SCSI device present on 
                          a SCSI channel.
    
Returns:
  EFI_SUCCESS           - The Target ID and Lun of the next SCSI device 
                          on the SCSI channel was returned in Target and Lun.
  EFI_NOT_FOUND         - There are no more SCSI devices on this SCSI channel.
  EFI_INVALID_PARAMETER - Target is not 0xFFFFFFFF,and Target and Lun were not
                          returned on a previous call to GetNextDevice().

--*/
;

EFI_STATUS
CheckSCSIRequestPacket (
  EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET      *Packet
  )
/*++

Routine Description:

  Checks the parameters in the SCSI Request Packet to make sure
  they are valid for a SCSI Pass Thru request.

Arguments:

  Packet         -  The pointer of EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET   

Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
SubmitBlockingIoCommand (
  ATAPI_SCSI_PASS_THRU_DEV                  *AtapiScsiPrivate,
  UINT32                                    Target,
  EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET    *Packet
  )
/*++

Routine Description:

  Performs blocking I/O request.
    
Arguments:

  AtapiScsiPrivate:   Private data structure for the specified channel.
  Target:             The Target ID of the ATAPI device to send the SCSI 
                      Request Packet. To ATAPI devices attached on an IDE
                      Channel, Target ID 0 indicates Master device;Target
                      ID 1 indicates Slave device.
  Packet:             The SCSI Request Packet to send to the ATAPI device 
                      specified by Target.
  
  Returns:            EFI_STATUS  

--*/
;

BOOLEAN
IsCommandValid (
  EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET   *Packet
  )
 /*++

Routine Description:

  Checks the requested SCSI command: 
  Is it supported by this driver?
  Is the Data transfer direction reasonable?

Arguments:

  Packet         -  The pointer of EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET   

Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
CheckExtSCSIRequestPacket (
  EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET      *Packet
  )
/*++

Routine Description:

  Checks the parameters in the SCSI Request Packet to make sure
  they are valid for a SCSI Pass Thru request.

Arguments:

  Packet       - The pointer of EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET
  
Returns:
  
  EFI_STATUS

--*/
;


BOOLEAN
IsExtCommandValid (
  EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET   *Packet
  )
/*++
  
Routine Description:

  Checks the requested SCSI command: 
  Is it supported by this driver?
  Is the Data transfer direction reasonable?

Arguments:

  Packet         -  The pointer of EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET   

Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
SubmitExtBlockingIoCommand (
  ATAPI_SCSI_PASS_THRU_DEV                      *AtapiScsiPrivate,
  UINT8                                         Target,
  EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET    *Packet
  )
/*++

Routine Description:

  Performs blocking I/O request.
    
Arguments:

  AtapiScsiPrivate:   Private data structure for the specified channel.
  Target:             The Target ID of the ATAPI device to send the SCSI 
                      Request Packet. To ATAPI devices attached on an IDE
                      Channel, Target ID 0 indicates Master device;Target
                      ID 1 indicates Slave device.
  Packet:             The SCSI Request Packet to send to the ATAPI device 
                      specified by Target.
  
  Returns:            EFI_STATUS  
  
--*/
;

EFI_STATUS
RequestSenseCommand (
  ATAPI_SCSI_PASS_THRU_DEV    *AtapiScsiPrivate,
  UINT32                      Target,
  UINT64                      Timeout,
  VOID                        *SenseData,
  UINT8                       *SenseDataLength
  )
/*++

Routine Description:

  Submit request sense command

Arguments:

  AtapiScsiPrivate  - The pointer of ATAPI_SCSI_PASS_THRU_DEV
  Target            - The target ID
  Timeout           - The time to complete the command
  SenseData         - The buffer to fill in sense data
  SenseDataLength   - The length of buffer

Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
AtapiPacketCommand (
  ATAPI_SCSI_PASS_THRU_DEV                  *AtapiScsiPrivate,
  UINT32                                    Target,
  UINT8                                     *PacketCommand,
  VOID                                      *Buffer,
  UINT32                                    *ByteCount,
  DATA_DIRECTION                            Direction,
  UINT64                                    TimeOutInMicroSeconds
  )
/*++

Routine Description:

  Submits ATAPI command packet to the specified ATAPI device.
    
Arguments:

  AtapiScsiPrivate:   Private data structure for the specified channel.
  Target:             The Target ID of the ATAPI device to send the SCSI 
                      Request Packet. To ATAPI devices attached on an IDE
                      Channel, Target ID 0 indicates Master device;Target
                      ID 1 indicates Slave device.
  PacketCommand:      Points to the ATAPI command packet.
  Buffer:             Points to the transferred data.
  ByteCount:          When input,indicates the buffer size; when output,
                      indicates the actually transferred data size.
  Direction:          Indicates the data transfer direction. 
  TimeoutInMicroSeconds:
                      The timeout, in micro second units, to use for the 
                      execution of this ATAPI command.
                      A TimeoutInMicroSeconds value of 0 means that 
                      this function will wait indefinitely for the ATAPI 
                      command to execute.
                      If TimeoutInMicroSeconds is greater than zero, then 
                      this function will return EFI_TIMEOUT if the time 
                      required to execute the ATAPI command is greater 
                      than TimeoutInMicroSeconds.
  
Returns:

  EFI_STATUS

--*/
;


UINT8
ReadPortB (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port
  )
/*++

Routine Description:

  Read one byte from a specified I/O port.

Arguments:

  PciIo      - The pointer of EFI_PCI_IO_PROTOCOL
  Port       - IO port
  
Returns:

  A byte read out

--*/
;


UINT16
ReadPortW (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port
  )
/*++

Routine Description:

  Read one word from a specified I/O port.

Arguments:

  PciIo      - The pointer of EFI_PCI_IO_PROTOCOL
  Port       - IO port
  
Returns:     

  A word read out

--*/
;


VOID
WritePortB (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINT8                 Data
  )
/*++

Routine Description:

  Write one byte to a specified I/O port.

Arguments:

  PciIo      - The pointer of EFI_PCI_IO_PROTOCOL
  Port       - IO port
  Data       - The data to write
  
Returns:
 
  NONE
 
--*/
;


VOID
WritePortW (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINT16                Data
  )
/*++

Routine Description:

  Write one word to a specified I/O port.

Arguments:

  PciIo      - The pointer of EFI_PCI_IO_PROTOCOL
  Port       - IO port
  Data       - The data to write
  
Returns:

  NONE
  
--*/
;

EFI_STATUS
StatusDRQClear (
  ATAPI_SCSI_PASS_THRU_DEV        *AtapiScsiPrivate,
  UINT64                          TimeOutInMicroSeconds
  )
/*++

Routine Description:

  Check whether DRQ is clear in the Status Register. (BSY must also be cleared)
  If TimeoutInMicroSeconds is zero, this routine should wait infinitely for
  DRQ clear. Otherwise, it will return EFI_TIMEOUT when specified time is 
  elapsed.

Arguments:

  AtapiScsiPrivate            - The pointer of ATAPI_SCSI_PASS_THRU_DEV
  TimeoutInMicroSeconds       - The time to wait for
   
Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
AltStatusDRQClear (
  ATAPI_SCSI_PASS_THRU_DEV        *AtapiScsiPrivate,
  UINT64                          TimeOutInMicroSeconds
  )
/*++

Routine Description:

  Check whether DRQ is clear in the Alternate Status Register. 
  (BSY must also be cleared).If TimeoutInMicroSeconds is zero, this routine should 
  wait infinitely for DRQ clear. Otherwise, it will return EFI_TIMEOUT when specified time is 
  elapsed.

Arguments:

  AtapiScsiPrivate            - The pointer of ATAPI_SCSI_PASS_THRU_DEV
  TimeoutInMicroSeconds       - The time to wait for
   
Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
StatusDRQReady (
  ATAPI_SCSI_PASS_THRU_DEV        *AtapiScsiPrivate,
  UINT64                          TimeOutInMicroSeconds
  )
/*++

Routine Description:

  Check whether DRQ is ready in the Status Register. (BSY must also be cleared)
  If TimeoutInMicroSeconds is zero, this routine should wait infinitely for
  DRQ ready. Otherwise, it will return EFI_TIMEOUT when specified time is 
  elapsed.

Arguments:

  AtapiScsiPrivate            - The pointer of ATAPI_SCSI_PASS_THRU_DEV
  TimeoutInMicroSeconds       - The time to wait for
   
Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
AltStatusDRQReady (
  ATAPI_SCSI_PASS_THRU_DEV        *AtapiScsiPrivate,
  UINT64                          TimeOutInMicroSeconds
  )
/*++

Routine Description:

  Check whether DRQ is ready in the Alternate Status Register. 
  (BSY must also be cleared)
  If TimeoutInMicroSeconds is zero, this routine should wait infinitely for
  DRQ ready. Otherwise, it will return EFI_TIMEOUT when specified time is 
  elapsed.

Arguments:

  AtapiScsiPrivate            - The pointer of ATAPI_SCSI_PASS_THRU_DEV
  TimeoutInMicroSeconds       - The time to wait for
   
Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
StatusWaitForBSYClear (
  ATAPI_SCSI_PASS_THRU_DEV    *AtapiScsiPrivate,
  UINT64                      TimeoutInMicroSeconds
  )
/*++

Routine Description:

  Check whether BSY is clear in the Status Register.
  If TimeoutInMicroSeconds is zero, this routine should wait infinitely for
  BSY clear. Otherwise, it will return EFI_TIMEOUT when specified time is 
  elapsed.

Arguments:

  AtapiScsiPrivate            - The pointer of ATAPI_SCSI_PASS_THRU_DEV
  TimeoutInMicroSeconds       - The time to wait for
   
Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
AltStatusWaitForBSYClear (
  ATAPI_SCSI_PASS_THRU_DEV    *AtapiScsiPrivate,
  UINT64                      TimeoutInMicroSeconds
  )
/*++

Routine Description:

  Check whether BSY is clear in the Alternate Status Register.
  If TimeoutInMicroSeconds is zero, this routine should wait infinitely for
  BSY clear. Otherwise, it will return EFI_TIMEOUT when specified time is 
  elapsed.

Arguments:

  AtapiScsiPrivate            - The pointer of ATAPI_SCSI_PASS_THRU_DEV
  TimeoutInMicroSeconds       - The time to wait for
   
Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
StatusDRDYReady (
  ATAPI_SCSI_PASS_THRU_DEV    *AtapiScsiPrivate,
  UINT64                      TimeoutInMicroSeconds
  )
/*++

Routine Description:

  Check whether DRDY is ready in the Status Register. 
  (BSY must also be cleared)
  If TimeoutInMicroSeconds is zero, this routine should wait infinitely for
  DRDY ready. Otherwise, it will return EFI_TIMEOUT when specified time is 
  elapsed.

Arguments:

  AtapiScsiPrivate            - The pointer of ATAPI_SCSI_PASS_THRU_DEV
  TimeoutInMicroSeconds       - The time to wait for
   
Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
AltStatusDRDYReady (
  ATAPI_SCSI_PASS_THRU_DEV    *AtapiScsiPrivate,
  UINT64                      TimeoutInMicroSeconds
  )
/*++

Routine Description:

  Check whether DRDY is ready in the Alternate Status Register. 
  (BSY must also be cleared)
  If TimeoutInMicroSeconds is zero, this routine should wait infinitely for
  DRDY ready. Otherwise, it will return EFI_TIMEOUT when specified time is 
  elapsed.

Arguments:

  AtapiScsiPrivate            - The pointer of ATAPI_SCSI_PASS_THRU_DEV
  TimeoutInMicroSeconds       - The time to wait for
   
Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
AtapiPassThruPioReadWriteData (
  ATAPI_SCSI_PASS_THRU_DEV  *AtapiScsiPrivate,
  UINT16                    *Buffer,
  UINT32                    *ByteCount,
  DATA_DIRECTION            Direction,
  UINT64                    TimeOutInMicroSeconds
  )
/*++

Routine Description:

  Performs data transfer between ATAPI device and host after the
  ATAPI command packet is sent.
    
Arguments:

  AtapiScsiPrivate:   Private data structure for the specified channel.    
  Buffer:             Points to the transferred data.
  ByteCount:          When input,indicates the buffer size; when output,
                      indicates the actually transferred data size.
  Direction:          Indicates the data transfer direction. 
  TimeoutInMicroSeconds:
                      The timeout, in micro second units, to use for the 
                      execution of this ATAPI command.
                      A TimeoutInMicroSeconds value of 0 means that 
                      this function will wait indefinitely for the ATAPI 
                      command to execute.
                      If TimeoutInMicroSeconds is greater than zero, then 
                      this function will return EFI_TIMEOUT if the time 
                      required to execute the ATAPI command is greater 
                      than TimeoutInMicroSeconds.
 Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
AtapiPassThruCheckErrorStatus (
  ATAPI_SCSI_PASS_THRU_DEV        *AtapiScsiPrivate
  )
/*++

Routine Description:

  Check Error Register for Error Information. 
  
Arguments:

  AtapiScsiPrivate            - The pointer of ATAPI_SCSI_PASS_THRU_DEV
   
Returns:

  EFI_STATUS

--*/
;


EFI_STATUS
GetIdeRegistersBaseAddr (
  IN  EFI_PCI_IO_PROTOCOL         *PciIo,
  OUT IDE_REGISTERS_BASE_ADDR     *IdeRegsBaseAddr
  )
/*++

Routine Description:
  Get IDE IO port registers' base addresses by mode. In 'Compatibility' mode,
  use fixed addresses. In Native-PCI mode, get base addresses from BARs in
  the PCI IDE controller's Configuration Space.

Arguments:
  PciIo             - Pointer to the EFI_PCI_IO_PROTOCOL instance
  IdeRegsBaseAddr   - Pointer to IDE_REGISTERS_BASE_ADDR to 
                      receive IDE IO port registers' base addresses
                      
Returns:

  EFI_STATUS
    
--*/
;


VOID
InitAtapiIoPortRegisters (
  IN  ATAPI_SCSI_PASS_THRU_DEV     *AtapiScsiPrivate,
  IN  IDE_REGISTERS_BASE_ADDR      *IdeRegsBaseAddr
  )
/*++

Routine Description:

  Initialize each Channel's Base Address of CommandBlock and ControlBlock.

Arguments:
    
  AtapiScsiPrivate            - The pointer of ATAPI_SCSI_PASS_THRU_DEV
  IdeRegsBaseAddr             - The pointer of IDE_REGISTERS_BASE_ADDR
  
Returns:
  
  None

--*/  
;

/**
  Installs Scsi Pass Thru and/or Ext Scsi Pass Thru 
  protocols based on feature flags. 

  @param Controller         The controller handle to 
                            install these protocols on.
  @param AtapiScsiPrivate   A pointer to the protocol private
                            data structure.

  @retval EFI_SUCCESS       The installation succeeds. 
  @retval other             The installation fails. 
   
**/
EFI_STATUS
InstallScsiPassThruProtocols (
  IN EFI_HANDLE                 *ControllerHandle,
  IN ATAPI_SCSI_PASS_THRU_DEV   *AtapiScsiPrivate
  );

#endif
