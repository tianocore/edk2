/** @file
  Tcp driver function header.

Copyright (c) 2005 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TCP4_DRIVER_H_
#define _TCP4_DRIVER_H_

#include <Protocol/ServiceBinding.h>
#include <Library/IpIoLib.h>

#define TCP4_DRIVER_SIGNATURE   SIGNATURE_32 ('T', 'C', 'P', '4')

#define TCP4_PORT_KNOWN         1024
#define TCP4_PORT_USER_RESERVED 65535

#define TCP4_FROM_THIS(a) \
  CR ( \
  (a), \
  TCP4_SERVICE_DATA, \
  Tcp4ServiceBinding, \
  TCP4_DRIVER_SIGNATURE \
  )
  
///
/// TCP heartbeat tick timer.
///
typedef struct _TCP4_HEARTBEAT_TIMER {
  EFI_EVENT  TimerEvent;         ///< The event assoiated with the timer
  INTN       RefCnt;             ///< Number of reference
} TCP4_HEARTBEAT_TIMER;

///
/// TCP service data
///
typedef struct _TCP4_SERVICE_DATA {
  UINT32                        Signature;
  EFI_HANDLE                    ControllerHandle;
  IP_IO                         *IpIo;  // IP Io consumed by TCP4
  EFI_SERVICE_BINDING_PROTOCOL  Tcp4ServiceBinding;
  EFI_HANDLE                    DriverBindingHandle;
  LIST_ENTRY                    SocketList;
} TCP4_SERVICE_DATA;

///
/// TCP protocol data
///
typedef struct _TCP4_PROTO_DATA {
  TCP4_SERVICE_DATA *TcpService;
  TCP_CB            *TcpPcb;
} TCP4_PROTO_DATA;


/**
  Packet receive callback function provided to IP_IO, used to call
  the proper function to handle the packet received by IP.

  @param  Status      Status of the received packet.
  @param  IcmpErr     ICMP error number.
  @param  NetSession  Pointer to the net session of this packet.
  @param  Pkt         Pointer to the recieved packet.
  @param  Context     Pointer to the context configured in IpIoOpen(), not used
                      now.

  @return None

**/
VOID
EFIAPI
Tcp4RxCallback (
  IN EFI_STATUS                       Status,
  IN UINT8                            IcmpErr,
  IN EFI_NET_SESSION_DATA             *NetSession,
  IN NET_BUF                          *Pkt,
  IN VOID                             *Context    OPTIONAL
  );

/**
  Send the segment to IP via IpIo function.

  @param  Tcb         Pointer to the TCP_CB of this TCP instance.
  @param  Nbuf        Pointer to the TCP segment to be sent.
  @param  Src         Source address of the TCP segment.
  @param  Dest        Destination address of the TCP segment.

  @retval 0           The segment was sent out successfully.
  @retval -1          The segment was failed to send.

**/
INTN
TcpSendIpPacket (
  IN TCP_CB    *Tcb,
  IN NET_BUF   *Nbuf,
  IN UINT32    Src,
  IN UINT32    Dest
  );

/**
  The procotol handler provided to the socket layer, used to
  dispatch the socket level requests by calling the corresponding
  TCP layer functions.

  @param  Sock                   Pointer to the socket of this TCP instance.
  @param  Request                The code of this operation request.
  @param  Data                   Pointer to the operation specific data passed in
                                 together with the operation request.

  @retval EFI_SUCCESS            The socket request is completed successfully.
  @retval other                  The error status returned by the corresponding TCP
                                 layer function.

**/
EFI_STATUS
Tcp4Dispatcher (
  IN SOCKET                  *Sock,
  IN UINT8                   Request,
  IN VOID                    *Data    OPTIONAL
  );


/**
  The entry point for Tcp4 driver, used to install Tcp4 driver on the ImageHandle.

  @param  ImageHandle   The firmware allocated handle for this
                        driver image.
  @param  SystemTable   Pointer to the EFI system table.

  @retval EFI_SUCCESS   Driver loaded.
  @retval other         Driver not loaded.

**/
EFI_STATUS
EFIAPI
Tcp4DriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );


