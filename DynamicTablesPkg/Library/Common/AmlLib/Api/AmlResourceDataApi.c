/** @file
  AML Update Resource Data.

  Copyright (c) 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

/* Even though this file has access to the internal Node definition,
   i.e. AML_ROOT_NODE, AML_OBJECT_NODE, etc. Only the external node
   handle types should be used, i.e. AML_NODE_HANDLE, AML_ROOT_NODE_HANDLE,
   etc.
   Indeed, the functions in the "Api" folder should be implemented only
   using the "safe" functions available in the "Include" folder. This
   makes the functions available in the "Api" folder easy to export.
*/
#include <AmlNodeDefines.h>

#include <AmlCoreInterface.h>
#include <AmlInclude.h>
#include <Api/AmlApiHelper.h>
#include <CodeGen/AmlResourceDataCodeGen.h>
#include <Utils/AmlUtility.h>

/* Macros to read General flags of a Resource Data Address Descriptors */
#define IS_RD_ADDR_POS_DECODE(GenFlag)  (((GenFlag) & BIT1) == 0)
#define IS_RD_ADDR_MIN_FIXED(GenFlag)   (((GenFlag) & BIT2) == BIT2)
#define IS_RD_ADDR_MAX_FIXED(GenFlag)   (((GenFlag) & BIT3) == BIT3)

