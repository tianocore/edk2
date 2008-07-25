/** @file
  
  EFI ARP Protocol Definition
  
  The EFI ARP Service Binding Protocol is used to locate EFI
  ARP Protocol drivers to create and destroy child of the
  driver to communicate with other host using ARP protocol.
  
  The EFI ARP Protocol provides services to map IP network
  address to hardware address used by a data link protocol.
  
  
  Copyright (c) 2006 - 2008, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
  
**/

#ifndef __EFI_ARP_PROTOCOL_H__
#define __EFI_ARP_PROTOCOL_H__

#define EFI_ARP_SERVICE_BINDING_PROTOCOL_GUID \
  { \
    0xf44c00ee, 0x1f2c, 0x4a00, {0xaa, 0x9, 0x1c, 0x9f, 0x3e, 0x8, 0x0, 0xa3 } \
  }

#define EFI_ARP_PROTOCOL_GUID \
  { \
    0xf4b427bb, 0xba21, 0x4f16, {0xbc, 0x4e, 0x43, 0xe4, 0x16, 0xab, 0x61, 0x9c } \
  }

typedef struct _EFI_ARP_PROTOCOL EFI_ARP_PROTOCOL;

typedef struct {
UINT32                      Size;
BOOLEAN                     DenyFlag;
BOOLEAN                     StaticFlag;
UINT16                      HwAddressType;
UINT16                      SwAddressType;
UINT8                       HwAddressLength;
UINT8                       SwAddressLength;
} EFI_ARP_FIND_DATA;

typedef struct {
  UINT16                    SwAddressType;      // Host byte order
  UINT8                     SwAddressLength;
  VOID                      *StationAddress;    // Network byte order
  UINT32                    EntryTimeOut;
  UINT32                    RetryCount;
  UINT32                    RetryTimeOut;
} EFI_ARP_CONFIG_DATA;


/**
  Assigns a station address (protocol type and network address) to this instance of the ARP cache.

  @param  This                  A pointer to the EFI_ARP_PROTOCOL instance.
  @param  ConfigData            A pointer to the EFI_ARP_CONFIG_DATA structure.Buffer

  @retval EFI_SUCCESS           The new station address was successfully registered.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
  @retval EFI_ACCESS_DENIED     The SwAddressType, SwAddressLength, or
                                StationAddress is different from the one that is already
                                registered.
  @retval EFI_OUT_OF_RESOURCES  Storage for the new StationAddress could not be allocated.

**/
typedef 
EFI_STATUS
(EFIAPI *EFI_ARP_CONFIGURE)(
  IN EFI_ARP_PROTOCOL       *This,
  IN EFI_ARP_CONFIG_DATA    *ConfigData   OPTIONAL
  )
;  

/**
  Inserts an entry to the ARP cache.

  @param  This            A pointer to the EFI_ARP_PROTOCOL instance.  
  @param  DenyFlag        Set to TRUE if this entry is a "deny" entry. Set to FALSE if this
                          entry is a "normal" entry.
  @param  TargetSwAddress Pointer to a protocol address to add (or deny). May be set to
                          NULL if DenyFlag is TRUE.
  @param  TargetHwAddress Pointer to a hardware address to add (or deny). May be set to
                          NULL if DenyFlag is TRUE.
  @param  TimeoutValue    Time in 100-ns units that this entry will remain in the ARP
                          cache. A value of zero means that the entry is permanent. A
                          nonzero value will override the one given by Configure() if
                          the entry to be added is dynamic entry.
  @param  Overwrite       If TRUE, the matching cache entry will be overwritten with the
                          supplied parameters. If FALSE, EFI_ACCESS_DENIED is returned 
                          if the corresponding cache entry already exists.

  @retval EFI_SUCCESS           The entry has been added or updated.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                This is NULL. DenyFlag is FALSE and TargetHwAddress is NULL.
                                DenyFlag is FALSE and TargetSwAddress is NULL. TargetHwAddress is NULL and TargetSwAddress is NULL. 
                                Both TargetSwAddress and TargetHwAddress are not NULL when DenyFlag is TRUE.
  @retval EFI_OUT_OF_RESOURCES  The new ARP cache entry could not be allocated.
  @retval EFI_ACCESS_DENIED     The ARP cache entry already exists and Overwrite is not true.
  @retval EFI_NOT_STARTED       The ARP driver instance has not been configured.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ARP_ADD)(
  IN EFI_ARP_PROTOCOL       *This,
  IN BOOLEAN                DenyFlag,
  IN VOID                   *TargetSwAddress  OPTIONAL,
  IN VOID                   *TargetHwAddress  OPTIONAL,
  IN UINT32                 TimeoutValue,
  IN BOOLEAN                Overwrite
  )
;  

/**
  Locates one or more entries in the ARP cache.

  @param  This            A pointer to the EFI_ARP_PROTOCOL instance.
  @param  BySwAddress     Set to TRUE to look for matching software protocol addresses.
                          Set to FALSE to look for matching hardware protocol addresses.
  @param  AddressBuffer   Pointer to address buffer. Set to NULL to match all addresses.
  @param  EntryLength     The size of an entry in the entries buffer. To keep the
                          EFI_ARP_FIND_DATA structure properly aligned, this field
                          may be longer than sizeof(EFI_ARP_FIND_DATA) plus
                          the length of the software and hardware addresses.
  @param  EntryCount      The number of ARP cache entries that are found by the specified criteria.
  @param  Entries         Pointer to the buffer that will receive the ARP cache entries.
  @param  Refresh         Set to TRUE to refresh the timeout value of the matching ARP
                          cache entry.

  @retval EFI_SUCCESS           The requested ARP cache entries were copied into the buffer.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                This is NULL. Both EntryCount and EntryLength are NULL, 
                                when Refresh is FALSE.
  @retval EFI_NOT_FOUND         No matching entries were found.
  @retval EFI_NOT_STARTED       The ARP driver instance has not been configured.

**/
typedef 
EFI_STATUS
(EFIAPI *EFI_ARP_FIND)(
  IN EFI_ARP_PROTOCOL       *This,
  IN BOOLEAN                BySwAddress,
  IN VOID                   *AddressBuffer    OPTIONAL,
  OUT UINT32                *EntryLength      OPTIONAL,
  OUT UINT32                *EntryCount       OPTIONAL,
  OUT EFI_ARP_FIND_DATA     **Entries         OPTIONAL,
  IN BOOLEAN                Refresh
  )
