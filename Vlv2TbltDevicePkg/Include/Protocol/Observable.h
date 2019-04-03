/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  Observable.h

Abstract:

  Interface and GUID definitions for Observable protocol.

**/

#ifndef _OBSERVABLE_PROTOCOL_H_
#define _OBSERVABLE_PROTOCOL_H_

//
// GUID Definitions
//
#define OBSERVABLE_PROTOCOL_GUID \
  { \
    0xe227c522, 0xd5fe, 0x4a53, 0x87, 0xb1, 0x0f, 0xbe, 0x57, 0x0f, 0x98, 0xe9 \
  }

extern EFI_GUID gObservableProtocolGuid;

typedef struct _OBS_OBSERVABLE_PROTOCOL OBS_OBSERVABLE_PROTOCOL;

//
// Interface Definitions
//

/**
  Remove all observables.

  Remove all observable guids and all interfaces subscribed to them.

  @param   VOID          No Parameters.

  @return  EFI_SUCCESS   Successfully removed all observables and subscribed interfaces.

**/
typedef
EFI_STATUS
(EFIAPI *OBS_REMOVE_ALL_OBSERVABLES) (
  VOID
  );

/**
  Interface for notification functions.

  Functions that are to be used as callbacks must inherit this interface in order to be used properly.

  @param   VOID*   Data  Parameter context to be passed to the notification function.

  @return  EFI_STATUS    Varies depending on implementation.

**/
typedef
EFI_STATUS
(EFIAPI *OBS_CALLBACK) (
  IN  OUT VOID*   Data
  );

/**
  Subscribe an interface with an observable guid.

  Use this to register a callback function with a guid. The function provided by CallbackInterface will be executed
  whenever the appropriate observable instance specified by ReferenceGuid calls Publish.

  @param   EFI_GUID                ReferenceGuid       The observable guid that the callback interface will subscribe to.
           OBS_NOTIFY_INTERFACE    CallbackInterface   A pointer to the function that is subscribing to the observable.

  @return  EFI_SUCCESS           Successfully subscribed the interface to the observable guid.
           EFI_NOT_FOUND         No match could be found between the provided guid and existing observables.
           EFI_OUT_OF_RESOURCES  Could not subscribe to this observer due to resource limitations.
           EFI_INVALID_PARAMETER Interface is already subscribed to this observer.
**/
typedef
EFI_STATUS
(EFIAPI *OBS_SUBSCRIBE) (
  IN      EFI_GUID        ReferenceGuid,
  IN      OBS_CALLBACK    CallbackInterface
  );

/**
  Unsubscribe an interface with an observable guid.

  Use this to remove an interface from the callback list associated with an observable guid.

  @param   EFI_GUID                ReferenceGuid   The observable guid to unsubscribe the interface from.
           OBS_NOTIFY_INTERFACE    NotifyCallback  A pointer to the interface that is being unsubscribed.

  @return  EFI_SUCCESS           Successfully unsubscribed the interface from the observable guid.

**/
typedef
EFI_STATUS
(EFIAPI *OBS_UNSUBSCRIBE) (
  IN      EFI_GUID        ReferenceGuid,
  IN      OBS_CALLBACK    CallbackInterface
  );

/**
  Notify observing functions.

  Use this to notify all functions who are subscribed to the guid specified by ReferenceGuid.

  @param   EFI_GUID          ReferenceGuid   The observable guid that contains the the list of interfaces to be notified.
           VOID*             Data            Parameter context to be passed to the notification function.

  @return  EFI_SUCCESS       Successfully notified all observers listening to this guid.
           EFI_NOT_FOUND     No match could be found between the provided guid and existing observables.

**/
typedef
EFI_STATUS
(EFIAPI *OBS_PUBLISH) (
  IN      EFI_GUID        ReferenceGuid,
  IN  OUT VOID*           Data
  );

/**
  Creates a new observable.

  Create a new observable that can be observed with the use of Subscribe function.

  @param   EFI_GUID              ReferenceGuid   The observable guid to add.

  @return  EFI_SUCCESS           Successfully added observable.
           EFI_INVALID_PARAMETER Observable already exists.

**/
typedef
EFI_STATUS
(EFIAPI *OBS_ADD_OBSERVABLE) (
  IN      EFI_GUID        ReferenceGuid
  );

/**
  Remove an observable.

  Remove an observable so that it can no longer be subscribed to. In addition, unsubscribe any functions
  that are subscribed to this guid.

  @param   EFI_GUID              ReferenceGuid   The observable guid to remove.

  @return  EFI_SUCCESS           Successfully removed observable.

**/
typedef
EFI_STATUS
(EFIAPI *OBS_REMOVE_OBSERVABLE) (
  IN      EFI_GUID        ReferenceGuid
  );

//
// Protocol Definitions
//
typedef struct _OBS_LEAF {
  OBS_CALLBACK      Observer;
  struct _OBS_LEAF* Next;
} OBS_LEAF;

typedef struct _OBS_TREE {
  EFI_GUID              ObservableGuid;
  OBS_LEAF*             Leaf;
  struct _OBS_TREE*     Next;
} OBS_TREE;

struct _OBS_OBSERVABLE_PROTOCOL {
  OBS_ADD_OBSERVABLE          AddObservable;
  OBS_REMOVE_OBSERVABLE       RemoveObservable;
  OBS_SUBSCRIBE               Subscribe;
  OBS_UNSUBSCRIBE             Unsubscribe;
  OBS_PUBLISH                 Publish;
  OBS_REMOVE_ALL_OBSERVABLES  RemoveAllObservables;
} ;

#endif
