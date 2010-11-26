/*++

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    HiiDatabase.h
    
Abstract:

    EFI_HII_DATABASE_PROTOCOL from UEFI 2.1 specification.
    
    This protocol is a database manager for HII related data structures.

Revision History

--*/

#ifndef __EFI_HII_DATABASE_PROTOCOL_H__
#define __EFI_HII_DATABASE_PROTOCOL_H__

#include "EfiHii.h"

//
// Global ID for the Hii Database Protocol.
//

#define EFI_HII_DATABASE_PROTOCOL_GUID \
  { \
    0xef9fc172, 0xa1b2, 0x4693, {0xb3, 0x27, 0x6d, 0x32, 0xfc, 0x41, 0x60, 0x42} \
  }

#define EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID \
  { \
    0x14982a4f, 0xb0ed, 0x45b8, {0xa8, 0x11, 0x5a, 0x7a, 0x9b, 0xc2, 0x32, 0xdf} \
  }

EFI_FORWARD_DECLARATION (EFI_HII_DATABASE_PROTOCOL);
               
typedef UINTN EFI_HII_DATABASE_NOTIFY_TYPE;

#define EFI_HII_DATABASE_NOTIFY_NEW_PACK     0x00000001
#define EFI_HII_DATABASE_NOTIFY_REMOVE_PACK  0x00000002
#define EFI_HII_DATABASE_NOTIFY_EXPORT_PACK  0x00000004
#define EFI_HII_DATABASE_NOTIFY_ADD_PACK     0x00000008

typedef
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_NOTIFY) (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  )
/*++

  Routine Description:
    Functions which are registered to receive notification of database events have this prototype. The
    actual event is encoded in NotifyType. The following table describes how PackageType,             
    PackageGuid, Handle, and Package are used for each of the notification types.                     
        
  Arguments:              
    PackageType       - Package type of the notification.
    PackageGuid       - If PackageType is EFI_HII_PACKAGE_TYPE_GUID, then this is the pointer to  
                        the GUID which must match the Guid field of                               
                        EFI_HII_GUID_PACKAGE_GUID_HDR. Otherwise, it must be NULL.                
    Package           - Points to the package referred to by the notification.                        
    Handle            - The handle of the package list which contains the specified package.
    NotifyType        - The type of change concerning the database.
    
  Returns:
    EFI status code.
     
--*/  
;

//
// EFI_HII_DATABASE_PROTOCOL protocol prototypes
//

typedef
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_NEW_PACK) (
  IN CONST EFI_HII_DATABASE_PROTOCOL    *This,
  IN CONST EFI_HII_PACKAGE_LIST_HEADER  *PackageList,
  IN CONST EFI_HANDLE                   DriverHandle,
  OUT EFI_HII_HANDLE                    *Handle
  )
/*++

  Routine Description:
    This function adds the packages in the package list to the database and returns a handle. If there is a
    EFI_DEVICE_PATH_PROTOCOL associated with the DriverHandle, then this function will                     
    create a package of type EFI_PACKAGE_TYPE_DEVICE_PATH and add it to the package list.                      
    
  Arguments:          
    This              - A pointer to the EFI_HII_DATABASE_PROTOCOL instance.
    PackageList       - A pointer to an EFI_HII_PACKAGE_LIST_HEADER structure.
    DriverHandle      - Associate the package list with this EFI handle.    
    Handle            - A pointer to the EFI_HII_HANDLE instance.
    
  Returns:
    EFI_SUCCESS            - The package list associated with the Handle
                             was added to the HII database.             
    EFI_OUT_OF_RESOURCES   - Unable to allocate necessary resources for the
                             new database contents.                        
    EFI_INVALID_PARAMETER  - PackageList is NULL or Handle is NULL.
     
--*/  
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_REMOVE_PACK) (
  IN CONST EFI_HII_DATABASE_PROTOCOL    *This,
  IN EFI_HII_HANDLE                     Handle
  )
