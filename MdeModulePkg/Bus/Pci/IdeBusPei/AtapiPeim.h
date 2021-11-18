/** @file
Private Include file for IdeBus PEIM.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RECOVERY_ATAPI_H_
#define _RECOVERY_ATAPI_H_

#include <PiPei.h>

#include <Ppi/BlockIo.h>
#include <Ppi/BlockIo2.h>
#include <Ppi/AtaController.h>

#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/Atapi.h>

#define MAX_SENSE_KEY_COUNT  6
#define MAX_IDE_CHANNELS     4 // Ide and Sata Primary, Secondary Channel.
#define MAX_IDE_DEVICES      8 // Ide, Sata Primary, Secondary and Master, Slave device.

typedef enum {
  IdePrimary    = 0,
  IdeSecondary  = 1,
  IdeMaxChannel = 2
} EFI_IDE_CHANNEL;

typedef enum {
  IdeMaster    = 0,
  IdeSlave     = 1,
  IdeMaxDevice = 2
} EFI_IDE_DEVICE;

//
// IDE Registers
//
typedef union {
  UINT16    Command;      /* when write */
  UINT16    Status;       /* when read */
} IDE_CMD_OR_STATUS;

typedef union {
  UINT16    Error;        /* when read */
  UINT16    Feature;      /* when write */
} IDE_ERROR_OR_FEATURE;

typedef union {
  UINT16    AltStatus;     /* when read */
  UINT16    DeviceControl; /* when write */
} IDE_ALTSTATUS_OR_DEVICECONTROL;

//
// IDE registers set
//
typedef struct {
  UINT16                            Data;
  IDE_ERROR_OR_FEATURE              Reg1;
  UINT16                            SectorCount;
  UINT16                            SectorNumber;
  UINT16                            CylinderLsb;
  UINT16                            CylinderMsb;
  UINT16                            Head;
  IDE_CMD_OR_STATUS                 Reg;

  IDE_ALTSTATUS_OR_DEVICECONTROL    Alt;
  UINT16                            DriveAddress;
} IDE_BASE_REGISTERS;

typedef struct {
  UINTN                      DevicePosition;
  EFI_PEI_BLOCK_IO_MEDIA     MediaInfo;
  EFI_PEI_BLOCK_IO2_MEDIA    MediaInfo2;
} PEI_ATAPI_DEVICE_INFO;

#define ATAPI_BLK_IO_DEV_SIGNATURE  SIGNATURE_32 ('a', 'b', 'i', 'o')
typedef struct {
  UINTN                             Signature;

  EFI_PEI_RECOVERY_BLOCK_IO_PPI     AtapiBlkIo;
  EFI_PEI_RECOVERY_BLOCK_IO2_PPI    AtapiBlkIo2;
  EFI_PEI_PPI_DESCRIPTOR            PpiDescriptor;
  EFI_PEI_PPI_DESCRIPTOR            PpiDescriptor2;
  PEI_ATA_CONTROLLER_PPI            *AtaControllerPpi;

  UINTN                             DeviceCount;
  PEI_ATAPI_DEVICE_INFO             DeviceInfo[MAX_IDE_DEVICES];    // for max 8 device
  IDE_BASE_REGISTERS                IdeIoPortReg[MAX_IDE_CHANNELS]; // for max 4 channel.
} ATAPI_BLK_IO_DEV;

#define PEI_RECOVERY_ATAPI_FROM_BLKIO_THIS(a)   CR (a, ATAPI_BLK_IO_DEV, AtapiBlkIo, ATAPI_BLK_IO_DEV_SIGNATURE)
#define PEI_RECOVERY_ATAPI_FROM_BLKIO2_THIS(a)  CR (a, ATAPI_BLK_IO_DEV, AtapiBlkIo2, ATAPI_BLK_IO_DEV_SIGNATURE)

#define STALL_1_MILLI_SECOND  1000  // stall 1 ms
#define STALL_1_SECONDS       1000 * STALL_1_MILLI_SECOND

