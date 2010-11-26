/*++

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    HiiFont.h
    
Abstract:

    EFI_HII_FONT_PROTOCOL from UEFI 2.1 specification.
    
    This protocol provides interfaces to retrieve font information.

Revision History

--*/

#ifndef __EFI_HII_FONT_PROTOCOL_H__
#define __EFI_HII_FONT_PROTOCOL_H__

#include EFI_PROTOCOL_DEFINITION (GraphicsOutput)
#include EFI_PROTOCOL_DEFINITION (HiiImage)

//
// Global ID for the Hii Font Protocol.
//

#define EFI_HII_FONT_PROTOCOL_GUID \
  { \
    0xe9ca4775, 0x8657, 0x47fc, {0x97, 0xe7, 0x7e, 0xd6, 0x5a, 0x8, 0x43, 0x24} \
  }

EFI_FORWARD_DECLARATION (EFI_HII_FONT_PROTOCOL);

typedef UINT32  EFI_HII_OUT_FLAGS;
typedef UINT32  EFI_FONT_INFO_MASK;
typedef VOID*   EFI_FONT_HANDLE;

typedef struct _EFI_HII_ROW_INFO {
  UINTN StartIndex;
  UINTN EndIndex;
  UINTN LineHeight;
  UINTN LineWidth;
  UINTN BaselineOffset;
} EFI_HII_ROW_INFO;

typedef struct {
  EFI_HII_FONT_STYLE FontStyle;
  UINT16             FontSize; // character cell height in pixels
  CHAR16             FontName[1];
} EFI_FONT_INFO;

typedef struct _EFI_FONT_DISPLAY_INFO {
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL ForegroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL BackgroundColor;
  EFI_FONT_INFO_MASK            FontInfoMask;
  EFI_FONT_INFO                 FontInfo;  
} EFI_FONT_DISPLAY_INFO;

#define EFI_HII_OUT_FLAG_CLIP         0x00000001
#define EFI_HII_OUT_FLAG_WRAP         0x00000002
#define EFI_HII_OUT_FLAG_CLIP_CLEAN_Y 0x00000004
#define EFI_HII_OUT_FLAG_CLIP_CLEAN_X 0x00000008
#define EFI_HII_OUT_FLAG_TRANSPARENT  0x00000010
#define EFI_HII_IGNORE_IF_NO_GLYPH    0x00000020
#define EFI_HII_IGNORE_LINE_BREAK     0x00000040
#define EFI_HII_DIRECT_TO_SCREEN      0x00000080

#define EFI_FONT_INFO_SYS_FONT        0x00000001
#define EFI_FONT_INFO_SYS_SIZE        0x00000002
#define EFI_FONT_INFO_SYS_STYLE       0x00000004
#define EFI_FONT_INFO_SYS_FORE_COLOR  0x00000010
#define EFI_FONT_INFO_SYS_BACK_COLOR  0x00000020
#define EFI_FONT_INFO_RESIZE          0x00001000
#define EFI_FONT_INFO_RESTYLE         0x00002000
#define EFI_FONT_INFO_ANY_FONT        0x00010000
#define EFI_FONT_INFO_ANY_SIZE        0x00020000
#define EFI_FONT_INFO_ANY_STYLE       0x00040000

typedef
EFI_STATUS
(EFIAPI *EFI_HII_STRING_TO_IMAGE) (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  EFI_HII_OUT_FLAGS              Flags,
  IN  CONST EFI_STRING               String,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfo       OPTIONAL,
  IN  OUT EFI_IMAGE_OUTPUT           **Blt,
  IN  UINTN                          BltX,
  IN  UINTN                          BltY,
  OUT EFI_HII_ROW_INFO               **RowInfoArray    OPTIONAL,
  OUT UINTN                          *RowInfoArraySize OPTIONAL,
  OUT UINTN                          *ColumnInfoArray  OPTIONAL
  )
