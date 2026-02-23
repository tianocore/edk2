/** @file MockPeiFirmwareVolumeShadowPpi.h
  Declare mock Pei firmware volume shadow ppi

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_EDKII_PEI_FIRMWARE_VOLUME_SHADOW_PPI_H_
#define MOCK_EDKII_PEI_FIRMWARE_VOLUME_SHADOW_PPI_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <PiPei.h>
  #include <Ppi/FirmwareVolumeShadowPpi.h>
}

struct MockPeiFirmwareVolumeShadowPpi {
  MOCK_INTERFACE_DECLARATION (MockPeiFirmwareVolumeShadowPpi);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    FirmwareVolumeShadow,
    (
     IN EFI_PHYSICAL_ADDRESS  FirmwareVolumeBase,
     IN VOID                  *Destination,
     IN UINTN                 DestinationLength
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockPeiFirmwareVolumeShadowPpi);
MOCK_FUNCTION_DEFINITION (MockPeiFirmwareVolumeShadowPpi, FirmwareVolumeShadow, 3, EFIAPI);

#define MOCK_EDKII_PEI_FIRMWARE_VOLUME_SHADOW_PPI_INSTANCE(NAME)  \
  EDKII_PEI_FIRMWARE_VOLUME_SHADOW_PPI NAME##_INSTANCE = {   \
    FirmwareVolumeShadow                                         \
  };                                                    \
  EDKII_PEI_FIRMWARE_VOLUME_SHADOW_PPI  *NAME = &NAME##_INSTANCE;

#endif