/*++

  Routine Description:
    This function removes the package list that is associated with a handle Handle 
    from the HII database. Before removing the package, any registered functions 
    with the notification type REMOVE_PACK and the same package type will be called.
    
  Arguments:          
    This              - A pointer to the EFI_HII_DATABASE_PROTOCOL instance.
    Handle            - The handle that was registered to the data that is requested 
                        for removal.
    
  Returns:
    EFI_SUCCESS            - The data associated with the Handle was removed from 
                             the HII database.
    EFI_NOT_FOUND          - The specified Handle is not in database.
     
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_UPDATE_PACK) (
  IN CONST EFI_HII_DATABASE_PROTOCOL    *This,
  IN EFI_HII_HANDLE                     Handle,
  IN CONST EFI_HII_PACKAGE_LIST_HEADER  *PackageList
  )
/*++

  Routine Description:
    This function updates the existing package list (which has the specified Handle) 
    in the HII databases, using the new package list specified by PackageList.
    
  Arguments:          
    This              - A pointer to the EFI_HII_DATABASE_PROTOCOL instance.
    Handle            - The handle that was registered to the data that is 
                        requested to be updated.
    PackageList       - A pointer to an EFI_HII_PACKAGE_LIST_HEADER package.
    
  Returns:
    EFI_SUCCESS            - The HII database was successfully updated.
    EFI_OUT_OF_RESOURCES   - Unable to allocate enough memory for the updated database.
    EFI_INVALID_PARAMETER  - PackageList was NULL.
    EFI_NOT_FOUND          - The specified Handle is not in database.
     
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_LIST_PACKS) (
  IN  CONST EFI_HII_DATABASE_PROTOCOL   *This,
  IN  UINT8                             PackageType,
  IN  CONST EFI_GUID                    *PackageGuid,
  IN  OUT UINTN                         *HandleBufferLength,
  OUT EFI_HII_HANDLE                    *Handle
  )
/*++

  Routine Description:
    This function returns a list of the package handles of the specified type 
    that are currently active in the database. The pseudo-type 
    EFI_HII_PACKAGE_TYPE_ALL will cause all package handles to be listed.
    
  Arguments:          
    This               - A pointer to the EFI_HII_DATABASE_PROTOCOL instance.    
    PackageType        - Specifies the package type of the packages to list or
                         EFI_HII_PACKAGE_TYPE_ALL for all packages to be listed.
    PackageGuid        - If PackageType is EFI_HII_PACKAGE_TYPE_GUID, then this 
                         is the pointer to the GUID which must match the Guid
                         field of EFI_HII_GUID_PACKAGE_GUID_HDR. Otherwise, 
                         it must be NULL.                
    HandleBufferLength - On input, a pointer to the length of the handle buffer. 
                         On output, the length of the handle buffer that is
                         required for the handles found.
    Handle             - An array of EFI_HII_HANDLE instances returned.
        
  Returns:
    EFI_SUCCESS            - The matching handles are outputed successfully.
                             HandleBufferLength is updated with the actual length.
    EFI_BUFFER_TO_SMALL    - The HandleBufferLength parameter indicates that
                             Handle is too small to support the number of handles.
                             HandleBufferLength is updated with a value that will 
                             enable the data to fit.
    EFI_NOT_FOUND          - No matching handle could not be found in database.
    EFI_INVALID_PARAMETER  - Handle or HandleBufferLength was NULL.
    EFI_INVALID_PARAMETER  - PackageType is not a EFI_HII_PACKAGE_TYPE_GUID but
                             PackageGuid is not NULL, PackageType is a EFI_HII_
                             PACKAGE_TYPE_GUID but PackageGuid is NULL.
     
--*/  
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_EXPORT_PACKS) (
  IN  CONST EFI_HII_DATABASE_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                    Handle,
  IN  OUT UINTN                         *BufferSize,
  OUT EFI_HII_PACKAGE_LIST_HEADER       *Buffer
  )
