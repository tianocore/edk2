/** @file
  SMBIOS Protocol as defined in PI1.2 Specification VOLUME 5 Standard.

  SMBIOS protocol allows consumers to log SMBIOS data records, and enables the producer 
  to create the SMBIOS tables for a platform.

  This protocol provides an interface to add, remove or discover SMBIOS records. The driver which
  produces this protocol is responsible for creating the SMBIOS data tables and installing the pointer
  to the tables in the EFI System Configuration Table.
  The caller is responsible for only adding SMBIOS records that are valid for the SMBIOS
  MajorVersion and MinorVersion. When an enumerated SMBIOS field's values are
  controlled by the DMTF, new values can be used as soon as they are defined by the DMTF without
  requiring an update to MajorVersion and MinorVersion.
  The SMBIOS protocol can only be called a TPL < TPL_NOTIFY.

  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __SMBIOS_PROTOCOL_H__
#define __SMBIOS_PROTOCOL_H__

#define EFI_SMBIOS_PROTOCOL_GUID \
    { 0x3583ff6, 0xcb36, 0x4940, { 0x94, 0x7e, 0xb9, 0xb3, 0x9f, 0x4a, 0xfa, 0xf7 }}
    
//
// SMBIOS type macros which is according to SMBIOS specification.
//    
#define EFI_SMBIOS_TYPE_BIOS_INFORMATION                    0
#define EFI_SMBIOS_TYPE_SYSTEM_INFORMATION                  1
#define EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION               2
#define EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE                    3
#define EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION               4
#define EFI_SMBIOS_TYPE_MEMORY_CONTROLLER_INFORMATION       5
#define EFI_SMBIOS_TYPE_MEMORY_MODULE_INFORMATON            6
#define EFI_SMBIOS_TYPE_CACHE_INFORMATION                   7
#define EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION          8
#define EFI_SMBIOS_TYPE_SYSTEM_SLOTS                        9
#define EFI_SMBIOS_TYPE_ONBOARD_DEVICE_INFORMATION          10
#define EFI_SMBIOS_TYPE_OEM_STRINGS                         11
#define EFI_SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS        12
#define EFI_SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION           13
#define EFI_SMBIOS_TYPE_GROUP_ASSOCIATIONS                  14
#define EFI_SMBIOS_TYPE_SYSTEM_EVENT_LOG                    15
#define EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY               16
#define EFI_SMBIOS_TYPE_MEMORY_DEVICE                       17
#define EFI_SMBIOS_TYPE_32BIT_MEMORY_ERROR_INFORMATION      18
#define EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS         19
#define EFI_SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS        20
#define EFI_SMBIOS_TYPE_BUILT_IN_POINTING_DEVICE            21
#define EFI_SMBIOS_TYPE_PORTABLE_BATTERY                    22
#define EFI_SMBIOS_TYPE_SYSTEM_RESET                        23
#define EFI_SMBIOS_TYPE_HARDWARE_SECURITY                   24
#define EFI_SMBIOS_TYPE_SYSTEM_POWER_CONTROLS               25
#define EFI_SMBIOS_TYPE_VOLTAGE_PROBE                       26
#define EFI_SMBIOS_TYPE_COOLING_DEVICE                      27
#define EFI_SMBIOS_TYPE_TEMPERATURE_PROBE                   28
#define EFI_SMBIOS_TYPE_ELECTRICAL_CURRENT_PROBE            29
#define EFI_SMBIOS_TYPE_OUT_OF_BAND_REMOTE_ACCESS           30
#define EFI_SMBIOS_TYPE_BOOT_INTEGRITY_SERVICE              31
#define EFI_SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION             32
#define EFI_SMBIOS_TYPE_64BIT_MEMORY_ERROR_INFORMATION      33
#define EFI_SMBIOS_TYPE_MANAGEMENT_DEVICE                   34
#define EFI_SMBIOS_TYPE_MANAGEMENT_DEVICE_COMPONENT         35
#define EFI_SMBIOS_TYPE_MANAGEMENT_DEVICE_THRESHOLD_DATA    36
#define EFI_SMBIOS_TYPE_MEMORY_CHANNEL                      37
#define EFI_SMBIOS_TYPE_IPMI_DEVICE_INFORMATION             38
#define EFI_SMBIOS_TYPE_SYSTEM_POWER_SUPPLY                 39
#define EFI_SMBIOS_TYPE_INACTIVE                            126
#define EFI_SMBIOS_TYPE_END_OF_TABLE                        127
#define EFI_SMBIOS_OEM_BEGIN                                128
#define EFI_SMBIOS_OEM_END                                  255

///
/// Text strings associated with a given SMBIOS structure are returned in the dmiStrucBuffer, appended directly after
/// the formatted portion of the structure. This method of returning string information eliminates the need for
/// application software to deal with pointers embedded in the SMBIOS structure. Each string is terminated with a null
/// (00h) BYTE and the set of strings is terminated with an additional null (00h) BYTE. When the formatted portion of
/// a SMBIOS structure references a string, it does so by specifying a non-zero string number within the structure's
/// string-set. For example, if a string field contains 02h, it references the second string following the formatted portion
/// of the SMBIOS structure. If a string field references no string, a null (0) is placed in that string field. If the
/// formatted portion of the structure contains string-reference fields and all the string fields are set to 0 (no string
/// references), the formatted section of the structure is followed by two null (00h) BYTES.
///
typedef UINT8  EFI_SMBIOS_STRING;  

///
/// Types 0 through 127 (7Fh) are reserved for and defined by this
/// specification. Types 128 through 256 (80h to FFh) are available for system- and OEM-specific information.  
///
typedef UINT8  EFI_SMBIOS_TYPE;

///
/// Specifies the structure's handle, a unique 16-bit number in the range 0 to 0FFFEh (for version
/// 2.0) or 0 to 0FEFFh (for version 2.1 and later). The handle can be used with the Get SMBIOS
/// Structure function to retrieve a specific structure; the handle numbers are not required to be
/// contiguous. For v2.1 and later, handle values in the range 0FF00h to 0FFFFh are reserved for
/// use by this specification.
/// If the system configuration changes, a previously assigned handle might no longer exist.
/// However once a handle has been assigned by the BIOS, the BIOS cannot re-assign that handle
/// number to another structure.
///
typedef UINT16 EFI_SMBIOS_HANDLE;
    
typedef struct {
  EFI_SMBIOS_TYPE   Type;
  UINT8             Length;
  EFI_SMBIOS_HANDLE Handle;
} EFI_SMBIOS_TABLE_HEADER;
    
typedef struct _EFI_SMBIOS_PROTOCOL EFI_SMBIOS_PROTOCOL;

/**
  Add an SMBIOS record.
  
  This function allows any agent to add SMBIOS records. The caller is responsible for ensuring
  Record is formatted in a way that matches the version of the SMBIOS specification as defined in
  the MajorRevision and MinorRevision fields of the EFI_SMBIOS_PROTOCOL.
  Record must follow the SMBIOS structure evolution and usage guidelines in the SMBIOS
  specification. Record starts with the formatted area of the SMBIOS structure and the length is
  defined by EFI_SMBIOS_TABLE_HEADER.Length. Each SMBIOS structure is terminated by a
  double-null (0x0000), either directly following the formatted area (if no strings are present) or
  directly following the last string. The number of optional strings is not defined by the formatted area,
  but is fixed by the call to Add(). A string can be a place holder, but it must not be a NULL string as
  two NULL strings look like the double-null that terminates the structure.
  
  @param[in]        This                The EFI_SMBIOS_PROTOCOL instance.
  @param[in]        ProducerHandle      The handle of the controller or driver associated with the SMBIOS information. NULL means no handle.
  @param[in, out]   SmbiosHandle        On entry, if non-zero, the handle of the SMBIOS record. If zero, then a unique handle
                                        will be assigned to the SMBIOS record. If the SMBIOS handle is already in use
                                        EFI_ALREADY_STARTED is returned and the SMBIOS record is not updated.
  @param[in]        Record              The data for the fixed portion of the SMBIOS record. The format of the record is
                                        determined by EFI_SMBIOS_TABLE_HEADER.Type. The size of the formatted
                                        area is defined by EFI_SMBIOS_TABLE_HEADER.Length and either followed
                                        by a double-null (0x0000) or a set of null terminated strings and a null.
  
  @retval EFI_SUCCESS                   Record was added.
  @retval EFI_OUT_OF_RESOURCES          Record was not added.
  @retval EFI_ALREADY_STARTED           The SmbiosHandle passed in was already in use.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMBIOS_ADD)(
  IN CONST      EFI_SMBIOS_PROTOCOL     *This,
  IN            EFI_HANDLE              ProducerHandle OPTIONAL,
  IN OUT        EFI_SMBIOS_HANDLE       *SmbiosHandle,
  IN            EFI_SMBIOS_TABLE_HEADER *Record
);

/**
  Update the string associated with an existing SMBIOS record.
  
  This function allows the update of specific SMBIOS strings. The number of valid strings for any
  SMBIOS record is defined by how many strings were present when Add() was called.
  
  @param[in]    This            The EFI_SMBIOS_PROTOCOL instance.
  @param[in]    SmbiosHandle    SMBIOS Handle of structure that will have its string updated.
  @param[in]    StringNumber    The non-zero string number of the string to update.
  @param[in]    String          Update the StringNumber string with String.
  
  @retval EFI_SUCCESS           SmbiosHandle had its StringNumber String updated.
  @retval EFI_INVALID_PARAMETER SmbiosHandle does not exist.
  @retval EFI_UNSUPPORTED       String was not added since it's longer than 64 significant characters.
  @retval EFI_NOT_FOUND         The StringNumber.is not valid for this SMBIOS record.    
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMBIOS_UPDATE_STRING)(
   IN CONST EFI_SMBIOS_PROTOCOL *This,
   IN       EFI_SMBIOS_HANDLE   *SmbiosHandle,
   IN       UINTN               *StringNumber,
   IN       CHAR8               *String
);

/**
  Remove an SMBIOS record.
  
  This function removes an SMBIOS record using the handle specified by SmbiosHandle.
  
  @param[in]    This                The EFI_SMBIOS_PROTOCOL instance.
  @param[in]    SmbiosHandle        The handle of the SMBIOS record to remove.
  
  @retval EFI_SUCCESS               SMBIOS record was removed.
  @retval EFI_INVALID_PARAMETER     SmbiosHandle does not specify a valid SMBIOS record.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMBIOS_REMOVE)(
   IN CONST EFI_SMBIOS_PROTOCOL *This,
   IN       EFI_SMBIOS_HANDLE   SmbiosHandle
);

/**
  Allow the caller to discover all or some of the SMBIOS records.
  
  This function allows all of the SMBIOS records to be discovered. It's possible to find
  only the SMBIOS records that match the optional Type argument.
  
  @param[in]        This            The EFI_SMBIOS_PROTOCOL instance.
  @param[in, out]   SmbiosHandle    On entry, points to the previous handle of the SMBIOS record. On exit, points to the
                                    next SMBIOS record handle. If it is zero on entry, then the first SMBIOS record
                                    handle will be returned. If it returns zero on exit, then there are no more SMBIOS records.
  @param[in]        Type            On entry, it points to the type of the next SMBIOS record to return. If NULL, it
                                    indicates that the next record of any type will be returned. Type is not
                                    modified by the this function.
  @param[out]       Record          On exit, points to a pointer to the the SMBIOS Record consisting of the formatted area
                                    followed by the unformatted area. The unformatted area optionally contains text strings.
  @param[out]       ProducerHandle  On exit, points to the ProducerHandle registered by Add(). If no
                                    ProducerHandle was passed into Add() NULL is returned. If a NULL pointer is
                                    passed in no data will be returned.
  @retval EFI_SUCCESS               SMBIOS record information was successfully returned in Record.
                                    SmbiosHandle is the handle of the current SMBIOS record
  @retval EFI_NOT_FOUND             The SMBIOS record with SmbiosHandle was the last available record.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMBIOS_GET_NEXT)(
   IN     CONST EFI_SMBIOS_PROTOCOL     *This,
   IN OUT       EFI_SMBIOS_HANDLE       *SmbiosHandle,
   IN           EFI_SMBIOS_TYPE         *Type              OPTIONAL,
   OUT          EFI_SMBIOS_TABLE_HEADER **Record,
   OUT          EFI_HANDLE              *ProducerHandle    OPTIONAL
);

struct _EFI_SMBIOS_PROTOCOL {
  EFI_SMBIOS_ADD           Add;
  EFI_SMBIOS_UPDATE_STRING UpdateString;
  EFI_SMBIOS_REMOVE        Remove;
  EFI_SMBIOS_GET_NEXT      GetNext;
  UINT8                    MajorVersion;    ///< The major revision of the SMBIOS specification supported.
  UINT8                    MinorVersion;    ///< The minor revision of the SMBIOS specification supported.
};
    
extern EFI_GUID gEfiSmbiosProtocolGuid;
    
#endif // __SMBIOS_PROTOCOL_H__
