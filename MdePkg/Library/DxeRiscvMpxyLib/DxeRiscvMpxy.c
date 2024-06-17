/** @file
  This module implements functions to be used by MPXY client

  Copyright (c) 2024, Ventana Micro Systems, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Base.h>
#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>
#include <Library/BaseRiscVSbiLib.h>
#include <Library/DxeRiscvMpxy.h>

#define INVAL_PHYS_ADDR  (-1U)
#define INVALID_CHAN     -1

#if defined (__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__  /* CPU(little-endian) */
#define LLE_TO_CPU(x)  (SwapBytes64(x))
#define CPU_TO_LLE(x)  (SwapBytes64(x))
#else
#define LLE_TO_CPU(x)  (x)
#define CPU_TO_LLE(x)  (x)
#endif

STATIC VOID     *gNonChanTempShmem     = NULL;
STATIC VOID     *gNonChanTempShmemPhys = NULL;
STATIC VOID     *gShmemVirt            = NULL;
STATIC VOID     *gShmemPhys            = NULL;
STATIC UINTN    gNrShmemPages          = 0;
STATIC UINT64   gShmemPhysHi           = INVAL_PHYS_ADDR;
STATIC UINT64   gShmemPhysLo           = INVAL_PHYS_ADDR;
STATIC UINT64   OldShmemPhysHi         = INVAL_PHYS_ADDR;
STATIC UINT64   OldShmemPhysLo         = INVAL_PHYS_ADDR;
STATIC UINT64   gShmemSize             = 0;
STATIC UINT64   gShmemSet              = 0;
STATIC BOOLEAN  gMpxyLibInitialized    = FALSE;
STATIC UINTN    gShmemRefCount         = 0;

///
/// Set Virtual Address Map Event
///
STATIC EFI_EVENT  mDxeRiscVMpxyLibVirtualNotifyEvent = NULL;

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
  //
  // If there have been no runtime registrations, then just return
  //
  if (gShmemVirt == NULL) {
    return;
  }

  //
  // Convert the channel memory which is of type  EfiRuntimeServicesData to a virtual address.
  //
  gRT->ConvertPointer (0, (VOID **)&gShmemVirt);
  gRT->ConvertPointer (0, (VOID **)&gNonChanTempShmem);
}

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

STATIC
EFI_STATUS
EFIAPI
SbiMpxySetShmem (
  IN UINT64   ShmemPhysHi,
  IN UINT64   ShmemPhysLo,
  IN UINT64   ShmemVirtLo,
  OUT UINT64  *PrevShmemPhysHi,
  OUT UINT64  *PrevShmemPhysLo,
  BOOLEAN     ReadBackOldShmem
  )
{
  SBI_RET  Ret;
  UINT32   Flags       = 0b00;
  UINT64   *PrevMemDet = NULL;

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

    if (ReadBackOldShmem) {
      ASSERT (ShmemVirtLo != INVAL_PHYS_ADDR);
      PrevMemDet       = (UINT64 *)ShmemVirtLo;
      *PrevShmemPhysLo = LLE_TO_CPU (PrevMemDet[0]);
      *PrevShmemPhysHi = LLE_TO_CPU (PrevMemDet[1]);
    }
  }

  return TranslateError (Ret.Error);
}

STATIC
EFI_STATUS
EFIAPI
SbiMpxyRestoreShmem (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = SbiMpxySetShmem (
             OldShmemPhysHi,
             OldShmemPhysLo,
             INVAL_PHYS_ADDR,
             NULL,
             NULL,
             FALSE
             );

  return Status;
}

