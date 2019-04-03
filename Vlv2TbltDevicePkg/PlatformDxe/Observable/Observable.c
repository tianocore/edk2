/*++
  This file contains 'Framework Code' and is licensed as such
  under the terms of your license agreement with Intel or your
  vendor.  This file may not be modified, except as allowed by
  additional terms of your license agreement.
--*/
/*++

Copyright (c)  2010  - 2014, Intel Corporation. All rights reserved
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  Observable.c

Abstract:

  The following contains all of the implementation for the Observable protocol. The
  protocol uses the observer design pattern to provide a way to publish events and
  to subscribe to those events so that a callback will be performed at the time of
  the event. The observables and subscribers are maintained by the static tree,
  mObservableDb. The difference between this protocol and the existing event protocol
  that exists within the EFI framework is that this protocol allows for parameters
  to be passed to the subscribed callbacks that can contain up to date context.

--*/

#include "Observable.h"

static OBS_TREE*                mObservableDb = NULL;
static EFI_HANDLE               mObservableHandle = NULL;
static OBS_OBSERVABLE_PROTOCOL  mObservable = {
  AddObservable,
  RemoveObservable,
  Subscribe,
  Unsubscribe,
  Publish,
  RemoveAllObservables
};

/** Install observable protocol.
 *
 * Install interface and initialize the observable protocol.
 *
 * @param   VOID          No parameters.
 *
 * @return  EFI_SUCCESS   Successfully installed and initialized the protocol.
 **/
EFI_STATUS
InitializeObservableProtocol(
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Install protocol.
  //
  Status = gBS->InstallProtocolInterface (
                  &mObservableHandle,
                  &gObservableProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mObservable
                  );

  return Status;
}

/** Deletes a subscriber
 *
 * This function removes the subscriber pointed to by Head.
 *
 * @param   OBS_TREE*     Head    Points to the current subscriber.
 *
 * @return  OBS_TREE*     Returns the tree after successfully removing the subscriber.
 **/
OBS_LEAF*
DeleteSubscriber(
  OBS_LEAF* Head
  )
{
  OBS_LEAF* Temp;

  if (Head) {
    Temp = Head;
    Head = Head->Next;
    gBS->FreePool(Temp);
  }

  return Head;
}

/** Finds and deletes all subscribers
 *
 * This function iterates recursively through the existing subscribers and delets them all.
 *
 * @param   OBS_TREE*     Head    Points to the current subscriber.
 *
 * @return  OBS_TREE*     Returns the tree after successfully removing the subscribers.
 **/
OBS_LEAF*
DeleteAllSubscribers(
  OBS_LEAF* Head
  )
{
  if (Head) {
    if (Head->Next) {
      //
      // We aren't at the end of the list yet.
      //
      Head->Next = DeleteAllSubscribers(Head->Next);
    }

    //
    // At the end, so delete the subscriber.
    //
    Head = DeleteSubscriber(Head);
  }

  return Head;
}

/** Deletes an observable
 *
 * This function removes the observable pointed to by Head.
 *
 * @param   OBS_TREE*     Head    Points to the current observable.
 *
 * @return  OBS_TREE*     Returns the tree after successfully removing the observable.
 **/
OBS_TREE*
DeleteObservable(
  OBS_TREE* Head
  )
{
  OBS_TREE* Temp;

  if (Head) {
    Temp = Head;
    Head = Head->Next;
    gBS->FreePool(Temp);
  }

  return Head;
}

/** Finds and deletes all observables
 *
 * This function iterates recursively through the existing observables database and, starting with
 * the last most observable, deletes all of its subscribers, then deletes the observable itself.
 *
 * @param   OBS_TREE*     Head    Points to the current observable.
 *
 * @return  OBS_TREE*     Returns the tree after successfully removing the observables.
 **/
OBS_TREE*
DeleteAllObservables(
  OBS_TREE* Head
  )
{
  if (Head) {
    if (Head->Next) {
      //
      // We aren't at the end of the list yet.
      //
      Head->Next = DeleteAllObservables(Head->Next);
    }

    //
    // This is the end of the list of observables.
    //
    Head->Leaf = DeleteAllSubscribers(Head->Leaf);

    //
    // Subscribers are deleted, so now delete the observable.
    //
    Head = DeleteObservable(Head);
  }

  return Head;
}

