/** @file
  PCI command register operations supporting functions declaration for PCI Bus module.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef _EFI_PCI_COMMAND_H_
#define _EFI_PCI_COMMAND_H_

//
// The PCI Command register bits owned by PCI Bus driver.
//
// They should be cleared at the beginning. The other registers
// are owned by chipset, we should not touch them.
//
#define EFI_PCI_COMMAND_BITS_OWNED                          ( \
                EFI_PCI_COMMAND_IO_SPACE                    | \
                EFI_PCI_COMMAND_MEMORY_SPACE                | \
                EFI_PCI_COMMAND_BUS_MASTER                  | \
                EFI_PCI_COMMAND_MEMORY_WRITE_AND_INVALIDATE | \
                EFI_PCI_COMMAND_VGA_PALETTE_SNOOP           | \
                EFI_PCI_COMMAND_FAST_BACK_TO_BACK             \
                )

//
// The PCI Bridge Control register bits owned by PCI Bus driver.
//
// They should be cleared at the beginning. The other registers
// are owned by chipset, we should not touch them.
//
#define EFI_PCI_BRIDGE_CONTROL_BITS_OWNED                   ( \
                EFI_PCI_BRIDGE_CONTROL_ISA                  | \
                EFI_PCI_BRIDGE_CONTROL_VGA                  | \
                EFI_PCI_BRIDGE_CONTROL_VGA_16               | \
                EFI_PCI_BRIDGE_CONTROL_FAST_BACK_TO_BACK      \
                )

//
// The PCCard Bridge Control register bits owned by PCI Bus driver.
//
// They should be cleared at the beginning. The other registers
// are owned by chipset, we should not touch them.
//
#define EFI_PCCARD_BRIDGE_CONTROL_BITS_OWNED                ( \
                EFI_PCI_BRIDGE_CONTROL_ISA                  | \
                EFI_PCI_BRIDGE_CONTROL_VGA                  | \
                EFI_PCI_BRIDGE_CONTROL_FAST_BACK_TO_BACK      \
                )


#define EFI_GET_REGISTER      1
#define EFI_SET_REGISTER      2
#define EFI_ENABLE_REGISTER   3
#define EFI_DISABLE_REGISTER  4

/**
  Operate the PCI register via PciIo function interface.

  @param PciIoDevice    Pointer to instance of PCI_IO_DEVICE.
  @param Command        Operator command.
  @param Offset         The address within the PCI configuration space for the PCI controller.
  @param Operation      Type of Operation.
  @param PtrCommand     Return buffer holding old PCI command, if operation is not EFI_SET_REGISTER.

  @return Status of PciIo operation.

**/
EFI_STATUS
PciOperateRegister (
  IN  PCI_IO_DEVICE *PciIoDevice,
  IN  UINT16        Command,
  IN  UINT8         Offset,
  IN  UINT8         Operation,
  OUT UINT16        *PtrCommand
  );

