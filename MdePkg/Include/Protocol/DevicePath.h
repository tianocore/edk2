/** @file
  The device path protocol as defined in EFI 1.0.

  The device path represents a programatic path to a device. It's the view
  from a software point of view. It also must persist from boot to boot, so 
  it can not contain things like PCI bus numbers that change from boot to boot.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  DevicePath.h

**/

#ifndef __EFI_DEVICE_PATH_PROTOCOL_H__
#define __EFI_DEVICE_PATH_PROTOCOL_H__

//
// Device Path protocol
//
#define EFI_DEVICE_PATH_PROTOCOL_GUID \
  { \
    0x9576e91, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

//
// Protocol GUID defined in EFI1.1.
// 

//
// Device Path information
//
#define DEVICE_PATH_PROTOCOL  EFI_DEVICE_PATH_PROTOCOL_GUID

#pragma pack(1)

typedef struct {
  UINT8 Type;
  UINT8 SubType;
  UINT8 Length[2];
} EFI_DEVICE_PATH_PROTOCOL;

//
// For backward-compatible with EFI1.1.
// 
typedef EFI_DEVICE_PATH_PROTOCOL  EFI_DEVICE_PATH;

//
// Hardware Device Paths
//
#define HARDWARE_DEVICE_PATH      0x01

#define HW_PCI_DP                 0x01
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT8                           Function;
  UINT8                           Device;
} PCI_DEVICE_PATH;

#define HW_PCCARD_DP              0x02
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT8                           FunctionNumber;
} PCCARD_DEVICE_PATH;

#define HW_MEMMAP_DP              0x03
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT32                          MemoryType;
  EFI_PHYSICAL_ADDRESS            StartingAddress;
  EFI_PHYSICAL_ADDRESS            EndingAddress;
} MEMMAP_DEVICE_PATH;

#define HW_VENDOR_DP              0x04
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  EFI_GUID                        Guid;
} VENDOR_DEVICE_PATH;

#define HW_CONTROLLER_DP          0x05
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT32                          ControllerNumber;
} CONTROLLER_DEVICE_PATH;

//
// ACPI Device Paths
//
#define ACPI_DEVICE_PATH          0x02

#define ACPI_DP                   0x01
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT32                          HID;
  UINT32                          UID;
} ACPI_HID_DEVICE_PATH;

#define ACPI_EXTENDED_DP          0x02
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT32                          HID;
  UINT32                          UID;
  UINT32                          CID;
  //
  // Optional variable length _HIDSTR
  // Optional variable length _UIDSTR
  //
} ACPI_EXTENDED_HID_DEVICE_PATH;

//
//  EISA ID Macro
//  EISA ID Definition 32-bits
//   bits[15:0] - three character compressed ASCII EISA ID.
//   bits[31:16] - binary number
//    Compressed ASCII is 5 bits per character 0b00001 = 'A' 0b11010 = 'Z'
//
#define PNP_EISA_ID_CONST         0x41d0
#define EISA_ID(_Name, _Num)      ((UINT32) ((_Name) | (_Num) << 16))
#define EISA_PNP_ID(_PNPId)       (EISA_ID(PNP_EISA_ID_CONST, (_PNPId)))
#define EFI_PNP_ID(_PNPId)        (EISA_ID(PNP_EISA_ID_CONST, (_PNPId)))

#define PNP_EISA_ID_MASK          0xffff
#define EISA_ID_TO_NUM(_Id)       ((_Id) >> 16)


#define ACPI_ADR_DP               0x03
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT32                          ADR;
} ACPI_ADR_DEVICE_PATH;


//
// Messaging Device Paths
//
#define MESSAGING_DEVICE_PATH     0x03

#define MSG_ATAPI_DP              0x01
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT8                           PrimarySecondary;
  UINT8                           SlaveMaster;
  UINT16                          Lun;
} ATAPI_DEVICE_PATH;

#define MSG_SCSI_DP               0x02
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT16                          Pun;
  UINT16                          Lun;
} SCSI_DEVICE_PATH;