/**
  Tests to see if this driver supports a given controller.
  
  If a child device is provided, it further tests to see if this driver supports 
  creating a handle for the specified child device.

  @param  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  ControllerHandle     The handle of the controller to test. This handle 
                               must support a protocol interface that supplies 
                               an I/O abstraction to the driver.
  @param  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                               This parameter is ignored by device drivers, and is optional for bus drivers.


  @retval EFI_SUCCESS          The device specified by ControllerHandle and
                               RemainingDevicePath is supported by the driver 
                               specified by This.
  @retval EFI_ALREADY_STARTED  The device specified by ControllerHandle and
                               RemainingDevicePath is already being managed by 
                               the driver specified by This.
  @retval EFI_ACCESS_DENIED    The device specified by ControllerHandle and
                               RemainingDevicePath is already being managed by a 
                               different driver or an application that requires 
                               exclusive access.
  @retval EFI_UNSUPPORTED      The device specified by ControllerHandle and
                               RemainingDevicePath is not supported by the driver 
                               specified by This.
                               
**/
EFI_STATUS
EFIAPI
Tcp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle. 
  
  The Start() function is designed to be invoked from the EFI boot service 
  ConnectController(). As a result, much of the error checking on the parameters 
  to Start() has been moved into this common boot service. It is legal to call 
  Start() from other locations, but the following calling restrictions must be 
  followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally 
     aligned EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified 
     by This must have been called with the same calling parameters, and Supported() 
     must have returned EFI_SUCCESS.

  @param  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  ControllerHandle     The handle of the controller to start. This handle 
                               must support a protocol interface that supplies 
                               an I/O abstraction to the driver.
  @param  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                               This parameter is ignored by device drivers, and is 
                               optional for bus drivers.

  @retval EFI_SUCCESS          The device was started.
  @retval EFI_ALREADY_STARTED  The device could not be started due to a device error.
  @retval EFI_OUT_OF_RESOURCES The request could not be completed due to a lack 
                               of resources.

**/
EFI_STATUS
EFIAPI
Tcp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle.
  
  The Stop() function is designed to be invoked from the EFI boot service 
  DisconnectController(). As a result, much of the error checking on the parameters 
  to Stop() has been moved into this common boot service. It is legal to call Stop() 
  from other locations, but the following calling restrictions must be followed 
  or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call 
     to this same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this 
     driver's Start() function, and the Start() function must have called OpenProtocol() 
     on ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.
  
  @param  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  ControllerHandle  A handle to the device being stopped. The handle must 
                            support a bus specific I/O protocol for the driver 
                            to use to stop the device.
  @param  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param  ChildHandleBuffer An array of child handles to be freed. May be NULL if 
                            NumberOfChildren is 0.

  @retval EFI_SUCCESS       The device was stopped.
  @retval EFI_DEVICE_ERROR  The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
Tcp4DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

/**
  Open Ip4 and device path protocols for a created socket, and insert it in 
  socket list.
  
  @param  This                Pointer to the socket just created
  @param  Context             Context of the socket
  
  @retval EFI_SUCCESS         This protocol is installed successfully.
  @retval other               Some error occured.
  
**/
EFI_STATUS
Tcp4CreateSocketCallback (
  IN SOCKET  *This,
  IN VOID    *Context
  );

/**
  Close Ip4 and device path protocols for a socket, and remove it from socket list. 
    
  @param  This                Pointer to the socket to be removed
  @param  Context             Context of the socket
  
**/
VOID
Tcp4DestroySocketCallback (
  IN SOCKET  *This,
  IN VOID    *Context
  );

/**
  Creates a child handle and installs a protocol.
  
  The CreateChild() function installs a protocol on ChildHandle. If ChildHandle 
  is a pointer to NULL, then a new handle is created and returned in ChildHandle. 
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing 
  ChildHandle.

  @param  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle Pointer to the handle of the child to create. If it is NULL, then 
                      a new handle is created. If it is a pointer to an existing UEFI 
                      handle, then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCES            The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources availabe to create
                                the child.
  @retval other                 The child handle was not created.

**/
EFI_STATUS
EFIAPI
Tcp4ServiceBindingCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
  );

/**
  Destroys a child handle with a protocol installed on it.
  
  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol 
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the 
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param  This         Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle  Handle of the child to destroy

  @retval EFI_SUCCES            The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is 
                                being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed.
  
**/
EFI_STATUS
EFIAPI
Tcp4ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  );

#endif