/*++

  Routine Description:
    Renders a string to a bitmap or to the display.

  Arguments:          
    This              - A pointer to the EFI_HII_FONT_PROTOCOL instance.
    Flags             - Describes how the string is to be drawn.                 
    String            - Points to the null-terminated string to be displayed.
    StringInfo        - Points to the string output information, including the color and font. 
                        If NULL, then the string will be output in the default system font and color.
    Blt               - If this points to a non-NULL on entry, this points to the image, which is Width pixels  
                        wide and Height pixels high. The string will be drawn onto this image and               
                        EFI_HII_OUT_FLAG_CLIP is implied. If this points to a NULL on entry, then a             
                        buffer will be allocated to hold the generated image and the pointer updated on exit. It
                        is the caller's responsibility to free this buffer.                                    
    BltX,BLTY         - Specifies the offset from the left and top edge of the image of the first character cell in
                        the image.                                                                                     
    RowInfoArray      - If this is non-NULL on entry, then on exit, this will point to an allocated buffer   
                        containing row information and RowInfoArraySize will be updated to contain the       
                        number of elements. This array describes the characters which were at least partially
                        drawn and the heights of the rows. It is the caller's responsibility to free this buffer.                            
    RowInfoArraySize  - If this is non-NULL on entry, then on exit it contains the number of elements in
                        RowInfoArray.                                                                   
    ColumnInfoArray   - If this is non-NULL, then on return it will be filled with the horizontal offset for each 
                        character in the string on the row where it is displayed. Non-printing characters will    
                        have the offset ~0. The caller is responsible to allocate a buffer large enough so that   
                        there is one entry for each character in the string, not including the null-terminator. It
                        is possible when character display is normalized that some character cells overlap.           
                     
  Returns:
    EFI_SUCCESS           - The string was successfully rendered.                           
    EFI_OUT_OF_RESOURCES  - Unable to allocate an output buffer for RowInfoArray or Blt.
    EFI_INVALID_PARAMETER - The String or Blt was NULL.
    EFI_INVALID_PARAMETER - Flags were invalid combination.
        
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_STRING_ID_TO_IMAGE) (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  EFI_HII_OUT_FLAGS              Flags,  
  IN  EFI_HII_HANDLE                 PackageList,
  IN  EFI_STRING_ID                  StringId,
  IN  CONST CHAR8*                   Language,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfo       OPTIONAL,
  IN  OUT EFI_IMAGE_OUTPUT           **Blt,
  IN  UINTN                          BltX,
  IN  UINTN                          BltY,
  OUT EFI_HII_ROW_INFO               **RowInfoArray    OPTIONAL,
  OUT UINTN                          *RowInfoArraySize OPTIONAL,
  OUT UINTN                          *ColumnInfoArray  OPTIONAL
  )
/*++

  Routine Description:
    Render a string to a bitmap or the screen containing the contents of the specified string.

  Arguments:          
    This              - A pointer to the EFI_HII_FONT_PROTOCOL instance.
    Flags             - Describes how the string is to be drawn.                 
    PackageList       - The package list in the HII database to search for the specified string.         
    StringId          - The string's id, which is unique within PackageList.                            
    Language          - Points to the language for the retrieved string. If NULL, then the current system
                        language is used.                                                                
    StringInfo        - Points to the string output information, including the color and font. 
                        If NULL, then the string will be output in the default system font and color.
    Blt               - If this points to a non-NULL on entry, this points to the image, which is Width pixels  
                        wide and Height pixels high. The string will be drawn onto this image and               
                        EFI_HII_OUT_FLAG_CLIP is implied. If this points to a NULL on entry, then a             
                        buffer will be allocated to hold the generated image and the pointer updated on exit. It
                        is the caller's responsibility to free this buffer.                                    
    BltX,BLTY         - Specifies the offset from the left and top edge of the image of the first character cell in
                        the image.                                                                                     
    RowInfoArray      - If this is non-NULL on entry, then on exit, this will point to an allocated buffer   
                        containing row information and RowInfoArraySize will be updated to contain the       
                        number of elements. This array describes the characters which were at least partially
                        drawn and the heights of the rows. It is the caller's responsibility to free this buffer.                            
    RowInfoArraySize  - If this is non-NULL on entry, then on exit it contains the number of elements in
                        RowInfoArray.                                                                   
    ColumnInfoArray   - If this is non-NULL, then on return it will be filled with the horizontal offset for each 
                        character in the string on the row where it is displayed. Non-printing characters will    
                        have the offset ~0. The caller is responsible to allocate a buffer large enough so that   
                        there is one entry for each character in the string, not including the null-terminator. It
                        is possible when character display is normalized that some character cells overlap.           
                     
  Returns:
    EFI_SUCCESS           - The string was successfully rendered.
    EFI_OUT_OF_RESOURCES  - Unable to allocate an output buffer for RowInfoArray or Blt.
    EFI_INVALID_PARAMETER - The Blt or PackageList was NULL.
    EFI_INVALID_PARAMETER - Flags were invalid combination.
    EFI_NOT_FOUND         - The specified PackageList is not in the Database or the stringid is not 
                            in the specified PackageList. 
        
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_GLYPH) (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  CHAR16                         Char,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfo,
  OUT EFI_IMAGE_OUTPUT               **Blt,
  OUT UINTN                          *Baseline OPTIONAL
  )
/*++

  Routine Description:
    Convert the glyph for a single character into a bitmap.

  Arguments:          
    This              - A pointer to the EFI_HII_FONT_PROTOCOL instance.
    Char              - Character to retrieve.
    StringInfo        - Points to the string font and color information or NULL if the string should use the
                        default system font and color.                                                      
    Blt               - Thus must point to a NULL on entry. A buffer will be allocated to hold the output and
                        the pointer updated on exit. It is the caller's responsibility to free this buffer. 
    Baseline          - Number of pixels from the bottom of the bitmap to the baseline.

  Returns:
    EFI_SUCCESS            - Glyph bitmap created.
    EFI_OUT_OF_RESOURCES   - Unable to allocate the output buffer Blt.    
    EFI_WARN_UNKNOWN_GLYPH - The glyph was unknown and was      
                             replaced with the glyph for Unicode
                             character 0xFFFD. 
    EFI_INVALID_PARAMETER  - Blt is NULL or *Blt is not NULL.
            
--*/    
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_FONT_INFO) (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  OUT   EFI_FONT_HANDLE          *FontHandle,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfoIn, OPTIONAL
  OUT       EFI_FONT_DISPLAY_INFO    **StringInfoOut,
  IN  CONST EFI_STRING               String OPTIONAL
  )