#define MSG_FIBRECHANNEL_DP       0x03
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT32                          Reserved;
  UINT64                          WWN;
  UINT64                          Lun;
} FIBRECHANNEL_DEVICE_PATH;

#define MSG_1394_DP               0x04
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT32                          Reserved;
  UINT64                          Guid;
} F1394_DEVICE_PATH;

#define MSG_USB_DP                0x05
typedef struct {
    EFI_DEVICE_PATH_PROTOCOL      Header;
    UINT8                         ParentPortNumber;
    UINT8                         InterfaceNumber;
} USB_DEVICE_PATH;

#define MSG_USB_CLASS_DP          0x0f
typedef struct {
    EFI_DEVICE_PATH_PROTOCOL      Header;
    UINT16                        VendorId;
    UINT16                        ProductId;
    UINT8                         DeviceClass;
    UINT8                         DeviceSubClass;
    UINT8                         DeviceProtocol;
} USB_CLASS_DEVICE_PATH;

#define MSG_USB_WWID_DP           0x10
typedef struct {
    EFI_DEVICE_PATH_PROTOCOL      Header;
    UINT16                        InterfaceNumber;
    UINT16                        VendorId;
    UINT16                        ProductId;
    // CHAR16                     SerialNumber[...];
} USB_WWID_DEVICE_PATH;


#define MSG_DEVICE_LOGICAL_UNIT_DP  0x11
typedef struct {
    EFI_DEVICE_PATH_PROTOCOL      Header;
    UINT8                         Lun;
} DEVICE_LOGICAL_UNIT_DEVICE_PATH;

#define MSG_SATA_DP               0x12
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT16                          HbaPortNumber;
  UINT16                          PortMultiplierPort;
  UINT16                          LogicalUnitNumber;
} SATA_DEVICE_PATH;

#define MSG_I2O_DP                0x06
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT32                          Tid;
} I2O_DEVICE_PATH;

#define MSG_MAC_ADDR_DP           0x0b
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  EFI_MAC_ADDRESS                 MacAddress;
  UINT8                           IfType;
} MAC_ADDR_DEVICE_PATH;

#define MSG_IPv4_DP               0x0c
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  EFI_IPv4_ADDRESS                LocalIpAddress;
  EFI_IPv4_ADDRESS                RemoteIpAddress;
  UINT16                          LocalPort;
  UINT16                          RemotePort;
  UINT16                          Protocol;
  BOOLEAN                         StaticIpAddress;
} IPv4_DEVICE_PATH;

#define MSG_IPv6_DP               0x0d
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  EFI_IPv6_ADDRESS                LocalIpAddress;
  EFI_IPv6_ADDRESS                RemoteIpAddress;
  UINT16                          LocalPort;
  UINT16                          RemotePort;
  UINT16                          Protocol;
  BOOLEAN                         StaticIpAddress;
} IPv6_DEVICE_PATH;

#define MSG_INFINIBAND_DP         0x09
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT32                          ResourceFlags;
  UINT8                           PortGid[16];
  UINT64                          ServiceId;
  UINT64                          TargetPortId;
  UINT64                          DeviceId;
} INFINIBAND_DEVICE_PATH;

#define INFINIBAND_RESOURCE_FLAG_IOC_SERVICE                0x01
#define INFINIBAND_RESOURCE_FLAG_EXTENDED_BOOT_ENVIRONMENT  0x02
#define INFINIBAND_RESOURCE_FLAG_CONSOLE_PROTOCOL           0x04
#define INFINIBAND_RESOURCE_FLAG_STORAGE_PROTOCOL           0x08
#define INFINIBAND_RESOURCE_FLAG_NETWORK_PROTOCOL           0x10

#define MSG_UART_DP               0x0e
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT32                          Reserved;
  UINT64                          BaudRate;
  UINT8                           DataBits;
  UINT8                           Parity;
  UINT8                           StopBits;
} UART_DEVICE_PATH;

