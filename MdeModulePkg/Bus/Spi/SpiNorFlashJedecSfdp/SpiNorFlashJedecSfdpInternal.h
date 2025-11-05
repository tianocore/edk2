/** @file
  SPI NOR flash driver internal definitions.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPI_NOR_FLASH_INSTANCE_H_
#define SPI_NOR_FLASH_INSTANCE_H_

#include <PiDxe.h>
#include <Protocol/SpiNorFlash.h>
#include <Protocol/SpiIo.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>

#define SPI_NOR_FLASH_SIGNATURE  SIGNATURE_32 ('s', 'n', 'f', 'm')

#define SPI_NOR_FLASH_FROM_THIS(a)  CR (a, SPI_NOR_FLASH_INSTANCE, Protocol, SPI_NOR_FLASH_SIGNATURE)

typedef struct {
  LIST_ENTRY    NextFastReadCap;     ///< Link list to next Fast read capability
  UINT8         FastReadInstruction; ///< Fast read instruction.
  UINT8         ModeClocks;          ///< Fast read clock.
  UINT8         WaitStates;          ///< Fast read wait dummy clocks
} SFPD_FAST_READ_CAPBILITY_RECORD;

typedef struct {
  LIST_ENTRY    NextEraseType;      ///< Link list to next erase type.
  UINT16        EraseType;          ///< Erase type this flash device supports.
  UINT8         EraseInstruction;   ///< Erase instruction
  UINT32        EraseSizeInByte;    ///< The size of byte in 2^EraseSize the erase type command
                                    ///< can erase.
  UINT32        EraseTypicalTime;   ///< Time the device typically takes to erase this type
                                    ///< size.
  UINT64        EraseTimeout;       ///< Maximum typical erase timeout.
} SFDP_SUPPORTED_ERASE_TYPE_RECORD;

typedef enum {
  SearchEraseTypeByType = 1,
  SearchEraseTypeByCommand,
  SearchEraseTypeBySize,
  SearchEraseTypeBySmallestSize,
  SearchEraseTypeByBiggestSize
} SFDP_SEARCH_ERASE_TYPE;

typedef struct {
  LIST_ENTRY                                NextCommand;          ///< Link list to next detection command.
  UINT32                                    CommandAddress;       ///< Address to issue the command.
  UINT8                                     CommandInstruction;   ///< Detection command instruction.
  UINT8                                     LatencyInClock;       ///< Command latency in clocks.
  SPDF_CONFIGURATION_COMMAND_ADDR_LENGTH    CommandAddressLength; ///< Adddress length of detection command.
  UINT8                                     ConfigurationBitMask; ///< The interest bit of the byte data retunred
                                                                  ///< after sending the detection command.
} SFDP_SECTOR_MAP_DETECTION_RECORD;

typedef struct {
  LIST_ENTRY    NextRegion;                                      ///< Link list to the next region.
  UINT32        RegionAddress;                                   ///< Region starting address.
  UINT32        RegionTotalSize;                                 ///< Region total size in bytes.
  UINT32        RegionSectors;                                   ///< Sectors in this region.
  UINT32        SectorSize;                                      ///< Sector size in byte (Minimum blcok erase size)
  UINT8         SupportedEraseTypeNum;                           ///< Number of erase type supported.
  UINT8         SupportedEraseType[SFDP_ERASE_TYPES_NUMBER];     ///< Erase types supported.
  UINT32        EraseTypeBySizeBitmap;                           ///< The bitmap of supoprted srase block sizes.
                                                                 ///< from big to small.
} SFDP_SECTOR_REGION_RECORD;

typedef struct {
  LIST_ENTRY    NextDescriptor;                                  ///< Link list to next flash map descriptor.
  UINT8         ConfigurationId;                                 ///< The ID of this configuration.
  UINT8         RegionCount;                                     ///< The regions of this sector map configuration.
  LIST_ENTRY    RegionList;                                      ///< The linked list of the regions.
} SFDP_SECTOR_MAP_RECORD;

typedef struct {
  UINTN                         Signature;
  EFI_HANDLE                    Handle;
  EFI_SPI_NOR_FLASH_PROTOCOL    Protocol;
  EFI_SPI_IO_PROTOCOL           *SpiIo;
  UINT32                        SfdpBasicFlashByteCount;
  UINT32                        SfdpSectorMapByteCount;
  SFDP_BASIC_FLASH_PARAMETER    *SfdpBasicFlash;
  SFDP_SECTOR_MAP_TABLE         *SfdpFlashSectorMap;
  UINT8                         *SpiTransactionWriteBuffer;
  UINT32                        SpiTransactionWriteBufferIndex;
  //
  // SFDP information.
  //
  SFDP_HEADER                   SfdpHeader;              ///< SFDP header.
  UINT32                        FlashDeviceSize;         ///< The total size of this flash device.
  UINT8                         CurrentAddressBytes;     ///< The current address bytes.

  //
  // This is a linked list in which the Fast Read capability tables
  // are linked from the low performance transfer to higher performance
  // transfer. The SPI read would use the first Fast Read entry for
  // SPI read operation.
  //
  LIST_ENTRY                    FastReadTableList;

  LIST_ENTRY                    SupportedEraseTypes;      ///< The linked list of supported erase types.
  BOOLEAN                       Uniform4KEraseSupported;  ///< The flash device supoprts uniform 4K erase.
  BOOLEAN                       WriteEnableLatchRequired; ///< Wether Write Enable Latch is supported.
  UINT8                         WriteEnableLatchCommand;  ///< Write Enable Latch command.
  //
  // Below is the linked list of flash device sector
  // map configuration detection command and map descriptors.
  //
  BOOLEAN                       ConfigurationCommandsNeeded; ///< Indicates whether sector map
                                                             ///< configuration detection is
                                                             ///< required.
  LIST_ENTRY                    ConfigurationCommandList;    ///< The linked list of configuration
                                                             ///< detection command sequence.
  LIST_ENTRY                    ConfigurationMapList;        ///< The linked list of configuration
                                                             ///< map descriptors.
  SFDP_SECTOR_MAP_RECORD        *CurrentSectorMap;           ///< The current activated flash device
                                                             ///< sector map.
} SPI_NOR_FLASH_INSTANCE;

/**
  This routine returns the desired Fast Read mode.

  @param[in]           Instance                 Spi Nor Flash Instance data with pointer to
                                                EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL
  @param[in,out]       FastReadInstruction      Fast Read instruction, the input is
                                                the default value.
  @param[in,out]       FastReadOperationClock   Fast Read operation clock, the input is
                                                the default value.
  @param[in,out]       FastReadDummyClocks      Fast Read wait state (Dummy clocks), the
                                                input is the default value.
  @retval EFI_SUCCESS     The parameters are updated.
  @retval EFI_NOT_FOUND   No desired Fas Read mode found.

**/
EFI_STATUS
GetFastReadParameter (
  IN     SPI_NOR_FLASH_INSTANCE  *Instance,
  IN OUT UINT8                   *FastReadInstruction,
  IN OUT UINT8                   *FastReadOperationClock,
  IN OUT UINT8                   *FastReadDummyClocks
  );