//
// Time Out Value For IDE Device Polling
//
// ATATIMEOUT is used for waiting time out for ATA device
//
#define ATATIMEOUT  1000  // 1 second
// ATAPITIMEOUT is used for waiting operation
// except read and write time out for ATAPI device
//
#define ATAPITIMEOUT  1000  // 1 second
// ATAPILONGTIMEOUT is used for waiting read and
// write operation timeout for ATAPI device
//
#define CDROMLONGTIMEOUT  2000  // 2 seconds
#define ATAPILONGTIMEOUT  5000  // 5 seconds

//
// PEI Recovery Block I/O PPI
//

/**
  Gets the count of block I/O devices that one specific block driver detects.

  This function is used for getting the count of block I/O devices that one
  specific block driver detects.  To the PEI ATAPI driver, it returns the number
  of all the detected ATAPI devices it detects during the enumeration process.
  To the PEI legacy floppy driver, it returns the number of all the legacy
  devices it finds during its enumeration process. If no device is detected,
  then the function will return zero.

  @param[in]  PeiServices          General-purpose services that are available
                                   to every PEIM.
  @param[in]  This                 Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI
                                   instance.
  @param[out] NumberBlockDevices   The number of block I/O devices discovered.

  @retval     EFI_SUCCESS          Operation performed successfully.

**/
EFI_STATUS
EFIAPI
AtapiGetNumberOfBlockDevices (
  IN   EFI_PEI_SERVICES               **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  OUT  UINTN                          *NumberBlockDevices
  );

/**
  Gets a block device's media information.

  This function will provide the caller with the specified block device's media
  information. If the media changes, calling this function will update the media
  information accordingly.

  @param[in]  PeiServices   General-purpose services that are available to every
                            PEIM
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, the PPIs that
                            want to talk to a single device must specify the
                            device index that was assigned during the enumeration
                            process. This index is a number from one to
                            NumberBlockDevices.
  @param[out] MediaInfo     The media information of the specified block media.
                            The caller is responsible for the ownership of this
                            data structure.

  @retval EFI_SUCCESS           Media information about the specified block device
                                was obtained successfully.
  @retval EFI_DEVICE_ERROR      Cannot get the media information due to a hardware
                                error.
  @retval Others                Other failure occurs.

**/
EFI_STATUS
EFIAPI
AtapiGetBlockDeviceMediaInfo (
  IN   EFI_PEI_SERVICES               **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  IN   UINTN                          DeviceIndex,
  OUT  EFI_PEI_BLOCK_IO_MEDIA         *MediaInfo
  );

/**
  Reads the requested number of blocks from the specified block device.

  The function reads the requested number of blocks from the device. All the
  blocks are read, or an error is returned. If there is no media in the device,
  the function returns EFI_NO_MEDIA.

  @param[in]  PeiServices   General-purpose services that are available to
                            every PEIM.
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, the PPIs that
                            want to talk to a single device must specify the device
                            index that was assigned during the enumeration process.
                            This index is a number from one to NumberBlockDevices.
  @param[in]  StartLBA      The starting logical block address (LBA) to read from
                            on the device
  @param[in]  BufferSize    The size of the Buffer in bytes. This number must be
                            a multiple of the intrinsic block size of the device.
  @param[out] Buffer        A pointer to the destination buffer for the data.
                            The caller is responsible for the ownership of the
                            buffer.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the read operation.
  @retval EFI_INVALID_PARAMETER   The read request contains LBAs that are not
                                  valid, or the buffer is not properly aligned.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_BAD_BUFFER_SIZE     The BufferSize parameter is not a multiple of
                                  the intrinsic block size of the device.

**/
EFI_STATUS
EFIAPI
AtapiReadBlocks (
  IN   EFI_PEI_SERVICES               **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  IN   UINTN                          DeviceIndex,
  IN   EFI_PEI_LBA                    StartLBA,
  IN   UINTN                          BufferSize,
  OUT  VOID                           *Buffer
  );

