/** @file
  RISC-V Interrupt Controllers parser.

  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

typedef struct {
  UINT32        Signature;
  LIST_ENTRY    Link;
  INT32         ExtIntcNode;
  UINT32        GsiBase;
} RISCV_EXT_INTC_DATA;

//
// Unique signature for private data structure.
//
#define RISCV_EXT_INTC_DATA_SIGNATURE  SIGNATURE_32 ('R','S','I','C')

#define RISCV_EXT_INTC_INFO_FROM_THIS(a)  \
  CR (a,                                       \
      RISCV_EXT_INTC_DATA,                \
      Link,                                    \
      RISCV_EXT_INTC_DATA_SIGNATURE       \
      );

STATIC LIST_ENTRY  mExtIntcList;

/** Create list of external interrupt controllers.

  In RISC-V, GSI space is divided among PLIC/APLICs. To convert
  the IRQ number in DT of a device to appropriate GSI number,
  create a list of external interrupt controllers.

  @param [in]  ExtIntcNode      External interrupt controller node.
  @param [in]  GsiBase          GSI base of the interrupt controller.

  @retval EFI_SUCCESS           External interript controller node is added in the list.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the list.
**/
EFI_STATUS
FdtCreateExtIntcList (
  INT32   ExtIntcNode,
  UINT32  GsiBase
  )
{
  RISCV_EXT_INTC_DATA  *Node;

  Node = AllocateZeroPool (sizeof (RISCV_EXT_INTC_DATA));
  if (Node == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialise the list head.
  InitializeListHead (&mExtIntcList);
  Node->Signature   = RISCV_EXT_INTC_DATA_SIGNATURE;
  Node->ExtIntcNode = ExtIntcNode;
  Node->GsiBase     = GsiBase;
  InsertTailList (&mExtIntcList, &Node->Link);
  return EFI_SUCCESS;
}

/** Convert the IRQ number in DT to ACPI GSI number

  In RISC-V, GSI space is divided among PLIC/APLICs. Hence, based
  on the parent interrupt controller, the IRQ number need to be
  converted into appropriate GSI number.

  @param [in]  ExtIntcNode    Parent interrupt controller node.
  @param [in]  Irq            Irq number to convert.

**/
UINT32
FdtConvertToGsi (
  INT32   ExtIntcNode,
  UINT32  Irq
  )
{
  LIST_ENTRY           *Node;
  RISCV_EXT_INTC_DATA  *Data;

  Node = GetFirstNode (&mExtIntcList);
  while (!IsNull (&mExtIntcList, Node)) {
    Data = RISCV_EXT_INTC_INFO_FROM_THIS (Node);
    if (Data->ExtIntcNode == ExtIntcNode) {
      return Data->GsiBase + Irq;
    }

    Node = GetNextNode (&mExtIntcList, Node);
  }

  return Irq;
}

/** Free the external interrupt controller/GSI base list

    @param [in]  ImageHandle  The handle to the image.
    @param [in]  SystemTable  Pointer to the System Table.

    @retval EFI_SUCCESS       The list is freed up.
**/
EFI_STATUS
EFIAPI
RiscVExtIntcDataDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  RISCV_EXT_INTC_DATA  *Data;
  LIST_ENTRY           *NextNode;
  LIST_ENTRY           *Node;

  Node = GetFirstNode (&mExtIntcList);
  while (!IsNull (&mExtIntcList, Node)) {
    Data     = RISCV_EXT_INTC_INFO_FROM_THIS (Node);
    NextNode = GetNextNode (&mExtIntcList, Node);
    RemoveEntryList (&Data->Link);
    FreePool (Data);

    Node = NextNode;
  }

  return EFI_SUCCESS;
}
