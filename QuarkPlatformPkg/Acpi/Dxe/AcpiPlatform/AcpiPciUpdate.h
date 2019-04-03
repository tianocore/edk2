/** @file
Update the _PRT and _PRW method for pci devices

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/
#ifndef _ACPI_PCI_UPDATE_H_
#define _ACPI_PCI_UPDATE_H_


//
// Primary OpCode
//
#define AML_ZERO_OP                  0x00
#define AML_ONE_OP                   0x01
#define AML_ALIAS_OP                 0x06
#define AML_NAME_OP                  0x08
#define AML_BYTE_PREFIX              0x0a
#define AML_WORD_PREFIX              0x0b
#define AML_DWORD_PREFIX             0x0c
#define AML_STRING_PREFIX            0x0d
#define AML_QWORD_PREFIX             0x0e
#define AML_SCOPE_OP                 0x10
#define AML_BUFFER_OP                0x11
#define AML_PACKAGE_OP               0x12
#define AML_VAR_PACKAGE_OP           0x13
#define AML_METHOD_OP                0x14
#define AML_DUAL_NAME_PREFIX         0x2e
#define AML_MULTI_NAME_PREFIX        0x2f
#define AML_NAME_CHAR_A              0x41
#define AML_NAME_CHAR_B              0x42
#define AML_NAME_CHAR_C              0x43
#define AML_NAME_CHAR_D              0x44
#define AML_NAME_CHAR_E              0x45
#define AML_NAME_CHAR_F              0x46
#define AML_NAME_CHAR_G              0x47
#define AML_NAME_CHAR_H              0x48
#define AML_NAME_CHAR_I              0x49
#define AML_NAME_CHAR_J              0x4a
#define AML_NAME_CHAR_K              0x4b
#define AML_NAME_CHAR_L              0x4c
#define AML_NAME_CHAR_M              0x4d
#define AML_NAME_CHAR_N              0x4e
#define AML_NAME_CHAR_O              0x4f
#define AML_NAME_CHAR_P              0x50
#define AML_NAME_CHAR_Q              0x51
#define AML_NAME_CHAR_R              0x52
#define AML_NAME_CHAR_S              0x53
#define AML_NAME_CHAR_T              0x54
#define AML_NAME_CHAR_U              0x55
#define AML_NAME_CHAR_V              0x56
#define AML_NAME_CHAR_W              0x57
#define AML_NAME_CHAR_X              0x58
#define AML_NAME_CHAR_Y              0x59
#define AML_NAME_CHAR_Z              0x5a
#define AML_ROOT_CHAR                0x5c
#define AML_PARENT_PREFIX_CHAR       0x5e
#define AML_NAME_CHAR__              0x5f
#define AML_LOCAL0                   0x60
#define AML_LOCAL1                   0x61
#define AML_LOCAL2                   0x62
#define AML_LOCAL3                   0x63
#define AML_LOCAL4                   0x64
#define AML_LOCAL5                   0x65
#define AML_LOCAL6                   0x66
#define AML_LOCAL7                   0x67
#define AML_ARG0                     0x68
#define AML_ARG1                     0x69
#define AML_ARG2                     0x6a
#define AML_ARG3                     0x6b
#define AML_ARG4                     0x6c
#define AML_ARG5                     0x6d
#define AML_ARG6                     0x6e
#define AML_STORE_OP                 0x70
#define AML_REF_OF_OP                0x71
#define AML_ADD_OP                   0x72
#define AML_CONCAT_OP                0x73
#define AML_SUBTRACT_OP              0x74
#define AML_INCREMENT_OP             0x75
#define AML_DECREMENT_OP             0x76
#define AML_MULTIPLY_OP              0x77
#define AML_DIVIDE_OP                0x78
#define AML_SHIFT_LEFT_OP            0x79
#define AML_SHIFT_RIGHT_OP           0x7a
#define AML_AND_OP                   0x7b
#define AML_NAND_OP                  0x7c
#define AML_OR_OP                    0x7d
#define AML_NOR_OP                   0x7e
#define AML_XOR_OP                   0x7f
#define AML_NOT_OP                   0x80
#define AML_FIND_SET_LEFT_BIT_OP     0x81
#define AML_FIND_SET_RIGHT_BIT_OP    0x82
#define AML_DEREF_OF_OP              0x83
#define AML_CONCAT_RES_OP            0x84
#define AML_MOD_OP                   0x85
#define AML_NOTIFY_OP                0x86
#define AML_SIZE_OF_OP               0x87
#define AML_INDEX_OP                 0x88
#define AML_MATCH_OP                 0x89
#define AML_CREATE_DWORD_FIELD_OP    0x8a
#define AML_CREATE_WORD_FIELD_OP     0x8b
#define AML_CREATE_BYTE_FIELD_OP     0x8c
#define AML_CREATE_BIT_FIELD_OP      0x8d
#define AML_OBJECT_TYPE_OP           0x8e
#define AML_CREATE_QWORD_FIELD_OP    0x8f
#define AML_LAND_OP                  0x90
#define AML_LOR_OP                   0x91
#define AML_LNOT_OP                  0x92
#define AML_LEQUAL_OP                0x93
#define AML_LGREATER_OP              0x94
#define AML_LLESS_OP                 0x95
#define AML_TO_BUFFER_OP             0x96
#define AML_TO_DEC_STRING_OP         0x97
#define AML_TO_HEX_STRING_OP         0x98
#define AML_TO_INTEGER_OP            0x99
#define AML_TO_STRING_OP             0x9c
#define AML_COPY_OBJECT_OP           0x9d
#define AML_MID_OP                   0x9e
#define AML_CONTINUE_OP              0x9f
#define AML_IF_OP                    0xa0
#define AML_ELSE_OP                  0xa1
#define AML_WHILE_OP                 0xa2
#define AML_NOOP_OP                  0xa3
#define AML_RETURN_OP                0xa4
#define AML_BREAK_OP                 0xa5
#define AML_BREAK_POINT_OP           0xcc
#define AML_ONES_OP                  0xff

//
// Extended OpCode
//
#define AML_EXT_OP                   0x5b

#define AML_EXT_MUTEX_OP             0x01
#define AML_EXT_EVENT_OP             0x02
#define AML_EXT_COND_REF_OF_OP       0x12
#define AML_EXT_CREATE_FIELD_OP      0x13
#define AML_EXT_LOAD_TABLE_OP        0x1f
#define AML_EXT_LOAD_OP              0x20
#define AML_EXT_STALL_OP             0x21
#define AML_EXT_SLEEP_OP             0x22
#define AML_EXT_ACQUIRE_OP           0x23
#define AML_EXT_SIGNAL_OP            0x24
#define AML_EXT_WAIT_OP              0x25
#define AML_EXT_RESET_OP             0x26
#define AML_EXT_RELEASE_OP           0x27
#define AML_EXT_FROM_BCD_OP          0x28
#define AML_EXT_TO_BCD_OP            0x29
#define AML_EXT_UNLOAD_OP            0x2a
#define AML_EXT_REVISION_OP          0x30
#define AML_EXT_DEBUG_OP             0x31
#define AML_EXT_FATAL_OP             0x32
#define AML_EXT_TIMER_OP             0x33
#define AML_EXT_REGION_OP            0x80
#define AML_EXT_FIELD_OP             0x81
#define AML_EXT_DEVICE_OP            0x82
#define AML_EXT_PROCESSOR_OP         0x83
#define AML_EXT_POWER_RES_OP         0x84
#define AML_EXT_THERMAL_ZONE_OP      0x85
#define AML_EXT_INDEX_FIELD_OP       0x86
#define AML_EXT_BANK_FIELD_OP        0x87
#define AML_EXT_DATA_REGION_OP       0x88

#pragma pack(1)

typedef struct {
  UINT32    BridgeAddress;
  UINT32    DeviceAddress;
  UINT8     INTA[2];    // the first member record the 8259 link, the second member record the io apic irq number
  UINT8     INTB[2];
  UINT8     INTC[2];
  UINT8     INTD[2];

  UINT8     GPEPin;
  UINT8     SxNum;
} PCI_DEVICE_INFO;

#pragma pack()

#define PCI_DEVICE_INFO_MAX_NUM  50
#define CURRENT_PCI_DEVICE_NUM   13

#define  PIRQ_LINKA       1
#define  PIRQ_LINKB       2
#define  PIRQ_LINKC       3
#define  PIRQ_LINKD       4
#define  PIRQ_LINKE       5
#define  PIRQ_LINKF       6
#define  PIRQ_LINKG       7
#define  PIRQ_LINKH       8
#define  PIRQ_INVALID  0xFF

typedef struct _PCI_DEVICE_SETTING{
  UINT8    PciDeviceInfoNumber;
  PCI_DEVICE_INFO PciDeviceInfo[PCI_DEVICE_INFO_MAX_NUM];
}PCI_DEVICE_SETTING;

typedef struct _AML_BYTE_ENCODING AML_BYTE_ENCODING;

//
// AML Handle Entry definition.
//
//  Signature must be set to EFI_AML_HANDLE_SIGNATURE or EFI_AML_ROOT_HANDLE_SIGNATURE
//  Buffer is the ACPI node buffer pointer, the first/second bytes are opcode.
//         This buffer should not be freed.
//  Size is the total size of this ACPI node buffer.
//
typedef struct {
  UINT32                  Signature;
  UINT8                   *Buffer;
  UINTN                   Size;
  AML_BYTE_ENCODING       *AmlByteEncoding;
  BOOLEAN                 Modified;
} EFI_AML_HANDLE;

typedef UINT32 AML_OP_PARSE_INDEX;

typedef UINT32 AML_OP_PARSE_FORMAT;

typedef UINT32 AML_OP_ATTRIBUTE;

struct _AML_BYTE_ENCODING {
  UINT8                      OpCode;
  UINT8                      SubOpCode;
  AML_OP_PARSE_INDEX         MaxIndex;
  AML_OP_PARSE_FORMAT        Format[6];
  AML_OP_ATTRIBUTE           Attribute;
};


//
// Check device info fucntion prototype
//
typedef
BOOLEAN
(* CHECK_HANDLE_INFO) (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        CheckHandle,
  IN VOID                   *Context
  );

extern EFI_ACPI_HANDLE mDsdtHandle;
extern EFI_ACPI_SDT_PROTOCOL *mAcpiSdt;

/**
  Init Pci Device Structure

  @param mConfigData    - Pointer of Pci Device information Structure

**/
VOID
InitPciDeviceInfoStructure (
  PCI_DEVICE_SETTING            *mConfigData
  );
