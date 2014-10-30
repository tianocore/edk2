/** @file
  Class for arbitrary sized FIFO queues.

  The FIFO is empty if both the Read and Write indexes are equal.
  The FIFO is full if the next write would make the Read and Write indexes equal.

  Member variable NumElements is the maximum number of elements that can be
  contained in the FIFO.
    If NumElements is ZERO, there is an error.
    NumElements should be in the range 1:N.

  Members WriteIndex and ReadIndex are indexes into the array implementing the
  FIFO.  They should be in the range 0:(NumElements - 1).

  One element of the FIFO is always reserved as the "terminator" element.  Thus,
  the capacity of a FIFO is actually NumElements-1.

  Copyright (c) 2012 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <Library/BaseLib.h>
#include  <Library/BaseMemoryLib.h>
#include  <Library/MemoryAllocationLib.h>

#include  <LibConfig.h>

#include  <assert.h>
#include  <errno.h>
#include  <stdlib.h>
#include  <stdint.h>
#include  <wchar.h>
#include  <Containers/Fifo.h>

/** Determine number of items available to read from the FIFO.

    The number of items are either the number of bytes, or the number of elements
    depending upon the value of the As enumerator.

    @param[in]    Self      Pointer to the FIFO instance.
    @param[in]    As        An enumeration variable whose value determines whether the
                            returned value is the number of bytes or the number of elements
                            currently contained by the FIFO.

    @retval   0       The FIFO is empty.
    @retval   >=0     The number of items contained by the FIFO.
**/
static
size_t
EFIAPI
FIFO_NumInQueue (
  cFIFO        *Self,
  FIFO_ElemBytes    As
)
{
  size_t    Count;

  if(Self->ReadIndex <= Self->WriteIndex) {
    Count = Self->WriteIndex - Self->ReadIndex;
  }
  else {
    Count = Self->NumElements - (Self->ReadIndex - Self->WriteIndex);
  }
  if(As == AsBytes) {
    Count *= Self->ElementSize;
  }
  return Count;
}

/** Determine amount of free space in the FIFO that can be written into.

    The number of items are either the number of bytes, or the number of elements
    depending upon the value of the As enumerator.

    @param[in]    Self      Pointer to the FIFO instance.
    @param[in]    As        An enumeration variable whose value determines whether the
                            returned value is the number of bytes or the number of elements
                            currently available in the FIFO.

    @retval   0       The FIFO is full.
    @retval   >=0     The number of items which can be accepted by the FIFO.
**/
static
size_t
EFIAPI
FIFO_FreeSpace (
  cFIFO            *Self,
  FIFO_ElemBytes    As
)
{
  size_t    Count;
  UINT32    RDex;
  UINT32    WDex;

  RDex = Self->ReadIndex;
  WDex = Self->WriteIndex;

  if(RDex <= WDex) {
    Count = (Self->NumElements - (WDex - RDex)) - 1;
  }
  else {
    Count = (RDex - WDex)-1;
  }
  if(As == AsBytes) {
    Count *= Self->ElementSize;
  }
  return Count;
}

/** Reduce the FIFO contents by NumElem elements.

    @param[in]    Self      Pointer to the FIFO instance.
    @param[in]    NumElem   Number of elements to delete from the FIFO.

    @retval   0   FIFO is now empty.
    @retval   N>0 There are still N elements in the FIFO.
    @retval   -1  There are fewer than NumElem elements in the FIFO.
**/
static
ssize_t
FIFO_Reduce (
  cFIFO    *Self,
  size_t    NumElem
  )
{
  size_t    QCount;
  ssize_t   RetVal;

  assert(Self != NULL);

  QCount = FIFO_NumInQueue(Self, AsElements);
  if(NumElem > QCount) {
    RetVal = -1;
    errno   = EINVAL;
  }
  else {
    RetVal = (ssize_t)ModuloAdd(Self->ReadIndex, (UINT32)NumElem, Self->NumElements);
    Self->ReadIndex = (UINT32)RetVal;

    RetVal = (ssize_t)(QCount - NumElem);
  }
  return RetVal;
}

