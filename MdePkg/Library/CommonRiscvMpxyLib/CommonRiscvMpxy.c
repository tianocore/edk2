/** @file
  This module implements functions to be used by MPXY client.
  It facilitates communication with the SBI MPXY extension for shared memory
  configuration, channel querying, and attribute reading.
  Copyright (c) 2024, Ventana Micro Systems, Inc.
  Copyright (c) 2025, Rivos Inc.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>
#include <Library/BaseRiscVSbiLib.h>
#include <Library/DxeRiscvMpxy.h>

#define INVAL_PHYS_ADDR  (-1U)
#define INVALID_CHAN     -1

#define MPXY_SHMEM_SIZE  4096

#if defined (__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define LLE_TO_CPU(x)  (SwapBytes64(x))
#define CPU_TO_LLE(x)  (SwapBytes64(x))
#else
#define LLE_TO_CPU(x)  (x)
#define CPU_TO_LLE(x)  (x)
#endif

STATIC VOID     *gShmemVirt         = NULL;
STATIC UINTN    gNrShmemPages       = 0;
STATIC UINT64   gShmemPhysHi        = INVAL_PHYS_ADDR;
STATIC UINT64   gShmemPhysLo        = INVAL_PHYS_ADDR;
STATIC UINT64   gShmemSize          = 0;
STATIC BOOLEAN  gMpxyLibInitialized = FALSE;
STATIC UINT64   gShmemSet           = 0;

STATIC
EFI_STATUS
EFIAPI
SbiMpxyGetShmemSize (
  OUT UINT64  *ShmemSize
  )
{
  SBI_RET  Ret;

  Ret = SbiCall (
          SBI_EXT_MPXY,
          SBI_EXT_MPXY_GET_SHMEM_SIZE,
          0
          );

  if (Ret.Error == SBI_SUCCESS) {
    *ShmemSize = Ret.Value;
    return EFI_SUCCESS;
  }

  return TranslateError (Ret.Error);
}

/**
  Configure the shared memory physical address using SBI MPXY extension.
  @param[in]  ShmemPhysHi   High Addr physical address.
  @param[in]  ShmemPhysLo   Low Addr physical address.
  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_DEVICE_ERROR      SBI call failed or invalid parameters.
**/
STATIC
EFI_STATUS
EFIAPI
SbiMpxySetShmem (
  IN UINT64   ShmemPhysHi,
  IN UINT64   ShmemPhysLo,
  OUT UINT64  *PrevShmemPhysHi,
  OUT UINT64  *PrevShmemPhysLo,
  BOOLEAN     ReadBackOldShmem
  )
{
  SBI_RET  Ret;
  UINT32   Flags = 0b00;
  UINT64   *PrevMemDet;

  if (ReadBackOldShmem) {
    Flags = 0b01;
  }

  Ret = SbiCall (
          SBI_EXT_MPXY,
          SBI_EXT_MPXY_SET_SHMEM,
          3,
          CPU_TO_LLE (ShmemPhysLo),
          CPU_TO_LLE (ShmemPhysHi),
          Flags
          );

  if (Ret.Error == SBI_SUCCESS) {
    if ((ShmemPhysLo == INVAL_PHYS_ADDR) && (ShmemPhysHi == INVAL_PHYS_ADDR)) {
      gShmemPhysHi = INVAL_PHYS_ADDR;
      gShmemPhysLo = INVAL_PHYS_ADDR;
      gShmemSet    = 0;
      return EFI_SUCCESS;
    }

    gShmemPhysLo = ShmemPhysLo;
    gShmemPhysHi = ShmemPhysHi;
    gShmemSet    = 1;

    PrevMemDet = (UINT64 *)gShmemPhysLo;

    if (ReadBackOldShmem) {
      *PrevShmemPhysLo = LLE_TO_CPU (PrevMemDet[0]);
      *PrevShmemPhysHi = LLE_TO_CPU (PrevMemDet[1]);
    }
  }

  return TranslateError (Ret.Error);
}

/**
  Disable shared memory by resetting the physical address.
  @retval EFI_SUCCESS           Successfully disabled.
  @retval EFI_DEVICE_ERROR      SBI call failed.
**/
STATIC
EFI_STATUS
EFIAPI
SbiMpxyDisableShmem (
  VOID
  )
{
  EFI_STATUS  Status;

  if (!gShmemSet) {
    return EFI_SUCCESS;
  }

  Status = SbiMpxySetShmem (
             INVAL_PHYS_ADDR,
             INVAL_PHYS_ADDR,
             NULL,
             NULL,
             FALSE
             );

  return Status;
}