/** Finds and deletes observable
 *
 * This function iterates recursively through the existing observable database in order to find the one
 * specified by ReferenceGuid so that it can be deleted. If the requested observable is found, before it
 * is deleted, all of the subscribers that are listening to this observable are deleted.
 *
 * @param   OBS_TREE*     Head              Points to the current observable.
 *          EFI_GUID      ReferenceGuid     Corresponds to the observable that we're looking for.
 *
 * @return  OBS_TREE*     Returns the tree after successfully removing (or not finding) the observable.
 **/
OBS_TREE*
FindAndDeleteObservable(
  OBS_TREE* Head,
  EFI_GUID  ReferenceGuid
  )
{
  if (Head) {
    if (CompareMem(&(Head->ObservableGuid), &ReferenceGuid, sizeof(ReferenceGuid)) == 0) {
      //
      // We found the observable. Delete all of it's subscribers, first.
      //
      Head->Leaf = DeleteAllSubscribers(Head->Leaf);
      //
      // Now we can safely remove the observable.
      //
      Head = DeleteObservable(Head);
    } else {
      //
      // Not found. Keep searching.
      //
      Head->Next = FindAndDeleteObservable(Head->Next, ReferenceGuid);
    }
  }

  return Head;
}

/** Finds and deletes subscriber
 *
 * This function iterates recursively through the existing subscribers that are listening to the
 * observable that was found when this function was called.
 *
 * @param   OBS_TREE*     Head              Points to the current subscriber.
 *          OBS_CALLBACK  CallbackInterface This is the subscriber that is requested be removed.
 *
 * @return  OBS_TREE*     Returns the tree after successfully removing (or not finding) the subscriber.
 **/
OBS_LEAF*
_FindAndDeleteSubscriber(
  OBS_LEAF*     Head,
  OBS_CALLBACK  CallbackInterface
  )
{
  if (Head) {
    if (Head->Observer == CallbackInterface) {
      //
      // Found it. Now let's delete it.
      //
      Head = DeleteSubscriber(Head);
    } else {
      //
      // Not found. Keep searching.
      //
      Head->Next = _FindAndDeleteSubscriber(Head->Next, CallbackInterface);
    }
  }

  return Head;
}

/** Finds and deletes subscriber
 *
 * This function iterates recursively through the existing observables database until it either finds
 * a matching guid or reaches the end of the list. After finding a match, it calls a helper function,
 * _FindAndDeleteSubscriber. At this point, all responsibility for finding and deleting the subscriber
 * lies on the helper function.
 *
 * @param   OBS_TREE*     Head              Points to the current observable.
 *          EFI_GUID      ReferenceGuid     Corresponds to the observable that we're looking for.
 *          OBS_CALLBACK  CallbackInterface This is the subscriber that is requested be removed.
 *
 * @return  OBS_TREE*     Returns the tree after successfully removing (or not finding) the subscriber.
 **/
OBS_TREE*
FindAndDeleteSubscriber(
  IN  OUT OBS_TREE*     Head,
  IN      EFI_GUID      ReferenceGuid,
  IN      OBS_CALLBACK  CallbackInterface
  )
{
  if (Head) {
    if (CompareMem(&(Head->ObservableGuid), &ReferenceGuid, sizeof(ReferenceGuid)) == 0) {
      //
      // We found the observer that matches ReferenceGuid. Find and delete the subscriber that is
      // listening to it.
      //
      Head->Leaf = _FindAndDeleteSubscriber(Head->Leaf, CallbackInterface);
    } else {
      //
      // Not found. Keep searching.
      //
      Head->Next = FindAndDeleteSubscriber(Head->Next, ReferenceGuid, CallbackInterface);
    }
  }

  return Head;
}

/** Remove all observables.
 *
 * Remove all observable guids and all interfaces subscribed to them.
 *
 * @param   VOID          No parameters.
 *
 * @return  EFI_SUCCESS   Successfully removed all observables and subscribed interfaces.
 **/
EFI_STATUS
EFIAPI
RemoveAllObservables(
  VOID
  )
{
  mObservableDb = DeleteAllObservables(mObservableDb);

  return EFI_SUCCESS;
}

