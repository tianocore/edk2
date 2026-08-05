/** @file
  This module implements functions used by MPXY clients.

  @par Glossary:
    - MPXY - Message Proxy extension in the RISC-V SBI specification

  Copyright (c) 2026, Qualcomm Technologies, Inc.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Base.h>
#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseRiscVSbiLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeRiscvMpxy.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#define MPXY_INVALID_SHMEM_ADDR  MAX_UINTN
#define MAX_MPXY_OPEN_CHANNELS   32

STATIC VOID  *mNonChanTempShmem     = NULL;
STATIC VOID  *mNonChanTempShmemPhys = NULL;

typedef struct {
  BOOLEAN    InUse;
  UINTN      ChannelId;
  VOID       *ShmemVirt;
  VOID       *ShmemPhys;
  UINTN      NrEfiPages;
  UINT32     MsgDataMaxLen;
  UINTN      RefCount;
} MPXY_CHANNEL_CONTEXT;

STATIC MPXY_CHANNEL_CONTEXT  mMpxyChannelCtx[MAX_MPXY_OPEN_CHANNELS];

///
/// Set Virtual Address Map Event
///
STATIC EFI_EVENT  mDxeRiscVMpxyLibVirtualNotifyEvent = NULL;

STATIC BOOLEAN  mMpxyLibraryInitialized = FALSE;

/**
  Convert the physical channel shared memory address.

  @param[in]    Event   The event that is being processed.
  @param[in]    Context The Event Context.
**/
VOID
EFIAPI
DxeRiscVMpxyLibVirtualNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN  Index;

  //
  // If there have been no runtime registrations, then just return
  //
  if (mNonChanTempShmem == NULL) {
    return;
  }

  for (Index = 0; Index < MAX_MPXY_OPEN_CHANNELS; Index++) {
    if (mMpxyChannelCtx[Index].InUse && (mMpxyChannelCtx[Index].ShmemVirt != NULL)) {
      gRT->ConvertPointer (0, (VOID **)&mMpxyChannelCtx[Index].ShmemVirt);
    }
  }

  gRT->ConvertPointer (0, (VOID **)&mNonChanTempShmem);
}

/**
  Find the channel context structure from a given ChannelID.

  @param[in]    ChannelId   Id of the channel being searched

  @retval On success, pointer to a MPXY channel context, NULL otherwise
**/
STATIC
MPXY_CHANNEL_CONTEXT *
FindChannelContext (
  IN UINTN  ChannelId
  )
{
  UINTN  Index;

  for (Index = 0; Index < MAX_MPXY_OPEN_CHANNELS; Index++) {
    if (mMpxyChannelCtx[Index].InUse && (mMpxyChannelCtx[Index].ChannelId == ChannelId)) {
      return &mMpxyChannelCtx[Index];
    }
  }

  return NULL;
}

/**
   Allocate a free channel context.

   @retval Pointer to free channel context
**/
STATIC
MPXY_CHANNEL_CONTEXT *
AllocChannelContext (
  VOID
  )
{
  UINTN  Index;

  for (Index = 0; Index < MAX_MPXY_OPEN_CHANNELS; Index++) {
    if (!mMpxyChannelCtx[Index].InUse) {
      return &mMpxyChannelCtx[Index];
    }
  }

  return NULL;
}

