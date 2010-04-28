/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  WatchdogTimer.h

Abstract:

  Watchdog Timer Architectural Protocol as defined in the DXE CIS

  Used to provide system watchdog timer services

--*/

#ifndef _ARCH_PROTOCOL_WATCHDOG_TIMER_H_
#define _ARCH_PROTOCOL_WATCHDOG_TIMER_H_

//
// Global ID for the Watchdog Timer Architectural Protocol
//
#define EFI_WATCHDOG_TIMER_ARCH_PROTOCOL_GUID \
  { 0x665E3FF5, 0x46CC, 0x11d4, {0x9A, 0x38, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D} }

//
// Declare forward reference for the Timer Architectural Protocol
//
EFI_FORWARD_DECLARATION (EFI_WATCHDOG_TIMER_ARCH_PROTOCOL);

typedef
VOID
(EFIAPI *EFI_WATCHDOG_TIMER_NOTIFY) (
  IN UINT64  Time
  );
/*++

Routine Description:

  A function of this type is called when the watchdog timer fires if a 
  handler has been registered.

Arguments:

  Time - The time in 100 ns units that has passed since the watchdog 
         timer was armed.  For the notify function to be called, this 
         must be greater than TimerPeriod.
  
Returns: 

  None.

--*/

typedef 
EFI_STATUS
(EFIAPI *EFI_WATCHDOG_TIMER_REGISTER_HANDLER) (
  IN EFI_WATCHDOG_TIMER_ARCH_PROTOCOL  *This,
  IN EFI_WATCHDOG_TIMER_NOTIFY                 NotifyFunction
  );
/*++

Routine Description:

  This function registers a handler that is to be invoked when the watchdog 
  timer fires.  By default, the EFI_WATCHDOG_TIMER protocol will call the 
  Runtime Service ResetSystem() when the watchdog timer fires.  If a 
  NotifyFunction is registered, then the NotifyFunction will be called before 
  the Runtime Service ResetSystem() is called.  If NotifyFunction is NULL, then 
  the watchdog handler is unregistered.  If a watchdog handler is registered, 
  then EFI_SUCCESS is returned.  If an attempt is made to register a handler 
  when a handler is already registered, then EFI_ALREADY_STARTED is returned.  
  If an attempt is made to uninstall a handler when a handler is not installed, 
  then return EFI_INVALID_PARAMETER.

Arguments:

  This           - The EFI_WATCHDOG_TIMER_ARCH_PROTOCOL instance.

  NotifyFunction - The function to call when the watchdog timer fires.  If this
                   is NULL, then the handler will be unregistered.

Returns: 

  EFI_SUCCESS           - The watchdog timer handler was registered or 
                          unregistered.

  EFI_ALREADY_STARTED   - NotifyFunction is not NULL, and a handler is already 
                          registered.

  EFI_INVALID_PARAMETER - NotifyFunction is NULL, and a handler was not 
                          previously registered.

--*/

typedef 
EFI_STATUS
(EFIAPI *EFI_WATCHDOG_TIMER_SET_TIMER_PERIOD) (
  IN EFI_WATCHDOG_TIMER_ARCH_PROTOCOL  *This,
  IN UINT64                                    TimerPeriod
  );
/*++

Routine Description:

  This function sets the amount of time to wait before firing the watchdog 
  timer to TimerPeriod 100 nS units.  If TimerPeriod is 0, then the watchdog 
  timer is disabled.

Arguments:

  This        - The EFI_WATCHDOG_TIMER_ARCH_PROTOCOL instance.

  TimerPeriod - The amount of time in 100 nS units to wait before the watchdog 
                timer is fired.  If TimerPeriod is zero, then the watchdog 
                timer is disabled.
  
Returns: 

  EFI_SUCCESS      - The watchdog timer has been programmed to fire in Time 
                     100 nS units.

  EFI_DEVICE_ERROR - A watchdog timer could not be programmed due to a device 
                     error.

--*/

typedef 
EFI_STATUS
(EFIAPI *EFI_WATCHDOG_TIMER_GET_TIMER_PERIOD) (
  IN  EFI_WATCHDOG_TIMER_ARCH_PROTOCOL  *This,
  OUT UINT64                            *TimerPeriod
  );
/*++

Routine Description:

  This function retrieves the amount of time the system will wait before firing 
  the watchdog timer.  This period is returned in TimerPeriod, and EFI_SUCCESS 
  is returned.  If TimerPeriod is NULL, then EFI_INVALID_PARAMETER is returned.

Arguments:

  This        - The EFI_WATCHDOG_TIMER_ARCH_PROTOCOL instance.

  TimerPeriod - A pointer to the amount of time in 100 nS units that the system 
                will wait before the watchdog timer is fired.  If TimerPeriod of
                zero is returned, then the watchdog timer is disabled.
  
Returns: 

  EFI_SUCCESS           - The amount of time that the system will wait before 
                          firing the watchdog timer was returned in TimerPeriod.

  EFI_INVALID_PARAMETER - TimerPeriod is NULL.

--*/

//
// Interface stucture for the Watchdog Timer Architectural Protocol
//
struct _EFI_WATCHDOG_TIMER_ARCH_PROTOCOL {
  EFI_WATCHDOG_TIMER_REGISTER_HANDLER  RegisterHandler;
  EFI_WATCHDOG_TIMER_SET_TIMER_PERIOD  SetTimerPeriod;
  EFI_WATCHDOG_TIMER_GET_TIMER_PERIOD  GetTimerPeriod;
};

/*++

  Protocol Description:
    This protocol provides the services required to implement the Boot Service 
    SetWatchdogTimer().  It provides a service to set the amount of time to wait 
    before firing the watchdog timer, and it also provides a service to register 
    a handler that is invoked when the watchdog timer fires.  This protocol can 
    implement the watchdog timer by using the event and timer Boot Services, or 
    it can make use of custom hardware.  When the watchdog timer fires, control 
    will be passed to a handler if one has been registered.  If no handler has 
    been registered, or the registered handler returns, then the system will be 
    reset by calling the Runtime Service ResetSystem().
  
  Parameters:

    RegisterHandler - Registers a handler that is invoked when the watchdog 
                      timer fires.

    SetTimerPeriod  - Sets the amount of time in 100 ns units to wait before the 
                      watchdog timer is fired.  If this function is supported, 
                      then the watchdog timer period will be rounded up to the 
                      nearest supported watchdog timer period.

    GetTimerPeriod  - Retrieves the amount of time in 100 ns units that the 
                      system will wait before the watchdog timer is fired.

--*/

extern EFI_GUID gEfiWatchdogTimerArchProtocolGuid;

#endif