/*++

  Routine Description:
    This function will export one or all package lists in the database to a buffer. 
    For each package list exported, this function will call functions registered 
    with EXPORT_PACK and then copy the package list to the buffer.    
    
  Arguments:          
    This               - A pointer to the EFI_HII_DATABASE_PROTOCOL instance.
    Handle             - An EFI_HII_HANDLE that corresponds to the desired package
                         list in the HII database to export or NULL to indicate 
                         all package lists should be exported.
    BufferSize         - On input, a pointer to the length of the buffer. 
                         On output, the length of the buffer that is required for
                         the exported data.
    Buffer             - A pointer to a buffer that will contain the results of 
                         the export function.
                         
  Returns:
    EFI_SUCCESS            - Package exported.
    EFI_BUFFER_TO_SMALL    - The HandleBufferLength parameter indicates that Handle
                             is too small to support the number of handles.     
                             HandleBufferLength is updated with a value that will
                             enable the data to fit.
    EFI_NOT_FOUND          - The specifiecd Handle could not be found in the current
                             database.
    EFI_INVALID_PARAMETER  - Handle or Buffer or BufferSize was NULL.
     
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_REGISTER_NOTIFY) (
  IN  CONST EFI_HII_DATABASE_PROTOCOL   *This,
  IN  UINT8                             PackageType,
  IN  CONST EFI_GUID                    *PackageGuid,
  IN  CONST EFI_HII_DATABASE_NOTIFY     PackageNotifyFn,
  IN  EFI_HII_DATABASE_NOTIFY_TYPE      NotifyType,
  OUT EFI_HANDLE                        *NotifyHandle
  )
/*++

  Routine Description:
    This function registers a function which will be called when specified actions related to packages of
    the specified type occur in the HII database. By registering a function, other HII-related drivers are
    notified when specific package types are added, removed or updated in the HII database.
    Each driver or application which registers a notification should use
    EFI_HII_DATABASE_PROTOCOL.UnregisterPackageNotify() before exiting. 
    
  Arguments:          
    This               - A pointer to the EFI_HII_DATABASE_PROTOCOL instance.    
    PackageType        - Specifies the package type of the packages to list or
                         EFI_HII_PACKAGE_TYPE_ALL for all packages to be listed.    
    PackageGuid        - If PackageType is EFI_HII_PACKAGE_TYPE_GUID, then this is the pointer to
                         the GUID which must match the Guid field of                             
                         EFI_HII_GUID_PACKAGE_GUID_HDR. Otherwise, it must be NULL.          
    PackageNotifyFn    - Points to the function to be called when the event specified by                           
                         NotificationType occurs.
    NotifyType         - Describes the types of notification which this function will be receiving.                     
    NotifyHandle       - Points to the unique handle assigned to the registered notification. Can be used in
                         EFI_HII_DATABASE_PROTOCOL.UnregisterPackageNotify() to stop notifications.                                                                     
                         
  Returns:
    EFI_SUCCESS            - Notification registered successfully.    
    EFI_OUT_OF_RESOURCES   - Unable to allocate necessary data structures    
    EFI_INVALID_PARAMETER  - NotifyHandle is NULL.
    EFI_INVALID_PARAMETER  - PackageGuid is not NULL when PackageType is not
                             EFI_HII_PACKAGE_TYPE_GUID.                     
    EFI_INVALID_PARAMETER  - PackageGuid is NULL when PackageType is EFI_HII_PACKAGE_TYPE_GUID.
     
--*/  
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_UNREGISTER_NOTIFY) (
  IN CONST EFI_HII_DATABASE_PROTOCOL    *This,
  IN EFI_HANDLE                         NotificationHandle
  )
/*++

  Routine Description:
    Removes the specified HII database package-related notification.
    
  Arguments:          
    This               - A pointer to the EFI_HII_DATABASE_PROTOCOL instance.        
    NotifyHandle       - The handle of the notification function being unregistered.                         
                         
  Returns:
    EFI_SUCCESS            - Notification is unregistered successfully.    
    EFI_NOT_FOUND          - The incoming notification handle does not exist 
                             in current hii database.
     
--*/  
;  

typedef
EFI_STATUS
(EFIAPI *EFI_HII_FIND_KEYBOARD_LAYOUTS) (
  IN  EFI_HII_DATABASE_PROTOCOL         *This,
  IN  OUT UINT16                        *KeyGuidBufferLength,
  OUT EFI_GUID                          *KeyGuidBuffer
  )
/*++

  Routine Description:
    This routine retrieves an array of GUID values for each keyboard layout that
    was previously registered in the system.
    
  Arguments:          
    This                - A pointer to the EFI_HII_DATABASE_PROTOCOL instance.
    KeyGuidBufferLength - On input, a pointer to the length of the keyboard GUID 
                          buffer. On output, the length of the handle buffer 
                          that is required for the handles found.
    KeyGuidBuffer       - An array of keyboard layout GUID instances returned.
    
  Returns:
    EFI_SUCCESS            - KeyGuidBuffer was updated successfully.
    EFI_BUFFER_TOO_SMALL   - The KeyGuidBufferLength parameter indicates   
                             that KeyGuidBuffer is too small to support the
                             number of GUIDs. KeyGuidBufferLength is       
                             updated with a value that will enable the data to fit.
    EFI_INVALID_PARAMETER  - The KeyGuidBuffer or KeyGuidBufferLength was NULL.
    EFI_NOT_FOUND          - There was no keyboard layout.

--*/  
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_KEYBOARD_LAYOUT) (
  IN  EFI_HII_DATABASE_PROTOCOL         *This,
  IN  EFI_GUID                          *KeyGuid,
  IN OUT UINT16                         *KeyboardLayoutLength,
  OUT EFI_HII_KEYBOARD_LAYOUT           *KeyboardLayout
  )