;  


/**
  Removes entries from the ARP cache.

  @param  This          A pointer to the EFI_ARP_PROTOCOL instance.
  @param  BySwAddress   Set to TRUE to delete matching protocol addresses.
                        Set to FALSE to delete matching hardware addresses.
  @param  AddressBuffer Pointer to the address buffer that is used as a key to look for the
                        cache entry. Set to NULL to delete all entries.

  @retval EFI_SUCCESS           The entry was removed from the ARP cache.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_NOT_FOUND         The specified deletion key was not found.
  @retval EFI_NOT_STARTED       The ARP driver instance has not been configured.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ARP_DELETE)(
  IN EFI_ARP_PROTOCOL       *This,
  IN BOOLEAN                BySwAddress,
  IN VOID                   *AddressBuffer   OPTIONAL
  )
;  

/**
  Removes all dynamic ARP cache entries that were added by this interface.

  @param  This                  A pointer to the EFI_ARP_PROTOCOL instance.
                                 
  @retval EFI_SUCCESS           The cache has been flushed.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_NOT_FOUND         There are no matching dynamic cache entries.
  @retval EFI_NOT_STARTED       The ARP driver instance has not been configured.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ARP_FLUSH)(
  IN EFI_ARP_PROTOCOL       *This
  )
;  

/**
  Starts an ARP request session.

  @param  This            A pointer to the EFI_ARP_PROTOCOL instance.
  @param  TargetSwAddress Pointer to the protocol address to resolve.
  @param  ResolvedEvent   Pointer to the event that will be signaled when the address is
                          resolved or some error occurs.
  @param  TargetHwAddress Pointer to the buffer for the resolved hardware address in
                          network byte order. The buffer must be large enough to hold the
                          resulting hardware address. TargetHwAddress must not be
                          NULL.

  @retval EFI_SUCCESS           The data was copied from the ARP cache into the
                                TargetHwAddress buffer.
  @retval EFI_INVALID_PARAMETER This or TargetHwAddress is NULL.
  @retval EFI_ACCESS_DENIED     The requested address is not present in the normal ARP cache but
                                is present in the deny address list. Outgoing traffic to that address is
                                forbidden.
  @retval EFI_NOT_STARTED       The ARP driver instance has not been configured.
  @retval EFI_NOT_READY         The request has been started and is not finished.
  @retval EFI_UNSUPPORTED       The requested conversion is not supported in this implementation or
                                configuration.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ARP_REQUEST)(
  IN EFI_ARP_PROTOCOL       *This, 
  IN VOID                   *TargetSwAddress  OPTIONAL,
  IN EFI_EVENT              ResolvedEvent     OPTIONAL,
  OUT VOID                  *TargetHwAddress  
  )
;  

/**
  Cancels an ARP request session.

  @param  This            A pointer to the EFI_ARP_PROTOCOL instance.
  @param  TargetSwAddress Pointer to the protocol address in previous request session.
  @param  ResolvedEvent   Pointer to the event that is used as the notification event in
                          previous request session.

  @retval EFI_SUCCESS           The pending request session(s) is/are aborted and corresponding
                                event(s) is/are signaled.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
  @retval EFI_NOT_STARTED       The ARP driver instance has not been configured.
  @retval EFI_NOT_FOUND         The request is not issued by
                                EFI_ARP_PROTOCOL.Request().

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ARP_CANCEL)(
  IN EFI_ARP_PROTOCOL       *This, 
  IN VOID                   *TargetSwAddress  OPTIONAL,
  IN EFI_EVENT              ResolvedEvent     OPTIONAL
  )
;  

/**
  @par Protocol Description:
  ARP is used to resolve local network protocol addresses into 
  network hardware addresses.

  @param Configure
  Adds a new station address (protocol type and network address) to the ARP cache.
  
  @param Add
  Manually inserts an entry to the ARP cache for administrative purpose.

  @param Find
  Locates one or more entries in the ARP cache.

  @param Delete
  Removes an entry from the ARP cache.
  
  @param Flush
  Removes all dynamic ARP cache entries of a specified protocol type.
  
  @param Request
  Starts an ARP request session.
  
  @param Cancel
  Abort previous ARP request session.

**/
struct _EFI_ARP_PROTOCOL {
  EFI_ARP_CONFIGURE         Configure;
  EFI_ARP_ADD               Add;
  EFI_ARP_FIND              Find;
  EFI_ARP_DELETE            Delete;
  EFI_ARP_FLUSH             Flush;
  EFI_ARP_REQUEST           Request;
  EFI_ARP_CANCEL            Cancel;
};


extern EFI_GUID gEfiArpServiceBindingProtocolGuid;
extern EFI_GUID gEfiArpProtocolGuid;

#endif
