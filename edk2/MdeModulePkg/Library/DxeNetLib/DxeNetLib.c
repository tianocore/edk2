/** @file
  Network library.
  
Copyright (c) 2005 - 2007, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Protocol/ServiceBinding.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/NicIp4Config.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/Dpc.h>

#include <Library/NetLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>

EFI_DPC_PROTOCOL *mDpc = NULL;

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 mNetLibHexStr[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

//
// All the supported IP4 maskes in host byte order.
//
IP4_ADDR  gIp4AllMasks[IP4_MASK_NUM] = {
  0x00000000,
  0x80000000,
  0xC0000000,
  0xE0000000,
  0xF0000000,
  0xF8000000,
  0xFC000000,
  0xFE000000,

  0xFF000000,
  0xFF800000,
  0xFFC00000,
  0xFFE00000,
  0xFFF00000,
  0xFFF80000,
  0xFFFC0000,
  0xFFFE0000,

  0xFFFF0000,
  0xFFFF8000,
  0xFFFFC000,
  0xFFFFE000,
  0xFFFFF000,
  0xFFFFF800,
  0xFFFFFC00,
  0xFFFFFE00,

  0xFFFFFF00,
  0xFFFFFF80,
  0xFFFFFFC0,
  0xFFFFFFE0,
  0xFFFFFFF0,
  0xFFFFFFF8,
  0xFFFFFFFC,
  0xFFFFFFFE,
  0xFFFFFFFF,
};

EFI_IPv4_ADDRESS  mZeroIp4Addr = {{0, 0, 0, 0}};

/**
  Return the length of the mask. 
  
  Return the length of the mask, the correct value is from 0 to 32.
  If the mask is invalid, return the invalid length 33, which is IP4_MASK_NUM.
  NetMask is in the host byte order.

  @param[in]  NetMask              The netmask to get the length from.

  @return The length of the netmask, IP4_MASK_NUM if the mask is invalid.
  
**/
INTN
EFIAPI
NetGetMaskLength (
  IN IP4_ADDR               NetMask
  )
{
  INTN                      Index;

  for (Index = 0; Index < IP4_MASK_NUM; Index++) {
    if (NetMask == gIp4AllMasks[Index]) {
      break;
    }
  }

  return Index;
}



/**
  Return the class of the IP address, such as class A, B, C.
  Addr is in host byte order.
  
  The address of class A  starts with 0.
  If the address belong to class A, return IP4_ADDR_CLASSA.
  The address of class B  starts with 10. 
  If the address belong to class B, return IP4_ADDR_CLASSB.
  The address of class C  starts with 110. 
  If the address belong to class C, return IP4_ADDR_CLASSC.
  The address of class D  starts with 1110. 
  If the address belong to class D, return IP4_ADDR_CLASSD.
  The address of class E  starts with 1111.
  If the address belong to class E, return IP4_ADDR_CLASSE.

  
  @param[in]   Addr                  The address to get the class from.

  @return IP address class, such as IP4_ADDR_CLASSA.

**/
INTN
EFIAPI
NetGetIpClass (
  IN IP4_ADDR               Addr
  )
{
  UINT8                     ByteOne;

  ByteOne = (UINT8) (Addr >> 24);

  if ((ByteOne & 0x80) == 0) {
    return IP4_ADDR_CLASSA;

  } else if ((ByteOne & 0xC0) == 0x80) {
    return IP4_ADDR_CLASSB;

  } else if ((ByteOne & 0xE0) == 0xC0) {
    return IP4_ADDR_CLASSC;

  } else if ((ByteOne & 0xF0) == 0xE0) {
    return IP4_ADDR_CLASSD;

  } else {
    return IP4_ADDR_CLASSE;

  }
}


/**
  Check whether the IP is a valid unicast address according to
  the netmask. If NetMask is zero, use the IP address's class to get the default mask.
  
  If Ip is 0, IP is not a valid unicast address.
  Class D address is used for multicasting and class E address is reserved for future. If Ip
  belongs to class D or class E, IP is not a valid unicast address. 
  If all bits of the host address of IP are 0 or 1, IP is also not a valid unicast address.

  @param[in]  Ip                    The IP to check against.
  @param[in]  NetMask               The mask of the IP.

  @return TRUE if IP is a valid unicast address on the network, otherwise FALSE.

**/
BOOLEAN
EFIAPI
Ip4IsUnicast (
  IN IP4_ADDR               Ip,
  IN IP4_ADDR               NetMask
  )
{
  INTN                      Class;

  Class = NetGetIpClass (Ip);

  if ((Ip == 0) || (Class >= IP4_ADDR_CLASSD)) {
    return FALSE;
  }

  if (NetMask == 0) {
    NetMask = gIp4AllMasks[Class << 3];
  }

  if (((Ip &~NetMask) == ~NetMask) || ((Ip &~NetMask) == 0)) {
    return FALSE;
  }

  return TRUE;
}


