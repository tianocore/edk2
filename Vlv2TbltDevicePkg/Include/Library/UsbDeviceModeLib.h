/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#ifndef __BASE_USBDEVICEMODE_LIB_H__
#define __BASE_USBDEVICEMODE_LIB_H__

#pragma pack(1)
typedef struct {
    UINT8  bLength;
    UINT8  bDescriptorType;
    UINT8  bMaxBurst;
    UINT8  bmAttributes;
    UINT16 wBytesPerInterval;
} endpointCompanionDescriptor;
#pragma pack()

#pragma pack(1)
typedef struct {
    UINT8  bLength;
    UINT8  bDescriptorType;
    UINT8  bEndpointAddress;
    UINT8  bmAttributes;
    UINT16 wMaxPacketSize;
    UINT8  bInterval;
} endpointDescriptor;
#pragma pack()

typedef struct {
    endpointDescriptor          *pEpDesc;
    endpointCompanionDescriptor *pEpCompDesc;
} USB_DEV_EP_INFO;    //usbdEpInfo;

typedef struct {
    VOID        *pBuf;
    UINT32    dataLen;
} USBD_IO_INFO;

typedef struct {
    USBD_IO_INFO     ioInfo;
    USB_DEV_EP_INFO  epInfo;
} USBD_IO_REQ;

UINTN
EFIAPI
usbdInitDCI (
  VOID
  );

BOOLEAN
EFIAPI
fbInit (
  OUT  VOID  *pParams
  );

BOOLEAN
EFIAPI
fbDeinit (
  VOID
  );

BOOLEAN
EFIAPI
fbStart (
  VOID
  );

BOOLEAN
EFIAPI
fbStop (
  VOID
  );

BOOLEAN
EFIAPI
usbdSetMmioBar (
  UINT32 mmioBar
  );

BOOLEAN
EFIAPI
udciDeinit (
  VOID    *pUdciHndl,
  UINT32  flags
  );

BOOLEAN
EFIAPI
udciIsr (
  VOID    *pUdciHndl
  );

BOOLEAN
EFIAPI
udciConnect (
  VOID    *pUdciHndl
  );

BOOLEAN
EFIAPI
udciDisconnect (
  VOID    *pUdciHndl
  );

BOOLEAN
EFIAPI
udciSetAddress (
  VOID    *pUdciHndl,
  UINT8   address
  );

BOOLEAN
EFIAPI
udciInitEp (
  VOID            *pUdciHndl,
  USB_DEV_EP_INFO *pEpInfo
  );

BOOLEAN
EFIAPI
udciEnableEp (
  VOID            *pUdciHndl,
  USB_DEV_EP_INFO *pEpInfo
  );

BOOLEAN
EFIAPI
udciDisableEp (
  VOID            *pUdciHndl,
  USB_DEV_EP_INFO *pEpInfo
  );

BOOLEAN
EFIAPI
udciStallEp (
  VOID            *pUdciHndl,
  USB_DEV_EP_INFO *pEpInfo
  );

BOOLEAN
EFIAPI
udciClearStallEp (
  VOID            *pUdciHndl,
  USB_DEV_EP_INFO *pEpInfo
  );


BOOLEAN
EFIAPI
udciEp0TxStatus (
  VOID            *pUdciHndl
  );


BOOLEAN
EFIAPI
udciEpTxData (
  VOID        *pUdciHndl,
  USBD_IO_REQ *pIoReq
  );

BOOLEAN
EFIAPI
udciEpRxData (
  VOID        *pUdciHndl,
  USBD_IO_REQ *pIoReq
  );

BOOLEAN
EFIAPI
udciRegisterCallbacks (
  VOID            *pUdciHndl
  );

#endif
