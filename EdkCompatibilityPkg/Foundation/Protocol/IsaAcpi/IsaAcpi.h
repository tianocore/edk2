/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    IsaAcpi.h
    
Abstract:

    EFI ISA Acpi Protocol

Revision History

--*/

#ifndef _ISA_ACPI_H_
#define _ISA_ACPI_H_

#define EFI_ISA_ACPI_PROTOCOL_GUID \
        {0x64a892dc, 0x5561, 0x4536, {0x92, 0xc7, 0x79, 0x9b, 0xfc, 0x18, 0x33, 0x55}}

EFI_FORWARD_DECLARATION (EFI_ISA_ACPI);

//
// Resource Attribute definition
//
#define EFI_ISA_ACPI_IRQ_TYPE_HIGH_TRUE_EDGE_SENSITIVE   0x01
#define EFI_ISA_ACPI_IRQ_TYPE_LOW_TRUE_EDGE_SENSITIVE    0x02
#define EFI_ISA_ACPI_IRQ_TYPE_HIGH_TRUE_LEVEL_SENSITIVE  0x04
#define EFI_ISA_ACPI_IRQ_TYPE_LOW_TRUE_LEVEL_SENSITIVE   0x08

#define EFI_ISA_ACPI_DMA_SPEED_TYPE_MASK                 0x03

#define EFI_ISA_ACPI_DMA_SPEED_TYPE_COMPATIBILITY        0x00
#define EFI_ISA_ACPI_DMA_SPEED_TYPE_A                    0x01
#define EFI_ISA_ACPI_DMA_SPEED_TYPE_B                    0x02
#define EFI_ISA_ACPI_DMA_SPEED_TYPE_F                    0x03
#define EFI_ISA_ACPI_DMA_COUNT_BY_BYTE                   0x04
#define EFI_ISA_ACPI_DMA_COUNT_BY_WORD                   0x08
#define EFI_ISA_ACPI_DMA_BUS_MASTER                      0x10
#define EFI_ISA_ACPI_DMA_TRANSFER_TYPE_8_BIT             0x20
#define EFI_ISA_ACPI_DMA_TRANSFER_TYPE_8_BIT_AND_16_BIT  0x40
#define EFI_ISA_ACPI_DMA_TRANSFER_TYPE_16_BIT            0x80

#define EFI_ISA_ACPI_MEMORY_WIDTH_MASK                   0x03

#define EFI_ISA_ACPI_MEMORY_WIDTH_8_BIT                  0x00
#define EFI_ISA_ACPI_MEMORY_WIDTH_16_BIT                 0x01
#define EFI_ISA_ACPI_MEMORY_WIDTH_8_BIT_AND_16_BIT       0x02
#define EFI_ISA_ACPI_MEMORY_WRITEABLE                    0x04
#define EFI_ISA_ACPI_MEMORY_CACHEABLE                    0x08
#define EFI_ISA_ACPI_MEMORY_SHADOWABLE                   0x10
#define EFI_ISA_ACPI_MEMORY_EXPANSION_ROM                0x20

#define EFI_ISA_ACPI_IO_DECODE_10_BITS                   0x01
#define EFI_ISA_ACPI_IO_DECODE_16_BITS                   0x02

//
// Resource List definition: 
// at first, the resource was defined as below
// but in the future, it will be defined again that follow ACPI spec: ACPI resource type
// so that, in this driver, we can interpret the ACPI table and get the ISA device information. 
//
 
typedef enum {
  EfiIsaAcpiResourceEndOfList,
  EfiIsaAcpiResourceIo,
  EfiIsaAcpiResourceMemory,
  EfiIsaAcpiResourceDma,
  EfiIsaAcpiResourceInterrupt
} EFI_ISA_ACPI_RESOURCE_TYPE;

typedef struct {
  EFI_ISA_ACPI_RESOURCE_TYPE  Type;
  UINT32                      Attribute;
  UINT32                      StartRange;
  UINT32                      EndRange;
} EFI_ISA_ACPI_RESOURCE;

typedef struct {
  UINT32                      HID;
  UINT32                      UID;
} EFI_ISA_ACPI_DEVICE_ID;

typedef struct {
  EFI_ISA_ACPI_DEVICE_ID      Device;
  EFI_ISA_ACPI_RESOURCE       *ResourceItem;
} EFI_ISA_ACPI_RESOURCE_LIST;

//
// Prototypes for the ISA ACPI Protocol
//
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_DEVICE_ENUMERATE) (
  IN EFI_ISA_ACPI            *This,
  OUT EFI_ISA_ACPI_DEVICE_ID         **Device
  );
  
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_SET_DEVICE_POWER) (
  IN EFI_ISA_ACPI            *This,
  IN EFI_ISA_ACPI_DEVICE_ID          *Device,
  IN BOOLEAN                         OnOff
  );
  
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_GET_CUR_RESOURCE) (
  IN EFI_ISA_ACPI            *This,
  IN EFI_ISA_ACPI_DEVICE_ID          *Device,
  OUT EFI_ISA_ACPI_RESOURCE_LIST     **ResourceList
  );

typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_GET_POS_RESOURCE) (
  IN EFI_ISA_ACPI            *This,
  IN EFI_ISA_ACPI_DEVICE_ID          *Device,
  OUT EFI_ISA_ACPI_RESOURCE_LIST     **ResourceList
  );
  
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_SET_RESOURCE) (
  IN EFI_ISA_ACPI            *This,
  IN EFI_ISA_ACPI_DEVICE_ID          *Device,
  IN EFI_ISA_ACPI_RESOURCE_LIST      *ResourceList
  );    

typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_ENABLE_DEVICE) (
  IN EFI_ISA_ACPI            *This,
  IN EFI_ISA_ACPI_DEVICE_ID          *Device,
  IN BOOLEAN                         Enable
  );    

typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_INIT_DEVICE) (
  IN EFI_ISA_ACPI            *This,
  IN EFI_ISA_ACPI_DEVICE_ID          *Device
  );  

typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_INTERFACE_INIT) (
  IN EFI_ISA_ACPI            *This
  );

//
// Interface structure for the ISA ACPI Protocol
//
typedef struct _EFI_ISA_ACPI {
  EFI_ISA_ACPI_DEVICE_ENUMERATE     DeviceEnumerate;
  EFI_ISA_ACPI_SET_DEVICE_POWER     SetPower;
  EFI_ISA_ACPI_GET_CUR_RESOURCE     GetCurResource;
  EFI_ISA_ACPI_GET_POS_RESOURCE     GetPosResource;
  EFI_ISA_ACPI_SET_RESOURCE         SetResource;
  EFI_ISA_ACPI_ENABLE_DEVICE        EnableDevice;
  EFI_ISA_ACPI_INIT_DEVICE          InitDevice;
  EFI_ISA_ACPI_INTERFACE_INIT       InterfaceInit;
} EFI_ISA_ACPI_PROTOCOL;

extern EFI_GUID gEfiIsaAcpiProtocolGuid;
  
#endif