/** Subscribe an interface with an observable guid.
 *
 * Use this to register a callback function with a guid. The function provided by CallbackInterface will be executed
 * whenever the appropriate observable instance specified by ReferenceGuid calls Publish.
 *
 * @param   EFI_GUID              ReferenceGuid       The observable guid that the callback interface will subscribe to.
 *          OBS_CASLLBACK         CallbackInterface   A pointer to the function that is subscribing to the observable.
 *
 * @return  EFI_SUCCESS           Successfully subscribed the interface to the observable guid.
 *          EFI_NOT_FOUND         No match could be found between the provided guid and existing observables.
 *          EFI_OUT_OF_RESOURCES  Could not subscribe to this observer due to resource limitations.
 *          EFI_INVALID_PARAMETER Interface is already subscribed to this observer.
 **/
EFI_STATUS
EFIAPI
Subscribe (
  IN      EFI_GUID        ReferenceGuid,
  IN      OBS_CALLBACK    CallbackInterface
  )
{
  EFI_STATUS  Status    = EFI_SUCCESS;
  OBS_TREE*   TempTree  = NULL;
  OBS_LEAF*   Last      = NULL;
  OBS_LEAF*   TempLeaf  = NULL;
  OBS_LEAF*   NewLeaf   = NULL;
  BOOLEAN     Found     = FALSE;

  if (mObservableDb != NULL) {
    //
    // Find the observable guid that we're looking for.
    //
    for (TempTree = mObservableDb; TempTree != NULL; TempTree = TempTree->Next) {
      if (CompareMem(&(TempTree->ObservableGuid), &ReferenceGuid, sizeof(ReferenceGuid)) == 0) {
        Found = TRUE;
        break;
      }
    }
    if (Found) {
      //
      // Prepare to add a new leaf.
      //
      NewLeaf = AllocateZeroPool(sizeof(OBS_LEAF));
      if (!NewLeaf) {
        Status = EFI_OUT_OF_RESOURCES;
      } else {
        NewLeaf->Next = NULL;
        NewLeaf->Observer = CallbackInterface;
        //
        // Go to the end of the list of observers.
        //
        if (TempTree->Leaf != NULL) {
          //
          // First check to see if this is a duplicate observer.
          //
          Found = FALSE;
          TempLeaf = TempTree->Leaf;
          do {
            Last = TempLeaf;
            if (TempLeaf->Observer == CallbackInterface) {
              //
              // It is, so let's abort this process.
              //
              Found = TRUE;
              break;
            }
            TempLeaf = TempLeaf->Next;
          } while (TempLeaf != NULL);
          TempLeaf = Last;

          //
          // Check for duplicates.
          //
          if (Found) {
            gBS->FreePool(NewLeaf);
            Status = EFI_INVALID_PARAMETER;
          } else {
            //
            // At this point, TempLeaf->Next will be the end of the list.
            //
            TempLeaf->Next = NewLeaf;
          }
        } else {
          //
          // There are no observers listening to this guid. Start a new list.
          //
          TempTree->Leaf = NewLeaf;
        }
      }
    } else {
      Status = EFI_NOT_FOUND;
    }
  } else {
    Status = EFI_NOT_FOUND;
  }

  return Status;
}

/** Unsubscribe an interface with an observable guid.
 *
 * Use this to remove an interface from the callback list associated with an observable guid.
 *
 * @param   EFI_GUID                ReferenceGuid   The observable guid to unsubscribe the interface from.
 *          OBS_NOTIFY_INTERFACE    NotifyCallback  A pointer to the interface that is being unsubscribed.
 *
 * @return  EFI_SUCCESS           Successfully unsubscribed the interface from the observable guid.
 **/
EFI_STATUS
EFIAPI
Unsubscribe (
  IN      EFI_GUID        ReferenceGuid,
  IN      OBS_CALLBACK    CallbackInterface
  )
{
  mObservableDb = FindAndDeleteSubscriber(mObservableDb, ReferenceGuid, CallbackInterface);

  return EFI_SUCCESS;
}

/** Notify observing functions.
 *
 * Use this to notify all functions who are subscribed to the guid specified by ReferenceGuid.
 *
 * @param   EFI_GUID          ReferenceGuid   The observable guid that contains the the list of interfaces to be notified.
 *          VOID*             Data            Parameter context to be passed to the notification function.
 *
 * @return  EFI_SUCCESS       Successfully notified all observers listening to this guid.
 *          EFI_NOT_FOUND     No match could be found between the provided guid and existing observables.
 **/
