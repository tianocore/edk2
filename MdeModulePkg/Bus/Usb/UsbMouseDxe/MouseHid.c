/** @file
  Helper functions to parse HID report descriptor and items.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbMouse.h"

/**
  Get next HID item from report descriptor.

  This function retrieves next HID item from report descriptor, according to
  the start position.
  According to USB HID Specification, An item is piece of information
  about the device. All items have a one-byte prefix that contains
  the item tag, item type, and item size.
  There are two basic types of items: short items and long items.
  If the item is a short item, its optional data size may be 0, 1, 2, or 4 bytes.
  Only short item is supported here.

  @param  StartPos          Start position of the HID item to get.
  @param  EndPos            End position of the range to get the next HID item.
  @param  HidItem           Buffer for the HID Item to return.

  @return Pointer to end of the HID item returned.
          NULL if no HID item retrieved.

**/
UINT8 *
GetNextHidItem (
  IN  UINT8     *StartPos,
  IN  UINT8     *EndPos,
  OUT HID_ITEM  *HidItem
  )
{
  UINT8  Temp;

  if (EndPos <= StartPos) {
    return NULL;
  }

  Temp = *StartPos;
  StartPos++;

  //
  // Bit format of prefix byte:
  // Bits 0-1: Size
  // Bits 2-3: Type
  // Bits 4-7: Tag
  //
  HidItem->Type = BitFieldRead8 (Temp, 2, 3);
  HidItem->Tag  = BitFieldRead8 (Temp, 4, 7);

  if (HidItem->Tag == HID_ITEM_TAG_LONG) {
    //
    // Long Items are not supported, although we try to parse it.
    //
    HidItem->Format = HID_ITEM_FORMAT_LONG;

    if ((EndPos - StartPos) >= 2) {
      HidItem->Size = *StartPos++;
      HidItem->Tag  = *StartPos++;

      if ((EndPos - StartPos) >= HidItem->Size) {
        HidItem->Data.LongData = StartPos;
        StartPos              += HidItem->Size;
        return StartPos;
      }
    }
  } else {
    HidItem->Format = HID_ITEM_FORMAT_SHORT;
    HidItem->Size   = BitFieldRead8 (Temp, 0, 1);

    switch (HidItem->Size) {
      case 0:
        //
        // No data
        //
        return StartPos;

      case 1:
        //
        // 1-byte data
        //
        if ((EndPos - StartPos) >= 1) {
          HidItem->Data.Uint8 = *StartPos++;
          return StartPos;
        }

      case 2:
        //
        // 2-byte data
        //
        if ((EndPos - StartPos) >= 2) {
          CopyMem (&HidItem->Data.Uint16, StartPos, sizeof (UINT16));
          StartPos += 2;
          return StartPos;
        }

      case 3:
        //
        // 4-byte data, adjust size
        //
        HidItem->Size = 4;
        if ((EndPos - StartPos) >= 4) {
          CopyMem (&HidItem->Data.Uint32, StartPos, sizeof (UINT32));
          StartPos += 4;
          return StartPos;
        }
    }
  }

  return NULL;
}

/**
  Get data from HID item.

  This function retrieves data from HID item.
  It only supports short items, which has 4 types of data:
  0, 1, 2, or 4 bytes.

  @param  HidItem       Pointer to the HID item.

  @return The data of HID item.

**/
UINT32
GetItemData (
  IN  HID_ITEM  *HidItem
  )
{
  //
  // Get data from HID item.
  //
  switch (HidItem->Size) {
    case 1:
      return HidItem->Data.Uint8;
    case 2:
      return HidItem->Data.Uint16;
    case 4:
      return HidItem->Data.Uint32;
  }

  return 0;
}

/**
  Parse HID item from report descriptor.

  There are three item types: Main, Global, and Local.
  This function parses these types of HID items according
  to tag info.

  @param  UsbMouse          The instance of USB_MOUSE_DEV
  @param  HidItem           The HID item to parse

**/
VOID
ParseHidItem (
  IN  USB_MOUSE_DEV  *UsbMouse,
  IN  HID_ITEM       *HidItem
  )
{
  UINT8  Data;

  switch (HidItem->Type) {
    case HID_ITEM_TYPE_MAIN:
      //
      // we don't care any main items, just skip
      //
      return;

    case HID_ITEM_TYPE_GLOBAL:
      //
      // For global items, we only care Usage Page tag for Button Page here
      //
      if (HidItem->Tag == HID_GLOBAL_ITEM_TAG_USAGE_PAGE) {
        Data = (UINT8)GetItemData (HidItem);
        if (Data == 0x09) {
          //
          // Button Page
          //
          UsbMouse->PrivateData.ButtonDetected = TRUE;
        }
      }

      return;

    case HID_ITEM_TYPE_LOCAL:
      if (HidItem->Size == 0) {
        //
        // No expected data for local item
        //
        return;
      }

      Data = (UINT8)GetItemData (HidItem);

      switch (HidItem->Tag) {
        case HID_LOCAL_ITEM_TAG_USAGE_MINIMUM:
          if (UsbMouse->PrivateData.ButtonDetected) {
            UsbMouse->PrivateData.ButtonMinIndex = Data;
          }

          return;

        case HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM:
        {
          if (UsbMouse->PrivateData.ButtonDetected) {
            UsbMouse->PrivateData.ButtonMaxIndex = Data;
          }

          return;
        }

        default:
          return;
      }
  }
}

/**
  Parse Mouse Report Descriptor.

  According to USB HID Specification, report descriptors are
  composed of pieces of information. Each piece of information
  is called an Item. This function retrieves each item from
  the report descriptor and updates USB_MOUSE_DEV.

  @param  UsbMouse          The instance of USB_MOUSE_DEV
  @param  ReportDescriptor  Report descriptor to parse
  @param  ReportSize        Report descriptor size

  @retval EFI_SUCCESS       Report descriptor successfully parsed.
  @retval EFI_UNSUPPORTED   Report descriptor contains long item.

**/
EFI_STATUS
ParseMouseReportDescriptor (
  OUT USB_MOUSE_DEV  *UsbMouse,
  IN  UINT8          *ReportDescriptor,
  IN  UINTN          ReportSize
  )
{
  UINT8     *DescriptorEnd;
  UINT8     *Ptr;
  HID_ITEM  HidItem;

  DescriptorEnd = ReportDescriptor + ReportSize;

  Ptr = GetNextHidItem (ReportDescriptor, DescriptorEnd, &HidItem);
  while (Ptr != NULL) {
    if (HidItem.Format != HID_ITEM_FORMAT_SHORT) {
      //
      // Long Item is not supported at current HID revision
      //
      return EFI_UNSUPPORTED;
    }

    ParseHidItem (UsbMouse, &HidItem);

    Ptr = GetNextHidItem (Ptr, DescriptorEnd, &HidItem);
  }

  UsbMouse->NumberOfButtons = (UINT8)(UsbMouse->PrivateData.ButtonMaxIndex - UsbMouse->PrivateData.ButtonMinIndex + 1);
  UsbMouse->XLogicMax       = 127;
  UsbMouse->YLogicMax       = 127;
  UsbMouse->XLogicMin       = -127;
  UsbMouse->YLogicMin       = -127;

  return EFI_SUCCESS;
}