/** Update the first interrupt of an Interrupt resource data node.

  The flags of the Interrupt resource data are left unchanged.

  The InterruptRdNode corresponds to the Resource Data created by the
  "Interrupt ()" ASL macro. It is an Extended Interrupt Resource Data.
  See ACPI 6.3 specification, s6.4.3.6 "Extended Interrupt Descriptor"
  for more information about Extended Interrupt Resource Data.

  @param  [in]  InterruptRdNode   Pointer to the an extended interrupt
                                  resource data node.
  @param  [in]  Irq               Interrupt value to update.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
EFIAPI
AmlUpdateRdInterrupt (
  IN  AML_DATA_NODE_HANDLE  InterruptRdNode,
  IN  UINT32                Irq
  )
{
  EFI_STATUS  Status;
  UINT32      *FirstInterrupt;
  UINT8       *QueryBuffer;
  UINT32      QueryBufferSize;

  if ((InterruptRdNode == NULL)                                           ||
      (AmlGetNodeType ((AML_NODE_HANDLE)InterruptRdNode) != EAmlNodeData) ||
      (!AmlNodeHasDataType (
          InterruptRdNode,
          EAmlNodeDataTypeResourceData
          ))                                  ||
      (!AmlNodeHasRdDataType (
          InterruptRdNode,
          AML_RD_BUILD_LARGE_DESC_ID (
            ACPI_LARGE_EXTENDED_IRQ_DESCRIPTOR_NAME
            )
          )))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  QueryBuffer = NULL;

  // Get the size of the InterruptRdNode buffer.
  Status = AmlGetDataNodeBuffer (
             InterruptRdNode,
             NULL,
             &QueryBufferSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Check the Buffer is large enough.
  if (QueryBufferSize < sizeof (EFI_ACPI_EXTENDED_INTERRUPT_DESCRIPTOR)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Allocate a buffer to fetch the data.
  QueryBuffer = AllocatePool (QueryBufferSize);
  if (QueryBuffer == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // Get the data.
  Status = AmlGetDataNodeBuffer (
             InterruptRdNode,
             QueryBuffer,
             &QueryBufferSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // Get the address of the first interrupt field.
  FirstInterrupt =
    ((EFI_ACPI_EXTENDED_INTERRUPT_DESCRIPTOR *)QueryBuffer)->InterruptNumber;

  *FirstInterrupt = Irq;

  // Update the InterruptRdNode buffer.
  Status = AmlUpdateDataNode (
             InterruptRdNode,
             EAmlNodeDataTypeResourceData,
             QueryBuffer,
             QueryBufferSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
  }

error_handler:
  if (QueryBuffer != NULL) {
    FreePool (QueryBuffer);
  }

  return Status;
}

/** Update the interrupt list of an interrupt resource data node.

  The InterruptRdNode corresponds to the Resource Data created by the
  "Interrupt ()" ASL function. It is an Extended Interrupt Resource Data.
  See ACPI 6.3 specification, s6.4.3.6 "Extended Interrupt Descriptor"
  for more information about Extended Interrupt Resource Data.

  @param  [in]  InterruptRdNode   Pointer to the an extended interrupt
                                  resource data node.
  @param  [in]  ResourceConsumer    The device consumes the specified interrupt
                                    or produces it for use by a child device.
  @param  [in]  EdgeTriggered       The interrupt is edge triggered or
                                    level triggered.
  @param  [in]  ActiveLow           The interrupt is active-high or active-low.
  @param  [in]  Shared              The interrupt can be shared with other
                                    devices or not (Exclusive).
  @param  [in]  IrqList           Interrupt list. Must be non-NULL.
  @param  [in]  IrqCount          Interrupt count. Must be non-zero.


  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
EFIAPI
AmlUpdateRdInterruptEx (
  IN  AML_DATA_NODE_HANDLE  InterruptRdNode,
  IN  BOOLEAN               ResourceConsumer,
  IN  BOOLEAN               EdgeTriggered,
  IN  BOOLEAN               ActiveLow,
  IN  BOOLEAN               Shared,
  IN  UINT32                *IrqList,
  IN  UINT8                 IrqCount
  )
{
  EFI_STATUS  Status;

  EFI_ACPI_EXTENDED_INTERRUPT_DESCRIPTOR  *RdInterrupt;
  UINT32                                  *FirstInterrupt;
  UINT8                                   *UpdateBuffer;
  UINT16                                  UpdateBufferSize;

  if ((InterruptRdNode == NULL)                                              ||
      (AmlGetNodeType ((AML_NODE_HANDLE)InterruptRdNode) != EAmlNodeData)    ||
      (!AmlNodeHasDataType (
          InterruptRdNode,
          EAmlNodeDataTypeResourceData
          ))                                     ||
      (!AmlNodeHasRdDataType (
          InterruptRdNode,
          AML_RD_BUILD_LARGE_DESC_ID (
            ACPI_LARGE_EXTENDED_IRQ_DESCRIPTOR_NAME
            )
          ))                       ||
      (IrqList == NULL)                                                      ||
      (IrqCount == 0))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  UpdateBuffer     = NULL;
  UpdateBufferSize = sizeof (EFI_ACPI_EXTENDED_INTERRUPT_DESCRIPTOR) +
                     ((IrqCount - 1) * sizeof (UINT32));

  // Allocate a buffer to update the data.
  UpdateBuffer = AllocatePool (UpdateBufferSize);
  if (UpdateBuffer == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // Update the Resource Data information (structure size, interrupt count).
  RdInterrupt                     = (EFI_ACPI_EXTENDED_INTERRUPT_DESCRIPTOR *)UpdateBuffer;
  RdInterrupt->Header.Header.Byte =
    AML_RD_BUILD_LARGE_DESC_ID (ACPI_LARGE_EXTENDED_IRQ_DESCRIPTOR_NAME);
  RdInterrupt->Header.Length =
    UpdateBufferSize - sizeof (ACPI_LARGE_RESOURCE_HEADER);
  RdInterrupt->InterruptTableLength = IrqCount;
  RdInterrupt->InterruptVectorFlags = (ResourceConsumer ? BIT0 : 0) |
                                      (EdgeTriggered ? BIT1 : 0)    |
                                      (ActiveLow ? BIT2 : 0)        |
                                      (Shared ? BIT3 : 0);

  // Get the address of the first interrupt field.
  FirstInterrupt =
    ((EFI_ACPI_EXTENDED_INTERRUPT_DESCRIPTOR *)UpdateBuffer)->InterruptNumber;

  // Copy the input list of interrupts.
  CopyMem (FirstInterrupt, IrqList, (sizeof (UINT32) * IrqCount));

  // Update the InterruptRdNode buffer.
  Status = AmlUpdateDataNode (
             InterruptRdNode,
             EAmlNodeDataTypeResourceData,
             UpdateBuffer,
             UpdateBufferSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
  }

  // Cleanup
  FreePool (UpdateBuffer);

  return Status;
}

/** Update the base address and length of a QWord resource data node.

  @param  [in] QWordRdNode         Pointer a QWord resource data
                                   node.
  @param  [in] BaseAddress         Base address.
  @param  [in] BaseAddressLength   Base address length.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
EFIAPI
AmlUpdateRdQWord (
  IN  AML_DATA_NODE_HANDLE  QWordRdNode,
  IN  UINT64                BaseAddress,
  IN  UINT64                BaseAddressLength
  )
{
  EFI_STATUS                               Status;
  EFI_ACPI_QWORD_ADDRESS_SPACE_DESCRIPTOR  *RdQWord;

  UINT8   *QueryBuffer;
  UINT32  QueryBufferSize;

  if ((QWordRdNode == NULL)                                             ||
      (AmlGetNodeType ((AML_NODE_HANDLE)QWordRdNode) != EAmlNodeData)   ||
      (!AmlNodeHasDataType (QWordRdNode, EAmlNodeDataTypeResourceData)) ||
      (!AmlNodeHasRdDataType (
          QWordRdNode,
          AML_RD_BUILD_LARGE_DESC_ID (
            ACPI_LARGE_QWORD_ADDRESS_SPACE_DESCRIPTOR_NAME
            )
          )) ||
      (BaseAddressLength == 0))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the size of the QWordRdNode's buffer.
  Status = AmlGetDataNodeBuffer (
             QWordRdNode,
             NULL,
             &QueryBufferSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Allocate a buffer to fetch the data.
  QueryBuffer = AllocatePool (QueryBufferSize);
  if (QueryBuffer == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // Get the data.
  Status = AmlGetDataNodeBuffer (
             QWordRdNode,
             QueryBuffer,
             &QueryBufferSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  RdQWord = (EFI_ACPI_QWORD_ADDRESS_SPACE_DESCRIPTOR *)QueryBuffer;

  // Update the Base Address and Length.
  RdQWord->AddrRangeMin = BaseAddress;
  RdQWord->AddrRangeMax = BaseAddress + BaseAddressLength - 1;
  RdQWord->AddrLen      = BaseAddressLength;

  Status = CheckAddressSpaceFields (
             IS_RD_ADDR_MIN_FIXED (RdQWord->GenFlag),
             IS_RD_ADDR_MAX_FIXED (RdQWord->GenFlag),
             RdQWord->AddrSpaceGranularity,
             RdQWord->AddrRangeMin,
             RdQWord->AddrRangeMax,
             RdQWord->AddrTranslationOffset,
             RdQWord->AddrLen
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // Update Base Address Resource Data node.
  Status = AmlUpdateDataNode (
             QWordRdNode,
             EAmlNodeDataTypeResourceData,
             QueryBuffer,
             QueryBufferSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
  }

error_handler:
  if (QueryBuffer != NULL) {
    FreePool (QueryBuffer);
  }

  return Status;
}

/** Update the base address and length of a DWord resource data node.

  @param  [in] DWordRdNode         Pointer a DWord resource data
                                   node.
  @param  [in] BaseAddress         Base address.
  @param  [in] BaseAddressLength   Base address length.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
EFIAPI
AmlUpdateRdDWord (
  IN  AML_DATA_NODE_HANDLE  DWordRdNode,
  IN  UINT32                BaseAddress,
  IN  UINT32                BaseAddressLength
  )
{
  EFI_STATUS                               Status;
  EFI_ACPI_DWORD_ADDRESS_SPACE_DESCRIPTOR  *RdDWord;

  UINT8   *QueryBuffer;
  UINT32  QueryBufferSize;

  if ((DWordRdNode == NULL)                                             ||
      (AmlGetNodeType ((AML_NODE_HANDLE)DWordRdNode) != EAmlNodeData)   ||
      (!AmlNodeHasDataType (DWordRdNode, EAmlNodeDataTypeResourceData)) ||
      (!AmlNodeHasRdDataType (
          DWordRdNode,
          AML_RD_BUILD_LARGE_DESC_ID (
            ACPI_LARGE_DWORD_ADDRESS_SPACE_DESCRIPTOR_NAME
            )
          )) ||
      (BaseAddressLength == 0))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the size of the DWordRdNode's buffer.
  Status = AmlGetDataNodeBuffer (
             DWordRdNode,
             NULL,
             &QueryBufferSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Allocate a buffer to fetch the data.
  QueryBuffer = AllocatePool (QueryBufferSize);
  if (QueryBuffer == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // Get the data.
  Status = AmlGetDataNodeBuffer (
             DWordRdNode,
             QueryBuffer,
             &QueryBufferSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  RdDWord = (EFI_ACPI_DWORD_ADDRESS_SPACE_DESCRIPTOR *)QueryBuffer;

  // Update the Base Address and Length.
  RdDWord->AddrRangeMin = BaseAddress;
  RdDWord->AddrRangeMax = BaseAddress + BaseAddressLength - 1;
  RdDWord->AddrLen      = BaseAddressLength;

  Status = CheckAddressSpaceFields (
             IS_RD_ADDR_MIN_FIXED (RdDWord->GenFlag),
             IS_RD_ADDR_MAX_FIXED (RdDWord->GenFlag),
             RdDWord->AddrSpaceGranularity,
             RdDWord->AddrRangeMin,
             RdDWord->AddrRangeMax,
             RdDWord->AddrTranslationOffset,
             RdDWord->AddrLen
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // Update Base Address Resource Data node.
  Status = AmlUpdateDataNode (
             DWordRdNode,
             EAmlNodeDataTypeResourceData,
             QueryBuffer,
             QueryBufferSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
  }

error_handler:
  if (QueryBuffer != NULL) {
    FreePool (QueryBuffer);
  }

  return Status;
}

/** Update the base address and length of a Word resource data node.

  @param  [in] WordRdNode          Pointer a Word resource data
                                   node.
  @param  [in] BaseAddress         Base address.
  @param  [in] BaseAddressLength   Base address length.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
EFIAPI
AmlUpdateRdWord (
  IN  AML_DATA_NODE_HANDLE  WordRdNode,
  IN  UINT16                BaseAddress,
  IN  UINT16                BaseAddressLength
  )
{
  EFI_STATUS                              Status;
  EFI_ACPI_WORD_ADDRESS_SPACE_DESCRIPTOR  *RdWord;

  UINT8   *QueryBuffer;
  UINT32  QueryBufferSize;

  if ((WordRdNode == NULL)                                             ||
      (AmlGetNodeType ((AML_NODE_HANDLE)WordRdNode) != EAmlNodeData)   ||
      (!AmlNodeHasDataType (WordRdNode, EAmlNodeDataTypeResourceData)) ||
      (!AmlNodeHasRdDataType (
          WordRdNode,
          AML_RD_BUILD_LARGE_DESC_ID (
            ACPI_LARGE_WORD_ADDRESS_SPACE_DESCRIPTOR_NAME
            )
          )) ||
      (BaseAddressLength == 0))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the size of the WordRdNode's buffer.
  Status = AmlGetDataNodeBuffer (
             WordRdNode,
             NULL,
             &QueryBufferSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Allocate a buffer to fetch the data.
  QueryBuffer = AllocatePool (QueryBufferSize);
  if (QueryBuffer == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // Get the data.
  Status = AmlGetDataNodeBuffer (
             WordRdNode,
             QueryBuffer,
             &QueryBufferSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  RdWord = (EFI_ACPI_WORD_ADDRESS_SPACE_DESCRIPTOR *)QueryBuffer;

  // Update the Base Address and Length.
  RdWord->AddrRangeMin = BaseAddress;
  RdWord->AddrRangeMax = BaseAddress + BaseAddressLength - 1;
  RdWord->AddrLen      = BaseAddressLength;

  Status = CheckAddressSpaceFields (
             IS_RD_ADDR_MIN_FIXED (RdWord->GenFlag),
             IS_RD_ADDR_MAX_FIXED (RdWord->GenFlag),
             RdWord->AddrSpaceGranularity,
             RdWord->AddrRangeMin,
             RdWord->AddrRangeMax,
             RdWord->AddrTranslationOffset,
             RdWord->AddrLen
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // Update Base Address Resource Data node.
  Status = AmlUpdateDataNode (
             WordRdNode,
             EAmlNodeDataTypeResourceData,
             QueryBuffer,
             QueryBufferSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
  }

error_handler:
  if (QueryBuffer != NULL) {
    FreePool (QueryBuffer);
  }

  return Status;
}
