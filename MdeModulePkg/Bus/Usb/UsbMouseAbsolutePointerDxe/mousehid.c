/** @file

Copyright (c) 2004, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Mousehid.c

Abstract:
  Parse mouse hid descriptor


**/

#include "mousehid.h"


//
// Get an item from report descriptor
//

/**
  Get Next Item

  @param  StartPos          Start Position
  @param  EndPos            End Position
  @param  HidItem           HidItem to return

  @return Position

**/
UINT8 *
GetNextItem (
  IN  UINT8    *StartPos,
  IN  UINT8    *EndPos,
  OUT HID_ITEM *HidItem
  )
{
  UINT8 Temp;

  if ((EndPos - StartPos) <= 0) {
    return NULL;
  }

  Temp = *StartPos;
  StartPos++;
  //
  // bit 2,3
  //
  HidItem->Type = (UINT8) ((Temp >> 2) & 0x03);
  //
  // bit 4-7
  //
  HidItem->Tag = (UINT8) ((Temp >> 4) & 0x0F);

  if (HidItem->Tag == HID_ITEM_TAG_LONG) {
    //
    // Long Items are not supported by HID rev1.0,
    // although we try to parse it.
    //
    HidItem->Format = HID_ITEM_FORMAT_LONG;

    if ((EndPos - StartPos) >= 2) {
      HidItem->Size = *StartPos++;
      HidItem->Tag  = *StartPos++;

      if ((EndPos - StartPos) >= HidItem->Size) {
        HidItem->Data.LongData = StartPos;
        StartPos += HidItem->Size;
        return StartPos;
      }
    }
  } else {
    HidItem->Format = HID_ITEM_FORMAT_SHORT;
    //
    // bit 0, 1
    //
    HidItem->Size   = (UINT8) (Temp & 0x03);
    switch (HidItem->Size) {

    case 0:
      //
      // No data
      //
      return StartPos;

    case 1:
      //
      // One byte data
      //
      if ((EndPos - StartPos) >= 1) {
        HidItem->Data.U8 = *StartPos++;
        return StartPos;
      }

    case 2:
      //
      // Two byte data
      //
      if ((EndPos - StartPos) >= 2) {
        CopyMem (&HidItem->Data.U16, StartPos, sizeof (UINT16));
        StartPos += 2;
        return StartPos;
      }

    case 3:
      //
      // 4 byte data, adjust size
      //
      HidItem->Size++;
      if ((EndPos - StartPos) >= 4) {
        CopyMem (&HidItem->Data.U32, StartPos, sizeof (UINT32));
        StartPos += 4;
        return StartPos;
      }
    }
  }

  return NULL;
}


/**
  Get Item Data

  @param  HidItem           HID_ITEM

  @return HidItem Data

**/
UINT32
GetItemData (
  IN  HID_ITEM *HidItem
  )
{
  //
  // Get Data from HID_ITEM structure
  //
  switch (HidItem->Size) {

  case 1:
    return HidItem->Data.U8;

  case 2:
    return HidItem->Data.U16;

  case 4:
    return HidItem->Data.U32;
  }

  return 0;
}


/**
  Parse Local Item

  @param  UsbMouseAbsolutePointer          USB_MOUSE_ABSOLUTE_POINTER_DEV
  @param  LocalItem         Local Item


**/
VOID
ParseLocalItem (
  IN  USB_MOUSE_ABSOLUTE_POINTER_DEV   *UsbMouseAbsolutePointer,
  IN  HID_ITEM        *LocalItem
  )
{
  UINT32  Data;

  if (LocalItem->Size == 0) {
    //
    // No expected data for local item
    //
    return ;
  }

  Data = GetItemData (LocalItem);

  switch (LocalItem->Tag) {

  case HID_LOCAL_ITEM_TAG_DELIMITER:
    //
    // we don't support delimiter here
    //
    return ;

  case HID_LOCAL_ITEM_TAG_USAGE:
    return ;

  case HID_LOCAL_ITEM_TAG_USAGE_MINIMUM:
    if (UsbMouseAbsolutePointer->PrivateData.ButtonDetected) {
      UsbMouseAbsolutePointer->PrivateData.ButtonMinIndex = (UINT8) Data;
    }

    return ;

  case HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM:
    {
      if (UsbMouseAbsolutePointer->PrivateData.ButtonDetected) {
        UsbMouseAbsolutePointer->PrivateData.ButtonMaxIndex = (UINT8) Data;
      }

      return ;
    }
  }
}