/**
  Initialize a random seed using current time.
  
  Get current time first. Then initialize a random seed based on some basic 
  mathematics operation on the hour, day, minute, second, nanosecond and year 
  of the current time.
  
  @return The random seed initialized with current time.

**/
UINT32
EFIAPI
NetRandomInitSeed (
  VOID
  )
{
  EFI_TIME                  Time;
  UINT32                    Seed;

  gRT->GetTime (&Time, NULL);
  Seed = (~Time.Hour << 24 | Time.Day << 16 | Time.Minute << 8 | Time.Second);
  Seed ^= Time.Nanosecond;
  Seed ^= Time.Year << 7;

  return Seed;
}


/**
  Extract a UINT32 from a byte stream.
  
  Copy a UINT32 from a byte stream, then converts it from Network 
  byte order to host byte order. Use this function to avoid alignment error.

  @param[in]  Buf                 The buffer to extract the UINT32.

  @return The UINT32 extracted.

**/
UINT32
EFIAPI
NetGetUint32 (
  IN UINT8                  *Buf
  )
{
  UINT32                    Value;

  CopyMem (&Value, Buf, sizeof (UINT32));
  return NTOHL (Value);
}


/**
  Put a UINT32 to the byte stream in network byte order. 
  
  Converts a UINT32 from host byte order to network byte order. Then copy it to the 
  byte stream.

  @param[in, out]  Buf          The buffer to put the UINT32.
  @param[in]      Data          The data to put.
  
**/
VOID
EFIAPI
NetPutUint32 (
  IN OUT UINT8                 *Buf,
  IN     UINT32                Data
  )
{
  Data = HTONL (Data);
  CopyMem (Buf, &Data, sizeof (UINT32));
}


/**
  Remove the first node entry on the list, and return the removed node entry.
  
  Removes the first node Entry from a doubly linked list. It is up to the caller of
  this function to release the memory used by the first node if that is required. On
  exit, the removed node is returned. 

  If Head is NULL, then ASSERT().
  If Head was not initialized, then ASSERT().
  If PcdMaximumLinkedListLength is not zero, and the number of nodes in the
  linked list including the head node is greater than or equal to PcdMaximumLinkedListLength,
  then ASSERT().    

  @param[in, out]  Head                  The list header.

  @return The first node entry that is removed from the list, NULL if the list is empty.

**/
LIST_ENTRY *
EFIAPI
NetListRemoveHead (
  IN OUT LIST_ENTRY            *Head
  )
{
  LIST_ENTRY            *First;

  ASSERT (Head != NULL);

  if (IsListEmpty (Head)) {
    return NULL;
  }

  First                         = Head->ForwardLink;
  Head->ForwardLink             = First->ForwardLink;
  First->ForwardLink->BackLink  = Head;

  DEBUG_CODE (
    First->ForwardLink  = (LIST_ENTRY *) NULL;
    First->BackLink     = (LIST_ENTRY *) NULL;
  );

  return First;
}


/**
  Remove the last node entry on the list and and return the removed node entry.

  Removes the last node entry from a doubly linked list. It is up to the caller of
  this function to release the memory used by the first node if that is required. On
  exit, the removed node is returned. 

  If Head is NULL, then ASSERT().
  If Head was not initialized, then ASSERT().
  If PcdMaximumLinkedListLength is not zero, and the number of nodes in the
  linked list including the head node is greater than or equal to PcdMaximumLinkedListLength,
  then ASSERT(). 
  
  @param[in, out]  Head                  The list head.

  @return The last node entry that is removed from the list, NULL if the list is empty.

**/
LIST_ENTRY *
EFIAPI
NetListRemoveTail (
  IN OUT LIST_ENTRY            *Head
  )
{
  LIST_ENTRY            *Last;

  ASSERT (Head != NULL);

  if (IsListEmpty (Head)) {
    return NULL;
  }

  Last                        = Head->BackLink;
  Head->BackLink              = Last->BackLink;
  Last->BackLink->ForwardLink = Head;

  DEBUG_CODE (
    Last->ForwardLink = (LIST_ENTRY *) NULL;
    Last->BackLink    = (LIST_ENTRY *) NULL;
  );

  return Last;
}


/**
  Insert a new node entry after a designated node entry of a doubly linked list.
  
  Inserts a new node entry donated by NewEntry after the node entry donated by PrevEntry
  of the doubly linked list.
 
  @param[in, out]  PrevEntry             The previous entry to insert after.
  @param[in, out]  NewEntry              The new entry to insert.

**/
VOID
EFIAPI
NetListInsertAfter (
  IN OUT LIST_ENTRY         *PrevEntry,
  IN OUT LIST_ENTRY         *NewEntry
  )
{
  NewEntry->BackLink                = PrevEntry;
  NewEntry->ForwardLink             = PrevEntry->ForwardLink;
  PrevEntry->ForwardLink->BackLink  = NewEntry;
  PrevEntry->ForwardLink            = NewEntry;
}


