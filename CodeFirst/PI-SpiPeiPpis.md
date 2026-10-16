# Title: SPI Host, Configuration, I/O, and NOR Flash PPIs <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
## Status: Draft <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
## Document: UEFI Platform Initialization Specification Version 1.8 A <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
## License <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
SPDX-License-Identifier: CC-BY-4.0 <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
## Submitter: [TianoCore Community](https://www.tianocore.org) <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
## Summary of the change <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
Add four PEI PPIs whose structures match the SPI protocols introduced in PI Specification 1.8 A: <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
- EFI_PEI_SPI_HC_PPI <!-- [CODE_FIRST] 13226 -->
- EFI_PEI_SPI_CONFIGURATION_PPI <!-- [CODE_FIRST] 13226 -->
- EFI_PEI_SPI_IO_PPI <!-- [CODE_FIRST] 13226 -->
- EFI_PEI_SPI_NOR_FLASH_PPI <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
The PPIs use new GUIDs. EFI_PEI_SPI_IO_PPI has no single well-known GUID. A producer installs one instance per SPI peripheral, and consumers locate that instance with the peripheral's SpiPeripheralDriverGuid. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
## Benefits of the change <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
PI 1.8 A defines SPI host, configuration, I/O, and NOR flash access only as DXE protocols (and the corresponding SMM protocols). Firmware that must read or write SPI NOR before DXE, including firmware-volume construction and early board configuration, has no standard PEI interface. These PPIs keep the same layering and the same structure layout, so a platform can share one description of its SPI buses between PEI and DXE. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
## Impact of the change <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
This is an additive change. No existing protocol GUID, structure, or calling convention changes. Platforms that do not produce the new PPIs are unaffected. The reference PEIMs in EDK II MdeModulePkg are not part of this specification change. Silicon host controllers, board tables, and firmware volume block drivers remain platform code. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
The C definitions may be typedef aliases of the matching protocol types. The layouts below are normative and are identical, member for member, to the PI 1.8 A protocols. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
## Detailed description of the change [normative updates] <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
### EFI_PEI_SPI_HC_PPI <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
GUID: <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
  gEfiPeiSpiHcPpiGuid = {0x8f8f8b8e, 0x6c2a, 0x4d3e, {0x9a, 0x11, 0x2b, 0x7c, 0x5d, 0x0e, 0x84, 0x21}} <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
Summary: <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
  The SPI host controller PPI. Produced by the SPI host controller PEIM. Consumed by the SPI bus PEIM. The member list and the behavior of ChipSelect, Clock, and Transaction are the same as EFI_SPI_HC_PROTOCOL. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
Members: <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
- UINT32 Attributes <!-- [CODE_FIRST] 13226 -->
- UINT32 FrameSizeSupportMask <!-- [CODE_FIRST] 13226 -->
- UINT32 MaximumTransferBytes <!-- [CODE_FIRST] 13226 -->
- ChipSelect <!-- [CODE_FIRST] 13226 -->
- Clock <!-- [CODE_FIRST] 13226 -->
- Transaction <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
ChipSelect, Clock, and Transaction use the same parameters and return codes as the matching EFI_SPI_HC_PROTOCOL members, including EFI_SUCCESS, EFI_NOT_READY, EFI_INVALID_PARAMETER, EFI_UNSUPPORTED, and EFI_BAD_BUFFER_SIZE where those codes are already defined for the protocol. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
### EFI_PEI_SPI_CONFIGURATION_PPI <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
GUID: <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
  gEfiPeiSpiConfigurationPpiGuid = {0x2d6a1c74, 0x9b3f, 0x4e58, {0xa2, 0x0d, 0x71, 0x36, 0x5f, 0x8c, 0x19, 0xb4}} <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
Summary: <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
  The board SPI configuration PPI. Produced by platform PEI code. Consumed by the SPI bus PEIM. BusCount and Buslist, and the EFI_SPI_BUS, EFI_SPI_PERIPHERAL, and EFI_SPI_PART structures they reference, are the same as EFI_SPI_CONFIGURATION_PROTOCOL. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
Members: <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
- UINT32 BusCount <!-- [CODE_FIRST] 13226 -->
- CONST EFI_SPI_BUS *CONST *CONST Buslist <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
### EFI_PEI_SPI_IO_PPI <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
GUID: <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
  None. The producer installs one PPI instance per peripheral. The PPI GUID is SpiPeripheralDriverGuid from that peripheral's EFI_SPI_PART description. This is the same lookup rule as EFI_SPI_IO_PROTOCOL. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
