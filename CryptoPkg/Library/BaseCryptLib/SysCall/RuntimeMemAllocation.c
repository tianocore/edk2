/** @file
  Light-weight Memory Management Routines for OpenSSL-based Crypto
  Library at Runtime Phase.

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenSslSupport.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Guid/EventGroup.h>

//----------------------------------------------------------------
// Initial version. Needs further optimizations.
//----------------------------------------------------------------

//
// Definitions for Runtime Memory Operations
//
#define RT_PAGE_SIZE                0x200
#define RT_PAGE_MASK                0x1FF
#define RT_PAGE_SHIFT               9

#define RT_SIZE_TO_PAGES(a)         (((a) >> RT_PAGE_SHIFT) + (((a) & RT_PAGE_MASK) ? 1 : 0))
#define RT_PAGES_TO_SIZE(a)         ((a) << RT_PAGE_SHIFT)

//
// Page Flag Definitions
//
#define RT_PAGE_FREE                0x00000000
#define RT_PAGE_USED                0x00000001

#define MIN_REQUIRED_BLOCKS         600

//
// Memory Page Table
//
typedef struct {
  UINTN   StartPageOffset;      // Offset of the starting page allocated.
                                // Only available for USED pages.
  UINT32  PageFlag;             // Page Attributes.
} RT_MEMORY_PAGE_ENTRY;

typedef struct {
  UINTN                 PageCount;
  UINTN                 LastEmptyPageOffset;
  UINT8                 *DataAreaBase;         // Pointer to data Area.
  RT_MEMORY_PAGE_ENTRY  Pages[1];              // Page Table Entries.
} RT_MEMORY_PAGE_TABLE;

//
// Global Page Table for Runtime Cryptographic Provider.
//
RT_MEMORY_PAGE_TABLE  *mRTPageTable = NULL;

//
// Event for Runtime Address Conversion.
//
EFI_EVENT             mVirtualAddressChangeEvent;


/**
  Initializes pre-allocated memory pointed by ScratchBuffer for subsequent
  runtime use.

  @param[in, out]  ScratchBuffer      Pointer to user-supplied memory buffer.
  @param[in]       ScratchBufferSize  Size of supplied buffer in bytes.

  @retval EFI_SUCCESS  Successful initialization.

**/
EFI_STATUS
InitializeScratchMemory (
  IN OUT  UINT8  *ScratchBuffer,
  IN      UINTN  ScratchBufferSize
  )
{
  UINTN  Index;
  UINTN  MemorySize;

  //
  // Parameters Checking
  //
  if (ScratchBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ScratchBufferSize < MIN_REQUIRED_BLOCKS * 1024) {
    return EFI_BUFFER_TOO_SMALL;
  }

  mRTPageTable = (RT_MEMORY_PAGE_TABLE *)ScratchBuffer;

  //
  // Initialize Internal Page Table for Memory Management
  //
  SetMem (mRTPageTable, ScratchBufferSize, 0xFF);
  MemorySize = ScratchBufferSize - sizeof (RT_MEMORY_PAGE_TABLE) + sizeof (RT_MEMORY_PAGE_ENTRY);

  mRTPageTable->PageCount           = MemorySize / (RT_PAGE_SIZE + sizeof (RT_MEMORY_PAGE_ENTRY));
  mRTPageTable->LastEmptyPageOffset = 0x0;

  for (Index = 0; Index < mRTPageTable->PageCount; Index++) {
    mRTPageTable->Pages[Index].PageFlag        = RT_PAGE_FREE;
    mRTPageTable->Pages[Index].StartPageOffset = 0;
  }

  mRTPageTable->DataAreaBase = ScratchBuffer + sizeof (RT_MEMORY_PAGE_TABLE) +
                               (mRTPageTable->PageCount - 1) * sizeof (RT_MEMORY_PAGE_ENTRY);

  return EFI_SUCCESS;
}


