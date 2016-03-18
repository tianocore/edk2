/** @file
  Class for arbitrary sized FIFO queues.

  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _FIFO_CLASS_H
#define _FIFO_CLASS_H
#include  <Uefi.h>
#include  <wchar.h>
#include  <Containers/ModuloUtil.h>
#include  <sys/types.h>

__BEGIN_DECLS

typedef struct _FIFO_CLASS  cFIFO;

/// Constants to select what is counted by the FIFO_NumInQueue function.
typedef enum {
  AsElements,     ///< Count the number of readable elements in the queue.
  AsBytes         ///< Count the number of readable bytes in the queue.
} FIFO_ElemBytes;

/** Construct a new instance of a FIFO Queue.

    @param[in]    NumElements   Number of elements to be contained in the new FIFO.
    @param[in]    ElementSize   Size, in bytes, of an element

    @retval   NULL      Unable to create the instance.
    @retval   NonNULL   Pointer to the new FIFO instance.
**/
cFIFO * EFIAPI New_cFIFO(UINT32 NumElements, size_t ElementSize);

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
typedef size_t  (EFIAPI *cFIFO_Enqueue) (cFIFO *Self, const void *ElementPointer, size_t Count);

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
typedef size_t  (EFIAPI *cFIFO_Dequeue) (cFIFO *Self, void *ElementPointer, size_t Count);

/** Make a copy of the FIFO's data.
    The contents of the FIFO is copied out and linearized without affecting the
    FIFO contents.

    @param[in]    Self              Pointer to the FIFO instance.
    @param[out]   ElementPointer    Pointer to where to store the elements copied from the FIFO.
    @param[in]    Count             Number of elements to copy.

    @retval   0       The FIFO is empty.
    @retval   >=0     The number of elements copied from the FIFO.
**/
typedef size_t  (EFIAPI *cFIFO_Copy) (cFIFO *Self, void *ElementPointer, size_t Count);

/** Test whether the FIFO is empty.

    @param[in]    Self      Pointer to the FIFO instance.

    @retval   TRUE    The FIFO is empty.
    @retval   FALSE   The FIFO is NOT empty.
**/
typedef BOOLEAN     (EFIAPI *cFIFO_IsEmpty) (cFIFO *Self);

/** Test whether the FIFO is full.

    @param[in]    Self      Pointer to the FIFO instance.

    @retval   TRUE    The FIFO is full.
    @retval   FALSE   The FIFO is NOT full.
**/
typedef BOOLEAN     (EFIAPI *cFIFO_IsFull)  (cFIFO *Self);

/** Determine number of items available to read from the FIFO.

    The number of items are either the number of bytes, or the number of elements
    depending upon the value of the As enumerator.

    @param[in]    Self      Pointer to the FIFO instance.
    @param[in]    As        An enumeration variable whose value determines whether the
                            returned value is the number of bytes or the number of elements
                            currently contained by the FIFO.

    @retval   0       The FIFO is empty.
    @retval   >=0     The number of items contained in the FIFO.
**/
typedef size_t      (EFIAPI *cFIFO_NumInQueue) (cFIFO *Self, FIFO_ElemBytes As);

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
typedef size_t      (EFIAPI *cFIFO_FreeSpace) (cFIFO *Self, FIFO_ElemBytes As);

/** Empty the FIFO, discarding up to NumToFlush elements.

    @param[in]    Self              Pointer to the FIFO instance.
    @param[in]    NumToFlush        Number of elements to flush from the FIFO.
                                    If larger than the number of elements in the
                                    FIFO, the FIFO is emptied.

    @return     Returns the number of elements remaining in the FIFO after the flush.
**/
typedef size_t     (EFIAPI *cFIFO_Flush)   (cFIFO *Self, size_t NumToFlush);

/** Remove the most recent element from the FIFO.

    @param[in]    Self              Pointer to the FIFO instance.

    @return     Returns the number of elements remaining in the FIFO.
**/
typedef size_t        (EFIAPI *cFIFO_Truncate)  (cFIFO *Self);

/** Cleanly delete a FIFO instance.

    @param[in]    Self              Pointer to the FIFO instance.
**/
typedef void        (EFIAPI *cFIFO_Delete)  (cFIFO *Self);

/** Get the FIFO's current Read Index.

    @param[in]    Self      Pointer to the FIFO instance.

    @return   The current value of the FIFO's ReadIndex member is returned.
**/
typedef UINT32      (EFIAPI *cFIFO_GetRDex) (cFIFO *Self);

/** Get the FIFO's current Write Index.

    @param[in]    Self      Pointer to the FIFO instance.

    @return   The current value of the FIFO's WriteIndex member is returned.
**/
typedef UINT32      (EFIAPI *cFIFO_GetWDex) (cFIFO *Self);

/// Structure declaration for FIFO objects.
struct _FIFO_CLASS {
  /* ######## Public Functions ######## */
  cFIFO_Enqueue     Write;            ///< Write an element into the FIFO.
  cFIFO_Dequeue     Read;             ///< Read an element from the FIFO.
  cFIFO_Copy        Copy;             ///< Non-destructive copy from FIFO.
  cFIFO_IsEmpty     IsEmpty;          ///< Test whether the FIFO is empty.
  cFIFO_IsFull      IsFull;           ///< Test whether the FIFO is full.
  cFIFO_NumInQueue  Count;            ///< Return the number of elements contained in the FIFO.
  cFIFO_FreeSpace   FreeSpace;        ///< Return the number of available elements in the FIFO.
  cFIFO_Flush       Flush;            ///< Remove the N earliest elements from the FIFO.
  cFIFO_Truncate    Truncate;         ///< Remove the most recent element from the FIFO.
  cFIFO_Delete      Delete;           ///< Delete the FIFO object.

  /* ######## Protected Functions ######## */
  cFIFO_GetRDex     GetRDex;          ///< Get a copy of the current Read Index.
  cFIFO_GetWDex     GetWDex;          ///< Get a copy of the current Write Index.

  /* ######## PRIVATE Data ######## */
  void             *Queue;            ///< The FIFO's data storage.
  UINT32            ElementSize;      ///< Number of bytes in an element.
  UINT32            NumElements;      ///< Number of elements the FIFO can store.
  UINT32            ReadIndex;        ///< Index of next element to Read.
  UINT32            WriteIndex;       ///< Index of where next element will be Written.
};

__END_DECLS
#endif  /* _FIFO_CLASS_H */
