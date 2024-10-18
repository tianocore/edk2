/** @file
  EDKII_UFS_HC_PLATFORM_PPI definition.

Copyright (c) 2024, American Megatrends International LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EDKII_PEI_UFS_HC_PLATFORM_PPI_H_
#define _EDKII_PEI_UFS_HC_PLATFORM_PPI_H_

#define EDKII_UFS_HC_PLATFORM_PPI_VERSION  1

extern EFI_GUID  gEdkiiUfsHcPlatformPpiGuid;

///
/// Forward declaration for the UFS_HOST_CONTROLLER_PPI.
///
typedef struct _EDKII_UFS_HC_PLATFORM_PPI EDKII_UFS_HC_PLATFORM_PPI;

typedef struct {
  UINT32    Capabilities;
  UINT32    Version;
} EDKII_UFS_HC_INFO;

/**
  Allows platform PPI to override host controller information

  @param[in]      ControllerHandle  Handle of the UFS controller.
  @param[in, out] HcInfo            Pointer EDKII_UFS_HC_INFO associated with host controller.

  @retval EFI_SUCCESS            Function completed successfully.
  @retval EFI_INVALID_PARAMETER  HcInfo is NULL.
  @retval Others                 Function failed to complete.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_UFS_HC_PLATFORM_OVERRIDE_HC_INFO)(
  IN     EFI_HANDLE         ControllerHandle,
  IN OUT EDKII_UFS_HC_INFO  *HcInfo
  );

typedef enum {
  EdkiiUfsHcPreHce,
  EdkiiUfsHcPostHce,
  EdkiiUfsHcPreLinkStartup,
  EdkiiUfsHcPostLinkStartup
} EDKII_UFS_HC_PLATFORM_CALLBACK_PHASE;

typedef enum {
  EdkiiUfsCardRefClkFreq19p2Mhz,
  EdkiiUfsCardRefClkFreq26Mhz,
  EdkiiUfsCardRefClkFreq38p4Mhz,
  EdkiiUfsCardRefClkFreqObsolete
} EDKII_UFS_CARD_REF_CLK_FREQ_ATTRIBUTE;

/**
  Callback function for platform driver.

  @param[in] UfsHcBaseddr     The pointer to UfsHcBase address.
  @param[in] CallbackPhase    Specifies when the platform ppi is called

  @retval EFI_SUCCESS            Override function completed successfully.
  @retval EFI_INVALID_PARAMETER  CallbackPhase is invalid or CallbackData is NULL when phase expects valid data.
  @retval Others                 Function failed to complete.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_UFS_HC_PLATFORM_PEI_CALLBACK)(
  IN     UINTN                                 *UfsHcBaseAddr,
  IN     EDKII_UFS_HC_PLATFORM_CALLBACK_PHASE  CallbackPhase
  );

///
/// This PPI contains a set of services to interact with the UFS host controller.
///
struct _EDKII_UFS_HC_PLATFORM_PPI {
  ///
  /// Version of the PPI.
  ///
  UINT32                                    Version;
  ///
  /// Allows platform driver to override host controller information.
  ///
  EDKII_UFS_HC_PLATFORM_OVERRIDE_HC_INFO    OverrideHcInfo;
  ///
  /// Allows platform driver to implement platform specific flows
  /// for host controller.
  ///
  EDKII_UFS_HC_PLATFORM_PEI_CALLBACK        Callback;
  ///
  /// Reference Clock Frequency Ufs Card Attribute that need to be set in this Ufs Host Environment.
  ///
  EDKII_UFS_CARD_REF_CLK_FREQ_ATTRIBUTE     RefClkFreq;
};

#endif