VOID
ParseGlobalItem (
  IN  USB_MOUSE_ABSOLUTE_POINTER_DEV   *UsbMouseAbsolutePointer,
  IN  HID_ITEM        *GlobalItem
  )
{
  UINT8 UsagePage;

  switch (GlobalItem->Tag) {
  case HID_GLOBAL_ITEM_TAG_USAGE_PAGE:
    {
      UsagePage = (UINT8) GetItemData (GlobalItem);

      //
      // We only care Button Page here
      //
      if (UsagePage == 0x09) {
        //
        // Button Page
        //
        UsbMouseAbsolutePointer->PrivateData.ButtonDetected = TRUE;
        return ;
      }
      break;
    }

  }
}



/**
  Parse Main Item

  @param  UsbMouseAbsolutePointer   USB_MOUSE_ABSOLUTE_POINTER_DEV
  @param  MainItem          HID_ITEM to parse

  @return VOID

**/
VOID
ParseMainItem (
  IN  USB_MOUSE_ABSOLUTE_POINTER_DEV   *UsbMouseAbsolutePointer,
  IN  HID_ITEM        *MainItem
  )
{
  //
  // we don't care any main items, just skip
  //
  return ;
}


/**
  Parse Hid Item

  @param  UsbMouseAbsolutePointer          USB_MOUSE_ABSOLUTE_POINTER_DEV
  @param  HidItem           HidItem to parse

  @return VOID

**/
VOID
ParseHidItem (
  IN  USB_MOUSE_ABSOLUTE_POINTER_DEV   *UsbMouseAbsolutePointer,
  IN  HID_ITEM        *HidItem
  )
{
  switch (HidItem->Type) {

  case HID_ITEM_TYPE_MAIN:
    //
    // For Main Item, parse main item
    //
    ParseMainItem (UsbMouseAbsolutePointer, HidItem);
    break;

  case HID_ITEM_TYPE_GLOBAL:
    //
    // For global Item, parse global item
    //
    ParseGlobalItem (UsbMouseAbsolutePointer, HidItem);
    break;

  case HID_ITEM_TYPE_LOCAL:
    //
    // For Local Item, parse local item
    //
    ParseLocalItem (UsbMouseAbsolutePointer, HidItem);
    break;
  }
}
//
// A simple parse just read some field we are interested in
//

/**
  Parse Mouse Report Descriptor

  @param  UsbMouse          USB_MOUSE_DEV
  @param  ReportDescriptor  Report descriptor to parse
  @param  ReportSize        Report descriptor size

  @retval EFI_DEVICE_ERROR  Report descriptor error
  @retval EFI_SUCCESS       Success

**/
EFI_STATUS
ParseMouseReportDescriptor (
  IN  USB_MOUSE_ABSOLUTE_POINTER_DEV   *UsbMouseAbsolutePointer,
  IN  UINT8           *ReportDescriptor,
  IN  UINTN           ReportSize
  )
{
  UINT8     *DescriptorEnd;
  UINT8     *ptr;
  HID_ITEM  HidItem;

  DescriptorEnd = ReportDescriptor + ReportSize;

  ptr           = GetNextItem (ReportDescriptor, DescriptorEnd, &HidItem);

  while (ptr != NULL) {
    if (HidItem.Format != HID_ITEM_FORMAT_SHORT) {
      //
      // Long Format Item is not supported at current HID revision
      //
      return EFI_DEVICE_ERROR;
    }

    ParseHidItem (UsbMouseAbsolutePointer, &HidItem);

    ptr = GetNextItem (ptr, DescriptorEnd, &HidItem);
  }

  UsbMouseAbsolutePointer->NumberOfButtons                 = (UINT8) (UsbMouseAbsolutePointer->PrivateData.ButtonMaxIndex - UsbMouseAbsolutePointer->PrivateData.ButtonMinIndex + 1);
  UsbMouseAbsolutePointer->XLogicMax                       = UsbMouseAbsolutePointer->YLogicMax = 1023;
  UsbMouseAbsolutePointer->XLogicMin                       = UsbMouseAbsolutePointer->YLogicMin = -1023;

  return EFI_SUCCESS;
}