/**
   Get the size of shared memory for MPXY set with SBI.

   @param[out]    ShmemSize    Size of the shared memory
   @retval EFI_SUCCESS on success
   @retval Error as returned by SBI translated to corresponding EFI error
**/
STATIC
EFI_STATUS
EFIAPI
SbiMpxyGetShmemSize (
  OUT UINT64  *ShmemSize
  )
{
  SBI_RET  Ret;

  if (ShmemSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Ret = SbiCall (
          SBI_EXT_MPXY,
          SBI_EXT_MPXY_GET_SHMEM_SIZE,
          0
          );

  if (Ret.Error == SBI_SUCCESS) {
    *ShmemSize = Ret.Value;
    return EFI_SUCCESS;
  }

  *ShmemSize = 0;
  return TranslateError (Ret.Error);
}

/**
   Set shared memory for communication over MPXY channel. Optionally
   previous address can be read.

   @param[in]  ShmemPhys        Physical address of shared memory
   @param[in]  ShmemVirt        Virtual address of shared memory
   @param[out] PrevShmemPhys    Physical address previously set
   @param[in]  ReadBackOldShmem Flag to read the previous physical address

   @retval EFI_SUCCESS On success
   @retval Error returned by SBI translated to corresponding EFI error
 **/
STATIC
EFI_STATUS
EFIAPI
SbiMpxySetShmem (
  IN UINTN   ShmemPhys,
  IN UINTN   ShmemVirt,
  OUT UINTN  *PrevShmemPhys,
  BOOLEAN    ReadBackOldShmem
  )
{
  SBI_RET  Ret;
  UINTN    Flags;
  UINTN    ShmemPhysLo;
  UINTN    ShmemPhysHi;
  UINTN    *PrevMemDet;

  if (ReadBackOldShmem && ((PrevShmemPhys == NULL) || (ShmemVirt == 0))) {
    return EFI_INVALID_PARAMETER;
  }

  Flags = ReadBackOldShmem ?
          SBI_EXT_MPXY_SHMEM_FLAG_OVERWRITE_RETURN :
          SBI_EXT_MPXY_SHMEM_FLAG_OVERWRITE;

  if (ShmemPhys == MPXY_INVALID_SHMEM_ADDR) {
    ShmemPhysLo = MAX_UINTN;
    ShmemPhysHi = MAX_UINTN;
  } else {
    ShmemPhysLo = ShmemPhys;
    /* Upper XLEN bits are unused for native-width addresses. */
    ShmemPhysHi = 0;
  }

  Ret = SbiCall (
          SBI_EXT_MPXY,
          SBI_EXT_MPXY_SET_SHMEM,
          3,
          ShmemPhysLo,
          ShmemPhysHi,
          Flags
          );

  if (Ret.Error != SBI_SUCCESS) {
    return TranslateError (Ret.Error);
  }

  if (ReadBackOldShmem) {
    PrevMemDet = (UINTN *)ShmemVirt;
    if ((PrevMemDet[0] == MAX_UINTN) && (PrevMemDet[1] == MAX_UINTN)) {
      *PrevShmemPhys = MPXY_INVALID_SHMEM_ADDR;
    } else {
      *PrevShmemPhys = PrevMemDet[0];
    }
  }

  return EFI_SUCCESS;
}

/**
   Get list of MPXY channels available in SBI.

   @param[in]  StartIndex    First channel ID to start list from
   @param[out] ChannelList   Array of the MPXY channels IDs returned by SBI
   @param[out] Remaining     Pointer containing the number of channel Ids to be read
   @param[out] Returned      Pointer to the number of channel Ids returned in this call

   @retval EFI_SUCCESS on success
   @retval EFI_NOT_READY if library is not initialised
   @retval EFI error translated from error returned by SBI
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
  UINTN       OldShmemPhys;
  EFI_STATUS  Status;
  EFI_STATUS  RestoreStatus;
  SBI_RET     Ret;
  UINT32      *Shmem;
  UINTN       Index;

  if ((ChannelList == NULL) || (Remaining == NULL) || (Returned == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mNonChanTempShmem == NULL) {
    return EFI_NOT_READY;
  }

  Shmem = mNonChanTempShmem;

  /* Set the shared memory to memory allocated for non-channel specific reads */
  Status = SbiMpxySetShmem (
             (UINTN)mNonChanTempShmemPhys,
             (UINTN)mNonChanTempShmem,
             &OldShmemPhys,
             TRUE /* Read back the old address */
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Ret = SbiCall (
          SBI_EXT_MPXY,
          SBI_EXT_MPXY_GET_CHANNEL_IDS,
          1,
          StartIndex
          );

  if (Ret.Error != SBI_SUCCESS) {
    Status = TranslateError (Ret.Error);
    goto RestoreShmem;
  }

  /* Index 0 contains number of channels pending to be read */
  *Remaining = Shmem[0];

  /* Number of channels returned */
  if (Shmem[1] > 0) {
    for (Index = 0; Index < Shmem[1]; Index++) {
      ChannelList[Index] = Shmem[Index + 2];
    }
  }

  *Returned = Shmem[1];

  /* Switch back to old shared memory */
  Status = EFI_SUCCESS;

RestoreShmem:
  RestoreStatus = SbiMpxySetShmem (
                    OldShmemPhys,
                    MPXY_INVALID_SHMEM_ADDR,
                    NULL,
                    FALSE /* Read back the old address */
                    );
  if (EFI_ERROR (RestoreStatus)) {
    return RestoreStatus;
  }

  return Status;
}

/**
   Read MPXY channel attributes from the SBI.

   @param[in]  ChannelId    ID of the channel to read attributes
   @param[in]  BaseAttrId   Base Attribute ID from where to read attributes
   @param[in]  NrAttrs      Number of attributes to read
   @param[out] Attrs        Array of the attributes read

   @retval EFI_SUCCESS on success
   @retval EFI_NOT_READY if MPXY library is not initialized
   @retval EFI error translated from SBI error code
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
  UINTN       OldShmemPhys;
  EFI_STATUS  Status;
  EFI_STATUS  RestoreStatus;
  SBI_RET     Ret;

  if ((Attrs == NULL) || (NrAttrs == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mNonChanTempShmem == NULL) {
    return EFI_NOT_READY;
  }

  /* Set the shared memory to memory allocated for non-channel specific reads */
  Status = SbiMpxySetShmem (
             (UINTN)mNonChanTempShmemPhys,
             (UINTN)mNonChanTempShmem,
             &OldShmemPhys,
             TRUE /* Read back the old address */
             );

  if (EFI_ERROR (Status)) {
    return Status;
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
    Status = TranslateError (Ret.Error);
    goto RestoreShmem;
  }

  CopyMem (
    Attrs,
    mNonChanTempShmem,
    sizeof (UINT32) * NrAttrs
    );

  Status = EFI_SUCCESS;

RestoreShmem:
  RestoreStatus = SbiMpxySetShmem (
                    OldShmemPhys,
                    MPXY_INVALID_SHMEM_ADDR,
                    NULL,
                    FALSE /* Read back the old address */
                    );
  if (EFI_ERROR (RestoreStatus)) {
    return RestoreStatus;
  }

  return Status;
}

