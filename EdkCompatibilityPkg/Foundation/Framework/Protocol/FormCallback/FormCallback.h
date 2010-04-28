/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    FormCallback.h

Abstract:

  The EFI_FORM_CALLBACK_PROTOCOL is the defined interface for access to custom 
  NV storage devices as well as communication of user selections in a more 
  interactive environment.  This protocol should be published by hardware 
  specific drivers which want to export access to custom hardware storage or 
  publish IFR which has a requirement to call back the original driver.

--*/

#ifndef _FORM_CALLBACK_H_
#define _FORM_CALLBACK_H_

#include EFI_PROTOCOL_DEFINITION (FormBrowser)

#define EFI_FORM_CALLBACK_PROTOCOL_GUID \
  { \
    0xf3e4543d, 0xcf35, 0x6cef, {0x35, 0xc4, 0x4f, 0xe6, 0x34, 0x4d, 0xfc, 0x54} \
  }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_FORM_CALLBACK_PROTOCOL  EFI_FORM_CALLBACK_PROTOCOL;

#define RESET_REQUIRED  1 // Flags setting to signify that the callback operation resulted in an eventual
// reset to be done upon exit of the browser
//
#define EXIT_REQUIRED   2   // Flags setting to signify that after the processing of the callback results - exit the browser
#define SAVE_REQUIRED   4   // Flags setting to signify that after the processing of the callback results - save the NV data
#define NV_CHANGED      8   // Flags setting to signify that after the processing of the callback results - turn the NV flag on
#define NV_NOT_CHANGED  16  // Flags setting to signify that after the processing of the callback results - turn the NV flag off
#pragma pack(1)
typedef struct {
  UINT8   OpCode;           // Likely a string, numeric, or one-of
  UINT8   Length;           // Length of the EFI_IFR_DATA_ENTRY packet
  UINT16  Flags;            // Flags settings to determine what behavior is desired from the browser after the callback
  VOID    *Data;            // The data in the form based on the op-code type - this is not a pointer to the data, the data follows immediately
  // If the OpCode is a OneOf or Numeric type - Data is a UINT16 value
  // If the OpCode is a String type - Data is a CHAR16[x] type
  // If the OpCode is a Checkbox type - Data is a UINT8 value
  // If the OpCode is a NV Access type - Data is a EFI_IFR_NV_DATA structure
  //
} EFI_IFR_DATA_ENTRY;

typedef struct {
  VOID                *NvRamMap;  // If the flag of the op-code specified retrieval of a copy of the NVRAM map,
  // this is a pointer to a buffer copy
  //
  UINT32              EntryCount; // How many EFI_IFR_DATA_ENTRY entries
  EFI_IFR_DATA_ENTRY  Data[1];    // The in-line Data entries.
} EFI_IFR_DATA_ARRAY;

typedef union {
  EFI_IFR_DATA_ARRAY  DataArray;  // Primarily used by those who call back to their drivers and use HII as a repository
  EFI_IFR_PACKET      DataPacket; // Primarily used by those which do not use HII as a repository
  CHAR16              String[1];  // If returning an error - fill the string with null-terminated contents
} EFI_HII_CALLBACK_PACKET;
#pragma pack()
//
// The following types are currently defined:
//
typedef
EFI_STATUS
(EFIAPI *EFI_NV_READ) (
  IN     EFI_FORM_CALLBACK_PROTOCOL    * This,
  IN     CHAR16                        *VariableName,
  IN     EFI_GUID                      * VendorGuid,
  OUT    UINT32                        *Attributes OPTIONAL,
  IN OUT UINTN                         *DataSize,
  OUT    VOID                          *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_NV_WRITE) (
  IN     EFI_FORM_CALLBACK_PROTOCOL    * This,
  IN     CHAR16                        *VariableName,
  IN     EFI_GUID                      * VendorGuid,
  IN     UINT32                        Attributes,
  IN     UINTN                         DataSize,
  IN     VOID                          *Buffer,
  OUT    BOOLEAN                       *ResetRequired
  );

typedef
EFI_STATUS
(EFIAPI *EFI_FORM_CALLBACK) (
  IN     EFI_FORM_CALLBACK_PROTOCOL    * This,
  IN     UINT16                        KeyValue,
  IN     EFI_IFR_DATA_ARRAY            * Data,
  OUT    EFI_HII_CALLBACK_PACKET       **Packet
  );

struct _EFI_FORM_CALLBACK_PROTOCOL {
  EFI_NV_READ       NvRead;
  EFI_NV_WRITE      NvWrite;
  EFI_FORM_CALLBACK Callback;
};

extern EFI_GUID gEfiFormCallbackProtocolGuid;

#endif