/**
  Gets the count of block I/O devices that one specific block driver detects.

  This function is used for getting the count of block I/O devices that one
  specific block driver detects.  To the PEI ATAPI driver, it returns the number
  of all the detected ATAPI devices it detects during the enumeration process.
  To the PEI legacy floppy driver, it returns the number of all the legacy
  devices it finds during its enumeration process. If no device is detected,
  then the function will return zero.

  @param[in]  PeiServices          General-purpose services that are available
                                   to every PEIM.
  @param[in]  This                 Indicates the EFI_PEI_RECOVERY_BLOCK_IO2_PPI
                                   instance.
  @param[out] NumberBlockDevices   The number of block I/O devices discovered.

  @retval     EFI_SUCCESS          Operation performed successfully.

**/
EFI_STATUS
EFIAPI
AtapiGetNumberOfBlockDevices2 (
  IN   EFI_PEI_SERVICES                **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *This,
  OUT  UINTN                           *NumberBlockDevices
  );

/**
  Gets a block device's media information.

  This function will provide the caller with the specified block device's media
  information. If the media changes, calling this function will update the media
  information accordingly.

  @param[in]  PeiServices   General-purpose services that are available to every
                            PEIM
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO2_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, the PPIs that
                            want to talk to a single device must specify the
                            device index that was assigned during the enumeration
                            process. This index is a number from one to
                            NumberBlockDevices.
  @param[out] MediaInfo     The media information of the specified block media.
                            The caller is responsible for the ownership of this
                            data structure.

  @retval EFI_SUCCESS           Media information about the specified block device
                                was obtained successfully.
  @retval EFI_DEVICE_ERROR      Cannot get the media information due to a hardware
                                error.
  @retval Others                Other failure occurs.

**/
EFI_STATUS
EFIAPI
AtapiGetBlockDeviceMediaInfo2 (
  IN   EFI_PEI_SERVICES                **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *This,
  IN   UINTN                           DeviceIndex,
  OUT  EFI_PEI_BLOCK_IO2_MEDIA         *MediaInfo
  );

/**
  Reads the requested number of blocks from the specified block device.

  The function reads the requested number of blocks from the device. All the
  blocks are read, or an error is returned. If there is no media in the device,
  the function returns EFI_NO_MEDIA.

  @param[in]  PeiServices   General-purpose services that are available to
                            every PEIM.
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO2_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, the PPIs that
                            want to talk to a single device must specify the device
                            index that was assigned during the enumeration process.
                            This index is a number from one to NumberBlockDevices.
  @param[in]  StartLBA      The starting logical block address (LBA) to read from
                            on the device
  @param[in]  BufferSize    The size of the Buffer in bytes. This number must be
                            a multiple of the intrinsic block size of the device.
  @param[out] Buffer        A pointer to the destination buffer for the data.
                            The caller is responsible for the ownership of the
                            buffer.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the read operation.
  @retval EFI_INVALID_PARAMETER   The read request contains LBAs that are not
                                  valid, or the buffer is not properly aligned.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_BAD_BUFFER_SIZE     The BufferSize parameter is not a multiple of
                                  the intrinsic block size of the device.

**/
EFI_STATUS
EFIAPI
AtapiReadBlocks2 (
  IN   EFI_PEI_SERVICES                **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *This,
  IN   UINTN                           DeviceIndex,
  IN   EFI_PEI_LBA                     StartLBA,
  IN   UINTN                           BufferSize,
  OUT  VOID                            *Buffer
  );

//
// Internal functions
//

/**
  Enumerate Atapi devices.

  This function is used to enumerate Atatpi device in Ide channel.

  @param[in]  AtapiBlkIoDev  A pointer to atapi block IO device

**/
VOID
AtapiEnumerateDevices (
  IN  ATAPI_BLK_IO_DEV  *AtapiBlkIoDev
  );

/**
  Detect Atapi devices.

  @param[in]  AtapiBlkIoDev   A pointer to atapi block IO device.
  @param[in]  DevicePosition  An integer to signify device position.
  @param[out] MediaInfo       The media information of the specified block media.
  @param[out] MediaInfo2      The media information 2 of the specified block media.

  @retval TRUE                Atapi device exists in specified position.
  @retval FALSE               Atapi device does not exist in specified position.

**/
BOOLEAN
DiscoverAtapiDevice (
  IN  ATAPI_BLK_IO_DEV         *AtapiBlkIoDev,
  IN  UINTN                    DevicePosition,
  OUT EFI_PEI_BLOCK_IO_MEDIA   *MediaInfo,
  OUT EFI_PEI_BLOCK_IO2_MEDIA  *MediaInfo2
  );

