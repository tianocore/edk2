/** @file
Provides some data struct used by OHCI controller driver.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef _OHCI_URB_H
#define _OHCI_URB_H

#include "Descriptor.h"


//
// Func List
//


/**

  Create a TD

  @Param  Ohc                   UHC private data

  @retval                       TD structure pointer

**/
TD_DESCRIPTOR *
OhciCreateTD (
  IN USB_OHCI_HC_DEV      *Ohc
  );

/**

  Free a TD

  @Param  Ohc                   UHC private data
  @Param  Td                    Pointer to a TD to free

  @retval  EFI_SUCCESS          TD freed

**/
EFI_STATUS
OhciFreeTD (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN TD_DESCRIPTOR        *Td
  );

/**

  Create a ED

  @Param   Ohc                  Device private data

  @retval  ED                   descriptor pointer

**/
ED_DESCRIPTOR *
OhciCreateED (
  USB_OHCI_HC_DEV          *Ohc
  );


/**

  Free a ED

  @Param  Ohc                   UHC private data
  @Param  Ed                    Pointer to a ED to free

  @retval  EFI_SUCCESS          ED freed

**/

EFI_STATUS
OhciFreeED (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN ED_DESCRIPTOR        *Ed
  );

/**

  Free  ED

  @Param  Ohc                    Device private data
  @Param  Ed                     Pointer to a ED to free

  @retval  EFI_SUCCESS           ED freed

**/
EFI_STATUS
OhciFreeAllTDFromED (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN ED_DESCRIPTOR        *Ed
  );

/**

  Find a working ED match the requirement

  @Param  EdHead                Head of the ED list
  @Param  DeviceAddress         Device address to search
  @Param  EndPointNum           End point num to search
  @Param  EdDir                 ED Direction to search

  @retval   ED descriptor searched

**/

ED_DESCRIPTOR *
OhciFindWorkingEd (
  IN ED_DESCRIPTOR       *EdHead,
  IN UINT8               DeviceAddress,
  IN UINT8               EndPointNum,
  IN UINT8               EdDir
  );


/**

  Initialize interrupt list.

  @Param Ohc                    Device private data

  @retval  EFI_SUCCESS          Initialization done

**/
EFI_STATUS
OhciInitializeInterruptList (
  USB_OHCI_HC_DEV          *Ohc
  );

/**

  Attach an ED

  @Param  Ed                    Ed to be attached
  @Param  NewEd                 Ed to attach

  @retval EFI_SUCCESS           NewEd attached to Ed
  @retval EFI_INVALID_PARAMETER Ed is NULL

**/
EFI_STATUS
OhciAttachED (
  IN ED_DESCRIPTOR        *Ed,
  IN ED_DESCRIPTOR        *NewEd
  );

/**

  Count ED number on a ED chain

  @Param  Ed                    Head of the ED chain

  @retval                       ED number on the chain

**/

UINTN
CountEdNum (
  IN ED_DESCRIPTOR      *Ed
  );

/**

  Find the minimal burn ED list on a specific depth level

  @Param  Ohc                   Device private data
  @Param  Depth                 Depth level

  @retval                       ED list found

**/

ED_DESCRIPTOR *
OhciFindMinInterruptEDList (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Depth
  );

/**

  Attach an ED to an ED list

  @Param  OHC                   UHC private data
  @Param  ListType              Type of the ED list
  @Param  Ed                    ED to attach
  @Param  EdList                ED list to be attached

  @retval  EFI_SUCCESS          ED attached to ED list

**/
ED_DESCRIPTOR *
OhciAttachEDToList (
  IN USB_OHCI_HC_DEV       *Ohc,
  IN DESCRIPTOR_LIST_TYPE  ListType,
  IN ED_DESCRIPTOR         *Ed,
  IN ED_DESCRIPTOR         *EdList
  );

/**

  Remove interrupt EDs that match requirement

  @Param  Ohc                   UHC private data
  @Param  IntEd                 The address of Interrupt endpoint

  @retval  EFI_SUCCESS          EDs match requirement removed

**/

EFI_STATUS
OhciFreeInterruptEdByEd (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN ED_DESCRIPTOR        *IntEd
  );

/**

  Remove interrupt EDs that match requirement

  @Param  Ohc                   UHC private data
  @Param  FunctionAddress       Requirement on function address
  @Param  EndPointNum           Requirement on end point number

  @retval  EFI_SUCCESS          EDs match requirement removed

**/
EFI_STATUS
OhciFreeInterruptEdByAddr (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT8                FunctionAddress,
  IN UINT8                EndPointNum
  );


/**

  Link Td2 to the end of Td1

  @Param Td1                    TD to be linked
  @Param Td2                    TD to link

  @retval EFI_SUCCESS           TD successfully linked
  @retval EFI_INVALID_PARAMETER Td1 is NULL

**/
EFI_STATUS
OhciLinkTD (
  IN TD_DESCRIPTOR        *Td1,
  IN TD_DESCRIPTOR        *Td2
  );


/**

  Attach TD list to ED

  @Param  Ed                    ED which TD list attach on
  @Param  HeadTd                Head of the TD list to attach

  @retval  EFI_SUCCESS          TD list attached on the ED

**/
EFI_STATUS
OhciAttachTDListToED (
  IN ED_DESCRIPTOR        *Ed,
  IN TD_DESCRIPTOR        *HeadTd
  );


/**

  Set value to ED specific field

  @Param  Ed                    ED to be set
  @Param  Field                 Field to be set
  @Param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/
EFI_STATUS
OhciSetEDField (
  IN ED_DESCRIPTOR        *Ed,
  IN UINT32               Field,
  IN UINT32               Value
  );


/**

  Get value from an ED's specific field

  @Param  Ed                    ED pointer
  @Param  Field                 Field to get value from

  @retval                       Value of the field

**/
UINT32
OhciGetEDField (
  IN ED_DESCRIPTOR        *Ed,
  IN UINT32               Field
  );


/**

  Set value to TD specific field

  @Param  Td                    TD to be set
  @Param  Field                 Field to be set
  @Param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/
EFI_STATUS
OhciSetTDField (
  IN TD_DESCRIPTOR        *Td,
  IN UINT32               Field,
  IN UINT32               Value
  );


/**

  Get value from ED specific field

  @Param  Td                    TD pointer
  @Param  Field                 Field to get value from

  @retval                       Value of the field

**/

UINT32
OhciGetTDField (
  IN TD_DESCRIPTOR      *Td,
  IN UINT32             Field
  );
/**

  Free the Ed,Td,buffer that were created during transferring

  @Param  Ohc                   Device private data
**/

VOID
OhciFreeDynamicIntMemory(
  IN USB_OHCI_HC_DEV      *Ohc
  );

/**

  Free the Ed that were initilized during driver was starting,
  those memory were used as interrupt ED head

  @Param  Ohc                   Device private data


**/
VOID
OhciFreeFixedIntMemory (
  IN USB_OHCI_HC_DEV      *Ohc
  );
/**

  Release all OHCI used memory when OHCI going to quit

  @Param  Ohc                   Device private data

  @retval EFI_SUCCESS          Memory released

**/

EFI_STATUS
OhciFreeIntTransferMemory (
  IN USB_OHCI_HC_DEV           *Ohc
  );

#endif