EFI_STATUS
EFIAPI
Publish (
  IN      EFI_GUID                  ReferenceGuid,
  IN  OUT VOID*                     Data
  )
{
  EFI_STATUS  Status    = EFI_SUCCESS;
  OBS_TREE*   TempTree  = NULL;
  OBS_LEAF*   TempLeaf  = NULL;
  BOOLEAN     Found     = FALSE;

  if (mObservableDb != NULL) {
    //
    // Find the observable guid that we're looking for.
    //
    for (TempTree = mObservableDb; TempTree != NULL; TempTree = TempTree->Next) {
      if (CompareMem(&(TempTree->ObservableGuid), &ReferenceGuid, sizeof(ReferenceGuid)) == 0) {
        Found = TRUE;
        break;
      }
    }
    if (Found) {
      //
      // Notify every listener by performing each provided callback.
      //
      for (TempLeaf = TempTree->Leaf; TempLeaf != NULL; TempLeaf = TempLeaf->Next) {
        if (TempLeaf->Observer != NULL) {
          //
          // Execute the callback.
          //
          TempLeaf->Observer(Data);
        }
      }
    } else {
      Status = EFI_NOT_FOUND;
    }
  } else {
    Status = EFI_NOT_FOUND;
  }

  return Status;
}

/** Creates a new observable.
 *
 * Create a new observable that can be observed with the use of Subscribe function.
 *
 * @param   EFI_GUID              ReferenceGuid   The observable guid to add.
 *
 * @return  EFI_SUCCESS           Successfully added observable.
 *          EFI_INVALID_PARAMETER Observable already exists.
 **/
EFI_STATUS
EFIAPI
AddObservable (
  IN      EFI_GUID                  ReferenceGuid
  )
{
  EFI_STATUS  Status    = EFI_SUCCESS;
  OBS_TREE*   TempTree  = NULL;
  OBS_TREE*   Last      = NULL;
  OBS_TREE*   NewTree   = NULL;
  BOOLEAN     Found     = FALSE;

  if (mObservableDb != NULL) {
    if (mObservableDb->Next != NULL) {
      //
      // Iterate to the end of the observable list while checking to see if we aren't creating a duplicate.
      //
      TempTree = mObservableDb->Next;
      do {
        Last = TempTree;
        if (CompareMem(&(TempTree->ObservableGuid), &ReferenceGuid, sizeof(ReferenceGuid)) == 0) {
          Found = TRUE;
          break;
        }
        TempTree = TempTree->Next;
      } while (TempTree != NULL);
      TempTree = Last;
    } else {
      TempTree = mObservableDb;
    }
    if (Found) {
      //
      // Duplicate, so reject the parameter.
      //
      Status = EFI_INVALID_PARAMETER;
    } else {
      //
      // TempTree->Next is our target. Prepare to add a new tree link.
      //
      NewTree = AllocateZeroPool(sizeof(OBS_TREE));
      if (NewTree) {
        NewTree->Next = NULL;
        NewTree->Leaf = NULL;
        CopyMem(&(NewTree->ObservableGuid), &ReferenceGuid, sizeof(ReferenceGuid));
        TempTree->Next = NewTree;
      } else {
        Status = EFI_OUT_OF_RESOURCES;
      }
    }
  } else {
    //
    // mObservableDb has not been created yet. Let's do that.
    //
    NewTree = AllocateZeroPool(sizeof(OBS_TREE));
    if (NewTree) {
      NewTree->Next = NULL;
      NewTree->Leaf = NULL;
      CopyMem(&(NewTree->ObservableGuid), &ReferenceGuid, sizeof(ReferenceGuid));
      mObservableDb = NewTree;
    } else {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

  return Status;
}

/** Remove an observable.
 *
 * Remove an observable so that it can no longer be subscribed to. In addition, unsubscribe any functions
 * that are subscribed to this guid.
 *
 * @param   EFI_GUID              ReferenceGuid   The observable guid to remove.
 *
 * @return  EFI_SUCCESS           Successfully removed observable.
 **/
EFI_STATUS
EFIAPI
RemoveObservable (
  IN      EFI_GUID        ReferenceGuid
  )
{
  mObservableDb = FindAndDeleteObservable(mObservableDb, ReferenceGuid);

  return EFI_SUCCESS;
}
