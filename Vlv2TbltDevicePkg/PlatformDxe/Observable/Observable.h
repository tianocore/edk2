/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

  Observable.h

Abstract:

  Prototypes for Observable protocol implementation
--*/

#ifndef _OBSERVABLE_H_
#define _OBSERVABLE_H_
#include "PlatformDxe.h"
#include "Protocol/Observable.h"

//
// Prototypes
//

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
  );

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
  );

/** Subscribe an interface with an observable guid.
 *
 * Use this to register a callback function with a guid. The function provided by CallbackInterface will be executed
 * whenever the appropriate observable instance specified by ReferenceGuid calls Publish.
 *
 * @param   EFI_GUID              ReferenceGuid       The observable guid that the callback interface will subscribe to.
 *          OBS_CALLBACK          CallbackInterface   A pointer to the function that is subscribing to the observable.
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
  );

/** Unsubscribe an interface with an observable guid.
 *
 * Use this to remove an interface from the callback list associated with an observable guid.
 *
 * @param   EFI_GUID              ReferenceGuid      The observable guid to unsubscribe the interface from.
 *          OBS_CALLBACK          CallbackInterface  A pointer to the interface that is being unsubscribed.
 *
 * @return  EFI_SUCCESS           Successfully unsubscribed the interface from the observable guid.
 **/
EFI_STATUS
EFIAPI
Unsubscribe (
  IN      EFI_GUID            ReferenceGuid,
  IN      OBS_CALLBACK    CallbackInterface
  );

/** Notify observing functions.
 *
 * Use this to notify all functions who are subscribed to the guid specified by ReferenceGuid.
 *
 * @param   EFI_GUID          ReferenceGuid   The observable guid that contains the list of interfaces to be notified.
 *          VOID*             Data            Parameter context to be passed to the subscribed function.
 *
 * @return  EFI_SUCCESS       Successfully notified all observers listening to this guid.
 *          EFI_NOT_FOUND     No match could be found between the provided guid and existing observables.
 **/
EFI_STATUS
EFIAPI
Publish (
  IN      EFI_GUID        ReferenceGuid,
  IN  OUT VOID*           Data
  );

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
  IN      EFI_GUID        ReferenceGuid
  );

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
  );

#endif
