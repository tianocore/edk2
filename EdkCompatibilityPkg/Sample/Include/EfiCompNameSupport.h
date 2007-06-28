/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiCompNameSupport.h

Abstract:

  Private data structures for the Console Splitter driver

--*/

#ifndef EFI_COMPONENT_NAME_SUPPORT_H
#define EFI_COMPONENT_NAME_SUPPORT_H

#include "Tiano.h"

#ifndef EFI_SIZE_REDUCTION_APPLIED

#define INSTALL_ALL_DRIVER_PROTOCOLS(ImageHandle,               \
                                     SystemTable,               \
                                     DriverBinding,             \
                                     DriverBindingHandle,       \
                                     ComponentName,             \
                                     DriverConfiguration,       \
                                     DriverDiagnostics)         \
        EfiLibInstallAllDriverProtocols ((ImageHandle),         \
                                         (SystemTable),         \
                                         (DriverBinding),       \
                                         (DriverBindingHandle), \
                                         (ComponentName),       \
                                         (DriverConfiguration), \
                                         (DriverDiagnostics))
#else

#define INSTALL_ALL_DRIVER_PROTOCOLS(ImageHandle,             \
                                     SystemTable,             \
                                     DriverBinding,           \
                                     DriverBindingHandle,     \
                                     ComponentName,           \
                                     DriverConfiguration,     \
                                     DriverDiagnostics)       \
        EfiLibInstallDriverBinding ((ImageHandle),            \
                                    (SystemTable),            \
                                    (DriverBinding),          \
                                    (DriverBindingHandle))
#endif

#endif