/**
  Read SFDP parameters into buffer

  This routine reads the JEDEC SPI Flash Discoverable Parameters from the SPI
  chip.

  @param[in]  Instance    Spi Nor Flash Instance data with pointer to
  EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL

  @retval EFI_SUCCESS            The SPI part size is filled.
  @retval EFI_DEVICE_ERROR       Invalid data received from SPI flash part.

**/
EFI_STATUS
ReadSfdpBasicParameterTable (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  );

/**
  Read SFDP Sector Map Parameter into buffer

  This routine reads the JEDEC SPI Flash Discoverable Parameters from the SPI
  chip.

  @param[in]  Instance           Spi Nor Flash Instance data with pointer to
                                 EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL

  @retval EFI_SUCCESS            The SPI part size is filled.
  @retval EFI_DEVICE_ERROR       Invalid data received from SPI flash part.

**/
EFI_STATUS
ReadSfdpSectorMapParameterTable (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  );

/**
  Return flash device size from SFDP Basic Flash Parameter Table DWORD 2

  @param[in]  Instance    Spi Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and
                          EFI_SPI_IO_PROTOCOL.

* @retval   UINT32        Flash device size in byte, zero indicates error.

**/
UINT32
SfdpGetFlashSize (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  );

/**
  Read SFDP
  This routine reads the JEDEC SPI Flash Discoverable Parameters. We just
  read the necessary tables in this routine.

  @param[in]  Instance    Spi Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL

  @retval EFI_SUCCESS            Header is filled in
  @retval EFI_DEVICE_ERROR       Invalid data received from SPI flash part.

**/
EFI_STATUS
ReadSfdp (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  );