//
// Use VENDOR_DEVICE_PATH struct
//
#define MSG_VENDOR_DP             0x0a
typedef VENDOR_DEVICE_PATH        VENDOR_DEFINED_DEVICE_PATH;

#define DEVICE_PATH_MESSAGING_PC_ANSI     EFI_PC_ANSI_GUID
#define DEVICE_PATH_MESSAGING_VT_100      EFI_VT_100_GUID
#define DEVICE_PATH_MESSAGING_VT_100_PLUS EFI_VT_100_PLUS_GUID
#define DEVICE_PATH_MESSAGING_VT_UTF8     EFI_VT_UTF8_GUID

#define DEVICE_PATH_MESSAGING_UART_FLOW_CONTROL   EFI_UART_DEVICE_PATH_GUID

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  EFI_GUID                        Guid;
  UINT32                          FlowControlMap;
} UART_FLOW_CONTROL_DEVICE_PATH;

#define DEVICE_PATH_MESSAGING_SAS                 EFI_SAS_DEVICE_PATH_GUID

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  EFI_GUID                        Guid;
  UINT32                          Reserved;
  UINT64                          SasAddress;
  UINT64                          Lun;
  UINT16                          DeviceTopology;
  UINT16                          RelativeTargetPort;
} SAS_DEVICE_PATH;

#define MSG_ISCSI_DP              0x13
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT16                          NetworkProtocol;
  UINT16                          LoginOption;
  UINT16                          Reserved;
  UINT16                          TargetPortalGroupTag;
  UINT64                          LUN;
  // CHAR8                        iSCSI Target Name
} ISCSI_DEVICE_PATH;

#define ISCSI_LOGIN_OPTION_NO_HEADER_DIGEST             0x0000
#define ISCSI_LOGIN_OPTION_HEADER_DIGEST_USING_CRC32C   0x0002
#define ISCSI_LOGIN_OPTION_NO_DATA_DIGEST               0x0000
#define ISCSI_LOGIN_OPTION_DATA_DIGEST_USING_CRC32C     0x0008
#define ISCSI_LOGIN_OPTION_AUTHMETHOD_CHAP              0x0000
#define ISCSI_LOGIN_OPTION_AUTHMETHOD_NON               0x1000
#define ISCSI_LOGIN_OPTION_CHAP_BI                      0x0000
#define ISCSI_LOGIN_OPTION_CHAP_UNI                     0x2000

//
// Media Device Path
//
#define MEDIA_DEVICE_PATH         0x04

#define MEDIA_HARDDRIVE_DP        0x01
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT32                          PartitionNumber;
  UINT64                          PartitionStart;
  UINT64                          PartitionSize;
  UINT8                           Signature[16];
  UINT8                           MBRType;
  UINT8                           SignatureType;
} HARDDRIVE_DEVICE_PATH;

#define MBR_TYPE_PCAT             0x01
#define MBR_TYPE_EFI_PARTITION_TABLE_HEADER 0x02

#define SIGNATURE_TYPE_MBR        0x01
#define SIGNATURE_TYPE_GUID       0x02

#define MEDIA_CDROM_DP            0x02
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT32                          BootEntry;
  UINT64                          PartitionStart;
  UINT64                          PartitionSize;
} CDROM_DEVICE_PATH;

//
// Use VENDOR_DEVICE_PATH struct
//
#define MEDIA_VENDOR_DP           0x03

#define MEDIA_FILEPATH_DP         0x04
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  CHAR16                          PathName[1];
} FILEPATH_DEVICE_PATH;

#define SIZE_OF_FILEPATH_DEVICE_PATH EFI_FIELD_OFFSET(FILEPATH_DEVICE_PATH,PathName)

#define MEDIA_PROTOCOL_DP         0x05
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  EFI_GUID                        Protocol;
} MEDIA_PROTOCOL_DEVICE_PATH;


#define MEDIA_PIWG_FW_VOL_DP      0x6
typedef MEDIA_PROTOCOL_DEVICE_PATH MEDIA_FW_VOL_FILEPATH_DEVICE_PATH;


