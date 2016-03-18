/** @file
  EFI ISA ACPI Protocol is used to enumerate and manage all the ISA controllers on
  the platform's ISA Bus.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ISA_ACPI_H_
#define __ISA_ACPI_H_

///
/// Global ID for the EFI ISA ACPI Protocol.
///
#define EFI_ISA_ACPI_PROTOCOL_GUID \
  { \
    0x64a892dc, 0x5561, 0x4536, { 0x92, 0xc7, 0x79, 0x9b, 0xfc, 0x18, 0x33, 0x55 } \
  }

///
/// Forward declaration fo the EFI ISA ACPI Protocol
///
typedef struct _EFI_ISA_ACPI_PROTOCOL EFI_ISA_ACPI_PROTOCOL;

///
/// ISA ACPI Protocol interrupt resource attributes.
///
#define EFI_ISA_ACPI_IRQ_TYPE_HIGH_TRUE_EDGE_SENSITIVE   0x01   ///< Edge triggered interrupt on a rising edge.
#define EFI_ISA_ACPI_IRQ_TYPE_LOW_TRUE_EDGE_SENSITIVE    0x02   ///< Edge triggered interrupt on a falling edge.
#define EFI_ISA_ACPI_IRQ_TYPE_HIGH_TRUE_LEVEL_SENSITIVE  0x04   ///< Level sensitive interrupt active high.
#define EFI_ISA_ACPI_IRQ_TYPE_LOW_TRUE_LEVEL_SENSITIVE   0x08   ///< Level sensitive interrupt active low.

///
/// ISA ACPI Protocol DMA resource attributes.
///
#define EFI_ISA_ACPI_DMA_SPEED_TYPE_MASK                 0x03   ///< Bit mask of supported DMA speed attributes.
#define EFI_ISA_ACPI_DMA_SPEED_TYPE_COMPATIBILITY        0x00   ///< ISA controller supports compatibility mode DMA transfers.
#define EFI_ISA_ACPI_DMA_SPEED_TYPE_A                    0x01   ///< ISA controller supports type A DMA transfers.
#define EFI_ISA_ACPI_DMA_SPEED_TYPE_B                    0x02   ///< ISA controller supports type B DMA transfers.
#define EFI_ISA_ACPI_DMA_SPEED_TYPE_F                    0x03   ///< ISA controller supports type F DMA transfers.
#define EFI_ISA_ACPI_DMA_COUNT_BY_BYTE                   0x04   ///< ISA controller increments DMA address by bytes (8-bit).
#define EFI_ISA_ACPI_DMA_COUNT_BY_WORD                   0x08   ///< ISA controller increments DMA address by words (16-bit).
#define EFI_ISA_ACPI_DMA_BUS_MASTER                      0x10   ///< ISA controller is a DMA bus master.
#define EFI_ISA_ACPI_DMA_TRANSFER_TYPE_8_BIT             0x20   ///< ISA controller only supports 8-bit DMA transfers.
#define EFI_ISA_ACPI_DMA_TRANSFER_TYPE_8_BIT_AND_16_BIT  0x40   ///< ISA controller both 8-bit and 16-bit DMA transfers.
#define EFI_ISA_ACPI_DMA_TRANSFER_TYPE_16_BIT            0x80   ///< ISA controller only supports 16-bit DMA transfers.

///
/// ISA ACPI Protocol MMIO resource attributes
///
#define EFI_ISA_ACPI_MEMORY_WIDTH_MASK                   0x03   ///< Bit mask of supported ISA memory width attributes.
#define EFI_ISA_ACPI_MEMORY_WIDTH_8_BIT                  0x00   ///< ISA MMIO region only supports 8-bit access.
#define EFI_ISA_ACPI_MEMORY_WIDTH_16_BIT                 0x01   ///< ISA MMIO region only supports 16-bit access.
#define EFI_ISA_ACPI_MEMORY_WIDTH_8_BIT_AND_16_BIT       0x02   ///< ISA MMIO region supports both 8-bit and 16-bit access.
#define EFI_ISA_ACPI_MEMORY_WRITEABLE                    0x04   ///< ISA MMIO region supports write transactions.
#define EFI_ISA_ACPI_MEMORY_CACHEABLE                    0x08   ///< ISA MMIO region supports being cached.
#define EFI_ISA_ACPI_MEMORY_SHADOWABLE                   0x10   ///< ISA MMIO region may be shadowed.
#define EFI_ISA_ACPI_MEMORY_EXPANSION_ROM                0x20   ///< ISA MMIO region is an expansion ROM.

///
/// ISA ACPI Protocol I/O resource attributes
///
#define EFI_ISA_ACPI_IO_DECODE_10_BITS                   0x01    ///< ISA controllers uses a 10-bit address decoder for I/O cycles.
#define EFI_ISA_ACPI_IO_DECODE_16_BITS                   0x02    ///< ISA controllers uses a 16-bit address decoder for I/O cycles.

///
/// EFI ISA ACPI resource type 
///
typedef enum {
  EfiIsaAcpiResourceEndOfList,    ///< Marks the end if a resource list.
  EfiIsaAcpiResourceIo,           ///< ISA I/O port resource range.
  EfiIsaAcpiResourceMemory,       ///< ISA MMIO resource range.
  EfiIsaAcpiResourceDma,          ///< ISA DMA resource. 
  EfiIsaAcpiResourceInterrupt     ///< ISA interrupt resource.
} EFI_ISA_ACPI_RESOURCE_TYPE;

///
/// EFI ISA ACPI generic resource structure
///
typedef struct {
  EFI_ISA_ACPI_RESOURCE_TYPE  Type;         ///< The type of resource (I/O, MMIO, DMA, Interrupt).
  UINT32                      Attribute;    ///< Bit mask of attributes associated with this resource.  See EFI_ISA_ACPI_xxx macros for valid combinations.
  UINT32                      StartRange;   ///< The start of the resource range.
  UINT32                      EndRange;     ///< The end of the resource range.
} EFI_ISA_ACPI_RESOURCE;

///
/// EFI ISA ACPI resource device identifier
///
typedef struct {
  UINT32  HID;   ///< The ACPI Hardware Identifier value associated with an ISA controller.  Matchs ACPI DSDT contents.
  UINT32  UID;   ///< The ACPI Unique Identifier value associated with an ISA controller.  Matches ACPI DSDT contents.
} EFI_ISA_ACPI_DEVICE_ID;

///
/// EFI ISA ACPI resource list
///
typedef struct {
  EFI_ISA_ACPI_DEVICE_ID  Device;          ///< The ACPI HID/UID associated with an ISA controller.
  EFI_ISA_ACPI_RESOURCE   *ResourceItem;   ///< A pointer to the list of resources associated with an ISA controller.
} EFI_ISA_ACPI_RESOURCE_LIST;

/**
  Enumerates the ISA controllers on an ISA bus.

  This service allows all the ISA controllers on an ISA bus to be enumerated.  If
  Device is a pointer to a NULL value, then the first ISA controller on the ISA
  bus is returned in Device and EFI_SUCCESS is returned.  If Device is a pointer 
  to a value that was returned on a prior call to DeviceEnumerate(), then the next 
  ISA controller on the ISA bus is returned in Device and EFI_SUCCESS is returned.
  If Device is a pointer to the last ISA controller on the ISA bus, then 
  EFI_NOT_FOUND is returned.

  @param[in]  This     The pointer to the EFI_ISA_ACPI_PROTOCOL instance.
  @param[out] Device   The pointer to an ISA controller named by ACPI HID/UID.

  @retval EFI_SUCCESS    The next ISA controller on the ISA bus was returned.
  @retval EFI_NOT_FOUND  No device found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_DEVICE_ENUMERATE)(
  IN  EFI_ISA_ACPI_PROTOCOL   *This,
  OUT EFI_ISA_ACPI_DEVICE_ID  **Device
  );

/**
  Sets the power state of an ISA controller.

  This services sets the power state of the ISA controller specified by Device to 
  the power state specified by OnOff.  TRUE denotes on, FALSE denotes off. 
  If the power state is sucessfully set on the ISA Controller, then
  EFI_SUCCESS is returned.

  @param[in] This     The pointer to the EFI_ISA_ACPI_PROTOCOL instance.
  @param[in] Device   The pointer to an ISA controller named by ACPI HID/UID.
  @param[in] OnOff    TRUE denotes on, FALSE denotes off.

  @retval EFI_SUCCESS   Successfully set the power state of the ISA controller.
  @retval Other         The ISA controller could not be placed in the requested power state.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_SET_DEVICE_POWER)(
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device,
  IN BOOLEAN                 OnOff
  );
  
/**
  Retrieves the current set of resources associated with an ISA controller.

  Retrieves the set of I/O, MMIO, DMA, and interrupt resources currently 
  assigned to the ISA controller specified by Device.  These resources
  are returned in ResourceList.

  @param[in]  This          The pointer to the EFI_ISA_ACPI_PROTOCOL instance.
  @param[in]  Device        The pointer to an ISA controller named by ACPI HID/UID.
  @param[out] ResourceList  The pointer to the current resource list for Device.

  @retval EFI_SUCCESS    Successfully retrieved the current resource list.
  @retval EFI_NOT_FOUND  The resource list could not be retrieved.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_GET_CUR_RESOURCE)(
  IN  EFI_ISA_ACPI_PROTOCOL       *This,
  IN  EFI_ISA_ACPI_DEVICE_ID      *Device,
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **ResourceList
  );

/**
  Retrieves the set of possible resources that may be assigned to an ISA controller
  with SetResource().

  Retrieves the possible sets of I/O, MMIO, DMA, and interrupt resources for the
  ISA controller specified by Device.  The sets are returned in ResourceList.

  @param[in]  This           The pointer to the EFI_ISA_ACPI_PROTOCOL instance.
  @param[in]  Device         The pointer to an ISA controller named by ACPI HID/UID.
  @param[out] ResourceList   The pointer to the returned list of resource lists.

  @retval EFI_UNSUPPORTED  This service is not supported.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_GET_POS_RESOURCE)(
  IN EFI_ISA_ACPI_PROTOCOL        *This,
  IN EFI_ISA_ACPI_DEVICE_ID       *Device,
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **ResourceList
  );

/**
  Assigns resources to an ISA controller.

  Assigns the I/O, MMIO, DMA, and interrupt resources specified by ResourceList
  to the ISA controller specified by Device.  ResourceList must match a resource list returned by GetPosResource() for the same ISA controller.

  @param[in] This           The pointer to the EFI_ISA_ACPI_PROTOCOL instance.
  @param[in] Device         The pointer to an ISA controller named by ACPI HID/UID.
  @param[in] ResourceList   The pointer to a resources list that must be one of the 
                            resource lists returned by GetPosResource() for the
                            ISA controller specified by Device.

  @retval EFI_SUCCESS  Successfully set resources on the ISA controller.
  @retval Other        The resources could not be set for the ISA controller.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_SET_RESOURCE)(
  IN EFI_ISA_ACPI_PROTOCOL       *This,
  IN EFI_ISA_ACPI_DEVICE_ID      *Device,
  IN EFI_ISA_ACPI_RESOURCE_LIST  *ResourceList
  );    

/**
  Enables or disables an ISA controller.

  @param[in] This     The pointer to the EFI_ISA_ACPI_PROTOCOL instance.
  @param[in] Device   The pointer to the ISA controller to enable/disable.
  @param[in] Enable   TRUE to enable the ISA controller.  FALSE to disable the
                      ISA controller.

  @retval EFI_SUCCESS   Successfully enabled/disabled the ISA controller.
  @retval Other         The ISA controller could not be placed in the requested state.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_ENABLE_DEVICE)(
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device,
  IN BOOLEAN                 Enable
  );    

/**
  Initializes an ISA controller, so that it can be used.  This service must be called
  before SetResource(), EnableDevice(), or SetPower() will behave as expected.

  @param[in] This     The pointer to the EFI_ISA_ACPI_PROTOCOL instance.
  @param[in] Device   The pointer to an ISA controller named by ACPI HID/UID.

  @retval EFI_SUCCESS   Successfully initialized an ISA controller.
  @retval Other         The ISA controller could not be initialized.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_INIT_DEVICE)(
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device
  );  

/**
  Initializes all the HW states required for the ISA controllers on the ISA bus 
  to be enumerated and managed by the rest of the services in this prorotol.
  This service must be called before any of the other services in this
  protocol will function as expected.
 
  @param[in] This  The pointer to the EFI_ISA_ACPI_PROTOCOL instance.

  @retval EFI_SUCCESS   Successfully initialized all required hardware states.
  @retval Other         The ISA interface could not be initialized.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_ACPI_INTERFACE_INIT)(
  IN EFI_ISA_ACPI_PROTOCOL  *This
  );

///
/// The EFI_ISA_ACPI_PROTOCOL provides the services to enumerate and manage
/// ISA controllers on an ISA bus.  These services include the ability to initialize, 
/// enable, disable, and manage the power state of ISA controllers.  It also 
/// includes services to query current resources, query possible resources, 
/// and assign resources to an ISA controller.
///
struct _EFI_ISA_ACPI_PROTOCOL {
  EFI_ISA_ACPI_DEVICE_ENUMERATE  DeviceEnumerate;
  EFI_ISA_ACPI_SET_DEVICE_POWER  SetPower;
  EFI_ISA_ACPI_GET_CUR_RESOURCE  GetCurResource;
  EFI_ISA_ACPI_GET_POS_RESOURCE  GetPosResource;
  EFI_ISA_ACPI_SET_RESOURCE      SetResource;
  EFI_ISA_ACPI_ENABLE_DEVICE     EnableDevice;
  EFI_ISA_ACPI_INIT_DEVICE       InitDevice;
  EFI_ISA_ACPI_INTERFACE_INIT    InterfaceInit;
};

extern EFI_GUID gEfiIsaAcpiProtocolGuid;
  
#endif