/**
  Insert a new node entry before a designated node entry of a doubly linked list.
  
  Inserts a new node entry donated by NewEntry after the node entry donated by PostEntry
  of the doubly linked list.
 
  @param[in, out]  PostEntry             The entry to insert before.
  @param[in, out]  NewEntry              The new entry to insert.

**/
VOID
EFIAPI
NetListInsertBefore (
  IN OUT LIST_ENTRY     *PostEntry,
  IN OUT LIST_ENTRY     *NewEntry
  )
{
  NewEntry->ForwardLink             = PostEntry;
  NewEntry->BackLink                = PostEntry->BackLink;
  PostEntry->BackLink->ForwardLink  = NewEntry;
  PostEntry->BackLink               = NewEntry;
}


/**
  Initialize the netmap. Netmap is a reposity to keep the <Key, Value> pairs.
 
  Initialize the forward and backward links of two head nodes donated by Map->Used 
  and Map->Recycled of two doubly linked lists.
  Initializes the count of the <Key, Value> pairs in the netmap to zero.
   
  If Map is NULL, then ASSERT().
  If the address of Map->Used is NULL, then ASSERT().
  If the address of Map->Recycled is NULl, then ASSERT().
 
  @param[in, out]  Map                   The netmap to initialize.

**/
VOID
EFIAPI
NetMapInit (
  IN OUT NET_MAP                *Map
  )
{
  ASSERT (Map != NULL);

  InitializeListHead (&Map->Used);
  InitializeListHead (&Map->Recycled);
  Map->Count = 0;
}


/**
  To clean up the netmap, that is, release allocated memories.
  
  Removes all nodes of the Used doubly linked list and free memory of all related netmap items.
  Removes all nodes of the Recycled doubly linked list and free memory of all related netmap items.
  The number of the <Key, Value> pairs in the netmap is set to be zero.
  
  If Map is NULL, then ASSERT().
  
  @param[in, out]  Map                   The netmap to clean up.

**/
VOID
EFIAPI
NetMapClean (
  IN OUT NET_MAP            *Map
  )
{
  NET_MAP_ITEM              *Item;
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;

  ASSERT (Map != NULL);

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &Map->Used) {
    Item = NET_LIST_USER_STRUCT (Entry, NET_MAP_ITEM, Link);

    RemoveEntryList (&Item->Link);
    Map->Count--;

    gBS->FreePool (Item);
  }

  ASSERT ((Map->Count == 0) && IsListEmpty (&Map->Used));

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &Map->Recycled) {
    Item = NET_LIST_USER_STRUCT (Entry, NET_MAP_ITEM, Link);

    RemoveEntryList (&Item->Link);
    gBS->FreePool (Item);
  }

  ASSERT (IsListEmpty (&Map->Recycled));
}


/**
  Test whether the netmap is empty and return true if it is.
  
  If the number of the <Key, Value> pairs in the netmap is zero, return TRUE.
   
  If Map is NULL, then ASSERT().
 
  
  @param[in]  Map                   The net map to test.

  @return TRUE if the netmap is empty, otherwise FALSE.

**/
BOOLEAN
EFIAPI
NetMapIsEmpty (
  IN NET_MAP                *Map
  )
{
  ASSERT (Map != NULL);
  return (BOOLEAN) (Map->Count == 0);
}


/**
  Return the number of the <Key, Value> pairs in the netmap.

  @param[in]  Map                   The netmap to get the entry number.

  @return The entry number in the netmap.

**/
UINTN
EFIAPI
NetMapGetCount (
  IN NET_MAP                *Map
  )
{
  return Map->Count;
}


/**
  Return one allocated item. 
  
  If the Recycled doubly linked list of the netmap is empty, it will try to allocate 
  a batch of items if there are enough resources and add corresponding nodes to the begining
  of the Recycled doubly linked list of the netmap. Otherwise, it will directly remove
  the fist node entry of the Recycled doubly linked list and return the corresponding item.
  
  If Map is NULL, then ASSERT().
  
  @param[in, out]  Map          The netmap to allocate item for.

  @return                       The allocated item. If NULL, the
                                allocation failed due to resource limit.

**/
NET_MAP_ITEM *
NetMapAllocItem (
  IN OUT NET_MAP            *Map
  )
{
  NET_MAP_ITEM              *Item;
  LIST_ENTRY                *Head;
  UINTN                     Index;

  ASSERT (Map != NULL);

  Head = &Map->Recycled;

  if (IsListEmpty (Head)) {
    for (Index = 0; Index < NET_MAP_INCREAMENT; Index++) {
      Item = AllocatePool (sizeof (NET_MAP_ITEM));

      if (Item == NULL) {
        if (Index == 0) {
          return NULL;
        }

        break;
      }

      InsertHeadList (Head, &Item->Link);
    }
  }

  Item = NET_LIST_HEAD (Head, NET_MAP_ITEM, Link);
  NetListRemoveHead (Head);

  return Item;
}