#define MEDIA_PIWG_FW_FILE_DP     0x7
typedef MEDIA_PROTOCOL_DEVICE_PATH MEDIA_FW_VOL_DEVICE_PATH;

//
// BBS Device Path
//
#define BBS_DEVICE_PATH           0x05
#define BBS_BBS_DP                0x01
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL        Header;
  UINT16                          DeviceType;
  UINT16                          StatusFlag;
  CHAR8                           String[1];
} BBS_BBS_DEVICE_PATH;

//
// DeviceType definitions - from BBS specification
//
#define BBS_TYPE_FLOPPY           0x01
#define BBS_TYPE_HARDDRIVE        0x02
#define BBS_TYPE_CDROM            0x03
#define BBS_TYPE_PCMCIA           0x04
#define BBS_TYPE_USB              0x05
#define BBS_TYPE_EMBEDDED_NETWORK 0x06
#define BBS_TYPE_BEV              0x80
#define BBS_TYPE_UNKNOWN          0xFF


//
// Union of all possible Device Paths and pointers to Device Paths
//

typedef union {
  EFI_DEVICE_PATH_PROTOCOL             DevPath;
  PCI_DEVICE_PATH                      Pci;
  PCCARD_DEVICE_PATH                   PcCard;
  MEMMAP_DEVICE_PATH                   MemMap;
  VENDOR_DEVICE_PATH                   Vendor;

  CONTROLLER_DEVICE_PATH               Controller;
  ACPI_HID_DEVICE_PATH                 Acpi;

  ATAPI_DEVICE_PATH                    Atapi;
  SCSI_DEVICE_PATH                     Scsi;
  FIBRECHANNEL_DEVICE_PATH             FibreChannel;

  F1394_DEVICE_PATH                    F1394;
  USB_DEVICE_PATH                      Usb;
  SATA_DEVICE_PATH                     Sata;
  USB_CLASS_DEVICE_PATH                UsbClass;
  I2O_DEVICE_PATH                      I2O;
  MAC_ADDR_DEVICE_PATH                 MacAddr;
  IPv4_DEVICE_PATH                     Ipv4;
  IPv6_DEVICE_PATH                     Ipv6;
  INFINIBAND_DEVICE_PATH               InfiniBand;
  UART_DEVICE_PATH                     Uart;

  HARDDRIVE_DEVICE_PATH                HardDrive;
  CDROM_DEVICE_PATH                    CD;

  FILEPATH_DEVICE_PATH                 FilePath;
  MEDIA_PROTOCOL_DEVICE_PATH           MediaProtocol;

  BBS_BBS_DEVICE_PATH                  Bbs;
} EFI_DEV_PATH;



typedef union {
  EFI_DEVICE_PATH_PROTOCOL             *DevPath;
  PCI_DEVICE_PATH                      *Pci;
  PCCARD_DEVICE_PATH                   *PcCard;
  MEMMAP_DEVICE_PATH                   *MemMap;
  VENDOR_DEVICE_PATH                   *Vendor;

  CONTROLLER_DEVICE_PATH               *Controller;
  ACPI_HID_DEVICE_PATH                 *Acpi;
  ACPI_EXTENDED_HID_DEVICE_PATH        *ExtendedAcpi;

  ATAPI_DEVICE_PATH                    *Atapi;
  SCSI_DEVICE_PATH                     *Scsi;
  FIBRECHANNEL_DEVICE_PATH             *FibreChannel;

  F1394_DEVICE_PATH                    *F1394;
  USB_DEVICE_PATH                      *Usb;
  SATA_DEVICE_PATH                     *Sata;
  USB_CLASS_DEVICE_PATH                *UsbClass;
  I2O_DEVICE_PATH                      *I2O;
  MAC_ADDR_DEVICE_PATH                 *MacAddr;
  IPv4_DEVICE_PATH                     *Ipv4;
  IPv6_DEVICE_PATH                     *Ipv6;
  INFINIBAND_DEVICE_PATH               *InfiniBand;
  UART_DEVICE_PATH                     *Uart;

  HARDDRIVE_DEVICE_PATH                *HardDrive;
  CDROM_DEVICE_PATH                    *CD;

  FILEPATH_DEVICE_PATH                 *FilePath;
  MEDIA_PROTOCOL_DEVICE_PATH           *MediaProtocol;

  BBS_BBS_DEVICE_PATH                  *Bbs;
  UINT8                                *Raw;
} EFI_DEV_PATH_PTR;