/**
  update pci routing information in acpi table based on pcd settings

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param DsdtHandle       ACPI root handle
  @param PciDeviceInfo    Pointer to PCI_DEVICE_INFO

**/
EFI_STATUS
SdtUpdatePciRouting (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        DsdtHandle,
  IN PCI_DEVICE_INFO        *PciDeviceInfo
  );


/**
  update power resource wake up information in acpi table based on pcd settings

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param DsdtHandle       ACPI root handle
  @param PciDeviceInfo    Pointer to PCI_DEVICE_INFO

**/
EFI_STATUS
SdtUpdatePowerWake (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        DsdtHandle,
  IN PCI_DEVICE_INFO        *PciDeviceInfo
  );

/**
  Get the root bridge handle by scanning the acpi table

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param DsdtHandle       ACPI root handle

  @retval EFI_ACPI_HANDLE the handle of the root bridge
**/
EFI_ACPI_HANDLE
SdtGetRootBridgeHandle (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        DsdtHandle
  );

/**
  Check input Pci device info is changed from the default values
  @param PciDeviceInfo    Pointer to PCI_DEVICE_INFO
  @param UpdatePRT        Pointer to BOOLEAN
  @param UpdatePRW        Pointer to BOOLEAN

**/
VOID
SdtCheckPciDeviceInfoChanged (
  IN PCI_DEVICE_INFO        *PciDeviceInfo,
  IN BOOLEAN                *UpdatePRT,
  IN BOOLEAN                *UpdatePRW
  );
#endif
