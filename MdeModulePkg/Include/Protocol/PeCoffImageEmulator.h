/** @file
  Copyright (c) 2019, Linaro, Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PECOFF_IMAGE_EMULATOR_PROTOCOL_GUID_H_
#define _PECOFF_IMAGE_EMULATOR_PROTOCOL_GUID_H_

#define EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL_GUID \
  { 0x96F46153, 0x97A7, 0x4793, { 0xAC, 0xC1, 0xFA, 0x19, 0xBF, 0x78, 0xEA, 0x97 } }

typedef struct _EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL;

/**
  Check whether the emulator supports executing a certain PE/COFF image

  @param[in] This         This pointer for EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL
                          structure
  @param[in] ImageType    Whether the image is an application, a boot time
                          driver or a runtime driver.
  @param[in] DevicePath   Path to device where the image originated
                          (e.g., a PCI option ROM)

  @retval TRUE            The image is supported by the emulator
  @retval FALSE           The image is not supported by the emulator.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_PECOFF_IMAGE_EMULATOR_IS_IMAGE_SUPPORTED)(
  IN  EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL    *This,
  IN  UINT16                                  ImageType,
  IN  EFI_DEVICE_PATH_PROTOCOL                *DevicePath   OPTIONAL
  );

/**
  Register a supported PE/COFF image with the emulator. After this call
  completes successfully, the PE/COFF image may be started as usual, and
  it is the responsibility of the emulator implementation that any branch
  into the code section of the image (including returns from functions called
  from the foreign code) is executed as if it were running on the machine
  type it was built for.

  @param[in]      This          This pointer for
                                EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL structure
  @param[in]      ImageBase     The base address in memory of the PE/COFF image
  @param[in]      ImageSize     The size in memory of the PE/COFF image
  @param[in,out]  EntryPoint    The entry point of the PE/COFF image. Passed by
                                reference so that the emulator may modify it.

  @retval EFI_SUCCESS           The image was registered with the emulator and
                                can be started as usual.
  @retval other                 The image could not be registered.

  If the PE/COFF machine type or image type are not supported by the emulator,
  then ASSERT().
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PECOFF_IMAGE_EMULATOR_REGISTER_IMAGE)(
  IN      EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL    *This,
  IN      EFI_PHYSICAL_ADDRESS                    ImageBase,
  IN      UINT64                                  ImageSize,
  IN  OUT EFI_IMAGE_ENTRY_POINT                   *EntryPoint
  );

/**
  Unregister a PE/COFF image that has been registered with the emulator.
  This should be done before the image is unloaded from memory.

  @param[in] This         This pointer for EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL
                          structure
  @param[in] ImageBase    The base address in memory of the PE/COFF image

  @retval EFI_SUCCESS     The image was unregistered with the emulator.
  @retval other           Image could not be unloaded.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PECOFF_IMAGE_EMULATOR_UNREGISTER_IMAGE)(
  IN  EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL    *This,
  IN  EFI_PHYSICAL_ADDRESS                    ImageBase
  );

#define EDKII_PECOFF_IMAGE_EMULATOR_VERSION  0x1

typedef struct _EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL {
  EDKII_PECOFF_IMAGE_EMULATOR_IS_IMAGE_SUPPORTED    IsImageSupported;
  EDKII_PECOFF_IMAGE_EMULATOR_REGISTER_IMAGE        RegisterImage;
  EDKII_PECOFF_IMAGE_EMULATOR_UNREGISTER_IMAGE      UnregisterImage;

  // Protocol version implemented by the emulator
  UINT32                                            Version;
  // The machine type implemented by the emulator
  UINT16                                            MachineType;
} EDKII_PECOFF_IMAGE_EMULATOR_PROTOCOL;

extern EFI_GUID  gEdkiiPeCoffImageEmulatorProtocolGuid;

#endif