/**
  Detect if an IDE controller exists in specified position.

  @param[in]  AtapiBlkIoDev   A pointer to atapi block IO device.
  @param[in]  DevicePosition  An integer to signify device position.

  @retval TRUE         The Atapi device exists.
  @retval FALSE        The Atapi device does not present.

**/
BOOLEAN
DetectIDEController (
  IN  ATAPI_BLK_IO_DEV  *AtapiBlkIoDev,
  IN  UINTN             DevicePosition
  );

/**
  Wait specified time interval to poll for BSY bit clear in the Status Register.

  @param[in]  AtapiBlkIoDev          A pointer to atapi block IO device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        BSY bit is cleared in the specified time interval.
  @retval EFI_TIMEOUT        BSY bit is not cleared in the specified time interval.

**/
EFI_STATUS
WaitForBSYClear (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  );

/**
  Wait specified time interval to poll for DRDY bit set in the Status register.

  @param[in]  AtapiBlkIoDev          A pointer to atapi block IO device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRDY bit is set in the specified time interval.
  @retval EFI_TIMEOUT        DRDY bit is not set in the specified time interval.

**/
EFI_STATUS
DRDYReady (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  );

/**
  Wait specified time interval to poll for DRQ bit clear in the Status Register.

  @param[in]  AtapiBlkIoDev          A pointer to atapi block IO device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRQ bit is cleared in the specified time interval.
  @retval EFI_TIMEOUT        DRQ bit is not cleared in the specified time interval.

**/
EFI_STATUS
DRQClear (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  );

/**
  Wait specified time interval to poll for DRQ bit clear in the Alternate Status Register.

  @param[in]  AtapiBlkIoDev          A pointer to atapi block IO device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRQ bit is cleared in the specified time interval.
  @retval EFI_TIMEOUT        DRQ bit is not cleared in the specified time interval.

**/
EFI_STATUS
DRQClear2 (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  );

/**
  Wait specified time interval to poll for DRQ bit set in the Status Register.

  @param[in]  AtapiBlkIoDev          A pointer to atapi block IO device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRQ bit is set in the specified time interval.
  @retval EFI_TIMEOUT        DRQ bit is not set in the specified time interval.
  @retval EFI_ABORTED        Operation Aborted.

**/
EFI_STATUS
DRQReady (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  );

/**
  Wait specified time interval to poll for DRQ bit set in the Alternate Status Register.

  @param[in]  AtapiBlkIoDev          A pointer to atapi block IO device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRQ bit is set in the specified time interval.
  @retval EFI_TIMEOUT        DRQ bit is not set in the specified time interval.
  @retval EFI_ABORTED        Operation Aborted.

**/
EFI_STATUS
DRQReady2 (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  );

/**
  Check if there is an error in Status Register.

  @param[in]  AtapiBlkIoDev     A pointer to atapi block IO device.
  @param[in]  StatusReg         The address to IDE IO registers.

  @retval EFI_SUCCESS        Operation success.
  @retval EFI_DEVICE_ERROR   Device error.

**/
EFI_STATUS
CheckErrorStatus (
  IN  ATAPI_BLK_IO_DEV  *AtapiBlkIoDev,
  IN  UINT16            StatusReg
  );

/**
  Idendify Atapi devices.

  @param[in]  AtapiBlkIoDev     A pointer to atapi block IO device.
  @param[in]  DevicePosition    An integer to signify device position.

  @retval EFI_SUCCESS        Identify successfully.
  @retval EFI_DEVICE_ERROR   Device cannot be identified successfully.

**/
EFI_STATUS
ATAPIIdentify (
  IN  ATAPI_BLK_IO_DEV  *AtapiBlkIoDev,
  IN  UINTN             DevicePosition
  );