/**
  Check the capability supporting by given device.

  @param PciIoDevice   Pointer to instance of PCI_IO_DEVICE.

  @retval TRUE         Capability supported.
  @retval FALSE        Capability not supported.

**/
BOOLEAN
PciCapabilitySupport (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

/**
  Locate capability register block per capability ID.

  @param PciIoDevice       A pointer to the PCI_IO_DEVICE.
  @param CapId             The capability ID.
  @param Offset            A pointer to the offset returned.
  @param NextRegBlock      A pointer to the next block returned.

  @retval EFI_SUCCESS      Successfully located capability register block.
  @retval EFI_UNSUPPORTED  Pci device does not support capability.
  @retval EFI_NOT_FOUND    Pci device support but can not find register block.

**/
EFI_STATUS
LocateCapabilityRegBlock (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINT8          CapId,
  IN OUT UINT8      *Offset,
  OUT UINT8         *NextRegBlock OPTIONAL
  );

/**
  Locate PciExpress capability register block per capability ID.

  @param PciIoDevice       A pointer to the PCI_IO_DEVICE.
  @param CapId             The capability ID.
  @param Offset            A pointer to the offset returned.
  @param NextRegBlock      A pointer to the next block returned.

  @retval EFI_SUCCESS      Successfully located capability register block.
  @retval EFI_UNSUPPORTED  Pci device does not support capability.
  @retval EFI_NOT_FOUND    Pci device support but can not find register block.

**/
EFI_STATUS
LocatePciExpressCapabilityRegBlock (
  IN     PCI_IO_DEVICE *PciIoDevice,
  IN     UINT16        CapId,
  IN OUT UINT32        *Offset,
     OUT UINT32        *NextRegBlock OPTIONAL
  );

/**
  Macro that reads command register.

  @param a[in]            Pointer to instance of PCI_IO_DEVICE.
  @param b[out]           Pointer to the 16-bit value read from command register.

  @return status of PciIo operation

**/
#define PCI_READ_COMMAND_REGISTER(a,b) \
        PciOperateRegister (a, 0, PCI_COMMAND_OFFSET, EFI_GET_REGISTER, b)

/**
  Macro that writes command register.

  @param a[in]            Pointer to instance of PCI_IO_DEVICE.
  @param b[in]            The 16-bit value written into command register.

  @return status of PciIo operation

**/
#define PCI_SET_COMMAND_REGISTER(a,b) \
        PciOperateRegister (a, b, PCI_COMMAND_OFFSET, EFI_SET_REGISTER, NULL)

/**
  Macro that enables command register.

  @param a[in]            Pointer to instance of PCI_IO_DEVICE.
  @param b[in]            The enabled value written into command register.

  @return status of PciIo operation

**/
#define PCI_ENABLE_COMMAND_REGISTER(a,b) \
        PciOperateRegister (a, b, PCI_COMMAND_OFFSET, EFI_ENABLE_REGISTER, NULL)

/**
  Macro that disables command register.

  @param a[in]            Pointer to instance of PCI_IO_DEVICE.
  @param b[in]            The disabled value written into command register.

  @return status of PciIo operation

**/
#define PCI_DISABLE_COMMAND_REGISTER(a,b) \
        PciOperateRegister (a, b, PCI_COMMAND_OFFSET, EFI_DISABLE_REGISTER, NULL)

/**
  Macro that reads PCI bridge control register.

  @param a[in]            Pointer to instance of PCI_IO_DEVICE.
  @param b[out]           The 16-bit value read from control register.

  @return status of PciIo operation

**/
#define PCI_READ_BRIDGE_CONTROL_REGISTER(a,b) \
        PciOperateRegister (a, 0, PCI_BRIDGE_CONTROL_REGISTER_OFFSET, EFI_GET_REGISTER, b)

/**
  Macro that writes PCI bridge control register.

  @param a[in]            Pointer to instance of PCI_IO_DEVICE.
  @param b[in]            The 16-bit value written into control register.

  @return status of PciIo operation

**/
#define PCI_SET_BRIDGE_CONTROL_REGISTER(a,b) \
        PciOperateRegister (a, b, PCI_BRIDGE_CONTROL_REGISTER_OFFSET, EFI_SET_REGISTER, NULL)

/**
  Macro that enables PCI bridge control register.

  @param a[in]            Pointer to instance of PCI_IO_DEVICE.
  @param b[in]            The enabled value written into command register.

  @return status of PciIo operation

**/
#define PCI_ENABLE_BRIDGE_CONTROL_REGISTER(a,b) \
        PciOperateRegister (a, b, PCI_BRIDGE_CONTROL_REGISTER_OFFSET, EFI_ENABLE_REGISTER, NULL)

/**
 Macro that disables PCI bridge control register.

  @param a[in]            Pointer to instance of PCI_IO_DEVICE.
  @param b[in]            The disabled value written into command register.

  @return status of PciIo operation

**/
#define PCI_DISABLE_BRIDGE_CONTROL_REGISTER(a,b) \
        PciOperateRegister (a, b, PCI_BRIDGE_CONTROL_REGISTER_OFFSET, EFI_DISABLE_REGISTER, NULL)

#endif