/**
  Set EraseBlockBytes in SPI NOR Flash Protocol

  @param[in]  Instance    Spi Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL

  @retval EFI_SUCCESS     The erase block size is returned.
  @retval Otherwise       Failed to get erase block size.

**/
EFI_STATUS
SetSectorEraseBlockSize (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  );

/**
  Get the erase block attribute for the target address.

  @param[in]      Instance              Spi Nor Flash Instance data with pointer to
                                        EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL
  @param[in]      FlashRegion           The region the flash address belong.
  @param[in]      FlashAddress          The target flash address.
  @param[in]      RemainingSize         Remaining size to erase.
  @param[in, out] BlockSizeToErase      Input  - The block erase size for this continious blocks.
                                        Output - The determined block size for erasing.
  @param[in, out] BlockCountToErase     Input  - The expected blocks to erase.
                                        Output - The determined number of blocks to erase.
  @param[out]     BlockEraseCommand     The erase command used for this continious blocks.
  @param[out]     TypicalTime           Pointer to receive the typical time in millisecond
                                        to erase this erase type size.
  @param[out]     MaximumTimeout        Pointer to receive the maximum timeout in millisecond
                                        to erase this erase type size.
  @retval EFI_SUCCESS          The erase block attribute is returned.
  @retval EFI_DEVICE_ERROR     No valid SFDP discovered.
  @retval EFI_NOT_FOUND        No valud erase block attribute found.

**/
EFI_STATUS
GetEraseBlockAttribute (
  IN     SPI_NOR_FLASH_INSTANCE     *Instance,
  IN     SFDP_SECTOR_REGION_RECORD  *FlashRegion,
  IN     UINT32                     FlashAddress,
  IN     UINT32                     RemainingSize,
  IN OUT UINT32                     *BlockSizeToErase,
  IN OUT UINT32                     *BlockCountToErase,
  OUT    UINT8                      *BlockEraseCommand,
  OUT    UINT32                     *TypicalTime,
  OUT    UINT64                     *MaximumTimeout
  );

/**
  Get the erase block attribute for the target address.

  @param[in]   Instance                Spi Nor Flash Instance data with pointer to
                                       EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL
  @param[in]   FlashAddress            The target flash address.
  @param[out]  FlashRegion             The target flash address.

  @retval EFI_SUCCESS             The region is returned.
  @retval EFI_INVALID_PARAMETER   FlashAddress is not belong to any region.
  @retval EFI_INVALID_PARAMETER   Other errors.

**/
EFI_STATUS
GetRegionByFlashAddress (
  IN  SPI_NOR_FLASH_INSTANCE     *Instance,
  IN  UINT32                     FlashAddress,
  OUT SFDP_SECTOR_REGION_RECORD  **FlashRegion
  );

/**
  Initial SPI_NOR_FLASH_INSTANCE structure.

  @param[in]   Instance                Pointer to SPI_NOR_FLASH_INSTANCE.
                                       EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL

  @retval EFI_SUCCESS                  SPI_NOR_FLASH_INSTANCE is initialized according to
                                       SPI NOR Flash SFDP specification.
  @retval Otherwisw                    Failed to initial SPI_NOR_FLASH_INSTANCE structure.

**/
EFI_STATUS
InitialSpiNorFlashSfdpInstance (
  IN SPI_NOR_FLASH_INSTANCE  *Instance
  );

#endif // SPI_NOR_FLASH_INSTANCE_H_
