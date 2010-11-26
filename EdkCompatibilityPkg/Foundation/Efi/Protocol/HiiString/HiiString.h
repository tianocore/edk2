/*++

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    HiiString.h
    
Abstract:

    EFI_HII_STRING_PROTOCOL from UEFI 2.1 specification.
    
    This protocol provides interfaces to manipulate string data.

Revision History

--*/

#ifndef __EFI_HII_STRING_PROTOCOL_H__
#define __EFI_HII_STRING_PROTOCOL_H__

#include EFI_PROTOCOL_DEFINITION (HiiFont)

//
// Global ID for the Hii String Protocol.
//
#define EFI_HII_STRING_PROTOCOL_GUID \
  { \
    0xfd96974, 0x23aa, 0x4cdc, {0xb9, 0xcb, 0x98, 0xd1, 0x77, 0x50, 0x32, 0x2a} \
  }

EFI_FORWARD_DECLARATION (EFI_HII_STRING_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *EFI_HII_NEW_STRING) (
  IN  CONST EFI_HII_STRING_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                  PackageList,
  OUT EFI_STRING_ID                   *StringId,
  IN  CONST CHAR8                     *Language,
  IN  CONST CHAR16                    *LanguageName, OPTIONAL  
  IN  CONST EFI_STRING                String,
  IN  CONST EFI_FONT_INFO             *StringFontInfo OPTIONAL
  )
/*++

  Routine Description:
    This function adds the string String to the group of strings owned by PackageList, with the
    specified font information StringFontInfo and returns a new string id.                         
    The new string identifier is guaranteed to be unique within the package list. 
    That new string identifier is reserved for all languages in the package list. 
    
  Arguments:          
    This              - A pointer to the EFI_HII_STRING_PROTOCOL instance.
    PackageList       - Handle of the package list where this string will be added.                        
    StringId          - On return, contains the new strings id, which is unique within PackageList.    
    Language          - Points to the language for the new string.
    LanguageName      - Points to the printable language name to associate with the passed in 
                        Language field.If LanguageName is not NULL and the string package header's LanguageName 
                        associated with a given Language is not zero, the LanguageName being passed 
                        in will be ignored.    
    String            - Points to the new null-terminated string.                                                                                     
    StringFontInfo    - Points to the new string's font information or NULL if the string should have the
                        default system font, size and style.                                                  

  Returns:
    EFI_SUCCESS            - The new string was added successfully.
    EFI_NOT_FOUND          - The specified PackageList could not be found in database.
    EFI_OUT_OF_RESOURCES   - Could not add the string due to lack of resources.
    EFI_INVALID_PARAMETER  - String is NULL or StringId is NULL or Language is NULL.                                            
    
--*/    
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_STRING) (
  IN  CONST EFI_HII_STRING_PROTOCOL   *This,
  IN  CONST CHAR8                     *Language,
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      String,
  IN  OUT UINTN                       *StringSize,
  OUT EFI_FONT_INFO                   **StringFontInfo OPTIONAL
  )
/*++

  Routine Description:
    This function retrieves the string specified by StringId which is associated 
    with the specified PackageList in the language Language and copies it into 
    the buffer specified by String.
    
  Arguments:          
    This              - A pointer to the EFI_HII_STRING_PROTOCOL instance.
    Language          - Points to the language for the retrieved string.
    PackageList       - The package list in the HII database to search for the 
                        specified string.    
    StringId          - The string's id, which is unique within PackageList.    
    String            - Points to the new null-terminated string.            
    StringSize        - On entry, points to the size of the buffer pointed to by 
                        String, in bytes. On return,
                        points to the length of the string, in bytes.
    StringFontInfo    - Points to a buffer that will be callee allocated and will 
                        have the string's font information into this buffer.  
                        The caller is responsible for freeing this buffer.  
                        If the parameter is NULL a buffer will not be allocated 
                        and the string font information will not be returned.

  Returns:
    EFI_SUCCESS            - The string was returned successfully.
    EFI_NOT_FOUND          - The string specified by StringId is not available.
                             The specified PackageList is not in the database.
    EFI_INVALID_LANGUAGE   - The string specified by StringId is available but
                             not in the specified language.                             
    EFI_BUFFER_TOO_SMALL   - The buffer specified by StringSize is too small to 
                             hold the string.                                                      
    EFI_INVALID_PARAMETER  - The String or Language or StringSize was NULL.
    EFI_OUT_OF_RESOURCES   - There were insufficient resources to complete the 
                             request.
    
--*/ 
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_SET_STRING) (
  IN CONST EFI_HII_STRING_PROTOCOL    *This,
  IN EFI_HII_HANDLE                   PackageList,
  IN EFI_STRING_ID                    StringId,
  IN CONST CHAR8                      *Language,
  IN CONST EFI_STRING                 String,
  IN CONST EFI_FONT_INFO              *StringFontInfo OPTIONAL
  )