/**
  Sends out ATAPI Test Unit Ready Packet Command to the specified device
  to find out whether device is accessible.

  @param[in]  AtapiBlkIoDev     A pointer to atapi block IO device.
  @param[in]  DevicePosition    An integer to signify device position.

  @retval EFI_SUCCESS        TestUnit command executed successfully.
  @retval EFI_DEVICE_ERROR   Device cannot be executed TestUnit command successfully.

**/
EFI_STATUS
TestUnitReady (
  IN  ATAPI_BLK_IO_DEV  *AtapiBlkIoDev,
  IN  UINTN             DevicePosition
  );

/**
  Send out ATAPI commands conforms to the Packet Command with PIO Data In Protocol.

  @param[in]  AtapiBlkIoDev         A pointer to atapi block IO device.
  @param[in]  DevicePosition        An integer to signify device position.
  @param[in]  Packet                A pointer to ATAPI command packet.
  @param[in]  Buffer                Buffer to contain requested transfer data from device.
  @param[in]  ByteCount             Requested transfer data length.
  @param[in]  TimeoutInMilliSeconds Time out value, in unit of milliseconds.

  @retval EFI_SUCCESS        Command executed successfully.
  @retval EFI_DEVICE_ERROR   Device cannot be executed command successfully.

**/
EFI_STATUS
AtapiPacketCommandIn (
  IN  ATAPI_BLK_IO_DEV      *AtapiBlkIoDev,
  IN  UINTN                 DevicePosition,
  IN  ATAPI_PACKET_COMMAND  *Packet,
  IN  UINT16                *Buffer,
  IN  UINT32                ByteCount,
  IN  UINTN                 TimeoutInMilliSeconds
  );

/**
  Sends out ATAPI Inquiry Packet Command to the specified device.
  This command will return INQUIRY data of the device.

  @param[in]  AtapiBlkIoDev   A pointer to atapi block IO device.
  @param[in]  DevicePosition  An integer to signify device position.
  @param[out] MediaInfo       The media information of the specified block media.
  @param[out] MediaInfo2      The media information 2 of the specified block media.

  @retval EFI_SUCCESS        Command executed successfully.
  @retval EFI_DEVICE_ERROR   Device cannot be executed command successfully.
  @retval EFI_UNSUPPORTED    Unsupported device type.

**/
EFI_STATUS
Inquiry (
  IN  ATAPI_BLK_IO_DEV         *AtapiBlkIoDev,
  IN  UINTN                    DevicePosition,
  OUT EFI_PEI_BLOCK_IO_MEDIA   *MediaInfo,
  OUT EFI_PEI_BLOCK_IO2_MEDIA  *MediaInfo2
  );

/**
  Used before read/write blocks from/to ATAPI device media.
  Since ATAPI device media is removable, it is necessary to detect
  whether media is present and get current present media's information.

  @param[in]  AtapiBlkIoDev     A pointer to atapi block IO device.
  @param[in]  DevicePosition    An integer to signify device position.
  @param[in, out] MediaInfo     The media information of the specified block media.
  @param[in, out] MediaInfo2    The media information 2 of the specified block media.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.
  @retval EFI_OUT_OF_RESOURCES  Can not allocate required resources.

**/
EFI_STATUS
DetectMedia (
  IN  ATAPI_BLK_IO_DEV            *AtapiBlkIoDev,
  IN  UINTN                       DevicePosition,
  IN OUT EFI_PEI_BLOCK_IO_MEDIA   *MediaInfo,
  IN OUT EFI_PEI_BLOCK_IO2_MEDIA  *MediaInfo2
  );

