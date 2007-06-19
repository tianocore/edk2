/** @file
  The file provides services to manipulate string data.
  
  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name: HiiString.h

**/

#ifndef __HII_STRING_H__
#define __HII_STRING_H__

#define EFI_HII_STRING_PROTOCOL_GUID \
  { 0xfd96974, 0x23aa, 0x4cdc, { 0xb9, 0xcb, 0x98, 0xd1, 0x77, 0x50, 0x32, 0x2a } }


typedef struct _EFI_HII_STRING_PROTOCOL EFI_HII_STRING_PROTOCOL;



/**
  This function adds the string String to the group of strings
  owned by PackageList, with the specified font information
  StringFontInfo and returns a new string id.

  @param This A pointer to the EFI_HII_STRING_PROTOCOL instance.

  @param PackageList  Handle of the package list where this
                      string will be added.

  @param Language Points to the language for the new string.

  @param String   Points to the new null-terminated string.

  @param StringFontInfo Points to the new string's font
                        information or NULL if the string should
                        have the default system font, size and
                        style. StringId On return, contains the
                        new strings id, which is unique within
                        PackageList.

  @retval EFI_SUCCESS The new string was added successfully
  
  @retval EFI_OUT_OF_RESOURCES  Could not add the string.
  
  @retval EFI_INVALID_PARAMETER String is NULL or StringId is
                                NULL or Language is NULL.


**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_NEW_STRING) (
  IN CONST  EFI_HII_STRING_PROTOCOL   *This,
  IN CONST  EFI_HII_HANDLE            PackageList,
  OUT       EFI_STRING_ID             *StringId
  IN CONST  CHAR8                     *Language,
  IN CONST  EFI_STRING                String,
  IN CONST  EFI_FONT_INFO             *StringFontInfo OPTIONAL,
);


/**

  This function retrieves the string specified by StringId which
  is associated with the specified PackageList in the language
  Language and copies it into the buffer specified by String. If
  the string specified by StringId is not present in the
  specified PackageList, then EFI_NOT_FOUND is returned. If the
  string specified by StringId is present, but not in the
  specified language then EFI_INVALID_LANGUAGE is returned. If
  the buffer specified by StringSize is too small to hold the
  string, then EFI_BUFFER_TOO_SMALL will be returned. StringSize
  will be updated to the size of buffer actually required to
  hold the string.

  @param This A pointer to the EFI_HII_STRING_PROTOCOL instance.
  
  @param PackageList  The package list in the HII database to
                      search for the specified string.
  
  @param Language   Points to the language for the retrieved
                    string.
  
  @param StringId   The string's id, which is unique within
                    PackageList.
  
  @param String   Points to the new null-terminated string.
  
  @param StringSize On entry, points to the size of the buffer
                    pointed to by String, in bytes. On return,
                    points to the length of the string, in
                    bytes.
  
  @param StringFontInfo   Points to the string's font
                          information or NULL if the string font
                          information is not desired.
  
  @retval EFI_SUCCESS The string was returned successfully.
  
  @retval EFI_NOT_FOUND The string specified by StringId is not
                        available.
  
  @retval EFI_INVALID_LANGUAGE  The string specified by StringId
                                is available but not in the
                                specified language.
  
  @retval EFI_BUFFER_TOO_SMALL  The buffer specified by
                                StringLength is too small to
                                hold the string.
  
  @retval EFI_INVALID_PARAMETER The String or Language was NULL.
  
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_STRING) (
  IN CONST  EFI_HII_STRING_PROTOCOL *This,
  IN CONST  CHAR8                   *Language,
  IN CONST  EFI_HII_HANDLE          PackageList,
  IN CONST  EFI_STRING_ID           StringId,
  OUT       EFI_STRING              String,
  IN OUT    UINTN                   StringSize,
  OUT       EFI_FONT_INFO           *StringFontInfo OPTIONAL
);

/**
  This function updates the string specified by StringId in the
  specified PackageList to the text specified by String and,
  optionally, the font information specified by StringFontInfo.
  There is no way to change the font information without changing
  the string text.

  @param This A pointer to the EFI_HII_STRING_PROTOCOL instance.

  @param PackageList  The package list containing the strings.

  @param Language Points to the language for the updated string.

  @param StringId The string id, which is unique within
                  PackageList.

  @param String   Points to the new null-terminated string.

  @param StringFontInfo Points to the string's font information
                        or NULL if the string font information
                        is not changed.

  @retval EFI_SUCCESS   The string was successfully updated.
  
  @retval EFI_NOT_FOUND The string specified by StringId is not
                        in the database.
  
  @retval EFI_INVALID_PARAMETER The String or Language was NULL.
  
  @retval EFI_OUT_OF_RESOURCES  The system is out of resources
                                to accomplish the task.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_SET_STRING) (
  IN CONST  EFI_HII_STRING_PROTOCOL *This,
  IN CONST  EFI_HII_HANDLE          PackageList,
  IN CONST  EFI_STRING_ID           StringId,
  IN CONST  CHAR8                   *Language,
  IN CONST  EFI_STRING              String,
  IN CONST  EFI_FONT_INFO           *StringFontInfo OPTIONAL
);


/**

  This function returns the list of supported languages.

  @param This A pointer to the EFI_HII_STRING_PROTOCOL instance.

  @param PackageList  The package list to examine.

  @param Languages  Points to the buffer to hold the returned
                    string.

  @param LanguageSize   On entry, points to the size of the
                        buffer pointed to by Languages, in
                        bytes. On return, points to the length
                        of Languages, in bytes.


  @retval EFI_SUCCESS The languages were returned successfully.
  
  @retval EFI_BUFFER_TOO_SMALL  The LanguagesSize is too small
                                to hold the list of supported
                                languages. LanguageSize is
                                updated to contain the required
                                size.
  
  @retval EFI_INVALID_PARAMETER Languages is NULL.


**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_LANGUAGES) (
  IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
  IN CONST  EFI_HII_HANDLE            PackageList,
  IN OUT    CHAR8                     *Languages,
  IN OUT    UINTN                     LanguagesSize
);


/**

  Each string package has associated with it a single primary
  language and zero or more secondary languages. This routine
  returns the secondary languages associated with a package list.

  @param This   A pointer to the EFI_HII_STRING_PROTOCOL
                instance.

  @param PackageList  The package list to examine.

  @param FirstLanguage  Points to the primary language.

  @param Languages  are specified in the format specified in
                    Appendix M of the UEFI 2.0 specification.

  @param SecondaryLanguages Points to the buffer to hold the
                            returned list of secondary languages
                            for the specified FirstLanguage. If
                            there are no secondary languages,
                            the function returns successfully,
                            but this is set to NULL.

  @param SecondaryLanguageSize  On entry, points to the size of
                                the buffer pointed to by
                                Languages, in bytes. On return,
                                points to the length of
                                Languages in bytes.

  @retval EFI_SUCCESS   Secondary languages correctly returned

  @retval EFI_BUFFER_TOO_SMALL  The buffer specified by
                                SecondLanguagesSize is too small
                                to hold the returned
                                information. SecondLanguageSize
                                is updated to hold the size of
                                the buffer required.

  @retval EFI_INVALID_LANGUAGE  The language specified by
                                FirstLanguage is not present in
                                the specified package list.

  @retval EFI_INVALID_PARAMETER FirstLanguage is NULL or
                                SecondLanguage is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GET_2ND_LANGUAGES) (
  IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
  IN CONST  EFI_HII_HANDLE            PackageList,
  IN CONST  CHAR8                     *FirstLanguage;
  IN OUT    CHAR8                     *SecondLanguages,
  IN OUT    UINTN                     SecondLanguagesSize
);


/**
  Services to manipulate the string.
   
  @param NewString  Add a new string. GetString Retrieve a
                    string and related string information.

  @param SetString  Change a string. 

  @param GetLanguages   List the languages for a particular
                        package list.

  @param GetSecondaryLanguages  List supported secondary
                                languages for a particular
                                primary language.

**/
struct _EFI_HII_STRING_PROTOCOL {
  EFI_HII_NEW_STRING        NewString;
  EFI_HII_GET_STRING        GetString;
  EFI_HII_SET_STRING        SetString;
  EFI_HII_GET_LANGUAGES     GetLanguages;
  EFI_HII_GET_2ND_LANGUAGES GetSecondaryLanguages;
};


extern EFI_GUID gEfiHiiStringProtocolGuid;

#endif