BOOLEAN
SbiMpxyShmemInitialized (
  VOID
  )
{
  return (gMpxyLibInitialized);
}

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
  UINT32      *Shmem = gNonChanTempShmem;
  UINTN       i;

  if (!gMpxyLibInitialized) {
    return (EFI_DEVICE_ERROR);
  }

  /* Set the shared memory to memory allocated for non-channel specific reads */
  Status = SbiMpxySetShmem (
             0,
             (UINT64)gNonChanTempShmemPhys,
             (UINT64)gNonChanTempShmem,
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
             (UINT64)gNonChanTempShmemPhys,
             (UINT64)gNonChanTempShmem,
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
    gNonChanTempShmem,
    sizeof (UINT32) * NrAttrs
    );

  /* Switch back to old shared memory */
  Status = SbiMpxySetShmem (
             OPhysHi,
             OPhysLo,
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

EFI_STATUS
EFIAPI
SbiMpxyChannelOpen (
  IN UINTN  ChannelId
  )
{
  UINT32      Attributes[MpxyChanAttrMsgDataMaxLen]; // space to read id and version
  UINT32      ChanDataLen;
  VOID        *SbiShmem;
  UINTN       NrEfiPages;
  EFI_STATUS  Status;

  if (SbiMpxyShmemInitialized () == FALSE) {
    return (EFI_UNSUPPORTED);
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
  NrEfiPages  = EFI_SIZE_TO_PAGES (ChanDataLen);

  /*
   * If shared memory is already set and if this channel's memory requirement
   * is more than the current then reallocate memory.
   */

  if (gShmemVirt == NULL) {
    SbiShmem = AllocateAlignedRuntimePages (NrEfiPages, EFI_PAGE_SIZE);
    if (SbiShmem == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }

    /* Save the new shared memory */
    gShmemVirt    = SbiShmem;
    gShmemPhys    = SbiShmem;
    gNrShmemPages = NrEfiPages;
    /* Set the new shared memory */
    Status = SbiMpxySetShmem (
               0,
               (UINT64)SbiShmem,
               (UINT64)SbiShmem,
               &OldShmemPhysHi,
               &OldShmemPhysLo,
               TRUE
               );

    if (EFI_ERROR (Status)) {
      FreeAlignedPages (SbiShmem, NrEfiPages);
      return (EFI_DEVICE_ERROR);
    }
  } else {
    SbiShmem = gShmemPhys;
    /* Set the new shared memory */
    Status = SbiMpxySetShmem (
               0,
               (UINT64)SbiShmem,
               (UINT64)gShmemVirt,
               &OldShmemPhysHi,
               &OldShmemPhysLo,
               TRUE
               );

    if (EFI_ERROR (Status)) {
      /*Don't free */
      return (EFI_DEVICE_ERROR);
    }
  }

  /* Increase the reference count */
  gShmemRefCount++;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SbiMpxyChannelClose (
  IN UINTN  ChannelId
  )
{
  EFI_STATUS  Status;

  /* Ref count is zero. Release the memory */
  Status = SbiMpxyRestoreShmem ();
  if (EFI_ERROR (Status)) {
    return (EFI_DEVICE_ERROR);
  }

  return (EFI_SUCCESS);
}

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
  UINT64   Virt = (UINT64)gShmemVirt;

  if (!gShmemSet) {
    return EFI_DEVICE_ERROR;
  }

  if (MessageDataLen >= gShmemSize) {
    return EFI_INVALID_PARAMETER;
  }

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

  if ((Ret.Error == SBI_SUCCESS) && Response) {
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

  return TranslateError (Ret.Error);
}

/**
  Constructor allocates the global memory to store the registered guid and Handler list.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval  RETURN_SUCCESS            Allocated the global memory space to store guid and function tables.
  @retval  RETURN_OUT_OF_RESOURCES   Not enough memory to allocate.
**/
RETURN_STATUS
EFIAPI
SbiMpxyLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT64      ShmemSize;

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

  //
  // Allocate memory to be shared with OpenSBI for initial MPXY communications
  // untils channels are initialized by their respective drivers.
  //
  gNonChanTempShmem = AllocateAlignedRuntimePages (
                        EFI_SIZE_TO_PAGES (ShmemSize),
                        ShmemSize // Align
                        );

  gShmemSize = ShmemSize;

  if (gNonChanTempShmem == NULL) {
    return (0);
  }

  gNonChanTempShmemPhys = gNonChanTempShmem;
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
  ASSERT_EFI_ERROR (Status);

  gMpxyLibInitialized = TRUE;

  DEBUG ((DEBUG_WARN, "%a: initialization done\n", __func__));

  return (0);
}