/** Test whether the FIFO is empty.

    @param[in]    Self      Pointer to the FIFO instance.

    @retval   TRUE    The FIFO is empty.
    @retval   FALSE   There is data in the FIFO.
**/
static
BOOLEAN
EFIAPI
FIFO_IsEmpty (
  cFIFO *Self
  )
{
  assert(Self != NULL);

  return (BOOLEAN)(Self->WriteIndex == Self->ReadIndex);
}

/** Test whether the FIFO is full.

    @param[in]    Self      Pointer to the FIFO instance.

    @retval   TRUE    The FIFO is full.
    @retval   FALSE   There is free space in the FIFO.
**/
static
BOOLEAN
EFIAPI
FIFO_IsFull (
  cFIFO *Self
  )
{
  assert(Self != NULL);

  return (BOOLEAN)(ModuloIncrement(Self->WriteIndex, Self->NumElements) == (INT32)Self->ReadIndex);
}

/** Add one or more elements to the FIFO.

    This function allows one to add one or more elements, as specified by Count,
    to the FIFO.  Each element is of the size specified when the FIFO object
    was instantiated (FIFO.ElementSize).

    pElement points to the first byte of the first element to be added.
    If multiple elements are to be added, the elements are expected to be
    organized as a packed array.

    @param[in]    Self        Pointer to the FIFO instance.
    @param[in]    pElement    Pointer to the element(s) to enqueue (add).
    @param[in]    Count       Number of elements to add.

    @retval   0       The FIFO is full.
    @retval   >=0     The number of elements added to the FIFO.
**/
static
size_t
EFIAPI
FIFO_Enqueue (
  cFIFO        *Self,
  const void   *pElement,
  size_t        Count
  )
{
  uintptr_t     ElemPtr;
  uintptr_t     QPtr;
  size_t        i;
  UINT32        SizeOfElement;
  UINT32        Windex;

  assert(Self != NULL);
  assert(pElement != NULL);

  if(FIFO_IsFull(Self)) {                                                 // FIFO is full so can't add to it
    Count = 0;                                                              // Zero characters added
  }
  else {                                                                  // Otherwise, FIFO is not full...
    Count = MIN(Count, Self->FreeSpace(Self, AsElements));                  // Smaller of requested or available space
    SizeOfElement = Self->ElementSize;                                      // Size of Elements, in bytes
    Windex = Self->WriteIndex;                                              // Index of first writable slot in FIFO

    ElemPtr = (uintptr_t)pElement;                                          // Addr. of element to add, as an integer
    QPtr    = (uintptr_t)Self->Queue + (SizeOfElement * Windex);            // Addr. in FIFO to write, as an integer

    for(i = 0; i < Count; ++i) {                                            // For Count elements...
      (void)CopyMem((void *)QPtr, (const void *)ElemPtr, SizeOfElement);      // Copy an element into the FIFO
      Windex = (UINT32)ModuloIncrement(Windex, Self->NumElements);            // Increment the Write index, wrap if necessary
      if(Windex == 0) {                                                       // If the index wrapped
        QPtr = (uintptr_t)Self->Queue;                                          // Go to the beginning
      }
      else {
        QPtr += SizeOfElement;                                                // Otherwise, point to next in FIFO
      }
      ElemPtr += SizeOfElement;                                               // And also point to next Element to add
    }
    Self->WriteIndex = Windex;                                              // Finally, save the new Write Index
  }
  return Count;                                                           // Number of elements added to FIFO
}

