/** @file
  The EFI_FORM_CALLBACK_PROTOCOL is the defined interface for access to custom 
  NV storage devices as well as communication of user selections in a more 
  interactive environment.  This protocol should be published by hardware 
  specific drivers which want to export access to custom hardware storage or 
  publish IFR which has a requirement to call back the original driver.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  FormCallback.h

  @par Revision Reference:
  This protocol is defined in HII spec 0.92.

**/

#ifndef __FORM_CALLBACK_H__
#define __FORM_CALLBACK_H__


#define EFI_FORM_CALLBACK_PROTOCOL_GUID \
  { \
    0xf3e4543d, 0xcf35, 0x6cef, {0x35, 0xc4, 0x4f, 0xe6, 0x34, 0x4d, 0xfc, 0x54 } \
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
  //
  // EFI_IFR_DATA_ENTRY  Data[1];    // The in-line Data entries.
  //
} EFI_IFR_DATA_ARRAY;

typedef union {
  EFI_IFR_DATA_ARRAY  DataArray;  // Primarily used by those who call back to their drivers and use HII as a repository
  EFI_IFR_PACKET      DataPacket; // Primarily used by those which do not use HII as a repository
  CHAR16              *String;  // If returning an error - fill the string with null-terminated contents
} EFI_HII_CALLBACK_PACKET;

typedef struct {
  EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId;   // Offset into the map
  UINT8             StorageWidth; // Width of the value
  //
  // CHAR8             Data[1];      // The Data itself
  //
} EFI_IFR_NV_DATA;

#pragma pack()
//
// The following types are currently defined:
//
/**
  Returns the value of a variable. 

  @param  This                  A pointer to the EFI_FORM_CALLBACK_PROTOCOL instance.
  @param  VariableName          A NULL-terminated Unicode string that is the
                                name of the vendor's variable.
  @param  VendorGuid            A unique identifier for the vendor.
  @param  Attributes            If not NULL, a pointer to the memory location to
                                return the attribute's bit-mask for the variable.
  @param  DataSize              The size in bytes of the Buffer. A size of zero causes
                                the variable to be deleted.
  @param  Buffer                The buffer to return the contents of the variable.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the result.
                                DataSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_DEVICE_ERROR      The variable could not be saved due to a hardware failure.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_NV_READ) (
  IN     EFI_FORM_CALLBACK_PROTOCOL    *This,
  IN     CHAR16                        *VariableName,
  IN     EFI_GUID                      *VendorGuid,
  OUT    UINT32                        *Attributes OPTIONAL,
  IN OUT UINTN                         *DataSize,
  OUT    VOID                          *Buffer
  );

/**
  Sets the value of a variable. 

  @param  This                  A pointer to the EFI_FORM_CALLBACK_PROTOCOL instance.
  @param  VariableName          A NULL-terminated Unicode string that is the
                                name of the vendor's variable. Each VariableName is unique for each VendorGuid.
  @param  VendorGuid            A unique identifier for the vendor.
  @param  Attributes            Attributes bit-mask to set for the variable.
  @param  DataSize              The size in bytes of the Buffer. A size of zero causes
                                the variable to be deleted.
  @param  Buffer                The buffer containing the contents of the variable.
  @param  ResetRequired         Returns a value from the driver that abstracts
                                this information and will enable a system to know if a system reset
                                is required to achieve the configuration changes being enabled through
                                this function.

  @retval EFI_SUCCESS           The firmware has successfully stored the variable and
                                its data as defined by the Attributes.
  @retval EFI_OUT_OF_RESOURCES  Not enough storage is available to hold
                                the variable and its data.
  @retval EFI_INVALID_PARAMETER An invalid combination of Attributes bits
                                was supplied, or the DataSize exceeds the maximum allowed.
  @retval EFI_DEVICE_ERROR      The variable could not be saved due to a hardware failure.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_NV_WRITE) (
  IN     EFI_FORM_CALLBACK_PROTOCOL    *This,
  IN     CHAR16                        *VariableName,
  IN     EFI_GUID                      *VendorGuid,
  IN     UINT32                        Attributes,
  IN     UINTN                         DataSize,
  IN     VOID                          *Buffer,
  OUT    BOOLEAN                       *ResetRequired
  );

/**
  This function is called to provide results data to the driver. 

  @param  This                  A pointer to the EFI_FORM_CALLBACK_PROTOCOL instance.
  @param  KeyValue              A unique value which is sent to the original exporting
                                driver so that it can identify the type of data to expect. The format of
                                the data tends to vary based on the opcode that generated the callback.
  @param  Data                  A pointer to the data being sent to the original exporting driver.
  @param  Packet                A pointer to a packet of information which a driver passes
                                back to the browser.

  @return Status Code

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FORM_CALLBACK) (
  IN     EFI_FORM_CALLBACK_PROTOCOL    *This,
  IN     UINT16                        KeyValue,
  IN     EFI_IFR_DATA_ARRAY            *Data,
  OUT    EFI_HII_CALLBACK_PACKET       **Packet
  );

/**
  @par Protocol Description:
  The EFI_FORM_CALLBACK_PROTOCOL is the defined interface for access to 
  custom NVS devices as well as communication of user selections in a more 
  interactive environment. This protocol should be published by hardware-specific 
  drivers that want to export access to custom hardware storage or publish IFR 
  that has a requirement to call back the original driver.

  @param NvRead
  The read operation to access the NV data serviced by a hardware-specific driver.

  @param NvWrite
  The write operation to access the NV data serviced by a hardware-specific driver.

  @param Callback
  The function that is called from the configuration browser to communicate key value pairs. 

**/
struct _EFI_FORM_CALLBACK_PROTOCOL {
  EFI_NV_READ       NvRead;
  EFI_NV_WRITE      NvWrite;
  EFI_FORM_CALLBACK Callback;
};

extern EFI_GUID gEfiFormCallbackProtocolGuid;

#endif