#pragma pack()
                                             
#define EFI_DP_TYPE_MASK                     0x7F
#define EFI_DP_TYPE_UNPACKED                 0x80
#define END_DEVICE_PATH_TYPE                 0x7f
                                             
#define EFI_END_ENTIRE_DEVICE_PATH           0xff
#define EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE   0xff
#define EFI_END_INSTANCE_DEVICE_PATH         0x01
#define END_ENTIRE_DEVICE_PATH_SUBTYPE       EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE
#define END_INSTANCE_DEVICE_PATH_SUBTYPE     EFI_END_INSTANCE_DEVICE_PATH
                                             
#define EFI_END_DEVICE_PATH_LENGTH           (sizeof (EFI_DEVICE_PATH_PROTOCOL))
#define END_DEVICE_PATH_LENGTH               EFI_END_DEVICE_PATH_LENGTH
                                             
#define DP_IS_END_TYPE(a)                    
#define DP_IS_END_SUBTYPE(a)                 (((a)->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE)
#define DevicePathSubType(a)                 ((a)->SubType)
#define IsDevicePathUnpacked(a)              ((a)->Type & EFI_DP_TYPE_UNPACKED)
                                             
#define EfiDevicePathNodeLength(a)           (((a)->Length[0]) | ((a)->Length[1] << 8))
#define DevicePathNodeLength(a)              (EfiDevicePathNodeLength(a))
#define EfiNextDevicePathNode(a)             ((EFI_DEVICE_PATH_PROTOCOL *) (((UINT8 *) (a)) + EfiDevicePathNodeLength (a)))
#define NextDevicePathNode(a)                (EfiNextDevicePathNode(a)) 
                                             
#define EfiDevicePathType(a)                 (((a)->Type) & EFI_DP_TYPE_MASK)
#define DevicePathType(a)                    (EfiDevicePathType(a))
#define EfiIsDevicePathEndType(a)            (EfiDevicePathType (a) == END_DEVICE_PATH_TYPE)
#define IsDevicePathEndType(a)               (EfiIsDevicePathEndType(a)) 
                                             
                                             
#define EfiIsDevicePathEndSubType(a)         ((a)->SubType == EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE)
#define IsDevicePathEndSubType(a)            (EfiIsDevicePathEndSubType(a))
#define EfiIsDevicePathEndInstanceSubType(a) ((a)->SubType == EFI_END_INSTANCE_DEVICE_PATH)
                                             
#define EfiIsDevicePathEnd(a)                (EfiIsDevicePathEndType (a) && EfiIsDevicePathEndSubType (a))
#define IsDevicePathEnd(a)                   (EfiIsDevicePathEnd(a))
#define EfiIsDevicePathEndInstance(a)        (EfiIsDevicePathEndType (a) && EfiIsDevicePathEndInstanceSubType (a))


#define SetDevicePathNodeLength(a,l) {                           \
          (a)->Length[0] = (UINT8) (l);                          \
          (a)->Length[1] = (UINT8) ((l) >> 8);                   \
          }

#define SetDevicePathEndNode(a)  {                               \
          (a)->Type = END_DEVICE_PATH_TYPE;                      \
          (a)->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;         \
          (a)->Length[0] = sizeof(EFI_DEVICE_PATH_PROTOCOL);     \
          (a)->Length[1] = 0;                                    \
          }

extern EFI_GUID gEfiDevicePathProtocolGuid;

#endif