/** Read or copy elements from the FIFO.

    This function allows one to read one or more elements, as specified by Count,
    from the FIFO.  Each element is of the size specified when the FIFO object
    was instantiated (FIFO.ElementSize).

    pElement points to the destination of the first byte of the first element
    to be read. If multiple elements are to be read, the elements are expected
    to be organized as a packed array.

    @param[in]    Self        Pointer to the FIFO instance.
    @param[out]   pElement    Pointer to where to store the element(s) read from the FIFO.
    @param[in]    Count       Number of elements to dequeue.
    @param[in]    Consume     If TRUE, consume read elements.  Otherwise, preserve.

    @retval   0       The FIFO is empty.
    @retval   >=0     The number of elements read from the FIFO.
**/
static
size_t
EFIAPI
FIFO_Dequeue (
  cFIFO    *Self,
  void     *pElement,
  size_t    Count,
  BOOLEAN   Consume
  )
{
  UINTN         QPtr;
  UINT32        RDex;
  UINT32        SizeOfElement;
  UINT32        i;

  assert(Self != NULL);
  assert(pElement != NULL);

  if(FIFO_IsEmpty(Self)) {
    Count = 0;
  }
  else {
    RDex          = Self->ReadIndex;                                  // Get this FIFO's Read Index
    SizeOfElement = Self->ElementSize;                                // Get size of this FIFO's elements
    Count         = MIN(Count, Self->Count(Self, AsElements));        // Lesser of requested or actual

    QPtr = (UINTN)Self->Queue + (RDex * SizeOfElement);           // Point to Read location in FIFO
    for(i = 0; i < Count; ++i) {                                      // Iterate Count times...
      (void)CopyMem(pElement, (const void *)QPtr, SizeOfElement);   // Copy element from FIFO to caller's buffer
      RDex = (UINT32)ModuloIncrement(RDex, Self->NumElements);          // Increment Read Index
      if(RDex == 0) {                                                   // If the index wrapped
        QPtr = (UINTN)Self->Queue;                                        // Point back to beginning of data
      }
      else {                                                            // Otherwise
        QPtr += SizeOfElement;                                        // Point to the next element in FIFO
      }
      pElement = (char*)pElement + SizeOfElement;                   // Point to next element in caller's buffer
    }                                                                 // Iterate: for loop
    if(Consume) {                                                     // If caller requests data consumption
      Self->ReadIndex = RDex;                                           // Set FIFO's Read Index to new Index
    }
  }
  return Count;                                                     // Return number of elements actually read
}

/** Read elements from the FIFO.

    Read the specified number of elements from the FIFO, removing each element read.
    The number of elements actually read from the FIFO is returned.  This number can differ
    from the Count requested if more elements are requested than are in the FIFO.

    @param[in]    Self        Pointer to the FIFO instance.
    @param[out]   pElement    Pointer to where to store the element read from the FIFO.
    @param[in]    Count       Number of elements to dequeue.

    @retval   0       The FIFO is empty.
    @retval   >=0     The number of elements read from the FIFO.
**/
static
size_t
EFIAPI
FIFO_Read (
  cFIFO    *Self,
  void     *pElement,
  size_t    Count
  )
{
  return FIFO_Dequeue(Self, pElement, Count, TRUE);
}

/** Make a copy of the FIFO's data.
    The contents of the FIFO is copied out and linearized without affecting the
    FIFO contents.  This function is idempotent.

    @param[in]    Self        Pointer to the FIFO instance.
    @param[out]   pElement    Pointer to where to store the elements copied from the FIFO.
    @param[in]    Count       Number of elements to copy.

    @retval   0       The FIFO is empty.
    @retval   >=0     The number of elements copied from the FIFO.
**/
static
size_t
EFIAPI
FIFO_Copy (
  cFIFO    *Self,
  void     *pElement,
  size_t    Count
  )
{
  return FIFO_Dequeue(Self, pElement, Count, FALSE);
}

/** Get the FIFO's current Read Index.

    @param[in]    Self      Pointer to the FIFO instance.
**/
static
UINT32
EFIAPI
FIFO_GetRDex (
  cFIFO *Self
)
{
  assert(Self != NULL);

  return Self->ReadIndex;
}

/** Get the FIFO's current Write Index.

    @param[in]    Self      Pointer to the FIFO instance.

    @return   The current value of the FIFO's WriteIndex member is returned.
**/
static
UINT32
EFIAPI
FIFO_GetWDex (
  cFIFO *Self
)
{
  assert(Self != NULL);

  return Self->WriteIndex;
}