/*++

  Routine Description:
    This function updates the string specified by StringId in the specified PackageList to the text   
    specified by String and, optionally, the font information specified by StringFontInfo.         
    
  Arguments:          
    This              - A pointer to the EFI_HII_STRING_PROTOCOL instance.
    PackageList       - The package list containing the strings.
    StringId          - The string's id, which is unique within PackageList.    
    Language          - Points to the language for the updated string.
    String            - Points to the new null-terminated string.                   
    StringFontInfo    - Points to the string's font information or NULL if the string font information is not
                        changed.  

  Returns:
    EFI_SUCCESS            - The string was updated successfully.
    EFI_NOT_FOUND          - The string specified by StringId is not in the database.
                             The specified PackageList is not in the database.
    EFI_INVALID_PARAMETER  - The String or Language was NULL.
    EFI_OUT_OF_RESOURCES   - The system is out of resources to accomplish the task.
    
--*/ 
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_LANGUAGES) (
  IN CONST EFI_HII_STRING_PROTOCOL    *This,
  IN EFI_HII_HANDLE                   PackageList,
  IN OUT CHAR8                        *Languages,
  IN OUT UINTN                        *LanguagesSize
  )
/*++

  Routine Description:
    This function returns the list of supported languages, in the format specified
    in Appendix M of UEFI 2.1 spec.
    
  Arguments:          
    This              - A pointer to the EFI_HII_STRING_PROTOCOL instance.
    PackageList       - The package list to examine.
    Languages         - Points to the buffer to hold the returned string.
    LanguagesSize     - On entry, points to the size of the buffer pointed to by 
                        Languages, in bytes. On 
                        return, points to the length of Languages, in bytes.
                        
  Returns:
    EFI_SUCCESS            - The languages were returned successfully.    
    EFI_INVALID_PARAMETER  - The Languages or LanguagesSize was NULL.
    EFI_BUFFER_TOO_SMALL   - The LanguagesSize is too small to hold the list of 
                             supported languages. LanguageSize is updated to
                             contain the required size.
    EFI_NOT_FOUND          - The specified PackageList is not in the database.
    
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_2ND_LANGUAGES) (
  IN CONST EFI_HII_STRING_PROTOCOL   *This,
  IN EFI_HII_HANDLE                  PackageList,
  IN CONST CHAR8                     *FirstLanguage,
  IN OUT CHAR8                       *SecondLanguages,
  IN OUT UINTN                       *SecondLanguagesSize
  )
/*++

  Routine Description:
    Each string package has associated with it a single primary language and zero
    or more secondary languages. This routine returns the secondary languages
    associated with a package list.
    
  Arguments:          
    This                   - A pointer to the EFI_HII_STRING_PROTOCOL instance.
    PackageList            - The package list to examine.
    FirstLanguage          - Points to the primary language.
    SecondaryLanguages     - Points to the buffer to hold the returned list of 
                             secondary languages for the specified FirstLanguage.
                             If there are no secondary languages, the function 
                             returns successfully, but this is set to NULL.
    SecondaryLanguageSize  - On entry, points to the size of the buffer pointed to 
                             by SecondLanguages, in bytes. On return, points to
                             the length of SecondLanguages in bytes.
                        
  Returns:
    EFI_SUCCESS            - Secondary languages were correctly returned.
    EFI_INVALID_PARAMETER  - FirstLanguage or SecondLanguages or SecondLanguagesSize was NULL. 
    EFI_BUFFER_TOO_SMALL   - The buffer specified by SecondLanguagesSize is   
                             too small to hold the returned information.      
                             SecondLanguageSize is updated to hold the size of
                             the buffer required.
    EFI_INVALID_LANGUAGE   - The language specified by FirstLanguage is not
                             present in the specified package list.
    EFI_NOT_FOUND          - The specified PackageList is not in the Database.    
    
--*/
;
//
// Interface structure for the EFI_HII_STRING_PROTOCOL
//
struct _EFI_HII_STRING_PROTOCOL {
  EFI_HII_NEW_STRING        NewString;
  EFI_HII_GET_STRING        GetString;
  EFI_HII_SET_STRING        SetString;
  EFI_HII_GET_LANGUAGES     GetLanguages;
  EFI_HII_GET_2ND_LANGUAGES GetSecondaryLanguages;
};

extern EFI_GUID gEfiHiiStringProtocolGuid;

#endif
