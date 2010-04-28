/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UgaIo.h

Abstract:

  UGA IO protocol from the EFI 1.1 specification.

  Abstraction of a very simple graphics device.

--*/

#ifndef __UGA_IO_H__
#define __UGA_IO_H__

#define EFI_UGA_IO_PROTOCOL_GUID \
  { \
    0x61a4d49e, 0x6f68, 0x4f1b, {0xb9, 0x22, 0xa8, 0x6e, 0xed, 0xb, 0x7, 0xa2} \
  }

typedef struct _EFI_UGA_IO_PROTOCOL EFI_UGA_IO_PROTOCOL;

typedef UINT32                      UGA_STATUS;

typedef enum {
  UgaDtParentBus          = 1,
  UgaDtGraphicsController,
  UgaDtOutputController,
  UgaDtOutputPort,
  UgaDtOther
}
UGA_DEVICE_TYPE, *PUGA_DEVICE_TYPE;

typedef UINT32 UGA_DEVICE_ID, *PUGA_DEVICE_ID;

typedef struct {
  UGA_DEVICE_TYPE deviceType;
  UGA_DEVICE_ID   deviceId;
  UINT32          ui32DeviceContextSize;
  UINT32          ui32SharedContextSize;
}
UGA_DEVICE_DATA, *PUGA_DEVICE_DATA;

typedef struct _UGA_DEVICE {
  VOID                *pvDeviceContext;
  VOID                *pvSharedContext;
  VOID                *pvRunTimeContext;
  struct _UGA_DEVICE  *pParentDevice;
  VOID                *pvBusIoServices;
  VOID                *pvStdIoServices;
  UGA_DEVICE_DATA     deviceData;
}
UGA_DEVICE, *PUGA_DEVICE;

#ifndef UGA_IO_REQUEST_CODE
//
// Prevent conflicts with UGA typedefs.
//
typedef enum {
  UgaIoGetVersion             = 1,
  UgaIoGetChildDevice,
  UgaIoStartDevice,
  UgaIoStopDevice,
  UgaIoFlushDevice,
  UgaIoResetDevice,
  UgaIoGetDeviceState,
  UgaIoSetDeviceState,
  UgaIoSetPowerState,
  UgaIoGetMemoryConfiguration,
  UgaIoSetVideoMode,
  UgaIoCopyRectangle,
  UgaIoGetEdidSegment,
  UgaIoDeviceChannelOpen,
  UgaIoDeviceChannelClose,
  UgaIoDeviceChannelRead,
  UgaIoDeviceChannelWrite,
  UgaIoGetPersistentDataSize,
  UgaIoGetPersistentData,
  UgaIoSetPersistentData,
  UgaIoGetDevicePropertySize,
  UgaIoGetDeviceProperty,
  UgaIoBtPrivateInterface
}
UGA_IO_REQUEST_CODE, *PUGA_IO_REQUEST_CODE;

#endif

typedef struct {
  IN UGA_IO_REQUEST_CODE  ioRequestCode;
  IN VOID                 *pvInBuffer;
  IN UINT64               ui64InBufferSize;
  OUT VOID                *pvOutBuffer;
  IN UINT64               ui64OutBufferSize;
  OUT UINT64              ui64BytesReturned;
}
UGA_IO_REQUEST, *PUGA_IO_REQUEST;

typedef
EFI_STATUS
(EFIAPI *EFI_UGA_IO_PROTOCOL_CREATE_DEVICE) (
  IN  EFI_UGA_IO_PROTOCOL  * This,
  IN  UGA_DEVICE           * ParentDevice,
  IN  UGA_DEVICE_DATA      * DeviceData,
  IN  VOID                 *RunTimeContext,
  OUT UGA_DEVICE           **Device
  );

/*++

  Routine Description:

    Dynamically allocate storage for a child UGA_DEVICE .

  Arguments:

    This           - The EFI_UGA_IO_PROTOCOL instance. Type EFI_UGA_IO_PROTOCOL is 
                     defined in Section 10.7.

    ParentDevice   - ParentDevice specifies a pointer to the parent device of Device.

    DeviceData     - A pointer to UGA_DEVICE_DATA returned from a call to DispatchService()
                     with a UGA_DEVICE of Parent and an IoRequest of type UgaIoGetChildDevice.

    RuntimeContext - Context to associate with Device.

    Device         - The Device returns a dynamically allocated child UGA_DEVICE object
                     for ParentDevice. The caller is responsible for deleting Device.
      
  Returns:

    EFI_SUCCESS           - Device was returned.

    EFI_INVALID_PARAMETER - One of the arguments was not valid.

    EFI_DEVICE_ERROR      - The device had an error and could not complete the request.

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_UGA_IO_PROTOCOL_DELETE_DEVICE) (
  IN EFI_UGA_IO_PROTOCOL  * This,
  IN UGA_DEVICE           * Device
  );

/*++

  Routine Description:

    Delete a dynamically allocated child UGA_DEVICE object that was allocated via
    CreateDevice() .

  Arguments:

    This   - The EFI_UGA_IO_PROTOCOL instance. Type EFI_UGA_IO_PROTOCOL is defined 
             in Section 10.7.

    Device - The Device points to a UGA_DEVICE object that was dynamically
             allocated via a CreateDevice() call.

  Returns:

    EFI_SUCCESS           - Device was deleted.

    EFI_INVALID_PARAMETER - The Device was not allocated via CreateDevice()

--*/
typedef UGA_STATUS (EFIAPI *PUGA_FW_SERVICE_DISPATCH) (IN PUGA_DEVICE pDevice, IN OUT PUGA_IO_REQUEST pIoRequest);

/*++

  Routine Description:

    This is the main UGA service dispatch routine for all UGA_IO_REQUEST s.

  Arguments:

    pDevice    - pDevice specifies a pointer to a device object associated with a 
                 device enumerated by a pIoRequest->ioRequestCode of type 
                 UgaIoGetChildDevice. The root device for the EFI_UGA_IO_PROTOCOL 
                 is represented by pDevice being set to NULL.

    pIoRequest - pIoRequest points to a caller allocated buffer that contains data
                 defined by pIoRequest->ioRequestCode. See Related Definitions for
                 a definition of UGA_IO_REQUEST_CODE s and their associated data 
                 structures.

  Returns:

  Varies depending on pIoRequest.

--*/
struct _EFI_UGA_IO_PROTOCOL {
  EFI_UGA_IO_PROTOCOL_CREATE_DEVICE CreateDevice;
  EFI_UGA_IO_PROTOCOL_DELETE_DEVICE DeleteDevice;
  PUGA_FW_SERVICE_DISPATCH          DispatchService;
};

extern EFI_GUID gEfiUgaIoProtocolGuid;

//
// Data structure that is stored in the EFI Configuration Table with the
// EFI_UGA_IO_PROTOCOL_GUID.  The option ROMs listed in this table may have
// EBC UGA drivers.
//
typedef struct {
  UINT32  Version;
  UINT32  HeaderSize;
  UINT32  SizeOfEntries;
  UINT32  NumberOfEntries;
} EFI_DRIVER_OS_HANDOFF_HEADER;

typedef enum {
  EfiUgaDriverFromPciRom,
  EfiUgaDriverFromSystem,
  EfiDriverHandoffMax
} EFI_DRIVER_HANOFF_ENUM;

typedef struct {
  EFI_DRIVER_HANOFF_ENUM    Type;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  VOID                      *PciRomImage;
  UINT64                    PciRomSize;
} EFI_DRIVER_OS_HANDOFF;

#endif
