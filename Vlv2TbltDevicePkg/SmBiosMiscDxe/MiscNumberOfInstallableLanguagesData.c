/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscNumberOfInstallableLanguagesData.c

Abstract:

  Static data of the Number of installable languages information.
  Number of installable languages information is Misc. subclass type 11 and SMBIOS type 13.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA, NumberOfInstallableLanguages)
= {
  1,    // NumberOfInstallableLanguages
  {     // LanguageFlags
    1,  // AbbreviatedLanguageFormat
    0   // Reserved
  },
  STRING_TOKEN(STR_MISC_SYSTEM_LANGUAGE_EN_US)  // CurrentLanguageNumber
};