Summary: <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
  Managed SPI transactions for one peripheral. Produced by the SPI bus PEIM. Consumed by peripheral drivers such as the JEDEC SFDP NOR flash PEIM. The structure is the same as EFI_SPI_IO_PROTOCOL. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
Members: <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
- CONST EFI_SPI_PERIPHERAL *SpiPeripheral <!-- [CODE_FIRST] 13226 -->
- CONST EFI_SPI_PERIPHERAL *OriginalSpiPeripheral <!-- [CODE_FIRST] 13226 -->
- UINT32 FrameSizeSupportMask <!-- [CODE_FIRST] 13226 -->
- UINT32 MaximumTransferBytes <!-- [CODE_FIRST] 13226 -->
- UINT32 Attributes <!-- [CODE_FIRST] 13226 -->
- CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *LegacySpiProtocol <!-- [CODE_FIRST] 13226 -->
- Transaction <!-- [CODE_FIRST] 13226 -->
- UpdateSpiPeripheral <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
Transaction and UpdateSpiPeripheral use the same parameters and return codes as the matching EFI_SPI_IO_PROTOCOL members. LegacySpiProtocol may be NULL when the controller does not support the legacy SPI controller interface. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
### EFI_PEI_SPI_NOR_FLASH_PPI <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
GUID: <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
  gEfiPeiSpiNorFlashPpiGuid = {0x0b9e6f3a, 0x4c72, 0x4f9d, {0x8e, 0x53, 0x1a, 0x6b, 0x27, 0xd4, 0x90, 0x3c}} <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
Summary: <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
  Byte-level read, write, erase, and status access to a SPI NOR part. Produced by a NOR flash PEIM, such as one that parses JEDEC SFDP. The structure and the return codes of each member are the same as EFI_SPI_NOR_FLASH_PROTOCOL. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
Members: <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
- CONST EFI_SPI_PERIPHERAL *SpiPeripheral <!-- [CODE_FIRST] 13226 -->
- UINT32 FlashSize <!-- [CODE_FIRST] 13226 -->
- UINT8 Deviceid[3] <!-- [CODE_FIRST] 13226 -->
- UINT32 EraseBlockBytes <!-- [CODE_FIRST] 13226 -->
- GetFlashid <!-- [CODE_FIRST] 13226 -->
- ReadData <!-- [CODE_FIRST] 13226 -->
- LfReadData <!-- [CODE_FIRST] 13226 -->
- ReadStatus <!-- [CODE_FIRST] 13226 -->
- WriteStatus <!-- [CODE_FIRST] 13226 -->
- WriteData <!-- [CODE_FIRST] 13226 -->
- Erase <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
Return codes follow the matching protocol members. In particular, ReadData, WriteData, and Erase return EFI_SUCCESS, EFI_INVALID_PARAMETER, or EFI_DEVICE_ERROR, and GetFlashid returns EFI_SUCCESS, EFI_INVALID_PARAMETER, or EFI_DEVICE_ERROR. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
### Dispatch rules <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
1. Platform code installs EFI_PEI_SPI_CONFIGURATION_PPI before the SPI bus PEIM runs. <!-- [CODE_FIRST] 13226 -->
2. The host controller PEIM installs EFI_PEI_SPI_HC_PPI. <!-- [CODE_FIRST] 13226 -->
3. The SPI bus PEIM depends on both of those PPIs. For each peripheral it installs EFI_PEI_SPI_IO_PPI under that peripheral's SpiPeripheralDriverGuid. <!-- [CODE_FIRST] 13226 -->
4. A NOR flash PEIM locates EFI_PEI_SPI_IO_PPI with its driver GUID and installs EFI_PEI_SPI_NOR_FLASH_PPI. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
These dispatch rules describe the reference EDK II PEIMs. A platform may produce the PPIs with its own PEIMs if the structures and GUIDs match this section. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
## Special Instructions <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
Header files belong in MdePkg/Include/Ppi because these are proposed PI interfaces. gEdk2JedecSfdpSpiPeiDriverGuid is an EDK II implementation GUID, not a PI GUID, and is not part of this specification change. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
The implementation typedefs each PPI to the matching protocol type so one copy of the host, bus, and NOR flash code serves PEI, DXE, and SMM. If the layouts ever diverge, the typedefs must be replaced with distinct structure definitions. <!-- [CODE_FIRST] 13226 -->
<!-- [CODE_FIRST] 13226 -->