/**
  Allocate an item to save the <Key, Value> pair to the head of the netmap.
  
  Allocate an item to save the <Key, Value> pair and add corresponding node entry
  to the beginning of the Used doubly linked list. The number of the <Key, Value> 
  pairs in the netmap increase by 1.

  If Map is NULL, then ASSERT().
  
  @param[in, out]  Map                   The netmap to insert into.
  @param[in]       Key                   The user's key.
  @param[in]       Value                 The user's value for the key.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the memory for the item.
  @retval EFI_SUCCESS           The item is inserted to the head.

**/
EFI_STATUS
EFIAPI
NetMapInsertHead (
  IN OUT NET_MAP            *Map,
  IN VOID                   *Key,
  IN VOID                   *Value    OPTIONAL
  )
{
  NET_MAP_ITEM              *Item;

  ASSERT (Map != NULL);

  Item = NetMapAllocItem (Map);

  if (Item == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Item->Key   = Key;
  Item->Value = Value;
  InsertHeadList (&Map->Used, &Item->Link);

  Map->Count++;
  return EFI_SUCCESS;
}


/**
  Allocate an item to save the <Key, Value> pair to the tail of the netmap.

  Allocate an item to save the <Key, Value> pair and add corresponding node entry
  to the tail of the Used doubly linked list. The number of the <Key, Value> 
  pairs in the netmap increase by 1.

  If Map is NULL, then ASSERT().
  
  @param[in, out]  Map                   The netmap to insert into.
  @param[in]       Key                   The user's key.
  @param[in]       Value                 The user's value for the key.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the memory for the item.
  @retval EFI_SUCCESS           The item is inserted to the tail.

**/
EFI_STATUS
EFIAPI
NetMapInsertTail (
  IN OUT NET_MAP            *Map,
  IN VOID                   *Key,
  IN VOID                   *Value    OPTIONAL
  )
{
  NET_MAP_ITEM              *Item;

  ASSERT (Map != NULL);

  Item = NetMapAllocItem (Map);

  if (Item == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Item->Key   = Key;
  Item->Value = Value;
  InsertTailList (&Map->Used, &Item->Link);

  Map->Count++;

  return EFI_SUCCESS;
}


/**
  Check whether the item is in the Map and return TRUE if it is.

  @param[in]  Map                   The netmap to search within.
  @param[in]  Item                  The item to search.

  @return TRUE if the item is in the netmap, otherwise FALSE.

**/
BOOLEAN
NetItemInMap (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item
  )
{
  LIST_ENTRY            *ListEntry;

  NET_LIST_FOR_EACH (ListEntry, &Map->Used) {
    if (ListEntry == &Item->Link) {
      return TRUE;
    }
  }

  return FALSE;
}


/**
  Find the key in the netmap and returns the point to the item contains the Key.
  
  Iterate the Used doubly linked list of the netmap to get every item. Compare the key of every 
  item with the key to search. It returns the point to the item contains the Key if found.

  If Map is NULL, then ASSERT().
  
  @param[in]  Map                   The netmap to search within.
  @param[in]  Key                   The key to search.

  @return The point to the item contains the Key, or NULL if Key isn't in the map.

**/
NET_MAP_ITEM *
EFIAPI
NetMapFindKey (
  IN  NET_MAP               *Map,
  IN  VOID                  *Key
  )
{
  LIST_ENTRY              *Entry;
  NET_MAP_ITEM            *Item;

  ASSERT (Map != NULL);

  NET_LIST_FOR_EACH (Entry, &Map->Used) {
    Item = NET_LIST_USER_STRUCT (Entry, NET_MAP_ITEM, Link);

    if (Item->Key == Key) {
      return Item;
    }
  }

  return NULL;
}


/**
  Remove the node entry of the item from the netmap and return the key of the removed item.
  
  Remove the node entry of the item from the Used doubly linked list of the netmap. 
  The number of the <Key, Value> pairs in the netmap decrease by 1. Then add the node 
  entry of the item to the Recycled doubly linked list of the netmap. If Value is not NULL,
  Value will point to the value of the item. It returns the key of the removed item.
  
  If Map is NULL, then ASSERT().
  If Item is NULL, then ASSERT().
  if item in not in the netmap, then ASSERT().
  
  @param[in, out]  Map                   The netmap to remove the item from.
  @param[in, out]  Item                  The item to remove.
  @param[out]      Value                 The variable to receive the value if not NULL.

  @return                                The key of the removed item.

**/
VOID *
EFIAPI
NetMapRemoveItem (
  IN  OUT NET_MAP             *Map,
  IN  OUT NET_MAP_ITEM        *Item,
  OUT VOID                    **Value           OPTIONAL
  )
{
  ASSERT ((Map != NULL) && (Item != NULL));
  ASSERT (NetItemInMap (Map, Item));

  RemoveEntryList (&Item->Link);
  Map->Count--;
  InsertHeadList (&Map->Recycled, &Item->Link);

  if (Value != NULL) {
    *Value = Item->Value;
  }

  return Item->Key;
}


/**
  Remove the first node entry on the netmap and return the key of the removed item.

  Remove the first node entry from the Used doubly linked list of the netmap. 
  The number of the <Key, Value> pairs in the netmap decrease by 1. Then add the node 
  entry to the Recycled doubly linked list of the netmap. If parameter Value is not NULL,
  parameter Value will point to the value of the item. It returns the key of the removed item.
  
  If Map is NULL, then ASSERT().
  If the Used doubly linked list is empty, then ASSERT().
  
  @param[in, out]  Map                   The netmap to remove the head from.
  @param[out]      Value                 The variable to receive the value if not NULL.

  @return                                The key of the item removed.

**/
VOID *
EFIAPI
NetMapRemoveHead (
  IN OUT NET_MAP            *Map,
  OUT VOID                  **Value         OPTIONAL
  )
{
  NET_MAP_ITEM  *Item;

  //
  // Often, it indicates a programming error to remove
  // the first entry in an empty list
  //
  ASSERT (Map && !IsListEmpty (&Map->Used));

  Item = NET_LIST_HEAD (&Map->Used, NET_MAP_ITEM, Link);
  RemoveEntryList (&Item->Link);
  Map->Count--;
  InsertHeadList (&Map->Recycled, &Item->Link);

  if (Value != NULL) {
    *Value = Item->Value;
  }

  return Item->Key;
}


/**
  Remove the last node entry on the netmap and return the key of the removed item.

  Remove the last node entry from the Used doubly linked list of the netmap. 
  The number of the <Key, Value> pairs in the netmap decrease by 1. Then add the node 
  entry to the Recycled doubly linked list of the netmap. If parameter Value is not NULL,
  parameter Value will point to the value of the item. It returns the key of the removed item.
  
  If Map is NULL, then ASSERT().
  If the Used doubly linked list is empty, then ASSERT().
  
  @param[in, out]  Map                   The netmap to remove the tail from.
  @param[out]      Value                 The variable to receive the value if not NULL.

  @return                                The key of the item removed.

**/
VOID *
EFIAPI
NetMapRemoveTail (
  IN OUT NET_MAP            *Map,
  OUT VOID                  **Value       OPTIONAL
  )
{
  NET_MAP_ITEM              *Item;

  //
  // Often, it indicates a programming error to remove
  // the last entry in an empty list
  //
  ASSERT (Map && !IsListEmpty (&Map->Used));

  Item = NET_LIST_TAIL (&Map->Used, NET_MAP_ITEM, Link);
  RemoveEntryList (&Item->Link);
  Map->Count--;
  InsertHeadList (&Map->Recycled, &Item->Link);

  if (Value != NULL) {
    *Value = Item->Value;
  }

  return Item->Key;
}


/**
  Iterate through the netmap and call CallBack for each item.
  
  It will contiue the traverse if CallBack returns EFI_SUCCESS, otherwise, break
  from the loop. It returns the CallBack's last return value. This function is 
  delete safe for the current item.

  If Map is NULL, then ASSERT().
  If CallBack is NULL, then ASSERT().
  
  @param[in]  Map                   The Map to iterate through.
  @param[in]  CallBack              The callback function to call for each item.
  @param[in]  Arg                   The opaque parameter to the callback.

  @retval EFI_SUCCESS            There is no item in the netmap or CallBack for each item
                                 return EFI_SUCCESS.
  @retval Others                 It returns the CallBack's last return value.

**/
EFI_STATUS
EFIAPI
NetMapIterate (
  IN NET_MAP                *Map,
  IN NET_MAP_CALLBACK       CallBack,
  IN VOID                   *Arg
  )
{

  LIST_ENTRY            *Entry;
  LIST_ENTRY            *Next;
  LIST_ENTRY            *Head;
  NET_MAP_ITEM          *Item;
  EFI_STATUS            Result;

  ASSERT ((Map != NULL) && (CallBack != NULL));

  Head = &Map->Used;

  if (IsListEmpty (Head)) {
    return EFI_SUCCESS;
  }

  NET_LIST_FOR_EACH_SAFE (Entry, Next, Head) {
    Item   = NET_LIST_USER_STRUCT (Entry, NET_MAP_ITEM, Link);
    Result = CallBack (Map, Item, Arg);

    if (EFI_ERROR (Result)) {
      return Result;
    }
  }

  return EFI_SUCCESS;
}


/**
  This is the default unload handle for all the network drivers.

  Disconnect the driver specified by ImageHandle from all the devices in the handle database.
  Uninstall all the protocols installed in the driver entry point.
  
  @param[in]  ImageHandle       The drivers' driver image.

  @retval EFI_SUCCESS           The image is unloaded.
  @retval Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
NetLibDefaultUnload (
  IN EFI_HANDLE             ImageHandle
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        *DeviceHandleBuffer;
  UINTN                             DeviceHandleCount;
  UINTN                             Index;
  EFI_DRIVER_BINDING_PROTOCOL       *DriverBinding;
  EFI_COMPONENT_NAME_PROTOCOL       *ComponentName;
  EFI_COMPONENT_NAME2_PROTOCOL      *ComponentName2;

  //
  // Get the list of all the handles in the handle database.
  // If there is an error getting the list, then the unload
  // operation fails.
  //
  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &DeviceHandleCount,
                  &DeviceHandleBuffer
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Disconnect the driver specified by ImageHandle from all
  // the devices in the handle database.
  //
  for (Index = 0; Index < DeviceHandleCount; Index++) {
    Status = gBS->DisconnectController (
                    DeviceHandleBuffer[Index],
                    ImageHandle,
                    NULL
                    );
  }

  //
  // Uninstall all the protocols installed in the driver entry point
  //
  for (Index = 0; Index < DeviceHandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    DeviceHandleBuffer[Index],
                    &gEfiDriverBindingProtocolGuid,
                    (VOID **) &DriverBinding
                    );

    if (EFI_ERROR (Status)) {
      continue;
    }

    if (DriverBinding->ImageHandle != ImageHandle) {
      continue;
    }

    gBS->UninstallProtocolInterface (
          ImageHandle,
          &gEfiDriverBindingProtocolGuid,
          DriverBinding
          );
    Status = gBS->HandleProtocol (
                    DeviceHandleBuffer[Index],
                    &gEfiComponentNameProtocolGuid,
                    (VOID **) &ComponentName
                    );
    if (!EFI_ERROR (Status)) {
      gBS->UninstallProtocolInterface (
             ImageHandle,
             &gEfiComponentNameProtocolGuid,
             ComponentName
             );
    }

    Status = gBS->HandleProtocol (
                    DeviceHandleBuffer[Index],
                    &gEfiComponentName2ProtocolGuid,
                    (VOID **) &ComponentName2
                    );
    if (!EFI_ERROR (Status)) {
      gBS->UninstallProtocolInterface (
             ImageHandle,
             &gEfiComponentName2ProtocolGuid,
             ComponentName2
             );
    }
  }

  //
  // Free the buffer containing the list of handles from the handle database
  //
  if (DeviceHandleBuffer != NULL) {
    gBS->FreePool (DeviceHandleBuffer);
  }

  return EFI_SUCCESS;
}



/**
  Create a child of the service that is identified by ServiceBindingGuid.
  
  Get the ServiceBinding Protocol first, then use it to create a child.

  If ServiceBindingGuid is NULL, then ASSERT().
  If ChildHandle is NULL, then ASSERT().
  
  @param[in]       Controller            The controller which has the service installed.
  @param[in]       Image                 The image handle used to open service.
  @param[in]       ServiceBindingGuid    The service's Guid.
  @param[in, out]  ChildHandle           The handle to receive the create child.

  @retval EFI_SUCCESS           The child is successfully created.
  @retval Others                Failed to create the child.

**/
EFI_STATUS
EFIAPI
NetLibCreateServiceChild (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            Image,
  IN  EFI_GUID              *ServiceBindingGuid,
  IN  OUT EFI_HANDLE        *ChildHandle
  )
{
  EFI_STATUS                    Status;
  EFI_SERVICE_BINDING_PROTOCOL  *Service;


  ASSERT ((ServiceBindingGuid != NULL) && (ChildHandle != NULL));

  //
  // Get the ServiceBinding Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  ServiceBindingGuid,
                  (VOID **) &Service,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create a child
  //
  Status = Service->CreateChild (Service, ChildHandle);
  return Status;
}


/**
  Destory a child of the service that is identified by ServiceBindingGuid.
  
  Get the ServiceBinding Protocol first, then use it to destroy a child.
  
  If ServiceBindingGuid is NULL, then ASSERT().
  
  @param[in]   Controller            The controller which has the service installed.
  @param[in]   Image                 The image handle used to open service.
  @param[in]   ServiceBindingGuid    The service's Guid.
  @param[in]   ChildHandle           The child to destory.

  @retval EFI_SUCCESS           The child is successfully destoried.
  @retval Others                Failed to destory the child.

**/
EFI_STATUS
EFIAPI
NetLibDestroyServiceChild (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            Image,
  IN  EFI_GUID              *ServiceBindingGuid,
  IN  EFI_HANDLE            ChildHandle
  )
{
  EFI_STATUS                    Status;
  EFI_SERVICE_BINDING_PROTOCOL  *Service;

  ASSERT (ServiceBindingGuid != NULL);

  //
  // Get the ServiceBinding Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  ServiceBindingGuid,
                  (VOID **) &Service,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // destory the child
  //
  Status = Service->DestroyChild (Service, ChildHandle);
  return Status;
}


/**
  Convert the mac address of the simple network protocol installed on
  SnpHandle to a unicode string. Callers are responsible for freeing the
  string storage.

  Get the mac address of the Simple Network protocol from the SnpHandle. Then convert
  the mac address into a unicode string. It takes 2 unicode characters to represent 
  a 1 byte binary buffer. Plus one unicode character for the null-terminator.


  @param[in]   SnpHandle             The handle where the simple network protocol is
                                     installed on.
  @param[in]   ImageHandle           The image handle used to act as the agent handle to
                                     get the simple network protocol.
  @param[out]  MacString             The pointer to store the address of the string
                                     representation of  the mac address.
  
  @retval EFI_SUCCESS           Convert the mac address a unicode string successfully.
  @retval EFI_OUT_OF_RESOURCES  There are not enough memory resource.
  @retval Others                Failed to open the simple network protocol.

**/
EFI_STATUS
EFIAPI
NetLibGetMacString (
  IN  EFI_HANDLE            SnpHandle,
  IN  EFI_HANDLE            ImageHandle,
  OUT CHAR16                **MacString
  )
{
  EFI_STATUS                   Status;
  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;
  EFI_SIMPLE_NETWORK_MODE      *Mode;
  CHAR16                       *MacAddress;
  UINTN                        Index;

  *MacString = NULL;

  //
  // Get the Simple Network protocol from the SnpHandle.
  //
  Status = gBS->OpenProtocol (
                  SnpHandle,
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **) &Snp,
                  ImageHandle,
                  SnpHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Mode = Snp->Mode;

  //
  // It takes 2 unicode characters to represent a 1 byte binary buffer.
  // Plus one unicode character for the null-terminator.
  //
  MacAddress = AllocatePool ((2 * Mode->HwAddressSize + 1) * sizeof (CHAR16));
  if (MacAddress == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Convert the mac address into a unicode string.
  //
  for (Index = 0; Index < Mode->HwAddressSize; Index++) {
    MacAddress[Index * 2]     = (CHAR16) mNetLibHexStr[(Mode->CurrentAddress.Addr[Index] >> 4) & 0x0F];
    MacAddress[Index * 2 + 1] = (CHAR16) mNetLibHexStr[Mode->CurrentAddress.Addr[Index] & 0x0F];
  }

  MacAddress[Mode->HwAddressSize * 2] = L'\0';

  *MacString = MacAddress;

  return EFI_SUCCESS;
}

/**
  Check the default address used by the IPv4 driver is static or dynamic (acquired
  from DHCP).

  If the controller handle does not have the NIC Ip4 Config Protocol installed, the 
  default address is static. If the EFI variable to save the configuration is not found,
  the default address is static. Otherwise, get the result from the EFI variable which 
  saving the configuration.
   
  @param[in]   Controller     The controller handle which has the NIC Ip4 Config Protocol
                              relative with the default address to judge.

  @retval TRUE           If the default address is static.
  @retval FALSE          If the default address is acquired from DHCP.

**/
BOOLEAN
NetLibDefaultAddressIsStatic (
  IN EFI_HANDLE  Controller
  )
{
  EFI_STATUS                   Status;
  EFI_NIC_IP4_CONFIG_PROTOCOL  *NicIp4;
  UINTN                        Len;
  NIC_IP4_CONFIG_INFO          *ConfigInfo;
  BOOLEAN                      IsStatic;

  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiNicIp4ConfigProtocolGuid,
                  (VOID **) &NicIp4
                  );
  if (EFI_ERROR (Status)) {
    return TRUE;
  }

  Len = 0;
  Status = NicIp4->GetInfo (NicIp4, &Len, NULL);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return TRUE;
  }

  ConfigInfo = AllocatePool (Len);
  if (ConfigInfo == NULL) {
    return TRUE;
  }

  IsStatic = TRUE;
  Status = NicIp4->GetInfo (NicIp4, &Len, ConfigInfo);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  IsStatic = (BOOLEAN) (ConfigInfo->Source == IP4_CONFIG_SOURCE_STATIC);

ON_EXIT:

  gBS->FreePool (ConfigInfo);

  return IsStatic;
}

/**
  Create an IPv4 device path node.
  
  The header type of IPv4 device path node is MESSAGING_DEVICE_PATH.
  The header subtype of IPv4 device path node is MSG_IPv4_DP.
  The length of the IPv4 device path node in bytes is 19.
  Get other info from parameters to make up the whole IPv4 device path node.

  @param[in, out]  Node                  Pointer to the IPv4 device path node.
  @param[in]       Controller            The handle where the NIC IP4 config protocol resides.
  @param[in]       LocalIp               The local IPv4 address.
  @param[in]       LocalPort             The local port.
  @param[in]       RemoteIp              The remote IPv4 address.
  @param[in]       RemotePort            The remote port.
  @param[in]       Protocol              The protocol type in the IP header.
  @param[in]       UseDefaultAddress     Whether this instance is using default address or not.

**/
VOID
EFIAPI
NetLibCreateIPv4DPathNode (
  IN OUT IPv4_DEVICE_PATH  *Node,
  IN EFI_HANDLE            Controller,
  IN IP4_ADDR              LocalIp,
  IN UINT16                LocalPort,
  IN IP4_ADDR              RemoteIp,
  IN UINT16                RemotePort,
  IN UINT16                Protocol,
  IN BOOLEAN               UseDefaultAddress
  )
{
  Node->Header.Type    = MESSAGING_DEVICE_PATH;
  Node->Header.SubType = MSG_IPv4_DP;
  SetDevicePathNodeLength (&Node->Header, 19);

  CopyMem (&Node->LocalIpAddress, &LocalIp, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&Node->RemoteIpAddress, &RemoteIp, sizeof (EFI_IPv4_ADDRESS));

  Node->LocalPort  = LocalPort;
  Node->RemotePort = RemotePort;

  Node->Protocol = Protocol;

  if (!UseDefaultAddress) {
    Node->StaticIpAddress = TRUE;
  } else {
    Node->StaticIpAddress = NetLibDefaultAddressIsStatic (Controller);
  }
}


/**
  Find the UNDI/SNP handle from controller and protocol GUID.
  
  For example, IP will open a MNP child to transmit/receive
  packets, when MNP is stopped, IP should also be stopped. IP
  needs to find its own private data which is related the IP's
  service binding instance that is install on UNDI/SNP handle.
  Now, the controller is either a MNP or ARP child handle. But
  IP opens these handle BY_DRIVER, use that info, we can get the
  UNDI/SNP handle.

  @param[in]  Controller            Then protocol handle to check.
  @param[in]  ProtocolGuid          The protocol that is related with the handle.

  @return The UNDI/SNP handle or NULL for errors.

**/
EFI_HANDLE
EFIAPI
NetLibGetNicHandle (
  IN EFI_HANDLE             Controller,
  IN EFI_GUID               *ProtocolGuid
  )
{
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenBuffer;
  EFI_HANDLE                          Handle;
  EFI_STATUS                          Status;
  UINTN                               OpenCount;
  UINTN                               Index;

  Status = gBS->OpenProtocolInformation (
                  Controller,
                  ProtocolGuid,
                  &OpenBuffer,
                  &OpenCount
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Handle = NULL;

  for (Index = 0; Index < OpenCount; Index++) {
    if (OpenBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) {
      Handle = OpenBuffer[Index].ControllerHandle;
      break;
    }
  }

  gBS->FreePool (OpenBuffer);
  return Handle;
}

/**
  Add a Deferred Procedure Call to the end of the DPC queue.

  @param[in]  DpcTpl           The EFI_TPL that the DPC should be invoked.
  @param[in]  DpcProcedure     Pointer to the DPC's function.
  @param[in]  DpcContext       Pointer to the DPC's context.  Passed to DpcProcedure
                               when DpcProcedure is invoked.

  @retval  EFI_SUCCESS              The DPC was queued.
  @retval  EFI_INVALID_PARAMETER    DpcTpl is not a valid EFI_TPL, or DpcProcedure
                                    is NULL.
  @retval  EFI_OUT_OF_RESOURCES     There are not enough resources available to
                                    add the DPC to the queue.

**/
EFI_STATUS
EFIAPI
NetLibQueueDpc (
  IN EFI_TPL            DpcTpl,
  IN EFI_DPC_PROCEDURE  DpcProcedure,
  IN VOID               *DpcContext    OPTIONAL
  )
{
  return mDpc->QueueDpc (mDpc, DpcTpl, DpcProcedure, DpcContext);
}

/**
  Dispatch the queue of DPCs. ALL DPCs that have been queued with a DpcTpl
  value greater than or equal to the current TPL are invoked in the order that
  they were queued.  DPCs with higher DpcTpl values are invoked before DPCs with
  lower DpcTpl values.

  @retval  EFI_SUCCESS              One or more DPCs were invoked.
  @retval  EFI_NOT_FOUND            No DPCs were invoked.

**/
EFI_STATUS
EFIAPI
NetLibDispatchDpc (
  VOID
  )
{
  return mDpc->DispatchDpc(mDpc);
}

/**
  The constructor function caches the pointer to DPC protocol.

  The constructor function locates DPC protocol from protocol database.
  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
NetLibConstructor (
  IN EFI_HANDLE                ImageHandle,
  IN EFI_SYSTEM_TABLE          *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiDpcProtocolGuid, NULL, (VOID**) &mDpc);
  ASSERT_EFI_ERROR (Status);
  ASSERT (mDpc != NULL);

  return Status;
}