/**
   Open an MPXY channel identified by the channel Id.

   @param[in] ChannelId    Id of the channel to be opened

   @retval EFI_SUCCESS On success
   @retval EFI_NOT_READY if library is not initialized
   @retval EFI_INVALID_PARAMETER if Channel information or Channel Is is wrong
   @retval EFI_OUT_OF_RESOURCES if new channel cannot be allocated or allocation
           of the shared memory fails
**/
EFI_STATUS
EFIAPI
SbiMpxyChannelOpen (
  IN UINTN  ChannelId
  )
{
  MPXY_CHANNEL_CONTEXT  *ChannelCtx;
  UINT32                Attributes[MpxyChanAttrMax]; // space to read id and version
  UINT32                ChanDataLen;
  VOID                  *SbiShmem;
  UINTN                 NrEfiPages;
  EFI_STATUS            Status;

  if (mNonChanTempShmem == NULL) {
    return EFI_NOT_READY;
  }

  ChannelCtx = FindChannelContext (ChannelId);
  if (ChannelCtx != NULL) {
    ChannelCtx->RefCount++;
    return EFI_SUCCESS;
  }

  Status = SbiMpxyReadChannelAttrs (
             ChannelId,
             0,
             MpxyChanAttrMax,
             &Attributes[0]
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ChanDataLen = Attributes[MpxyChanAttrMsgDataMaxLen];
  if (ChanDataLen == 0) {
    return EFI_INVALID_PARAMETER;
  }

  NrEfiPages = EFI_SIZE_TO_PAGES (ChanDataLen);

  ChannelCtx = AllocChannelContext ();
  if (ChannelCtx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SbiShmem = AllocateAlignedRuntimePages (NrEfiPages, EFI_PAGE_SIZE);
  if (SbiShmem == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (ChannelCtx, sizeof (*ChannelCtx));
  ChannelCtx->InUse         = TRUE;
  ChannelCtx->ChannelId     = ChannelId;
  ChannelCtx->ShmemVirt     = SbiShmem;
  ChannelCtx->ShmemPhys     = SbiShmem;
  ChannelCtx->NrEfiPages    = NrEfiPages;
  ChannelCtx->MsgDataMaxLen = ChanDataLen;
  ChannelCtx->RefCount      = 1;

  return EFI_SUCCESS;
}

/**
   Close a given MPXY channel.

   @param[in] ChannelId   Id of the channel to be closed

   @retval EFI_SUCCESS on success
   @retval EFI_NOT_FOUND is context related to ChannelId could not be found
**/
EFI_STATUS
EFIAPI
SbiMpxyChannelClose (
  IN UINTN  ChannelId
  )
{
  MPXY_CHANNEL_CONTEXT  *ChannelCtx;

  ChannelCtx = FindChannelContext (ChannelId);
  if (ChannelCtx == NULL) {
    return EFI_NOT_FOUND;
  }

  if (ChannelCtx->RefCount > 1) {
    ChannelCtx->RefCount--;
    return EFI_SUCCESS;
  }

  if ((ChannelCtx->ShmemVirt != NULL) && (ChannelCtx->NrEfiPages != 0)) {
    FreeAlignedPages (ChannelCtx->ShmemVirt, ChannelCtx->NrEfiPages);
  }

  ZeroMem (ChannelCtx, sizeof (*ChannelCtx));
  return EFI_SUCCESS;
}

/**
   Send a message over an MPXY channel.

   @param[in] ChannelId     Id of the channel on which message is to be sent
   @param[in] MessageId     Id of the message being sent
   @param[in] Message       Pointer to the buffer containing the message
   @param[in] MessageDataLen Size of the message being sent
   @param[out] Response     Pointer to the buffer on which response will be received
   @param[out] ResponseLen  Size of the response

   @retval EFI_SUCCESS on success
   @retval EFI_NOT_READY if channel context is not found or is invalid
   @retval EFI_INVALID_PARAMETER if length of the message data is more than channel can carry
   @retval EFI error translated from error returned by SBI
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
  MPXY_CHANNEL_CONTEXT  *ChannelCtx;
  SBI_RET               Ret;
  EFI_STATUS            Status;
  EFI_STATUS            RestoreStatus;
  UINTN                 PrevShmemPhys;
  UINTN                 Virt;

  if (mNonChanTempShmem == NULL) {
    return EFI_NOT_READY;
  }

  ChannelCtx = FindChannelContext (ChannelId);
  if ((ChannelCtx == NULL) || !ChannelCtx->InUse || (ChannelCtx->ShmemVirt == NULL)) {
    return EFI_NOT_READY;
  }

  if (MessageDataLen > ChannelCtx->MsgDataMaxLen) {
    return EFI_INVALID_PARAMETER;
  }

  Status = SbiMpxySetShmem (
             (UINTN)ChannelCtx->ShmemPhys,
             (UINTN)ChannelCtx->ShmemVirt,
             &PrevShmemPhys,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Virt = (UINTN)ChannelCtx->ShmemVirt;

  /* Copy message to Hart's shared memory */
  CopyMem (
    (VOID *)Virt,
    Message,
    MessageDataLen
    );

  Ret = SbiCall (
          SBI_EXT_MPXY,
          SBI_EXT_MPXY_SEND_MSG_WITH_RESP,
          3,
          ChannelId,
          MessageId,
          MessageDataLen
          );

  if ((Ret.Error == SBI_SUCCESS) && (Response != NULL)) {
    /* Copy the response to out buffer */
    CopyMem (
      Response,
      (const VOID *)Virt,
      Ret.Value
      );
    if (ResponseLen != NULL) {
      *ResponseLen = Ret.Value;
    }
  }

  RestoreStatus = SbiMpxySetShmem (
                    PrevShmemPhys,
                    MPXY_INVALID_SHMEM_ADDR,
                    NULL,
                    FALSE
                    );

  if (EFI_ERROR (RestoreStatus)) {
    return RestoreStatus;
  }

  return TranslateError (Ret.Error);
}

/**
  Initialize the MPXY library

  @retval  EFI_SUCCESS            Library is successfully initialized.
  @retval  EFI_ALREADY_STARTED    Library is already initialized.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to allocate.
**/
RETURN_STATUS
EFIAPI
SbiMpxyLibInit (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT64      ShmemSize;

  if (mMpxyLibraryInitialized) {
    return EFI_ALREADY_STARTED;
  }

  Status = SbiProbeExtension (SBI_EXT_MPXY);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SbiMpxyGetShmemSize (&ShmemSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Allocate memory to be shared with OpenSBI for initial MPXY communications
  // until channels are initialized by their respective drivers.
  //
  mNonChanTempShmem = AllocateAlignedRuntimePages (
                        EFI_SIZE_TO_PAGES (ShmemSize),
                        ShmemSize // Align
                        );

  if (mNonChanTempShmem == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mNonChanTempShmemPhys = mNonChanTempShmem;

  //
  // Register SetVirtualAddressMap () notify function
  //
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                  TPL_NOTIFY,
                  DxeRiscVMpxyLibVirtualNotify,
                  NULL,
                  &mDxeRiscVMpxyLibVirtualNotifyEvent
                  );
  if (EFI_ERROR (Status)) {
    FreeAlignedPages (mNonChanTempShmem, EFI_SIZE_TO_PAGES (ShmemSize));
    mNonChanTempShmem     = NULL;
    mNonChanTempShmemPhys = NULL;
    return Status;
  }

  mMpxyLibraryInitialized = TRUE;

  return Status;
}
