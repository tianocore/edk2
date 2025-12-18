/** @file
  IPMI Command - NetFnStorage.

  Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiLib.h>

#include <IndustryStandard/Ipmi.h>

/**
  This function is used to retrieve FRU Inventory Area

  @param [in]  GetFruInventoryAreaInfoRequest   Pointer to IPMI_GET_FRU_INVENTORY_AREA_INFO_REQUEST.
  @param [out] GetFruInventoryAreaInfoResponse  Pointer to IPMI_GET_FRU_INVENTORY_AREA_INFO_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetFruInventoryAreaInfo (
  IN  IPMI_GET_FRU_INVENTORY_AREA_INFO_REQUEST   *GetFruInventoryAreaInfoRequest,
  OUT IPMI_GET_FRU_INVENTORY_AREA_INFO_RESPONSE  *GetFruInventoryAreaInfoResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*GetFruInventoryAreaInfoResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_STORAGE,
               IPMI_STORAGE_GET_FRU_INVENTORY_AREAINFO,
               (VOID *)GetFruInventoryAreaInfoRequest,
               sizeof (*GetFruInventoryAreaInfoRequest),
               (VOID *)GetFruInventoryAreaInfoResponse,
               &DataSize
               );
  return Status;
}

/**
  This function returns specified data from the FRU Inventory Info area.

  @param [in]  ReadFruDataRequest       Pointer to IPMI_READ_FRU_DATA_REQUEST.
  @param [in]  ReadFruDataResponse      Pointer to IPMI_READ_FRU_DATA_RESPONSE.
  @param [in, out] ReadFruDataResponseSize  Returns the size of ReadFruDataResponse.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiReadFruData (
  IN  IPMI_READ_FRU_DATA_REQUEST   *ReadFruDataRequest,
  OUT IPMI_READ_FRU_DATA_RESPONSE  *ReadFruDataResponse,
  IN OUT UINT32                    *ReadFruDataResponseSize
  )
{
  EFI_STATUS  Status;

  Status = IpmiSubmitCommand (
             IPMI_NETFN_STORAGE,
             IPMI_STORAGE_READ_FRU_DATA,
             (VOID *)ReadFruDataRequest,
             sizeof (*ReadFruDataRequest),
             (VOID *)ReadFruDataResponse,
             ReadFruDataResponseSize
             );
  return Status;
}

/**
  This function writes specified data from the FRU Inventory Info area.

  @param [in]  WriteFruDataRequest      Pointer to IPMI_WRITE_FRU_DATA_REQUEST.
  @param [in]  WriteFruDataRequestSize  Size of WriteFruDataRequest.
  @param [out] WriteFruDataResponse     Pointer to receive IPMI_WRITE_FRU_DATA_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiWriteFruData (
  IN  IPMI_WRITE_FRU_DATA_REQUEST   *WriteFruDataRequest,
  IN  UINT32                        WriteFruDataRequestSize,
  OUT IPMI_WRITE_FRU_DATA_RESPONSE  *WriteFruDataResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*WriteFruDataResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_STORAGE,
               IPMI_STORAGE_WRITE_FRU_DATA,
               (VOID *)WriteFruDataRequest,
               WriteFruDataRequestSize,
               (VOID *)WriteFruDataResponse,
               &DataSize
               );
  return Status;
}

/**
  This function returns the number of entries in the SEL

  @param [out] GetSelInfoResponse     Pointer to receive IPMI_GET_SEL_INFO_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetSelInfo (
  OUT IPMI_GET_SEL_INFO_RESPONSE  *GetSelInfoResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*GetSelInfoResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_STORAGE,
               IPMI_STORAGE_GET_SEL_INFO,
               NULL,
               0,
               (VOID *)GetSelInfoResponse,
               &DataSize
               );
  return Status;
}

/**
  This function retrieves entries from the SEL

  @param [in]  GetSelEntryRequest       Pointer to IPMI_GET_SEL_ENTRY_REQUEST.
  @param [out] GetSelEntryResponse      Pointer to receive IPMI_GET_SEL_ENTRY_RESPONSE.
  @param [in, out]  GetSelEntryResponseSize  Size of entire GetSelEntryResponse.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetSelEntry (
  IN IPMI_GET_SEL_ENTRY_REQUEST    *GetSelEntryRequest,
  OUT IPMI_GET_SEL_ENTRY_RESPONSE  *GetSelEntryResponse,
  IN OUT UINT32                    *GetSelEntryResponseSize
  )
{
  EFI_STATUS  Status;

  Status = IpmiSubmitCommand (
             IPMI_NETFN_STORAGE,
             IPMI_STORAGE_GET_SEL_ENTRY,
             (VOID *)GetSelEntryRequest,
             sizeof (*GetSelEntryRequest),
             (VOID *)GetSelEntryResponse,
             GetSelEntryResponseSize
             );
  return Status;
}

/**
  This function adds an entry in the SEL

  @param [in]  AddSelEntryRequest       Pointer to IPMI_ADD_SEL_ENTRY_REQUEST.
  @param [out] AddSelEntryResponse      Pointer to receive IPMI_ADD_SEL_ENTRY_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiAddSelEntry (
  IN IPMI_ADD_SEL_ENTRY_REQUEST    *AddSelEntryRequest,
  OUT IPMI_ADD_SEL_ENTRY_RESPONSE  *AddSelEntryResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*AddSelEntryResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_STORAGE,
               IPMI_STORAGE_ADD_SEL_ENTRY,
               (VOID *)AddSelEntryRequest,
               sizeof (*AddSelEntryRequest),
               (VOID *)AddSelEntryResponse,
               &DataSize
               );
  return Status;
}

/**
  This function adds SEL Entry command that allows the record to be incrementally
  added to the SEL.

  @param [in]  PartialAddSelEntryRequest       Pointer to IPMI_PARTIAL_ADD_SEL_ENTRY_REQUEST.
  @param [in]  PartialAddSelEntryRequestSize   Size of entire PartialAddSelEntryRequest.
  @param [out] PartialAddSelEntryResponse      Pointer to receive IPMI_PARTIAL_ADD_SEL_ENTRY_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiPartialAddSelEntry (
  IN IPMI_PARTIAL_ADD_SEL_ENTRY_REQUEST    *PartialAddSelEntryRequest,
  IN UINT32                                PartialAddSelEntryRequestSize,
  OUT IPMI_PARTIAL_ADD_SEL_ENTRY_RESPONSE  *PartialAddSelEntryResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*PartialAddSelEntryResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_STORAGE,
               IPMI_STORAGE_PARTIAL_ADD_SEL_ENTRY,
               (VOID *)PartialAddSelEntryRequest,
               PartialAddSelEntryRequestSize,
               (VOID *)PartialAddSelEntryResponse,
               &DataSize
               );
  return Status;
}

/**
  This function erases all contents of the System Event Log.

  @param [in]  ClearSelRequest       Pointer to IPMI_CLEAR_SEL_REQUEST.
  @param [out] ClearSelResponse      Pointer to receive IPMI_CLEAR_SEL_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiClearSel (
  IN IPMI_CLEAR_SEL_REQUEST    *ClearSelRequest,
  OUT IPMI_CLEAR_SEL_RESPONSE  *ClearSelResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*ClearSelResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_STORAGE,
               IPMI_STORAGE_CLEAR_SEL,
               (VOID *)ClearSelRequest,
               sizeof (*ClearSelRequest),
               (VOID *)ClearSelResponse,
               &DataSize
               );
  return Status;
}

/**
  This function returns the time from the SEL Device.

  @param [out]  GetSelTimeResponse       Pointer to IPMI_GET_SEL_TIME_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetSelTime (
  OUT IPMI_GET_SEL_TIME_RESPONSE  *GetSelTimeResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*GetSelTimeResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_STORAGE,
               IPMI_STORAGE_GET_SEL_TIME,
               NULL,
               0,
               (VOID *)GetSelTimeResponse,
               &DataSize
               );
  return Status;
}

/**
  This function set the time in the SEL Device.

  @param [in]  SetSelTimeRequest       Pointer to IPMI_SET_SEL_TIME_REQUEST.
  @param [out] CompletionCode         IPMI completetion code, refer to Ipmi.h.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiSetSelTime (
  IN IPMI_SET_SEL_TIME_REQUEST  *SetSelTimeRequest,
  OUT UINT8                     *CompletionCode
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*CompletionCode);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_STORAGE,
               IPMI_STORAGE_SET_SEL_TIME,
               (VOID *)SetSelTimeRequest,
               sizeof (*SetSelTimeRequest),
               (VOID *)CompletionCode,
               &DataSize
               );
  return Status;
}

/**
  This function returns the SDR command version for the SDR Repository

  @param [out] ClearSelResponse      Pointer to receive IPMI_GET_SDR_REPOSITORY_INFO_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetSdrRepositoryInfo (
  OUT IPMI_GET_SDR_REPOSITORY_INFO_RESPONSE  *GetSdrRepositoryInfoResp
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*GetSdrRepositoryInfoResp);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_STORAGE,
               IPMI_STORAGE_GET_SDR_REPOSITORY_INFO,
               NULL,
               0,
               (VOID *)GetSdrRepositoryInfoResp,
               &DataSize
               );
  return Status;
}

/**
  This function returns the sensor record specified by Record ID.

  @param [in]  GetSdrRequest       Pointer to IPMI_GET_SDR_REQUEST.
  @param [out] GetSdrResponse      Pointer to receive IPMI_GET_SDR_RESPONSE.
  @param [in, out]  GetSdrResponseSize  Size of entire GetSdrResponse.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetSdr (
  IN  IPMI_GET_SDR_REQUEST   *GetSdrRequest,
  OUT IPMI_GET_SDR_RESPONSE  *GetSdrResponse,
  IN OUT UINT32              *GetSdrResponseSize
  )
{
  EFI_STATUS  Status;

  Status = IpmiSubmitCommand (
             IPMI_NETFN_STORAGE,
             IPMI_STORAGE_GET_SDR,
             (VOID *)GetSdrRequest,
             sizeof (*GetSdrRequest),
             (VOID *)GetSdrResponse,
             GetSdrResponseSize
             );
  return Status;
}