/*++

  Routine Description:
    This routine retrieves the requested keyboard layout. The layout is a physical description of the keys
    on a keyboard and the character(s) that are associated with a particular set of key strokes.          
    
  Arguments:          
    This                 - A pointer to the EFI_HII_DATABASE_PROTOCOL instance.            
    KeyGuid              - A pointer to the unique ID associated with a given keyboard layout. If KeyGuid is
                           NULL then the current layout will be retrieved.             
    KeyboardLayoutLength - On input, a pointer to the length of the KeyboardLayout buffer. 
                           On output, the length of the data placed into KeyboardLayout.                         
    KeyboardLayout       - A pointer to a buffer containing the retrieved keyboard layout.                                           
    
  Returns:
    EFI_SUCCESS            - The keyboard layout was retrieved successfully.
    EFI_NOT_FOUND          - The requested keyboard layout was not found.
    EFI_INVALID_PARAMETER  - The KeyboardLayout or KeyboardLayoutLength was NULL.
     
--*/    
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_SET_KEYBOARD_LAYOUT) (
  IN EFI_HII_DATABASE_PROTOCOL          *This,
  IN EFI_GUID                           *KeyGuid
  )
/*++

  Routine Description:
    This routine sets the default keyboard layout to the one referenced by KeyGuid. When this routine  
    is called, an event will be signaled of the EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID                 
    group type. This is so that agents which are sensitive to the current keyboard layout being changed
    can be notified of this change.                                                                    
    
  Arguments:          
    This                - A pointer to the EFI_HII_DATABASE_PROTOCOL instance.            
    KeyGuid             - A pointer to the unique ID associated with a given keyboard layout.                                       
    
  Returns:
    EFI_SUCCESS            - The current keyboard layout was successfully set.
    EFI_NOT_FOUND          - The referenced keyboard layout was not found, so action was taken.                                                     
    EFI_INVALID_PARAMETER  - The KeyGuid was NULL.
     
--*/    
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_GET_PACK_HANDLE) (
  IN  EFI_HII_DATABASE_PROTOCOL         *This,
  IN  EFI_HII_HANDLE                    PackageListHandle,
  OUT EFI_HANDLE                        *DriverHandle
  )
/*++

  Routine Description:
    Return the EFI handle associated with a package list.
    
  Arguments:          
    This                - A pointer to the EFI_HII_DATABASE_PROTOCOL instance.            
    PackageListHandle   - An EFI_HII_HANDLE that corresponds to the desired package list in the
                          HIIdatabase.                                                         
    DriverHandle        - On return, contains the EFI_HANDLE which was registered with the package list in
                          NewPackageList().                                                               
                          
  Returns:
    EFI_SUCCESS            - The DriverHandle was returned successfully.    
    EFI_INVALID_PARAMETER  - The PackageListHandle was not valid or DriverHandle was NULL.
     
--*/    
;

struct _EFI_HII_DATABASE_PROTOCOL {
  EFI_HII_DATABASE_NEW_PACK             NewPackageList;
  EFI_HII_DATABASE_REMOVE_PACK          RemovePackageList;
  EFI_HII_DATABASE_UPDATE_PACK          UpdatePackageList;
  EFI_HII_DATABASE_LIST_PACKS           ListPackageLists;
  EFI_HII_DATABASE_EXPORT_PACKS         ExportPackageLists;
  EFI_HII_DATABASE_REGISTER_NOTIFY      RegisterPackageNotify;
  EFI_HII_DATABASE_UNREGISTER_NOTIFY    UnregisterPackageNotify;
  EFI_HII_FIND_KEYBOARD_LAYOUTS         FindKeyboardLayouts;
  EFI_HII_GET_KEYBOARD_LAYOUT           GetKeyboardLayout;
  EFI_HII_SET_KEYBOARD_LAYOUT           SetKeyboardLayout;
  EFI_HII_DATABASE_GET_PACK_HANDLE      GetPackageListHandle;
};

extern EFI_GUID gEfiHiiDatabaseProtocolGuid;

#endif
