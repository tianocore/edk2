/*++
 
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MiscPortInternalConnectorDesignatorFunction.c
  
Abstract: 

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

--*/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

//
//
//
MISC_SUBCLASS_TABLE_FUNCTION (
  MiscPortInternalConnectorDesignator
  )
/*++
Description:

  This function makes boot time changes to the contents of the
  MiscPortConnectorInformation (Type 8).

Parameters:

  RecordType
    Type of record to be processed from the Data Table.
    mMiscSubclassDataTable[].RecordType

  RecordLen
    Size of static RecordData from the Data Table.
    mMiscSubclassDataTable[].RecordLen

  RecordData
    Pointer to copy of RecordData from the Data Table.  Changes made
    to this copy will be written to the Data Hub but will not alter
    the contents of the static Data Table.

  LogRecordData
    Set *LogRecordData to TRUE to log RecordData to Data Hub.
    Set *LogRecordData to FALSE when there is no more data to log.

Returns:

  EFI_SUCCESS
    All parameters were valid and *RecordData and *LogRecordData have
    been set.

  EFI_UNSUPPORTED
    Unexpected RecordType value.

  EFI_INVALID_PARAMETER
    One of the following parameter conditions was true:
      RecordLen was zero.
      RecordData was NULL.
      LogRecordData was NULL.
--*/
{
  STATIC BOOLEAN                    Done                    = FALSE;
  STATIC PS2_CONN_DEVICE_PATH       mPs2KeyboardDevicePath  = { DP_ACPI, DP_PCI (0x1F, 0x00), DP_LPC (0x0303, 0), DP_END };
  STATIC PS2_CONN_DEVICE_PATH       mPs2MouseDevicePath     = { DP_ACPI, DP_PCI (0x1F, 0x00), DP_LPC (0x0303, 1), DP_END };
  STATIC SERIAL_CONN_DEVICE_PATH    mCom1DevicePath         = { DP_ACPI, DP_PCI (0x1F, 0x00), DP_LPC (0x0501, 0), DP_END };
  STATIC SERIAL_CONN_DEVICE_PATH    mCom2DevicePath         = { DP_ACPI, DP_PCI (0x1F, 0x00), DP_LPC (0x0501, 1), DP_END };
  STATIC PARALLEL_CONN_DEVICE_PATH  mLpt1DevicePath         = { DP_ACPI, DP_PCI (0x1F, 0x00), DP_LPC (0x0401, 0), DP_END };
  STATIC FLOOPY_CONN_DEVICE_PATH    mFloopyADevicePath      = { DP_ACPI, DP_PCI (0x1F, 0x00), DP_LPC (0x0604, 0), DP_END };
  STATIC FLOOPY_CONN_DEVICE_PATH    mFloopyBDevicePath      = { DP_ACPI, DP_PCI (0x1F, 0x00), DP_LPC (0x0604, 1), DP_END };
  STATIC USB_PORT_DEVICE_PATH       mUsb0DevicePath         = { DP_ACPI, DP_PCI (0x1d, 0x00), DP_END };
  STATIC USB_PORT_DEVICE_PATH       mUsb1DevicePath         = { DP_ACPI, DP_PCI (0x1d, 0x01), DP_END };
  STATIC USB_PORT_DEVICE_PATH       mUsb2DevicePath         = { DP_ACPI, DP_PCI (0x1d, 0x02), DP_END };
  STATIC USB_PORT_DEVICE_PATH       mUsb3DevicePath         = { DP_ACPI, DP_PCI (0x1d, 0x07), DP_END };
  STATIC IDE_DEVICE_PATH            mIdeDevicePath          = { DP_ACPI, DP_PCI (0x1F, 0x01), DP_END };
  STATIC GB_NIC_DEVICE_PATH         mGbNicDevicePath        = { DP_ACPI, DP_PCI( 0x03,0x00 ),DP_PCI( 0x1F,0x00 ),DP_PCI( 0x07,0x00 ), DP_END };
  EFI_DEVICE_PATH_PROTOCOL          EndDevicePath           = DP_END;

  //
  // First check for invalid parameters.
  //
  // Shanmu >> to fix the Device Path Issue...
  // if (RecordLen == 0 || RecordData == NULL || LogRecordData == NULL) {
  //
  if (*RecordLen == 0 || RecordData == NULL || LogRecordData == NULL) {
    //
    // End Shanmu
    //
    return EFI_INVALID_PARAMETER;
  }
  //
  // Then check for unsupported RecordType.
  //
  if (RecordType != EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_RECORD_NUMBER) {
    return EFI_UNSUPPORTED;
  }
  //
  // Is this the first time through this function?
  //
  if (!Done) {
    //
    // Yes, this is the first time.  Inspect/Change the contents of the
    // RecordData structure.
    //
    //
    // Device path is only updated here as it was not taking that in static data
    //
    // Shanmu >> to fix the Device Path Issue...
    //

    /*
    switch (((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortInternalConnectorDesignator) 
    {
      case STR_MISC_PORT_INTERNAL_MOUSE:
        {
          (EFI_DEVICE_PATH_PROTOCOL)((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortPath = *((EFI_DEVICE_PATH_PROTOCOL*)&mPs2MouseDevicePath);          
        }break;
      case STR_MISC_PORT_INTERNAL_KEYBOARD:
        {
          (EFI_DEVICE_PATH_PROTOCOL)((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortPath = *((EFI_DEVICE_PATH_PROTOCOL*)&mPs2KeyboardDevicePath);          
        }break;
      case STR_MISC_PORT_INTERNAL_COM1:
        {
          (EFI_DEVICE_PATH_PROTOCOL)((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortPath = *((EFI_DEVICE_PATH_PROTOCOL*)&mCom1DevicePath);          
        }break;
      case STR_MISC_PORT_INTERNAL_COM2:
        {
          (EFI_DEVICE_PATH_PROTOCOL)((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortPath = *((EFI_DEVICE_PATH_PROTOCOL*)&mCom2DevicePath);          
        }break;
      case STR_MISC_PORT_INTERNAL_LPT1:
        {
          (EFI_DEVICE_PATH_PROTOCOL)((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortPath = *((EFI_DEVICE_PATH_PROTOCOL*)&mLpt1DevicePath);          
        }break;
      case STR_MISC_PORT_INTERNAL_USB1:
        {
          (EFI_DEVICE_PATH_PROTOCOL)((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortPath = *((EFI_DEVICE_PATH_PROTOCOL*)&mUsb0DevicePath);          
        }break;
      case STR_MISC_PORT_INTERNAL_USB2:
        {
          (EFI_DEVICE_PATH_PROTOCOL)((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortPath = *((EFI_DEVICE_PATH_PROTOCOL*)&mUsb1DevicePath);          
        }break;
      case STR_MISC_PORT_INTERNAL_USB3:
        {
          (EFI_DEVICE_PATH_PROTOCOL)((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortPath = *((EFI_DEVICE_PATH_PROTOCOL*)&mUsb2DevicePath);          
        }break;
      case STR_MISC_PORT_INTERNAL_NETWORK:
        {
          (EFI_DEVICE_PATH_PROTOCOL)((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortPath = *((EFI_DEVICE_PATH_PROTOCOL*)&mGbNicDevicePath);          
        }break;
      case STR_MISC_PORT_INTERNAL_FLOPPY:
        {
          (EFI_DEVICE_PATH_PROTOCOL)((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortPath = *((EFI_DEVICE_PATH_PROTOCOL*)&mFloopyADevicePath);          
        }break;
      case STR_MISC_PORT_INTERNAL_IDE1:
        {
          (EFI_DEVICE_PATH_PROTOCOL)((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortPath = *((EFI_DEVICE_PATH_PROTOCOL*)&mIdeDevicePath);          
        }break;
      case STR_MISC_PORT_INTERNAL_IDE2:
        {
          (EFI_DEVICE_PATH_PROTOCOL)((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortPath = *((EFI_DEVICE_PATH_PROTOCOL*)&mIdeDevicePath);          
        }break;
      default:
        {
          (EFI_DEVICE_PATH_PROTOCOL)((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *)RecordData)->PortPath = EndDevicePath;
        }break;    
    }
    */
    switch (((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *) RecordData)->PortInternalConnectorDesignator) {
    case STR_MISC_PORT_INTERNAL_MOUSE:
      {
        CopyMem (
          &((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *) RecordData)->PortPath,
          &mPs2MouseDevicePath,
          GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &mPs2MouseDevicePath)
          );
        *RecordLen = *RecordLen - sizeof (EFI_MISC_PORT_DEVICE_PATH) + GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &mPs2MouseDevicePath);
      }
      break;

    case STR_MISC_PORT_INTERNAL_KEYBOARD:
      {
        CopyMem (
          &((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *) RecordData)->PortPath,
          &mPs2KeyboardDevicePath,
          GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &mPs2KeyboardDevicePath)
          );
        *RecordLen = *RecordLen - sizeof (EFI_MISC_PORT_DEVICE_PATH) + GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &mPs2KeyboardDevicePath);
      }
      break;

    case STR_MISC_PORT_INTERNAL_COM1:
      {
        CopyMem (
          &((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *) RecordData)->PortPath,
          &mCom1DevicePath,
          GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &mCom1DevicePath)
          );
        *RecordLen = *RecordLen - sizeof (EFI_MISC_PORT_DEVICE_PATH) + GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &mCom1DevicePath);
      }
      break;

    case STR_MISC_PORT_INTERNAL_COM2:
      {
        CopyMem (
          &((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *) RecordData)->PortPath,
          &mCom2DevicePath,
          GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &mCom2DevicePath)
          );
        *RecordLen = *RecordLen - sizeof (EFI_MISC_PORT_DEVICE_PATH) + GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &mCom2DevicePath);
      }
      break;

    case STR_MISC_PORT_INTERNAL_FLOPPY:
      {
        CopyMem (
          &((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *) RecordData)->PortPath,
          &mFloopyADevicePath,
          GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &mFloopyADevicePath)
          );
        *RecordLen = *RecordLen - sizeof (EFI_MISC_PORT_DEVICE_PATH) + GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &mFloopyADevicePath);
      }
      break;

    default:
      {
        CopyMem (
          &((EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *) RecordData)->PortPath,
          &EndDevicePath,
          GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &EndDevicePath)
          );
        *RecordLen = *RecordLen - sizeof (EFI_MISC_PORT_DEVICE_PATH) + GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &EndDevicePath);
      }
      break;
    }
    //
    // End Shanmu
    //
    // Set Done flag to TRUE for next pass through this function.
    // Set *LogRecordData to TRUE so data will get logged to Data Hub.
    //
    Done            = TRUE;
    *LogRecordData  = TRUE;
  } else {
    //
    // No, this is the second time.  Reset the state of the Done flag
    // to FALSE and tell the data logger that there is no more data
    // to be logged for this record type.  If any memory allocations
    // were made by earlier passes, they must be released now.
    //
    Done            = FALSE;
    *LogRecordData  = FALSE;
  }

  return EFI_SUCCESS;
}

/* eof - MiscSystemManufacturerFunction.c */