/**
  Check whether MPXY shared memory is initialized.
  @retval TRUE     Initialized.
  @retval FALSE    Not initialized.
**/
BOOLEAN
SbiMpxyShmemInitialized (
  VOID
  )
{
  return gMpxyLibInitialized;
}

/**
  Retrieve the list of MPXY channel IDs.
  @param[in]   StartIndex     Index to start from.
  @param[out]  ChannelList    Pointer to array to hold channel IDs.
  @param[out]  Remaining      Channels yet to be read.
  @param[out]  Returned       Number of channels returned.
  @retval EFI_SUCCESS         Channel list retrieved.
  @retval EFI_DEVICE_ERROR    MPXY not initialized or SBI error.
**/
EFI_STATUS
EFIAPI
SbiMpxyGetChannelList (
  IN  UINTN  StartIndex,
  OUT UINTN  *ChannelList,
  OUT UINTN  *Remaining,
  OUT UINTN  *Returned
  )
{
  UINT64      OPhysHi, OPhysLo;
  EFI_STATUS  Status;
  SBI_RET     Ret;
  UINT32      *Shmem = gShmemVirt;
  UINTN       i;

  if (!gMpxyLibInitialized) {
    return (EFI_DEVICE_ERROR);
  }

  /* Set the shared memory to memory allocated for non-channel specific reads */
  Status = SbiMpxySetShmem (
             0,
             (UINT64)gShmemVirt,
             &OPhysHi,
             &OPhysLo,
             TRUE /* Read back the old address */
             );

  if (EFI_ERROR (Status)) {
    return (EFI_DEVICE_ERROR);
  }

  Ret = SbiCall (
          SBI_EXT_MPXY,
          SBI_EXT_MPXY_GET_CHANNEL_IDS,
          1,
          StartIndex
          );

  if (Ret.Error != SBI_SUCCESS) {
    return TranslateError (Ret.Error);
  }

  /* Index 0 contains number of channels pending to be read */
  if (Shmem[0] == 0) {
    *Remaining = 0;
  }

  /* Number of channels returned */
  if (Shmem[1] > 0) {
    for (i = 0; i < Shmem[1]; i++) {
      ChannelList[i] = Shmem[i+2];
    }
  }

  *Returned = Shmem[1];

  /* Switch back to old shared memory */
  Status = SbiMpxySetShmem (
             OPhysHi,
             OPhysLo,
             NULL,
             NULL,
             FALSE /* Read back the old address */
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Read multiple attributes from a specified channel.
  @param[in]   ChannelId    Channel ID to query.
  @param[in]   BaseAttrId   Starting attribute ID.
  @param[in]   NrAttrs      Number of attributes to read.
  @param[out]  Attrs        Buffer to receive attributes.
  @retval EFI_SUCCESS       Attributes successfully read.
  @retval EFI_DEVICE_ERROR  MPXY not initialized or SBI call failed.
**/
EFI_STATUS
EFIAPI
SbiMpxyReadChannelAttrs (
  IN UINTN    ChannelId,
  IN UINT32   BaseAttrId,
  IN UINT32   NrAttrs,
  OUT UINT32  *Attrs
  )
{
  UINT64      OPhysHi, OPhysLo;
  EFI_STATUS  Status;
  SBI_RET     Ret;

  if (!gMpxyLibInitialized) {
    return (EFI_DEVICE_ERROR);
  }

  /* Set the shared memory to memory allocated for non-channel specific reads */
  Status = SbiMpxySetShmem (
             0,
             (UINT64)gShmemVirt,
             &OPhysHi,
             &OPhysLo,
             TRUE /* Read back the old address */
             );

  if (EFI_ERROR (Status)) {
    return (EFI_DEVICE_ERROR);
  }

  Ret = SbiCall (
          SBI_EXT_MPXY,
          SBI_EXT_MPXY_READ_ATTRS,
          3,
          ChannelId,
          BaseAttrId, /* Base attribute Id */
          NrAttrs     /* Number of attributes */
          );

  if (Ret.Error != SBI_SUCCESS) {
    return TranslateError (Ret.Error);
  }

  CopyMem (
    Attrs,
    gShmemVirt,
    sizeof (UINT32) * NrAttrs
    );

  /* Switch back to old shared memory */
  Status = SbiMpxySetShmem (
             OPhysHi,
             OPhysLo,
             NULL,
             NULL,
             FALSE /* Read back the old address */
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Initialize the MPXY library and shared memory.
  @retval EFI_SUCCESS         Successfully initialized.
  @retval EFI_OUT_OF_RESOURCES Memory allocation failed.
  @retval EFI_DEVICE_ERROR     SBI calls failed.
**/
EFI_STATUS
EFIAPI
SbiMpxyInit (
  VOID
  )
{
  VOID        *SbiShmem;
  UINTN       NrEfiPages;
  EFI_STATUS  Status;
  UINT64      ShmemSize;

  if (SbiMpxyShmemInitialized ()) {
    return EFI_SUCCESS;
  }

  Status = SbiProbeExtension (SBI_EXT_MPXY);
  ASSERT_EFI_ERROR (Status);

  Status = SbiMpxyGetShmemSize (&ShmemSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: Failed to get the shared memory size\n",
      __func__
      ));
    return 0;
  }

  DEBUG ((
    DEBUG_WARN,
    "%a: Shared memory size to be allocated: %lu bytes\n",
    __func__,
    ShmemSize
    ));

  NrEfiPages = (ShmemSize / EFI_PAGE_SIZE) + 1;
  gShmemSize = ShmemSize;

  SbiShmem = AllocateAlignedPages (NrEfiPages, EFI_PAGE_SIZE);
  if (SbiShmem == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = SbiMpxySetShmem (
             0,
             (UINT64)SbiShmem,
             NULL,
             NULL,
             FALSE /* Not interested in old memory */
             );
  if (EFI_ERROR (Status)) {
    FreeAlignedPages (SbiShmem, NrEfiPages);
    return EFI_DEVICE_ERROR;
  }

  gShmemVirt          = SbiShmem;
  gNrShmemPages       = NrEfiPages;
  gMpxyLibInitialized = TRUE;

  return EFI_SUCCESS;
}

/**
  De-initialize MPXY library and release shared memory.
  @retval EFI_SUCCESS            Successfully deinitialized.
  @retval EFI_INVALID_PARAMETER  Library not initialized.
  @retval EFI_DEVICE_ERROR       Failed to disable memory.
**/
EFI_STATUS
EFIAPI
SbiMpxyDeinit (
  VOID
  )
{
  EFI_STATUS  Status;

  if (!SbiMpxyShmemInitialized ()) {
    return EFI_INVALID_PARAMETER;
  }

  /* Ref count is zero. Release the memory */
  Status = SbiMpxyDisableShmem ();
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  FreeAlignedPages (gShmemVirt, gNrShmemPages);

  return EFI_SUCCESS;
}

/**
  Send a message to an MPXY channel and receive response.
  @param[in]  ChannelId      ID of the channel.
  @param[in]  MessageId      Message type identifier.
  @param[in]  Message        Pointer to message payload.
  @param[in]  MessageDataLen Length of message payload.
  @param[out] Response       Pointer to receive response.
  @param[out] ResponseLen    Actual response length returned.
  @retval EFI_SUCCESS            Message sent and response received.
  @retval EFI_INVALID_PARAMETER  Invalid message length or uninitialized state.
  @retval EFI_DEVICE_ERROR       SBI call failed.
**/
EFI_STATUS
EFIAPI
SbiMpxySendMessage (
  IN UINTN   ChannelId,
  IN UINTN   MessageId,
  IN VOID    *Message,
  IN UINTN   MessageDataLen,
  OUT VOID   *Response,
  OUT UINTN  *ResponseLen
  )
{
  SBI_RET  Ret;
  UINT64   Phys = gShmemPhysLo;

  if (!SbiMpxyShmemInitialized ()) {
    return EFI_INVALID_PARAMETER;
  }

  if (MessageDataLen >= gShmemSize) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem ((VOID *)Phys, Message, MessageDataLen);

  Ret = SbiCall (
          SBI_EXT_MPXY,
          SBI_EXT_MPXY_SEND_MSG_WITH_RESP,
          3,
          ChannelId,
          MessageId,
          MessageDataLen
          );

  if ((Ret.Error == SBI_SUCCESS) && Ret.Value) {
    CopyMem (Response, (const VOID *)Phys, Ret.Value);
    if (ResponseLen) {
      *ResponseLen = Ret.Value;
    }
  }

  return TranslateError (Ret.Error);
}
