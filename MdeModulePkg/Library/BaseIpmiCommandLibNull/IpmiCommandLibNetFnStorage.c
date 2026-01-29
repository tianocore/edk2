/** @file
  IPMI Command - NetFnStorage NULL instance library.

  Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <IndustryStandard/Ipmi.h>

/**
  This function gets FRU inventory area info.

  @param[in]  GetFruInventoryAreaInfoRequest    Get FRU inventory area command request.
  @param[out] GetFruInventoryAreaInfoResponse   get FRU inventory area command response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetFruInventoryAreaInfo (
  IN  IPMI_GET_FRU_INVENTORY_AREA_INFO_REQUEST   *GetFruInventoryAreaInfoRequest,
  OUT IPMI_GET_FRU_INVENTORY_AREA_INFO_RESPONSE  *GetFruInventoryAreaInfoResponse
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function reads FRU data.

  @param[in]      ReadFruDataRequest       Read FRU data command request.
  @param[out]     ReadFruDataResponse      Read FRU data command response.
  @param[in,out]  ReadFruDataResponseSize  Size of the read FRU data response.
                                           When input, the expected size of response data.
                                           When out, the exact size of response data.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiReadFruData (
  IN  IPMI_READ_FRU_DATA_REQUEST   *ReadFruDataRequest,
  OUT IPMI_READ_FRU_DATA_RESPONSE  *ReadFruDataResponse,
  IN OUT UINT32                    *ReadFruDataResponseSize
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets chassis capability.

  @param[in]    WriteFruDataRequest      Write FRU data command request.
  @param[in]    WriteFruDataRequestSize  Size of the write FRU data command request.
  @param[out]   WriteFruDataResponse     Write FRU data response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiWriteFruData (
  IN  IPMI_WRITE_FRU_DATA_REQUEST   *WriteFruDataRequest,
  IN  UINT32                        WriteFruDataRequestSize,
  OUT IPMI_WRITE_FRU_DATA_RESPONSE  *WriteFruDataResponse
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets SEL information.

  @param[out]    GetSelInfoResponse    Get SEL information command response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetSelInfo (
  OUT IPMI_GET_SEL_INFO_RESPONSE  *GetSelInfoResponse
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets SEL entry.

  @param[in]      GetSelEntryRequest       Get SEL entry command request.
  @param[out]     GetSelEntryResponse      Get SEL entry command response.
  @param[in,out]  GetSelEntryResponseSize  Size of Get SEL entry request.
                                           When input, the expected size of response data.
                                           When out, the exact size of response data.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetSelEntry (
  IN IPMI_GET_SEL_ENTRY_REQUEST    *GetSelEntryRequest,
  OUT IPMI_GET_SEL_ENTRY_RESPONSE  *GetSelEntryResponse,
  IN OUT UINT32                    *GetSelEntryResponseSize
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function adds SEL entry.

  @param[in]    AddSelEntryRequest       Add SEL entry command request.
  @param[out]   AddSelEntryResponse      Add SEL entry command response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiAddSelEntry (
  IN IPMI_ADD_SEL_ENTRY_REQUEST    *AddSelEntryRequest,
  OUT IPMI_ADD_SEL_ENTRY_RESPONSE  *AddSelEntryResponse
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function partially adds SEL entry.

  @param[in]    PartialAddSelEntryRequest      Partial add SEL entry command request.
  @param[in]    PartialAddSelEntryRequestSize  Size of partial add SEL entry command request.
  @param[out]   PartialAddSelEntryResponse     Partial add SEL entry command response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiPartialAddSelEntry (
  IN IPMI_PARTIAL_ADD_SEL_ENTRY_REQUEST    *PartialAddSelEntryRequest,
  IN UINT32                                PartialAddSelEntryRequestSize,
  OUT IPMI_PARTIAL_ADD_SEL_ENTRY_RESPONSE  *PartialAddSelEntryResponse
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function clears SEL entry.

  @param[in]    ClearSelRequest      Clear SEL command request.
  @param[out]   ClearSelResponse     Clear SEL command response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiClearSel (
  IN IPMI_CLEAR_SEL_REQUEST    *ClearSelRequest,
  OUT IPMI_CLEAR_SEL_RESPONSE  *ClearSelResponse
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets SEL time.

  @param[out]   GetSelTimeResponse    Get SEL time command response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetSelTime (
  OUT IPMI_GET_SEL_TIME_RESPONSE  *GetSelTimeResponse
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function sets SEL time.

  @param[in]    SetSelTimeRequest    Set SEL time command request.
  @param[out]   CompletionCode       Command completion code.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiSetSelTime (
  IN IPMI_SET_SEL_TIME_REQUEST  *SetSelTimeRequest,
  OUT UINT8                     *CompletionCode
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets SDR repository information.

  @param[out]    GetSdrRepositoryInfoResp    Get SDR repository response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetSdrRepositoryInfo (
  OUT IPMI_GET_SDR_REPOSITORY_INFO_RESPONSE  *GetSdrRepositoryInfoResp
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets SDR

  @param[in]      GetSdrRequest        Get SDR resquest.
  @param[out]     GetSdrResponse       Get SDR response.
  @param[in,out]  GetSdrResponseSize   The size of get SDR response.
                                       When input, the expected size of response data.
                                       When out, the exact size of response data.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetSdr (
  IN  IPMI_GET_SDR_REQUEST   *GetSdrRequest,
  OUT IPMI_GET_SDR_RESPONSE  *GetSdrResponse,
  IN OUT UINT32              *GetSdrResponseSize
  )
{
  return RETURN_UNSUPPORTED;
}