/**
  Look-up Free memory Region for object allocation.

  @param[in]  AllocationSize  Bytes to be allocated.

  @return  Return available page offset for object allocation.

**/
UINTN
LookupFreeMemRegion (
  IN  UINTN  AllocationSize
  )
{
  UINTN  StartPageIndex;
  UINTN  Index;
  UINTN  SubIndex;
  UINTN  ReqPages;

  StartPageIndex = RT_SIZE_TO_PAGES (mRTPageTable->LastEmptyPageOffset);
  ReqPages       = RT_SIZE_TO_PAGES (AllocationSize);

  //
  // Look up the free memory region with in current memory map table.
  //
  for (Index = StartPageIndex; Index <= (mRTPageTable->PageCount - ReqPages); ) {
    //
    // Check consecutive ReqPages pages.
    //
    for (SubIndex = 0; SubIndex < ReqPages; SubIndex++) {
      if ((mRTPageTable->Pages[SubIndex + Index].PageFlag & RT_PAGE_USED) != 0) {
        break;
      }
    }

    if (SubIndex == ReqPages) {
      //
      // Succeed! Return the Starting Offset.
      //
      return RT_PAGES_TO_SIZE (Index);
    }

    //
    // Failed! Skip current free memory pages and adjacent Used pages
    //
    while ((mRTPageTable->Pages[SubIndex + Index].PageFlag & RT_PAGE_USED) != 0) {
      SubIndex++;
    }

    Index += SubIndex;
  }

  //
  // Look up the free memory region from the beginning of the memory table
  // until the StartCursorOffset
  //
  for (Index = 0; Index < (StartPageIndex - ReqPages); ) {
    //
    // Check Consecutive ReqPages Pages.
    //
    for (SubIndex = 0; SubIndex < ReqPages; SubIndex++) {
      if ((mRTPageTable->Pages[SubIndex + Index].PageFlag & RT_PAGE_USED) != 0) {
        break;
      }
    }

    if (SubIndex == ReqPages) {
      //
      // Succeed! Return the Starting Offset.
      //
      return RT_PAGES_TO_SIZE (Index);
    }

    //
    // Failed! Skip current adjacent Used pages
    //
    while ((SubIndex < (StartPageIndex - ReqPages)) &&
           ((mRTPageTable->Pages[SubIndex + Index].PageFlag & RT_PAGE_USED) != 0)) {
      SubIndex++;
    }

    Index += SubIndex;
  }

  //
  // No availabe region for object allocation!
  //
  return (UINTN)(-1);
}


/**
  Allocates a buffer at runtime phase.

  @param[in]  AllocationSize    Bytes to be allocated.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
RuntimeAllocateMem (
  IN  UINTN  AllocationSize
  )
{
  UINT8  *AllocPtr;
  UINTN  ReqPages;
  UINTN  Index;
  UINTN  StartPage;
  UINTN  AllocOffset;

  AllocPtr = NULL;
  ReqPages = 0;

  //
  // Look for available consecutive memory region starting from LastEmptyPageOffset.
  // If no proper memory region found, look up from the beginning.
  // If still not found, return NULL to indicate failed allocation.
  //
  AllocOffset = LookupFreeMemRegion (AllocationSize);
  if (AllocOffset == (UINTN)(-1)) {
    return NULL;
  }

  //
  // Allocates consecutive memory pages with length of Size. Update the page
  // table status. Returns the starting address.
  //
  ReqPages  = RT_SIZE_TO_PAGES (AllocationSize);
  AllocPtr  = mRTPageTable->DataAreaBase + AllocOffset;
  StartPage = RT_SIZE_TO_PAGES (AllocOffset);
  Index     = 0;
  while (Index < ReqPages) {
    mRTPageTable->Pages[StartPage + Index].PageFlag       |= RT_PAGE_USED;
    mRTPageTable->Pages[StartPage + Index].StartPageOffset = AllocOffset;

    Index++;
  }

  mRTPageTable->LastEmptyPageOffset = AllocOffset + RT_PAGES_TO_SIZE (ReqPages);

  ZeroMem (AllocPtr, AllocationSize);

  //
  // Returns a void pointer to the allocated space
  //
  return AllocPtr;
}


/**
  Frees a buffer that was previously allocated at runtime phase.

  @param[in]  Buffer  Pointer to the buffer to free.

**/
VOID
RuntimeFreeMem (
  IN  VOID  *Buffer
  )
{
  UINTN  StartOffset;
  UINTN  StartPageIndex;

  StartOffset    = (UINTN) ((UINT8 *)Buffer - mRTPageTable->DataAreaBase);
  StartPageIndex = RT_SIZE_TO_PAGES (mRTPageTable->Pages[RT_SIZE_TO_PAGES(StartOffset)].StartPageOffset);

  while (StartPageIndex < mRTPageTable->PageCount) {
    if (((mRTPageTable->Pages[StartPageIndex].PageFlag & RT_PAGE_USED) != 0) &&
        (mRTPageTable->Pages[StartPageIndex].StartPageOffset == StartOffset)) {
        //
        // Free this page
        //
        mRTPageTable->Pages[StartPageIndex].PageFlag       &= ~RT_PAGE_USED;
        mRTPageTable->Pages[StartPageIndex].PageFlag       |= RT_PAGE_FREE;
        mRTPageTable->Pages[StartPageIndex].StartPageOffset = 0;

        StartPageIndex++;
    } else {
      break;
    }
  }

  return;
}


