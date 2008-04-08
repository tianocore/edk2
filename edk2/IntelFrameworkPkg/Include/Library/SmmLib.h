/** @file
  Library class name: SmmLib
  
  SMM Library Services that abstracts both S/W SMI generation and detection. 

  Copyright (c) 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.        

**/

#ifndef __SMM_LIB_H__
#define __SMM_LIB_H__


/**
  Triggers an SMI at boot time.  

  This function triggers a software SMM interrupt at boot time.

**/
VOID
EFIAPI
TriggerBootServiceSoftwareSmi (
  VOID
  );


/**
  Triggers an SMI at run time.  

  This function triggers a software SMM interrupt at run time.

**/
VOID
EFIAPI
TriggerRuntimeSoftwareSmi (
  VOID
  );


/**
  Test if a boot time software SMI happens.  

  This function tests if a software SMM interrupt happens. If a software SMM interrupt happens and
  it is triggered at boot time, it returns TRUE. Otherwise, it returns FALSE.

  @retval TRUE                 A software SMI triggered at boot time happens.
  @retval FLASE                No software SMI happens or the software SMI is triggered at run time.

**/
BOOLEAN
EFIAPI
IsBootServiceSoftwareSmi (
  VOID
  );


/**
  Test if a run time software SMI happens.  

  This function tests if a software SMM interrupt happens. If a software SMM interrupt happens and
  it is triggered at run time, it returns TRUE. Otherwise, it returns FALSE.

  @retval TRUE                 A software SMI triggered at run time happens.
  @retval FLASE                No software SMI happens or the software SMI is triggered at boot time.

**/
BOOLEAN
EFIAPI
IsRuntimeSoftwareSmi (
  VOID
  );

#endif