/** Cleanly delete a FIFO instance.

    @param[in]    Self              Pointer to the FIFO instance.
**/
static
void
EFIAPI
FIFO_Delete (
  cFIFO *Self
  )
{
  assert(Self != NULL);

  if(Self->Queue != NULL) {
    FreePool(Self->Queue);
    Self->Queue = NULL;     // Zombie catcher
  }
  FreePool(Self);
}

/** Empty the FIFO, discarding up to NumToFlush elements.

    @param[in]    Self              Pointer to the FIFO instance.
    @param[in]    NumToFlush        Number of elements to flush from the FIFO.
                                    If larger than the number of elements in the
                                    FIFO, the FIFO is emptied.

    @return     Returns the number of elements remaining in the FIFO after the flush.
**/
static
size_t
EFIAPI
FIFO_Flush (
  cFIFO  *Self,
  size_t  NumToFlush
  )
{
  size_t  NumInQ;
  size_t  Remainder;

  assert(Self != NULL);

  NumInQ = FIFO_NumInQueue(Self, AsElements);
  if(NumToFlush >= NumInQ) {
    Self->ReadIndex   = 0;
    Self->WriteIndex  = 0;
    Remainder = 0;
  }
  else {
    Remainder = FIFO_Reduce(Self, NumToFlush);
  }
  return Remainder;
}

/** Remove the most recently added element from the FIFO.

    @param[in]    Self              Pointer to the FIFO instance.

    @return     Returns the number of elements remaining in the FIFO.
**/
static
size_t
EFIAPI
FIFO_Truncate (
  cFIFO  *Self
  )
{
  size_t  Remainder;

  assert(Self != NULL);

  Remainder = FIFO_NumInQueue(Self, AsElements);
  if(Remainder > 0) {
    Self->WriteIndex = (UINT32)ModuloDecrement(Self->WriteIndex, Self->NumElements);
    --Remainder;
  }
  return Remainder;
}

/** Construct a new instance of a FIFO Queue.

    @param[in]    NumElements   Number of elements to be contained in the new FIFO.
    @param[in]    ElementSize   Size, in bytes, of an element.

    @retval   NULL      Unable to create the instance.
    @retval   NonNULL   Pointer to the new FIFO instance.
**/
cFIFO *
EFIAPI
New_cFIFO(
  UINT32    NumElements,
  size_t    ElementSize
  )
{
  cFIFO        *FIFO;
  UINT8        *Queue;

  FIFO = NULL;
  if((NumElements > 2) && (ElementSize > 0)) {
    FIFO = (cFIFO *)AllocatePool(sizeof(cFIFO));
    if(FIFO != NULL) {
      Queue = (UINT8 *)AllocateZeroPool(NumElements * ElementSize);
      if(Queue != NULL) {
        FIFO->Write       = FIFO_Enqueue;
        FIFO->Read        = FIFO_Read;
        FIFO->Copy        = FIFO_Copy;
        FIFO->IsEmpty     = FIFO_IsEmpty;
        FIFO->IsFull      = FIFO_IsFull;
        FIFO->Count       = FIFO_NumInQueue;
        FIFO->FreeSpace   = FIFO_FreeSpace;
        FIFO->Flush       = FIFO_Flush;
        FIFO->Truncate    = FIFO_Truncate;
        FIFO->Delete      = FIFO_Delete;
        FIFO->GetRDex     = FIFO_GetRDex;
        FIFO->GetWDex     = FIFO_GetWDex;

        FIFO->Queue       = Queue;
        FIFO->ElementSize = (UINT32)ElementSize;
        FIFO->NumElements = (UINT32)NumElements;
        FIFO->ReadIndex   = 0;
        FIFO->WriteIndex  = 0;
      }
      else {
        FreePool(FIFO);
        FIFO = NULL;
      }
    }
  }
  return FIFO;
}