/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE
  event. It converts a pointer to a new virtual address.

  @param[in]  Event      The event whose notification function is being invoked.
  @param[in]  Context    The pointer to the notification function's context.

**/
VOID
EFIAPI
RuntimeCryptLibAddressChangeEvent (
  IN  EFI_EVENT        Event,
  IN  VOID             *Context
  )
{
  //
  // Converts a pointer for runtime memory management to a new virtual address.
  //
  EfiConvertPointer (0x0, (VOID **) &mRTPageTable->DataAreaBase);
  EfiConvertPointer (0x0, (VOID **) &mRTPageTable);
}


/**
  Constructor routine for runtime crypt library instance.

  The constructor function pre-allocates space for runtime cryptographic operation.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The construction succeeded.
  @retval EFI_OUT_OF_RESOURCE  Failed to allocate memory.

**/
EFI_STATUS
EFIAPI
RuntimeCryptLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *Buffer;

  //
  // Pre-allocates runtime space for possible cryptographic operations
  //
  Buffer = AllocateRuntimePool (MIN_REQUIRED_BLOCKS * 1024);
  Status = InitializeScratchMemory (Buffer, MIN_REQUIRED_BLOCKS * 1024);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  RuntimeCryptLibAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}


//
// -- Memory-Allocation Routines Wrapper for UEFI-OpenSSL Library --
//

/* Allocates memory blocks */
void *malloc (size_t size)
{
  return RuntimeAllocateMem ((UINTN) size);
}

/* Reallocate memory blocks */
void *realloc (void *ptr, size_t size)
{
  VOID   *NewPtr;
  UINTN  StartOffset;
  UINTN  StartPageIndex;
  UINTN  PageCount;

  //
  // Get Original Size of ptr
  //
  StartOffset    = (UINTN) ((UINT8 *)ptr - mRTPageTable->DataAreaBase);
  StartPageIndex = RT_SIZE_TO_PAGES (mRTPageTable->Pages[RT_SIZE_TO_PAGES (StartOffset)].StartPageOffset);
  PageCount      = 0;
  while (StartPageIndex < mRTPageTable->PageCount) {
    if (((mRTPageTable->Pages[StartPageIndex].PageFlag & RT_PAGE_USED) != 0) &&
        (mRTPageTable->Pages[StartPageIndex].StartPageOffset == StartOffset)) {
        StartPageIndex++;
        PageCount++;
    } else {
      break;
    }
  }

  if (size <= RT_PAGES_TO_SIZE (PageCount)) {
    //
    // Return the original pointer, if Caller try to reduce region size;
    //
    return ptr;
  }

  NewPtr = RuntimeAllocateMem ((UINTN) size);
  if (NewPtr == NULL) {
    return NULL;
  }

  CopyMem (NewPtr, ptr, RT_PAGES_TO_SIZE (PageCount));

  RuntimeFreeMem (ptr);

  return NewPtr;
}

/* Deallocates or frees a memory block */
void free (void *ptr)
{
  RuntimeFreeMem (ptr);
}