/**
  Reset specified Atapi device.

  @param[in]  AtapiBlkIoDev     A pointer to atapi block IO device.
  @param[in]  DevicePosition    An integer to signify device position.
  @param[in]  Extensive         If TRUE, use ATA soft reset, otherwise use Atapi soft reset.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
ResetDevice (
  IN  ATAPI_BLK_IO_DEV  *AtapiBlkIoDev,
  IN  UINTN             DevicePosition,
  IN  BOOLEAN           Extensive
  );

/**
  Sends out ATAPI Request Sense Packet Command to the specified device.

  @param[in]      AtapiBlkIoDev   A pointer to atapi block IO device.
  @param[in]      DevicePosition  An integer to signify device position.
  @param[in]      SenseBuffers    Pointer to sense buffer.
  @param[in, out] SenseCounts     Length of sense buffer.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
RequestSense (
  IN  ATAPI_BLK_IO_DEV          *AtapiBlkIoDev,
  IN  UINTN                     DevicePosition,
  IN  ATAPI_REQUEST_SENSE_DATA  *SenseBuffers,
  IN OUT  UINT8                 *SenseCounts
  );

/**
  Sends out ATAPI Read Capacity Packet Command to the specified device.
  This command will return the information regarding the capacity of the
  media in the device.

  @param[in]  AtapiBlkIoDev   A pointer to atapi block IO device.
  @param[in]  DevicePosition  An integer to signify device position.
  @param[in, out] MediaInfo   The media information of the specified block media.
  @param[in, out] MediaInfo2  The media information 2 of the specified block media.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
ReadCapacity (
  IN  ATAPI_BLK_IO_DEV            *AtapiBlkIoDev,
  IN  UINTN                       DevicePosition,
  IN OUT EFI_PEI_BLOCK_IO_MEDIA   *MediaInfo,
  IN OUT EFI_PEI_BLOCK_IO2_MEDIA  *MediaInfo2
  );

/**
  Perform read from disk in block unit.

  @param[in]  AtapiBlkIoDev   A pointer to atapi block IO device.
  @param[in]  DevicePosition  An integer to signify device position.
  @param[in]  Buffer          Buffer to contain read data.
  @param[in]  StartLba        Starting LBA address.
  @param[in]  NumberOfBlocks  Number of blocks to read.
  @param[in]  BlockSize       Size of each block.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
ReadSectors (
  IN  ATAPI_BLK_IO_DEV  *AtapiBlkIoDev,
  IN  UINTN             DevicePosition,
  IN  VOID              *Buffer,
  IN  EFI_PEI_LBA       StartLba,
  IN  UINTN             NumberOfBlocks,
  IN  UINTN             BlockSize
  );

/**
  Check if there is media according to sense data.

  @param[in]  SenseData   Pointer to sense data.
  @param[in]  SenseCounts Count of sense data.

  @retval TRUE    No media
  @retval FALSE   Media exists

**/
BOOLEAN
IsNoMedia (
  IN  ATAPI_REQUEST_SENSE_DATA  *SenseData,
  IN  UINTN                     SenseCounts
  );

/**
  Check if device state is unclear according to sense data.

  @param[in]  SenseData   Pointer to sense data.
  @param[in]  SenseCounts Count of sense data.

  @retval TRUE    Device state is unclear
  @retval FALSE   Device state is clear

**/
BOOLEAN
IsDeviceStateUnclear (
  IN  ATAPI_REQUEST_SENSE_DATA  *SenseData,
  IN  UINTN                     SenseCounts
  );

/**
  Check if there is media error according to sense data.

  @param[in]  SenseData   Pointer to sense data.
  @param[in]  SenseCounts Count of sense data.

  @retval TRUE    Media error
  @retval FALSE   No media error

**/
BOOLEAN
IsMediaError (
  IN  ATAPI_REQUEST_SENSE_DATA  *SenseData,
  IN  UINTN                     SenseCounts
  );

/**
  Check if drive is ready according to sense data.

  @param[in]  SenseData   Pointer to sense data.
  @param[in]  SenseCounts Count of sense data.
  @param[out] NeedRetry   Indicate if retry is needed.

  @retval TRUE    Drive ready
  @retval FALSE   Drive not ready

**/
BOOLEAN
IsDriveReady (
  IN  ATAPI_REQUEST_SENSE_DATA  *SenseData,
  IN  UINTN                     SenseCounts,
  OUT BOOLEAN                   *NeedRetry
  );

#endif
