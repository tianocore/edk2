/** @file
  Extension Form Browser Protocol provides the services that can be used to 
  register the different hot keys for the standard Browser actions described in UEFI specification.

Copyright (c) 2011 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FORM_BROWSER_EXTENSION_H__
#define __FORM_BROWSER_EXTENSION_H__

#define FORM_BROWSER_EXTENSION_PROTOCOL_GUID  \
  { 0x1f73b18d, 0x4630, 0x43c1, { 0xa1, 0xde, 0x6f, 0x80, 0x85, 0x5d, 0x7d, 0xa4 } }

typedef struct _EDKII_FORM_BROWSER_EXTENSION_PROTOCOL   EDKII_FORM_BROWSER_EXTENSION_PROTOCOL;

//
// To be compatible, keep EFI_FORM_BROWSER_EXTENSION_PROTOCOL definition
//
typedef EDKII_FORM_BROWSER_EXTENSION_PROTOCOL   EFI_FORM_BROWSER_EXTENSION_PROTOCOL;

//
// Return value of SAVE_REMINDER() that describes whether the changed data is saved or discarded.
//
#define BROWSER_NO_CHANGES          0
#define BROWSER_SAVE_CHANGES        1
#define BROWSER_DISCARD_CHANGES     2
#define BROWSER_KEEP_CURRENT        3

//
// Browser actions. They can be cominbed together. 
// If more than one actions are specified, the action with low bit will be executed first. 
//
#define BROWSER_ACTION_UNREGISTER   0
#define BROWSER_ACTION_DISCARD      BIT0
#define BROWSER_ACTION_DEFAULT      BIT1
#define BROWSER_ACTION_SUBMIT       BIT2
#define BROWSER_ACTION_RESET        BIT3
#define BROWSER_ACTION_EXIT         BIT4
#define BROWSER_ACTION_GOTO         BIT5

//
// Scope for Browser action. It may be Form, FormSet or System level.
//
typedef enum {
  FormLevel,
  FormSetLevel,
  SystemLevel,
  MaxLevel
} BROWSER_SETTING_SCOPE;

/**
  Configure what scope the hot key will impact.
  All hot keys have the same scope. The mixed hot keys with the different level are not supported.
  If no scope is set, the default scope will be FormSet level.
  After all registered hot keys are removed, previous Scope can reset to another level.
  
  @param[in] Scope               Scope level to be set. 
  
  @retval EFI_SUCCESS            Scope is set correctly.
  @retval EFI_INVALID_PARAMETER  Scope is not the valid value specified in BROWSER_SETTING_SCOPE. 
  @retval EFI_UNSPPORTED         Scope level is different from current one that the registered hot keys have.

**/
typedef
EFI_STATUS
(EFIAPI *SET_SCOPE) (
  IN BROWSER_SETTING_SCOPE Scope
  );

/**
  Register the hot key with its browser action, or unregistered the hot key.
  If the action value is zero, the hot key will be unregistered if it has been registered.
  If the same hot key has been registered, the new action and help string will override the previous ones.
  
  @param[in] KeyData     A pointer to a buffer that describes the keystroke
                         information for the hot key. Its type is EFI_INPUT_KEY to 
                         be supported by all ConsoleIn devices.
  @param[in] Action      Action value that describes what action will be trigged when the hot key is pressed. 
  @param[in] DefaultId   Specifies the type of defaults to retrieve, which is only for DEFAULT action.
  @param[in] HelpString  Help string that describes the hot key information.
                         Its value may be NULL for the unregistered hot key.
  
  @retval EFI_SUCCESS            Hot key is registered or unregistered.
  @retval EFI_INVALID_PARAMETER  KeyData is NULL.
**/
typedef
EFI_STATUS
(EFIAPI *REGISTER_HOT_KEY) (
  IN EFI_INPUT_KEY *KeyData,
  IN UINT32        Action,
  IN UINT16        DefaultId,
  IN EFI_STRING    HelpString OPTIONAL
  );

/**
  This handler is responsbile for the left things on normal boot after all UI forms are closed.
  For example, it can continue to boot the first boot option. 

  It will be used only when EXIT action is trigged as system level.
**/
typedef
VOID
(EFIAPI *EXIT_HANDLER) (
  VOID
  );

/**
  Register Exit handler function. 
  When more than one handler function is registered, the latter one will override the previous one. 
  When NULL handler is specified, the previous Exit handler will be unregistered. 
  
  @param[in] Handler      Pointer to handler function. 

**/
typedef
VOID
(EFIAPI *REGISTER_EXIT_HANDLER) (
  IN EXIT_HANDLER Handler
  );

/**
  Create reminder to let user to choose save or discard the changed browser data.
  Caller can use it to actively check the changed browser data.

  @retval BROWSER_NO_CHANGES       No browser data is changed.
  @retval BROWSER_SAVE_CHANGES     The changed browser data is saved.
  @retval BROWSER_DISCARD_CHANGES  The changed browser data is discard.
  @retval BROWSER_KEEP_CURRENT     Browser keep current changes.

**/
typedef
UINT32
(EFIAPI *SAVE_REMINDER)(
  VOID
  );

struct _EDKII_FORM_BROWSER_EXTENSION_PROTOCOL {
  SET_SCOPE              SetScope;
  REGISTER_HOT_KEY       RegisterHotKey;
  REGISTER_EXIT_HANDLER  RegiserExitHandler;
  SAVE_REMINDER          SaveReminder;
};

extern EFI_GUID gEfiFormBrowserExProtocolGuid;
extern EFI_GUID gEdkiiFormBrowserExProtocolGuid;

#endif