/*++

  Routine Description:
    This function iterates through fonts which match the specified font, using 
    the specified criteria. If String is non-NULL, then all of the characters in 
    the string must exist in order for a candidate font to be returned.
    
  Arguments:          
    This              - A pointer to the EFI_HII_FONT_PROTOCOL instance.
    FontHandle        - On entry, points to the font handle returned by a previous 
                        call to GetFontInfo() or points to NULL to start with the 
                        first font. On return, points to the returned font handle or
                        points to NULL if there are no more matching fonts.
    StringInfoIn      - Upon entry, points to the font to return information about.
                        If NULL, then the information about the system default 
                        font will be returned.
    StringInfoOut     - Upon return, contains the matching font's information. 
                        If NULL, then no information is returned.
                        It's caller's responsibility to free this buffer.
    String            - Points to the string which will be tested to determine 
                        if all characters are available. If NULL, then any font 
                        is acceptable.

  Returns:
    EFI_SUCCESS            - Matching font returned successfully.
    EFI_NOT_FOUND          - No matching font was found.
    EFI_INVALID_PARAMETER  - StringInfoIn->FontInfoMask is an invalid combination.
    EFI_OUT_OF_RESOURCES   - There were insufficient resources to complete the request.
            
--*/
;


//
// Interface structure for the EFI_HII_FONT_PROTOCOL
//
struct _EFI_HII_FONT_PROTOCOL {
  EFI_HII_STRING_TO_IMAGE       StringToImage;
  EFI_HII_STRING_ID_TO_IMAGE    StringIdToImage;
  EFI_HII_GET_GLYPH             GetGlyph;
  EFI_HII_GET_FONT_INFO         GetFontInfo;
};

extern EFI_GUID gEfiHiiFontProtocolGuid;

#endif
