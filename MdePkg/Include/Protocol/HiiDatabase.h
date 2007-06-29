/** @file
  The file provides Database manager for HII-related data
  structures.
  
  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __HII_DATABASE_H__
#define __HII_DATABASE_H__

#error "UEFI 2.1 HII is not fully implemented for now, Please don't include this file now."

#define EFI_HII_DATABASE_PROTOCOL_GUID \
  { 0xef9fc172, 0xa1b2, 0x4693, { 0xb3, 0x27, 0x6d, 0x32, 0xfc, 0x41, 0x60, 0x42 } }


typedef struct _EFI_HII_DATABASE_PROTOCOL EFI_HII_DATABASE_PROTOCOL;

//
// ConfigurationS of HII.
// 
#define GLYPH_WIDTH         8
#define GLYPH_HEIGHT        19

/**
    
  Each package starts with a header, as defined above, which  
  indicates the size and type of the package. When added to a  
  pointer pointing to the start of the header, Length points at  
  the next package. The package lists form a package list when  
  concatenated together and terminated with an  
  EFI_HII_PACKAGE_HEADER with a Type of EFI_HII_PACKAGE_END. The  
  type EFI_HII_PACKAGE_TYPE_GUID is used for vendor-defined HII  
  packages, whose contents are determined by the Guid. The range  
  of package types starting with EFI_HII_PACKAGE_TYPE_SYSTEM_BEGIN  
  through EFI_HII_PACKAGE_TYPE_SYSTEM_END are reserved for system  
  firmware implementers.  
  
  @param Length The size of the package in bytes.
  
  @param Type   The package type. See EFI_HII_PACKAGE_TYPE_x,
                below.
  
  @param Data   The package data, the format of which is
                determined by Type.
  
**/
typedef struct {
  UINT32  Length:24;
  UINT32  Type:8;
  // UINT8  Data[...];
} EFI_HII_PACKAGE_HEADER;

//
// EFI_HII_PACKAGE_TYPE_x.
// 
#define EFI_HII_PACKAGE_TYPE_ALL      0x00
#define EFI_HII_PACKAGE_TYPE_GUID     0x01
#define EFI_HII_PACKAGE_FORM_CONFIG   0x02
#define EFI_HII_PACKAGE_FORM_APP      0x03
#define EFI_HII_PACKAGE_STRINGS       0x04
#define EFI_HII_PACKAGE_FONTS         0x05
#define EFI_HII_PACKAGE_IMAGES        0x06
#define EFI_HII_PACKAGE_SIMPLE_FONTS  0x07
#define EFI_HII_PACKAGE_DEVICE_PATH   0x08
#define EFI_HII_PACKAGE_END           0x09
#define EFI_HII_PACKAGE_TYPE_SYSTEM_BEGIN   0xE0
#define EFI_HII_PACKAGE_TYPE_SYSTEM_END     0xFF


/**
  
  This header uniquely identifies the package list.and is placed   
  in front of a list of packages. Package lists with the same   
  PackageListGuid value should contain the same data set. Updated   
  versions should have updated GUIDs.   
  
  @param PackageListGuid  The unique identifier applied to the
                          list of packages which follows.
  
  
  @param PackageLength    The size of the package list (in
                         bytes), including the header.

**/
typedef struct {
  EFI_GUID  PackageListGuid;
  UINT32    PackagLength;
} EFI_HII_PACKAGE_LIST_HEADER;

/**

  The fonts must be presented in Unicode sort order. That is,
  the primary sort key is the UnicodeWeight and the secondary
  sort key is the SurrogateWeight. It is up to developers who
  manage fonts to choose efficient mechanisms for accessing
  fonts. The contiguous presentation can easily be used because
  narrow and wide glyphs are not intermixed, so a binary search
  is possible (hence the requirement that the glyphs be sorted
  by weight).

  @param Header   The header contains a Length and Type field.
                  In the case of a font package, the type will
                  be EFI_HII_PACKAGE_SIMPLE_FONTS and the length
                  will be the total size of the font package
                  including the size of the narrow and wide
                  glyphs. See EFI_HII_PACKAGE_HEADER.

  @param NumberOfNarrowGlyphs   The number of NarrowGlyphs that
                                are included in the font package.

  @param NumberOfWideGlyphs   The number of WideGlyphs that are
                              included in the font package.

  @param NarrowGlyphs   An array of EFI_NARROW_GLYPH entries.
                        The number of entries is specified by
                        NumberOfNarrowGlyphs.

  @param WideGlyphs   An array of EFI_WIDE_GLYPH entries. The
                      number of entries is specified by
                      NumberOfWideGlyphs. To calculate the
                      offset of WideGlyphs, use the offset of
                      NarrowGlyphs and add the size of
                      EFI_NARROW_GLYPH multiplied by the
                      NumberOfNarrowGlyphs.

*/
typedef struct _EFI_HII_SIMPLE_FONT_PACKAGE_HDR {
  EFI_HII_PACKAGE_HEADER  Header;
  UINT16                  NumberOfNarrowGlyphs;
  UINT16                  NumberOfWideGlyphs;
  // EFI_NARROW_GLYPH         NarrowGlyphs[];
  // EFI_WIDE_GLYPH           WideGlyphs[];
} EFI_HII_SIMPLE_FONT_PACKAGE_HDR;

//
// Contents of EFI_NARROW_GLYPH.Attributes
//
#define EFI_GLYPH_NON_SPACING 0x01
#define EFI_GLYPH_WIDE        0x02

/**

  Glyphs are represented by two structures, one each for the two
  sizes of glyphs. The narrow glyph (EFI_NARROW_GLYPH) is the
  normal glyph used for text display.

  @param UnicodeWeight  The Unicode representation of the glyph.
                        The term weight is the technical term
                        for a character value.

  @param Attributes   The data element containing the glyph
                      definitions; see Related Definitions
                      below.

  @param GlyphCol1  The column major glyph representation of the
                    character. Bits   with values of one
                    indicate that the corresponding pixel is to
                    be on when normally displayed; those with
                    zero are off.

**/
typedef struct {
  CHAR16  UnicodeWeight;
  UINT8   Attributes;
  UINT8   GlyphCol1[19];
} EFI_NARROW_GLYPH;

/**
   
  Glyphs are represented via the two structures, one each for the
  two sizes of glyphs. The wide glyph (EFI_WIDE_GLYPH) is large
  enough to display logographic characters.

  @param UnicodeWeight  The Unicode representation of the glyph.
                        The term weight is the technical term
                        for a character value.

  @param Attributes   The data element containing the glyph
                      definitions; see Related Definitions in
                      EFI_NARROW_GLYPH for attribute values.
  
  @param GlyphCol1, GlyphCol2   The column major glyph
                                representation of the character.
                                Bits with values of one indicate
                                that the corresponding pixel is
                                to be on when normally
                                displayed; those with zero are
                                off.
  
  @param Pad  Ensures that sizeof(EFI_WIDE_GLYPH) is twice the
              sizeof(EFI_NARROW_GLYPH). The contents of Pad must
              bezero.


**/
typedef struct {
  CHAR16  UnicodeWeight;
  UINT8   Attributes;
  UINT8   GlyphCol1[GLYPH_HEIGHT];
  UINT8   GlyphCol2[GLYPH_HEIGHT];
  UINT8   Pad[3];
} EFI_WIDE_GLYPH;


//
// EFI_HII_FONT_STYLE
// 
typedef UINT32  EFI_HII_FONT_STYLE;
#define EFI_HII_FONT_STYLE_BOLD       0x00000001
#define EFI_HII_FONT_STYLE_ITALIC     0x00000002
#define EFI_HII_FONT_STYLE_EMBOSS     0x00010000
#define EFI_HII_FONT_STYLE_OUTLINE    0x00020000
#define EFI_HII_FONT_STYLE_SHADOW     0x00040000
#define EFI_HII_FONT_STYLE_UNDERLINE  0x00080000
#define EFI_HII_FONT_STYLE_DBL_UNDER  0x00100000

//
// EFI_HII_GLYPH_BLOCK.BlockType
// 
#define EFI_HII_GIBT_END            0x00
#define EFI_HII_GIBT_GLYPH          0x10
#define EFI_HII_GIBT_GLYPHS         0x11
#define EFI_HII_GIBT_GLYPH_DEFAULT  0x12
#define EFI_HII_GIBT_GLYPHS_DEFAULT 0x13
#define EFI_HII_GIBT_DUPLICATE      0x20
#define EFI_HII_GIBT_SKIP2          0x21 
#define EFI_HII_GIBT_SKIP1          0x22
#define EFI_HII_GIBT_DEFAULTS       0x23
#define EFI_HII_GIBT_EXT1           0x30
#define EFI_HII_GIBT_EXT2           0x31
#define EFI_HII_GIBT_EXT4           0x32

/**

  EFI_HII_GIBT_END block is found. When processing the glyph
  blocks, each block refers to the current character value
  (CharValueCurrent), which is initially set to one (1). Glyph
  blocks of an unknown type should be skipped. If they cannot be
  skipped, then processing halts.

**/
typedef struct _EFI_HII_GLYPH_BLOCK {
  UINT8 BlockType;
  UINT8 BlockBody[1];
} EFI_HII_GLYPH_BLOCK;


/**

  @param Width  Width of the character or character cell, in
                pixels. For fixed-pitch fonts, this is the same
                as the advance.

  @param Height   Height of the character or character cell, in
                  pixels.

  @param OffsetX  Offset to the horizontal edge of the character
                  cell.

  @param OffsetY  Offset to the vertical edge of the character
                  cell.

  @param AdvanceX   Number of pixels to advance to the right
                    when moving from the origin of the current
                    glyph to the origin of the next glyph.
   
**/
typedef struct _EFI_HII_GLYPH_INFO {
  UINT16  Width;
  UINT16  Height;
  INT16   OffsetX;
  INT16   OffsetY;
  INT16   AdvanceX;
} EFI_HII_GLYPH_INFO;


/**
   
  Changes the default cell information used for subsequent
  EFI_HII_GIBT_GLYPH_DEFAULT and EFI_HII_GIBT_GLYPHS_DEFAULT glyph
  blocks. The cell information described by Cell remains in effect
  until the next EFI_HII_GIBT_DEFAULTS is found. Prior to the
  first EFI_HII_GIBT_DEFAULTS block, the cell information in the
  fixed header are used.

  @param Header   Standard glyph block header, where
                  Header.BlockType = EFI_HII_GIBT_DEFAULTS.
  
  @param Cell   The new default cell information which will be
                applied to all subsequent GLYPH_DEFAULT and
                GLYPHS_DEFAULT blocks.

**/
typedef struct _EFI_HII_GIBT_DEFAULTS_BLOCK {
  EFI_HII_GLYPH_BLOCK   Header;
  EFI_HII_GLYPH_INFO    Cell;
} EFI_HII_GIBT_DEFAULTS_BLOCK;


/**
   
  Indicates that the glyph with character value CharValueCurrent
  has the same glyph as a previously defined character value and
  increments CharValueCurrent by one.

  @param Header   Standard glyph block header, where
                  Header.BlockType = EFI_HII_GIBT_DUPLICATE.

  @param CharValue  The previously defined character value with
                    the exact same glyph.

**/
typedef struct _EFI_HII_GIBT_DUPLICATE_BLOCK {
  EFI_HII_GLYPH_BLOCK   Header;
  CHAR16                CharValue;
} EFI_HII_GIBT_DUPLICATE_BLOCK;


/**
   
  Any glyphs with a character value greater than or equal to
  CharValueCurrent are empty.

  @param Header   Standard glyph block header, where
                  Header.BlockType = EFI_HII_GIBT_END.

**/
typedef struct _EFI_GLYPH_GIBT_END_BLOCK {
  EFI_HII_GLYPH_BLOCK   Header;
} EFI_GLYPH_GIBT_END_BLOCK;

/**
  
  These are reserved for future expansion, with length bytes
  included so that they can be easily skipped.

  @param Header   Standard glyph block header, where
                  Header.BlockType = EFI_HII_GIBT_EXT1,
                  EFI_HII_GIBT_EXT2 or EFI_HII_GIBT_EXT4.

  @param Length   Size of the glyph block, in bytes.

**/
typedef struct _EFI_HII_GIBT_EXT1_BLOCK {
  EFI_HII_GLYPH_BLOCK Header;
  UINT8               BlockType2;
  UINT8               Length;
} EFI_HII_GIBT_EXT1_BLOCK;


/**
  
  These are reserved for future expansion, with length bytes
  included so that they can be easily skipped.

  @param Header   Standard glyph block header, where
                  Header.BlockType = EFI_HII_GIBT_EXT1,
                  EFI_HII_GIBT_EXT2 or EFI_HII_GIBT_EXT4.

  @param Length   Size of the glyph block, in bytes.

**/
typedef struct _EFI_HII_GIBT_EXT2_BLOCK {
  EFI_HII_GLYPH_BLOCK Header;
  UINT8               BlockType2;
  UINT16              Length;
} EFI_HII_GIBT_EXT2_BLOCK;

/**
  
  These are reserved for future expansion, with length bytes
  included so that they can be easily skipped.

  @param Header   Standard glyph block header, where
                  Header.BlockType = EFI_HII_GIBT_EXT1,
                  EFI_HII_GIBT_EXT2 or EFI_HII_GIBT_EXT4.

  @param Length   Size of the glyph block, in bytes.

**/
typedef struct _EFI_HII_GIBT_EXT4_BLOCK {
  EFI_HII_GLYPH_BLOCK Header;
  UINT8               BlockType2;
  UINT32              Length;
} EFI_HII_GIBT_EXT4_BLOCK;


/**
   
  This block provides the bitmap for the character with the value
  CharValueCurrent and increments CharValueCurrent by one. Each
  glyph contains a glyph width and height, a drawing offset,
  number of pixels to advance after drawing and then the encoded
  bitmap.

  @param Header   Standard glyph block header, where
                  Header.BlockType = EFI_HII_GIBT_GLYPH.

  @param Cell   Contains the width and height of the encoded
                bitmap (Cell.Width and Cell.Height), the number
                of pixels (signed) right of the character cell
                origin where the left edge of the bitmap should
                be placed (Cell.OffsetX), the number of pixels
                above the character cell origin where the top
                edge of the bitmap should be placed
                (Cell.OffsetY) and the number of pixels (signed)
                to move right to find the origin for the next
                charactercell (Cell.AdvanceX).

  @param GlyphCount   The number of glyph bitmaps.

  @param BitmapData   The bitmap data specifies a series of
                      pixels, one bit per pixel, left-to-right,
                      top-tobottom. Each glyph bitmap only
                      encodes the portion of the bitmap enclosed
                      by its character-bounding box, but the
                      entire glyph is padded out to the nearest
                      byte. The number of bytes per bitmap can
                      be calculated as: ((Cell.Width + 7)/8) *
                      Cell.Height.


**/
typedef struct _EFI_HII_GIBT_GLYPH_BLOCK {
  EFI_HII_GLYPH_BLOCK   Header;
  EFI_HII_GLYPH_INFO    Cell;
  UINT16                GlyphCount;
  UINT8                 BitmapData[1];
} EFI_HII_GIBT_GLYPH_BLOCK;

/**
   
  Provides the bitmaps for the characters with the values
  CharValueCurrent through CharValueCurrent + Count -1 and
  increments CharValueCurrent by Count. These glyphs have
  identical cell information and the encoded bitmaps are exactly
  the same number of byes.

  @param Header   Standard glyph block header, where
                  Header.BlockType = EFI_HII_GIBT_GLYPHS.

  @param Cell     Contains the width and height of the encoded
                  bitmap (Cell.Width and Cell.Height), the
                  number of pixels (signed) right of the
                  character cell origin where the left edge of
                  the bitmap should be placed (Cell.OffsetX),
                  the number of pixels above the character cell
                  origin where the top edge of the bitmap should
                  be placed (Cell.OffsetY) and the number of
                  pixels(signed) to move right to find the
                  origin for the next character cell
                  (Cell.AdvanceX).

  @param BitmapData   The bitmap data specifies a series of
                      pixels, one bit per pixel, left-to-right,
                      top-tobottom, for each glyph. Each glyph
                      bitmap only encodes the portion of the
                      bitmap enclosed by its character-bounding
                      box. The number of bytes per bitmap can be
                      calculated as: ((Cell.Width + 7)/8) *
                      Cell.Height.

**/
typedef struct _EFI_HII_GIBT_GLYPHS_BLOCK {
  EFI_HII_GLYPH_BLOCK   Header;
  EFI_HII_GLYPH_INFO    Cell;
  UINT8                 BitmapData[1];
} EFI_HII_GIBT_GLYPHS_BLOCK;

/**
   
  Provides the bitmap for the character with the value
  CharValueCurrent and increments CharValueCurrent by 1. This
  glyph uses the default cell information. The default cell
  information is found in the font header or the most recently
  processed EFI_HII_GIBT_DEFAULTS.

  @param Header   Standard glyph block header, where
                  Header.BlockType = EFI_HII_GIBT_GLYPH_DEFAULT.

  @param BitmapData   The bitmap data specifies a series of
                      pixels, one bit per pixel, left-to-right,
                      top-tobottom. Each glyph bitmap only
                      encodes the portion of the bitmap enclosed
                      by its character-bounding box. The number
                      of bytes per bitmap can be calculated as:
                      ((Global.Cell.Width + 7)/8) *
                      Global.Cell.Height.

**/
typedef struct _EFI_HII_GIBT_GLYPH_DEFAULT_BLOCK {
  EFI_HII_GLYPH_BLOCK Header;
  UINT8               BitmapData[1];
} EFI_HII_GIBT_GLYPH_DEFAULT_BLOCK;




/**
   
  Provides the bitmaps for the characters with the values
  CharValueCurrent through CharValueCurrent + Count -1 and
  increments CharValueCurrent by Count. These glyphs use the
  default cell information and the encoded bitmaps have exactly
  the same number of byes.

  @param Header   Standard glyph block header, where
                  Header.BlockType =
                  EFI_HII_GIBT_GLYPHS_DEFAULT.

  @param Count    Number of glyphs in the glyph block.

  @param BitmapData   The bitmap data specifies a series of
                      pixels, one bit per pixel, left-to-right,
                      top-tobottom, for each glyph. Each glyph
                      bitmap only encodes the portion of the
                      bitmap enclosed by its character-bounding
                      box. The number of bytes per bitmap can be
                      calculated as: ((Global.Cell.Width + 7)/8)
                      Global.Cell.Height.

**/
typedef struct _EFI_HII_GIBT_GLYPHS_DEFAULT_BLOCK {
  EFI_HII_GLYPH_BLOCK Header;
  UINT16              Count;
  UINT8               BitmapData[1];
} EFI_HII_GIBT_GLYPHS_DEFAULT_BLOCK;

/**
   
  Increments the current character value CharValueCurrent by the
  number specified.

  @param Header   Standard glyph block header, where BlockType =
                  EFI_HII_GIBT_SKIP1 or EFI_HII_GIBT_SKIP2.

  @param SkipCount  The unsigned 8- or 16-bit value to add to
                    CharValueCurrent.

**/
typedef struct _EFI_HII_GIBT_SKIP2_BLOCK {
  EFI_HII_GLYPH_BLOCK Header;
  UINT16              SkipCount;
} EFI_HII_GIBT_SKIP2_BLOCK;


/**
   
  Increments the current character value CharValueCurrent by the
  number specified.

  @param Header   Standard glyph block header, where BlockType =
                  EFI_HII_GIBT_SKIP1 or EFI_HII_GIBT_SKIP2.

  @param SkipCount  The unsigned 8- or 16-bit value to add to
                    CharValueCurrent.

**/
typedef struct _EFI_HII_GIBT_SKIP1_BLOCK {
  EFI_HII_GLYPH_BLOCK Header;
  UINT8               SkipCount;
} EFI_HII_GIBT_SKIP1_BLOCK;


/**
   
  This package is created by NewPackageList() when the package
  list is first added to the HII database by locating the
  EFI_DEVICE_PATH_PROTOCOL attached to the driver handle passed in
  to that function.
  
**/
typedef EFI_DEVICE_PATH_PROTOCOL  EFI_HII_DEVICE_PATH_PACKAGE_HDR;


/**
   
  This is a free-form package type designed to allow extensibility
  by allowing the format to be specified using Guid.

  @param Guid   Identifier which describes the remaining data
                within the package.

**/
typedef struct _EFI_HII_GUID_PACKAGE_HDR {
  EFI_GUID Guid;
} EFI_HII_GUID_PACKAGE_HDR;


/**
   
  The Strings package record describes the mapping between string
  identifiers and the actual text of the strings themselves. The
  package consists of three parts: a fixed header, the string
  information and the font information.

  @param Header   The standard package header, where Header.Type
                  = EFI_HII_PACKAGE_STRINGS.

  @param HdrSize  Size of this header.

  @param StringInfoOffset   Offset, relative to the start of
                            this header, of the string information.

  @param LanguageWindow   Specifies the default values placed in
                          the static and dynamic windows before
                          processing each SCSU-encoded strings.


  @param LanguageName   String identifier within the current
                        string package of the full name of the
                        language specified by Language. Language
                        Language of the strings, as specified by
                        RFC 3066.

**/
typedef struct _EFI_HII_STRING_PACKAGE_HDR {
  EFI_HII_PACKAGE_HEADER  Header;
  UINT32                  HdrSize;
  UINT32                  StringInfoOffset;
  CHAR16                  LanguageWindow[16];
  EFI_STRING_ID           LanguageName;
  CHAR8 Language[1];
} EFI_HII_STRING_PACKAGE_HDR;



/**

  The fixed header consists of a standard record header and then
  the character values in this section, the flags (including the
  encoding method) and the offsets of the glyph information, the
  glyph bitmaps and the character map.

  @param Header   The standard package header, where Header.Size
                  EFI_HII_PACKAGE_FONTS.

  @param HdrSize  Size of this header.

  @param GlyphInfoOffset  The offset, relative to the start of
                          this header, of a series of
                          variable-length glyph blocks, each
                          describing information about the
                          bitmap associated with a glyph.

  @param Cell   This contains the measurement of the widest and
                tallest characters in the font (Cell.Width and
                Cell.Height). It also contains the offset to the
                horizontal and vertical origin point of the
                character cell (Cell.OffsetX and Cell.OffsetY).
                Finally, it contains the default AdvanceX. The
                individual glyph's OffsetX and OffsetY value is
                added to this position to determine where to
                draw the top-left pixel of the character's
                glyph. The character glyph's AdvanceX is added
                to this position to determine the origin point
                for the next character.

  @param FontStyle  The design style of the font, 1 bit per
                    style. See EFI_HII_FONT_STYLE.

  @param FontFamily   The null-terminated string with the name
                      of the font family to which the font
                      belongs.

**/
typedef struct _EFI_HII_FONT_PACKAGE_HDR {
  EFI_HII_PACKAGE_HEADER  Header;
  UINT32                  HdrSize;
  UINT32                  GlyphBlockOffset;
  EFI_HII_GLYPH_INFO      Cell;
  EFI_HII_FONT_STYLE      FontStyle;
  CHAR16                  FontFamily[1];
} EFI_HII_FONT_PACKAGE_HDR;


//
// EFI_HII_STRING_BLOCK.BlockType
// 
#define FI_HII_SIBT_END                 0x00
#define EFI_HII_SIBT_STRING_SCSU        0x10
#define EFI_HII_SIBT_STRING_SCSU_FONT   0x11
#define EFI_HII_SIBT_STRINGS_SCSU       0x12
#define EFI_HII_SIBT_STRINGS_SCSU_FONT  0x13
#define EFI_HII_SIBT_STRING_UCS2        0x14
#define EFI_HII_SIBT_STRING_UCS2_FONT   0x15
#define EFI_HII_SIBT_STRINGS_UCS2       0x16
#define EFI_HII_SIBT_STRINGS_UCS2_FONT  0x17
#define EFI_HII_SIBT_DUPLICATE          0x20
#define EFI_HII_SIBT_SKIP2              0x21
#define EFI_HII_SIBT_SKIP1              0x22
#define EFI_HII_SIBT_EXT1               0x30
#define EFI_HII_SIBT_EXT2               0x31
#define EFI_HII_SIBT_EXT4               0x32
#define EFI_HII_SIBT_FONT               0x40

/**
   
  String blocks specify the text and font for the current string
  identifier and increment to the next string identifier.
**/
typedef struct {
  UINT8   BlockType;
  UINT8   BlockBody[1];
} EFI_HII_STRING_BLOCK;


/**
   
  Indicates that the string with string identifier
  StringIdCurrent is the same as a previously defined string and
  increments StringIdCurrent by one.
  
  @param Header   Standard string block header, where
                  Header.BlockType = EFI_HII_SIBT_DUPLICATE.
  
  @param StringId   The string identifier of a previously
                    defined string with the exact same string
                    text. Description


**/
typedef struct _EFI_HII_SIBT_DUPLICATE_BLOCK {
  EFI_HII_STRING_BLOCK  Header;
  EFI_STRING_ID         StringId;
} EFI_HII_SIBT_DUPLICATE_BLOCK;

/**
   
  Any strings with a string identifier greater than or equal to
  StringIdCurrent are empty.

  @param Header   Standard string block header, where
                  Header.BlockType = EFI_HII_SIBT_END.

**/
typedef struct _EFI_HII_SIBT_END_BLOCK {
  EFI_HII_STRING_BLOCK  Header;
} EFI_HII_SIBT_END_BLOCK;


/**
   
  These are reserved for future expansion, with length bytes
  included so that they can be easily skip

  @param Header   Standard string block header, where
                  Header.BlockType = EFI_HII_SIBT_EXT1,
                  EFI_HII_SIBT_EXT2 or EFI_HII_SIBT_EXT4.
  
  @param Length   Size of the string block, in bytes.

**/
typedef struct _EFI_HII_SIBT_EXT1_BLOCK {
  EFI_HII_STRING_BLOCK  Header;
  UINT8                 BlockType2;
  UINT8                 Length;
} EFI_HII_SIBT_EXT1_BLOCK;

/**
   
  These are reserved for future expansion, with length bytes
  included so that they can be easily skip

  @param Header   Standard string block header, where
                  Header.BlockType = EFI_HII_SIBT_EXT1,
                  EFI_HII_SIBT_EXT2 or EFI_HII_SIBT_EXT4.
  
  @param Length   Size of the string block, in bytes.

**/
typedef struct _EFI_HII_SIBT_EXT2_BLOCK {
  EFI_HII_STRING_BLOCK  Header;
  UINT8                 BlockType2;
  UINT16                Length;
} EFI_HII_SIBT_EXT2_BLOCK;

/**
   
  These are reserved for future expansion, with length bytes
  included so that they can be easily skip

  @param Header   Standard string block header, where
                  Header.BlockType = EFI_HII_SIBT_EXT1,
                  EFI_HII_SIBT_EXT2 or EFI_HII_SIBT_EXT4.
  
  @param Length   Size of the string block, in bytes.

**/
typedef struct _EFI_HII_SIBT_EXT4_BLOCK {
  EFI_HII_STRING_BLOCK  Header;
  UINT8                 BlockType2;
  UINT32                Length;
} EFI_HII_SIBT_EXT4_BLOCK;

/**
   
  Associates a font identifier FontId with a font name FontName,
  size FontSize and style FontStyle. This font identifier may be
  used with the string blocks. The font identifier 0 is the
  default font for those string blocks which do not specify a font
  identifier.

  @param Header   Standard extended header, where
                  Header.BlockType = EFI_HII_SIBT_FONT.

  @param FontId   Font identifier, which must be unique within
                  the font package.

  @param FontSize   Character cell size, in pixels, of the font.

  @param FontStyle  Font style.

  @param FontName   Null-terminated font family name.

**/
typedef struct _EFI_HII_SIBT_FONT_BLOCK {
  EFI_HII_SIBT_EXT2_BLOCK   Header;
  UINT8                     FontId;
  UINT16                    FontSize;
  EFI_HII_FONT_STYLE        FontStyle;
  CHAR16                    FontName[1];
} EFI_HII_SIBT_FONT_BLOCK;

/**
   
  Increments the current string identifier StringIdCurrent by the
  number specified.

  @param Header   Standard string block header, where
                  Header.BlockType = EFI_HII_SIBT_SKIP1.
  
  @param SkipCount  The unsigned 8-bit value to add to
                    StringIdCurrent.

**/
typedef struct _EFI_HII_SIBT_SKIP1_BLOCK {
  EFI_HII_STRING_BLOCK  Header;
  UINT8                 SkipCount;
} EFI_HII_SIBT_SKIP1_BLOCK;

/**

  Increments the current string identifier StringIdCurrent by
  the number specified.

  @param Header   Standard string block header, where
                  Header.BlockType = EFI_HII_SIBT_SKIP2.

  @param SkipCount  The unsigned 16-bit value to add to
                    StringIdCurrent.

**/
typedef struct _EFI_HII_SIBT_SKIP2_BLOCK {
  EFI_HII_STRING_BLOCK  Header;
  UINT16                SkipCount;
} EFI_HII_SIBT_SKIP2_BLOCK;

/**
   
  This string block provides the SCSU-encoded text for the string
  in the default font with string identifier StringIdCurrent and
  increments StringIdCurrent by one.

  @param Header   Standard header where Header.BlockType =
                  EFI_HII_SIBT_STRING_SCSU.

  @param StringText   The string text is a null-terminated
                      string, which is assigned to the string
                      identifier StringIdCurrent.

**/
typedef struct _EFI_HII_SIBT_STRING_SCSU_BLOCK {
  EFI_HII_STRING_BLOCK Header;
  UINT8 StringText[1];
} EFI_HII_SIBT_STRING_SCSU_BLOCK;


/**

  This string block provides the SCSU-encoded text for the string
  in the font specified by FontIdentifier with string identifier
  StringIdCurrent and increments StringIdCurrent by one.

  @param Header   Standard string block header, where
                  Header.BlockType = EFI_HII_SIBT_STRING_SCSU_FONT.

  @param FontIdentifier   The identifier of the font to be used
                          as the starting font for the entire
                          string. The identifier must either be
                          0 for the default font or an
                          identifier previously specified by an
                          EFI_HII_SIBT_FONT block. Any string
                          characters that deviates from this
                          font family, size or style must
                          provide an explicit control character.

  @param StringText   The string text is a null-terminated
                      encoded string, which is assigned to the
                      string identifier StringIdCurrent.


**/
typedef struct _EFI_HII_SIBT_STRING_SCSU_FONT_BLOCK {
  EFI_HII_STRING_BLOCK  Header;
  UINT8                 FontIdentifier;
  UINT8                 StringText[1];
} EFI_HII_SIBT_STRING_SCSU_FONT_BLOCK;


/**
   
  This string block provides the SCSU-encoded text for StringCount
  strings which have the default font and which have sequential
  string identifiers. The strings are assigned the identifiers,
  starting with StringIdCurrent and continuing through
  StringIdCurrent + StringCount ??C 1. StringIdCurrent is
  incremented by StringCount.

  @param Header   Standard header where Header.BlockType =
                  EFI_HII_SIBT_STRINGS_SCSU.

  @param StringCount  Number of strings in StringText.

  @param StringText   The strings, where each string is a
                      null-terminated encoded string.

**/
typedef struct _EFI_HII_SIBT_STRINGS_SCSU_BLOCK {
  EFI_HII_STRING_BLOCK  Header;
  UINT16                StringCount;
  UINT8                 StringText[1];
} EFI_HII_SIBT_STRINGS_SCSU_BLOCK;


/**
   
  This string block provides the SCSU-encoded text for StringCount
  strings which have the font specified by FontIdentifier and
  which have sequential string identifiers. The strings are
  assigned the identifiers, starting with StringIdCurrent and
  continuing through StringIdCurrent + StringCount ??C 1.
  StringIdCurrent is incremented by StringCount.

  @param Header   Standard header where Header.BlockType =
                  EFI_HII_SIBT_STRINGS_SCSU_FONT.

  @param StringCount  Number of strings in StringText.

  @param FontIdentifier   The identifier of the font to be used
                          as the starting font for the entire
                          string. The identifier must either be
                          0 for the default font or an
                          identifier previously specified by an
                          EFI_HII_SIBT_FONT block. Any string
                          characters that deviates from this
                          font family, size or style must
                          provide an explicit control character.

  @param StringText   The strings, where each string is a
                      null-terminated encoded string.

**/
typedef struct _EFI_HII_SIBT_STRINGS_SCSU_FONT_BLOCK {
  EFI_HII_STRING_BLOCK  Header;
  UINT16                StringCount;
  UINT8                 FontIdentifier;
  UINT8                 StringText[1];
} EFI_HII_SIBT_STRINGS_SCSU_FONT_BLOCK;


/**
   
  This string block provides the UCS-2 encoded text for the string
  in the default font with string identifier StringIdCurrent and
  increments StringIdCurrent by one.

  @param Header   Standard header where Header.BlockType =
                  EFI_HII_SIBT_STRING_UCS2.

  @param StringText   The string text is a null-terminated UCS-2
                      string, which is assigned to the string
                      identifier StringIdCurrent.

**/
typedef struct _EFI_HII_SIBT_STRING_UCS2_BLOCK {
  EFI_HII_STRING_BLOCK  Header;
  CHAR16                StringText[1];
} EFI_HII_SIBT_STRING_UCS2_BLOCK;


/**
   
  This string block provides the UCS-2 encoded text for the string
  in the font specified by FontIdentifier with string identifier
  StringIdCurrent and increments StringIdCurrent by one

  @param Header   Standard header where Header.BlockType =
                  EFI_HII_SIBT_STRING_UCS2_FONT.

  @param FontIdentifier   The identifier of the font to be used
                          as the starting font for the entire
                          string. The identifier must either be
                          0 for the default font or an
                          identifier previously specified by an
                          EFI_HII_SIBT_FONT block. Any string
                          characters that deviates from this
                          font family, size or style must
                          provide an explicit control character.

  @param StringText   The string text is a null-terminated UCS-2
                      string, which is assigned to the string
                      identifier StringIdCurrent.

**/
typedef struct _EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK {
  EFI_HII_STRING_BLOCK  Header;
  UINT8                 FontIdentifier;
  CHAR16                StringText[1];
} EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK;


/**
   
  This string block provides the UCS-2 encoded text for the
  strings in the default font with string identifiers
  StringIdCurrent to StringIdCurrent + StringCount - 1 and
  increments StringIdCurrent by StringCount.

  @param Header   Standard header where Header.BlockType =
                  EFI_HII_SIBT_STRINGS_UCS2.

  @param StringCount  Number of strings in StringText.

  @param StringText   The string text is a series of
                      null-terminated UCS-2 strings, which are
                      assigned to the string identifiers
                      StringIdCurrent.to StringIdCurrent +
                      StringCount - 1.
  
**/
typedef struct _EFI_HII_SIBT_STRINGS_UCS2_BLOCK {
  EFI_HII_STRING_BLOCK  Header;
  UINT16                StringCount;
  CHAR16                StringText[1];
} EFI_HII_SIBT_STRINGS_UCS2_BLOCK;


/**
   
  The fixed header consists of a standard record header and the
  offsets of the image and palette information.

  @param Header   Standard package header, where Header.Type =
                  EFI_HII_PACKAGE_IMAGES. ImageInfoOffset
                  Offset, relative to this header, of the image
                  information. If this is zero, then there are
                  no images in the package.

  @param PaletteInfoOffset  Offset, relative to this header, of
                            the palette information. If this is
                            zero, then there are no palettes in
                            the image package.

**/
typedef struct _EFI_HII_IMAGE_PACKAGE_HDR {
  EFI_HII_PACKAGE_HEADER  Header;
  UINT32                  ImageInfoOffset;
  UINT32                  PaletteInfoOffset;
} EFI_HII_IMAGE_PACKAGE_HDR;


//
// EFI_HII_IMAGE_BLOCK
// 
typedef struct _EFI_HII_IMAGE_BLOCK {
  UINT8   BlockType;
  UINT8   BlockBody[1];
} EFI_HII_IMAGE_BLOCK;

//
// EFI_HII_IMAGE_BLOCK.BlockType.
// 
#define EFI_HII_IIBT_END                0x00
#define EFI_HII_IIBT_IMAGE_1BIT         0x10
#define EFI_HII_IIBT_IMAGE_1BIT_TRANS   0x11
#define EFI_HII_IIBT_IMAGE_4BIT         0x12
#define EFI_HII_IIBT_IMAGE_4BIT_TRANS   0x13
#define EFI_HII_IIBT_IMAGE_8BIT         0x14
#define EFI_HII_IIBT_IMAGE_8BIT_TRANS   0x15
#define EFI_HII_IIBT_IMAGE_24BIT        0x16
#define EFI_HII_IIBT_IMAGE_24BIT_TRANS  0x17
#define EFI_HII_IIBT_IMAGE_JPEG         0x18
#define EFI_HII_IIBT_DUPLICATE          0x20
#define EFI_HII_IIBT_SKIP2              0x21
#define EFI_HII_IIBT_SKIP1              0x22
#define EFI_HII_IIBT_EXT1               0x30
#define EFI_HII_IIBT_EXT2               0x31
#define EFI_HII_IIBT_EXT4               0x32


/**
   
  Any images with an image identifier greater than or equal to
  ImageIdCurrent are empty.

  @param Header   Standard image block header, where
                  Header.BlockType = EFI_HII_IIBT_END.

**/
typedef struct _EFI_HII_IIBT_END_BLOCK {
  EFI_HII_IMAGE_BLOCK   Header;
} EFI_HII_IIBT_END_BLOCK;


/**
   
  Future extensions for image records which need a length-byte
  length use this prefix.

  @param Header   Standard image block header, where
                  Header.BlockType = EFI_HII_IIBT_EXT1,
                  EFI_HII_IIBT_EXT2 or EFI_HII_IIBT_EXT4.
  
  @param Length   Size of the image block, in bytes, including
                  the image block header.

**/
typedef struct _EFI_HII_IIBT_EXT1_BLOCK {
  EFI_HII_IMAGE_BLOCK Header;
  UINT8               BlockType2;
  UINT8               Length;
} EFI_HII_IIBT_EXT1_BLOCK;

/**
   
  Future extensions for image records which need a length-byte
  length use this prefix.

  @param Header   Standard image block header, where
                  Header.BlockType = EFI_HII_IIBT_EXT1,
                  EFI_HII_IIBT_EXT2 or EFI_HII_IIBT_EXT4.
  
  @param Length   Size of the image block, in bytes, including
                  the image block header.

**/
typedef struct _EFI_HII_IIBT_EXT2_BLOCK {
  EFI_HII_IMAGE_BLOCK Header;
  UINT8               BlockType2;
  UINT16              Length;
} EFI_HII_IIBT_EXT2_BLOCK;

/**
   
  Future extensions for image records which need a length-byte
  length use this prefix.

  @param Header   Standard image block header, where
                  Header.BlockType = EFI_HII_IIBT_EXT1,
                  EFI_HII_IIBT_EXT2 or EFI_HII_IIBT_EXT4.
  
  @param Length   Size of the image block, in bytes, including
                  the image block header.

**/
typedef struct _EFI_HII_IIBT_EXT4_BLOCK {
  EFI_HII_IMAGE_BLOCK Header;
  UINT8               BlockType2;
  UINT32              Length;
} EFI_HII_IIBT_EXT4_BL0CK;

//
// EFI_HII_IIBT_IMAGE_1BIT_BASE
// 
typedef struct _EFI_HII_IIBT_IMAGE_1BIT_BASE {
  UINT16 Width;
  UINT16 Height;
  // UINT8 Data[...];
} EFI_HII_IIBT_IMAGE_1BIT_BASE;

/**

  This record assigns the 1-bit-per-pixel bitmap data to the
  ImageIdCurrent identifier and increment ImageIdCurrent by one.
  The data in the EFI_HII_IMAGE_1BIT_TRANS structure is exactly
  the same as the EFI_HII_IMAGE_1BIT structure, the difference is
  how the data is treated. The bitmap pixel value 0 is the
  transparency value and will not be written to the
  screen. The bitmap pixel value 1 will be translated to the color
  specified by Palette.

  @param Header   Standard image header, where Header.BlockType
                  = EFI_HII_IIBT_IMAGE_1BIT_TRANS.

  @param PaletteIndex   Index of the palette in the palette
                        information.

  @param Bitmap   The bitmap specifies a series of pixels, one
                  bit per pixel, left-to-right, top-to-bottom,
                  and is padded out to the nearest byte. The
                  number of bytes per bitmap can be calculated
                  as: ((Width + 7)/8) * Height.

**/
typedef struct _EFI_HII_IBIT_IMAGE_1BIT_BLOCK {
  EFI_HII_IMAGE_BLOCK           Header;
  UINT8                         PaletteIndex;
  EFI_HII_IIBT_IMAGE_1BIT_BASE  Bitmap;
} EFI_HII_IIBT_IMAGE_1BIT_BLOCK;

typedef EFI_HII_IIBT_IMAGE_1BIT_BLOCK   EFI_HII_IIBT_IMAGE_1BIT_TRANS_BLOCK;


//
// EFI_HII_RGB_PIXEL
// 
typedef struct _EFI_HII_RGB_PIXEL {
  UINT8 b;
  UINT8 g;
  UINT8 r;
} EFI_HII_RGB_PIXEL;

//
// FI_HII_IIBT_IMAGE_24BIT_BASE
// 
typedef struct _EFI_HII_IIBT_IMAGE_24BIT_BASE {
  UINT16 Width;
  UINT16 Height;
  // EFI_HII_RGB_PIXEL Bitmap[...];
} EFI_HII_IIBT_IMAGE_24BIT_BASE;

/**

  This record assigns the 24-bit-per-pixel bitmap data to the   
  ImageIdCurrent identifier and increment ImageIdCurrent by one.   
  The image's upper left hand corner pixel is composed of the
  first three bitmap bytes. The first byte is the pixel????s blue   
  component value, the next byte is the pixel????s green component   
  value, and the third byte is the pixel's red component value
  (B,G,R). Each color component value can vary from 0x00 (color   
  off) to 0xFF (color full on), allowing 16.8 millions colors that   
  can be specified.

  @param Header   Standard image header, where Header.BlockType
                  = EFI_HII_IIBT_IMAGE_24BIT. Bitmap The bitmap
                  specifies a series of pixels, 24 bits per
                  pixel, left-to-right, top-to-bottom. The
                  number of bytes per bitmap can be calculated
                  as: (Width * 3) * Height.

  @param Type     See EFI_HII_RGB_PIXEL definition.

**/
typedef struct {
  EFI_HII_IMAGE_BLOCK           Header;
  EFI_HII_IIBT_IMAGE_24BIT_BASE Bitmap;
} EFI_HII_IIBT_IMAGE_24BIT_BLOCK;

typedef EFI_HII_IIBT_IMAGE_24BIT_BLOCK EFI_HII_IIBT_IMAGE_24BIT_TRANS_BLOCK;



//
// EFI_HII_IIBT_IMAGE_4BIT_BASE
// 
typedef struct _EFI_HII_IIBT_IMAGE_4BIT_BASE {
  UINT16 Width;
  UINT16 Height;
  // UINT8 Data[...];
} EFI_HII_IIBT_IMAGE_4BIT_BASE;

/**
   
  This record assigns the 4-bit-per-pixel bitmap data to the
  ImageIdCurrent identifier using the specified palette and
  increment ImageIdCurrent by one. The image????s upper left hand
  corner pixel is the most significant nibble of the first bitmap
  byte.

  @param Header   Standard image header, where Header.BlockType
                  = EFI_HII_IIBT_IMAGE_4BIT.

  @param PaletteIndex   Index of the palette in the palette
                        information.

  @param Bitmap   The bitmap specifies a series of pixels, four
                  bits per pixel, left-to-right, top-to-bottom,
                  and is padded out to the nearest byte. The
                  number of bytes per bitmap can be calculated
                  as: ((Width + 1)/2) Height.

**/
typedef struct _EFI_HII_IIBT_IMAGE_4BIT_BLOCK {
  EFI_HII_IMAGE_BLOCK           Header;
  UINT8                         PaletteIndex;
  EFI_HII_IIBT_IMAGE_4BIT_BASE  Bitmap;
} EFI_HII_IIBT_IMAGE_4BIT_BLOCK;

typedef EFI_HII_IIBT_IMAGE_4BIT_BLOCK EFI_HII_IIBT_IMAGE_4BIT_TRANS_BLOCK;



//
// EFI_HII_IIBT_IMAGE_8BIT_BASE 
// 
typedef struct _EFI_HII_IIBT_IMAGE_8BIT_BASE {
  UINT16  Width;
  UINT16  Height;
  // UINT8 Data[...];
} EFI_HII_IIBT_IMAGE_8BIT_BASE;

/**
   
  This record assigns the 8-bit-per-pixel bitmap data to the
  ImageIdCurrent identifier using the specified palette and
  increment ImageIdCurrent by one. The image????s upper left hand
  corner pixel is the first bitmap byte.

  @param Header   Standard image header, where Header.BlockType
                  = EFI_HII_IIBT_IMAGE_8BIT.

  @param PaletteIndex   Index of the palette in the palette
                        information.

  @param Bitmap   The bitmap specifies a series of pixels, eight
                  bits per pixel, left-to-right, top-to-bottom.
                  The number of bytes per bitmap can be
                  calculated as: Width * Height.

**/
typedef struct _EFI_HII_IIBT_IMAGE_8BIT_PALETTE {
  EFI_HII_IMAGE_BLOCK           Header;
  UINT8                         PaletteIndex;
  EFI_HII_IIBT_IMAGE_8BIT_BASE  Bitmap;
} EFI_HII_IIBT_IMAGE_8BIT_PALETTE;

typedef EFI_HII_IIBT_IMAGE_8BIT_PALETTE   EFI_HII_IIBT_IMAGE_8BIT_TRANS_BLOCK;


/**
   
  Indicates that the image with image ID ImageValueCurrent has the
  same image as a previously defined image ID and increments
  ImageValueCurrent by one

  @param Header   Standard image header, where Header.BlockType
                  = EFI_HII_IIBT_DUPLICATE.

  @param ImageId  The previously defined image ID with the exact
                  same image.


**/
typedef struct _EFI_HII_IIBT_DUPLICATE_BLOCK {
  EFI_HII_IMAGE_BLOCK   Header;
  EFI_IMAGE_ID          ImageId;
} EFI_HII_IIBT_DUPLICATE_BLOCK;


/**
   
  This record assigns the JPEG image data to the ImageIdCurrent
  identifier and increment ImageIdCurrent by one. The JPEG decoder
  is only required to cover the basic JPEG encoding types, which
  are produced by standard available paint packages (for example:
  MSPaint under Windows from Microsoft). This would include JPEG
  encoding of high (1:1:1) and medium (4:1:1) quality with only
  three components (R,G,B) ??C no support for the special gray
  component encoding.

  @param Header   Standard image header, where Header.BlockType
                  = EFI_HII_IIBT_IMAGE_JPEG.
  
  @param Size   Specifies the size of the JPEG encoded data.
  
  @param Data   JPEG encoded data with ????JFIF???? signature at
                offset 6 in the data block. The JPEG encoded
                data, specifies type of encoding and final size
                of true-color image.

**/
typedef struct _EFI_HII_IIBT_JPEG {
  EFI_HII_IMAGE_BLOCK Header;
  UINT32              Size;
  //UINT8 Data[ бн ];
} EFI_HII_IIBT_JPEG;


/**
   
  Increments the current image ID ImageIdCurrent by the number
  specified.
  
  @param Header   Standard image header, where Header.BlockType
                  = EFI_HII_IIBT_SKIP1.
  
  @param SkipCount  The unsigned 8-bit value to add  to
                    ImageIdCurrent.

**/
typedef struct _EFI_HII_IIBT_SKIP1_BLOCK {
  EFI_HII_IMAGE_BLOCK Header;
  UINT8               SkipCount;
} EFI_HII_IIBT_SKIP1_BLOCK;

/**
  
  Increments the current image ID ImageIdCurrent by the number
  specified.
  
  @param Header   Standard image header, where Header.BlockType
                  = EFI_HII_IIBT_SKIP2.
  
  @param SkipCount  The unsigned 16-bit value to add to
                    ImageIdCurrent.

**/
typedef struct _EFI_HII_IIBT_SKIP2_BLOCK {
  EFI_HII_IMAGE_BLOCK   Header;
  UINT16                SkipCount;
} EFI_HII_IIBT_SKIP2_BLOCK;


/**
   
  This fixed header is followed by zero or more variable-length
  palette information records. The structures are assigned a
  number 1 to n.

  @param PaletteCount   Number of palettes.

**/
typedef struct _EFI_HII_IMAGE_PALETTE_INFO_HEADER {
  UINT16  PaletteCount;
} EFI_HII_IMAGE_PALETTE_INFO_HEADER;

/**
   
  Each palette information record is an array of 24-bit color
  structures. The first entry (PaletteValue[0]) corresponds to
  color 0 in the source image; the second entry (PaletteValue[1])
  corresponds to color 1, etc. Each palette entry is a three byte
  entry, with the first byte equal to the blue component of the
  color, followed by green, and finally red (B,G,R). Each color
  component value can vary from 0x00 (color off) to 0xFF (color
  full on), allowing 16.8 millions colors that can be specified.

  @param PaletteSize  Size of the palette information.
  
  @param PaletteValue   Array of color values.
  
**/
typedef struct _EFI_HII_IMAGE_PALETTE_INFO {
  UINT16 PaletteSize;
  // EFI_HII_RGB_PIXEL PaletteValue[...];
} EFI_HII_IMAGE_PALETTE_INFO;



//
// EFI_HII_DATABASE_NOTIFY_TYPE
// 
typedef UINTN   EFI_HII_DATABASE_NOTIFY_TYPE;
#define EFI_HII_DATABASE_NOTIFY_NEW_PACK    0x00000001
#define EFI_HII_DATABASE_NOTIFY_REMOVE_PACK 0x00000002
#define EFI_HII_DATABASE_NOTIFY_EXPORT_PACK 0x00000004
#define EFI_HII_DATABASE_NOTIFY_ADD_PACK    0x00000008
/**
   
  Functions which are registered to receive notification of
  database events have this prototype. The actual event is encoded
  in NotifyType. The following table describes how PackageType,
  PackageGuid, Handle, and Package are used for each of the
  notification types.

  @param PackageType  Package type of the notification.

  @param PackageGuid  If PackageType is
                      EFI_HII_PACKAGE_TYPE_GUID, then this is
                      the pointer to the GUID from the Guid
                      field of EFI_HII_PACKAGE_GUID_HEADER.
                      Otherwise, it must be NULL.

  @param Package  Points to the package referred to by the
                  notification Handle The handle of the package
                  list which contains the specified package.

  @param NotifyType   The type of change concerning the
                      database. See
                      EFI_HII_DATABASE_NOTIFY_TYPE.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_NOTIFY) (
  IN CONST  UINT8                         PackageType,
  IN CONST  EFI_GUID                      *PackageGuid,
  IN CONST  EFI_HII_PACKAGE_HEADER        *Package,
  IN CONST  EFI_HII_HANDLE                Handle,
  IN CONST  EFI_HII_DATABASE_NOTIFY_TYPE  NotifyType
);

/**

  This function adds the packages in the package list to the
  database and returns a handle. If there is a
  EFI_DEVICE_PATH_PROTOCOL associated with the DriverHandle, then
  this function will create a package of type
  EFI_PACKAGE_TYPE_DEVICE_PATH and add it to the package list. For
  each package in the package list, registered functions with the
  notification type NEW_PACK and having the same package type will
  be called. For each call to NewPackageList(), there should be a
  corresponding call to
  EFI_HII_DATABASE_PROTOCOL.RemovePackageList().

  @param This   A pointer to the EFI_HII_DATABASE_PROTOCOL
                instance.

  @param PackageList  A pointer to an
                      EFI_HII_PACKAGE_LIST_HEADER structure.

  @param DriverHandle   Associate the package list with this EFI
                        handle Handle A pointer to the
                        EFI_HII_HANDLE instance.

  @retval EFI_SUCCESS   The package list associated with the
                        Handle was added to the HII database.

  @retval EFI_OUT_OF_RESOURCES  Unable to allocate necessary
                                resources for the new database
                                contents.

  @retval EFI_INVALID_PARAMETER   PackageList is NULL or Handle
                                  is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_NEW_PACK) (
  IN CONST  EFI_HII_DATABASE_PROTOCOL   *This,
  IN CONST  EFI_HII_PACKAGE_LIST_HEADER *PackageList,
  IN CONST  EFI_HANDLE                  DriverHandle,
  OUT       EFI_HII_HANDLE              *Handle
);


/**

  This function removes the package list that is associated with a
  handle Handle from the HII database. Before removing the
  package, any registered functions with the notification type
  REMOVE_PACK and the same package type will be called. For each
  call to EFI_HII_DATABASE_PROTOCOL.NewPackageList(), there should
  be a corresponding call to RemovePackageList.

  @param This   A pointer to the EFI_HII_DATABASE_PROTOCOL
                instance.
  
  @param Handle   The handle that was registered to the data
                  that is requested for removal.
  
  @retval EFI_SUCCESS   The data associated with the Handle was
                        removed from the HII database.
  
  @retval EFI_INVALID_PARAMETER   The Handle was not valid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_REMOVE_PACK) (
  IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
  IN CONST  EFI_HII_HANDLE            Handle
);


/**
   
  This function updates the existing package list (which has the
  specified Handle) in the HII databases, using the new package
  list specified by PackageList. The update process has the
  following steps: Collect all the package types in the package
  list specified by PackageList. A package type consists of the
  Type field of EFI_HII_PACKAGE_HEADER and, if the Type is
  EFI_HII_PACKAGE_TYPE_GUID, the Guid field, as defined in
  EFI_HII_PACKAGE_GUID_HEADER. Iterate through the packages within
  the existing package list in the HII database specified by
  Handle. If a package??s type matches one of the types collected
  in step 1, then perform the following steps:
  - Call any functions registered with the notification type
  REMOVE_PACK.
  - Remove the package from the package list and the HII
  database.
  Add all of the packages within the new package list specified
  by PackageList, using the following steps:
  - Add the package to the package list and the HII database.
  - Call any functions registered with the notification type
  ADD_PACK.

  @param This   A pointer to the EFI_HII_DATABASE_PROTOCOL
                instance.
  
  @param Handle   The handle that was registered to the data
                  that is requested for removal.
  
  @param PackageList  A pointer to an EFI_HII_PACKAGE_LIST
                      package.
  
  @retval EFI_SUCCESS   The HII database was successfully
                        updated.
  
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate enough memory
                                for the updated database.
  
  @retval EFI_INVALID_PARAMETER   The Handle was not valid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_UPDATE_PACK) (
  IN CONST  EFI_HII_DATABASE_PROTOCOL   *This,
  IN CONST  EFI_HII_HANDLE              Handle,
  IN CONST  EFI_HII_PACKAGE_LIST_HEADER *PackageList
);


/**
  
  This function returns a list of the package handles of the   
  specified type that are currently active in the database. The   
  pseudo-type EFI_HII_PACKAGE_TYPE_ALL will cause all package   
  handles to be listed.
  
  @param This   A pointer to the EFI_HII_DATABASE_PROTOCOL
                instance.
  
  @param PackageType  Specifies the package type of the packages
                      to list or EFI_HII_PACKAGE_TYPE_ALL for
                      all packages to be listed.
  
  @param PackageGuid  If PackageType is
                      EFI_HII_PACKAGE_TYPE_GUID, then this is
                      the pointer to the GUID which must match
                      the Guid field of
                      EFI_HII_PACKAGE_GUID_HEADER. Otherwise, it
                      must be NULL.
  
  @param HandleBufferLength   On input, a pointer to the length
                              of the handle buffer. On output,
                              the length of the handle buffer
                              that is required for the handles
                              found.

  @param Handle   An array of EFI_HII_HANDLE instances returned.


  @retval EFI_SUCCESS   Handle was updated successfully.

  @retval EFI_BUFFER_TOO_SMALL  The HandleBufferLength parameter
                                indicates that Handle is too
                                small to support the number of
                                handles. HandleBufferLength is
                                updated with a value that will
                                enable the data to fit.


**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_LIST_PACKS) (
  IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
  IN CONST  UINT8                     PackageType,
  IN CONST  EFI_GUID                  *PackageGuid,
  IN OUT    UINTN                     *HandleBufferLength,
  OUT       EFI_HII_HANDLE            *Handle
);






/**

  This function will export one or all package lists in the
  database to a buffer. For each package list exported, this
  function will call functions registered with EXPORT_PACK and
  then copy the package list to the buffer. The registered
  functions may call EFI_HII_DATABASE_PROTOCOL.UpdatePackageList()
  to modify the package list before it is copied to the buffer. If
  the specified BufferSize is too small, then the status
  EFI_OUT_OF_RESOURCES will be returned and the actual package
  size will be returned in BufferSize.

  @param This   A pointer to the EFI_HII_DATABASE_PROTOCOL
                instance.

  @param Handle   An EFI_HII_HANDLE that corresponds to the
                  desired package list in the HII database to
                  export or NULL to indicate all package lists
                  should be exported. 

  @param BufferSize   On input, a pointer to the length of the
                      buffer. On output, the length of the
                      buffer that is required for the exported
                      data.

  @param Buffer   A pointer to a buffer that will contain the
                  results of the export function.
  
  
  @retval EFI_SUCCESS   Package exported.
  
  @retval EFI_OUT_OF_RESOURCES  BufferSize is too small to hold
                                the package.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_EXPORT_PACKS) (
  IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
  IN CONST  EFI_HII_HANDLE            Handle,
  IN OUT    UINTN                     *BufferSize,
  OUT       EFI_HII_PACKAGE_HEADER    *Buffer
);


/**
   
  
  This function registers a function which will be called when
  specified actions related to packages of the specified type
  occur in the HII database. By registering a function, other
  HII-related drivers are notified when specific package types
  are added, removed or updated in the HII database. Each driver
  or application which registers a notification should use
  EFI_HII_DATABASE_PROTOCOL.UnregisterPackageNotify() before
  exiting.
  
  
  @param This   A pointer to the EFI_HII_DATABASE_PROTOCOL
                instance.

  @param PackageType  The package type. See
                      EFI_HII_PACKAGE_TYPE_x in EFI_HII_PACKAGE_HEADER. 

  @param PackageGuid  If PackageType is
                      EFI_HII_PACKAGE_TYPE_GUID, then this is
                      the pointer to the GUID which must match
                      the Guid field of
                      EFI_HII_PACKAGE_GUID_HEADER. Otherwise, it
                      must be NULL.

  @param PackageNotifyFn  Points to the function to be called
                          when the event specified by
                          NotificationType occurs. See
                          EFI_HII_DATABASE_NOTIFY.

  @param NotifyType   Describes the types of notification which
                      this function will be receiving. See
                      EFI_HII_DATABASE_NOTIFY_TYPE for more a
                      list of types.

  @param NotifyHandle   Points to the unique handle assigned to
                        the registered notification. Can be used
                        in
                        EFI_HII_DATABASE_PROTOCOL.UnregisterPack
                        to stop notifications.


  @retval EFI_SUCCESS   Notification registered successfully.

  @retval EFI_OUT_OF_RESOURCES  Unable to allocate necessary
                                data structures.

  @retval EFI_INVALID_PARAMETER   PackageGuid is not NULL when
                                  PackageType is not
                                  EFI_HII_PACKAGE_TYPE_GUID.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_REGISTER_NOTIFY) (
  IN CONST  EFI_HII_DATABASE_PROTOCOL     *This,
  IN CONST  UINT8                         PackageType,
  IN CONST  EFI_GUID                      *PackageGuid,
  IN CONST  EFI_HII_DATABASE_NOTIFY       PackageNotifyFn,
  IN CONST  EFI_HII_DATABASE_NOTIFY_TYPE  NotifyType,
  OUT       EFI_HANDLE                    *NotifyHandle
);


/**
   
  Removes the specified HII database package-related notification.
  
  @param This   A pointer to the EFI_HII_DATABASE_PROTOCOL
                instance.
  
  @param NotificationHandle   The handle of the notification
                              function being unregistered.
  
  @retval EFI_SUCCESS   Unregister the notification
                        Successsfully
  
  @retval EFI_INVALID_PARAMETER   The Handle is invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_UNREGISTER_NOTIFY) (
  IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
  IN CONST  EFI_HANDLE                NotificationHandle
);


/**
   
  @param Header   The general pack header which defines both the
                  type of pack and the length of the entire
                  pack.

  @param LayoutCount  The number of keyboard layouts contained
                      in the entire keyboard pack.

  @param Layout   An array of LayoutCount number of keyboard
                  layouts.

**/
typedef struct {
  EFI_HII_PACKAGE_HEADER  Header;
  UINT16                  LayoutCount;
  // EFI_HII_KEYBOARD_LAYOUT Layout[...];
} EFI_HII_KEYBOARD_PACK;

/**
   
  @param LayoutLength   The length of the current keyboard
                        layout.

  @param Guid   The unique ID associated with this keyboard
                layout.

  @param LayoutDescriptorString   An offset location (0 is the
                                  beginning of the
                                  EFI_KEYBOARD_LAYOUT instance)
                                  of the string which describes
                                  this keyboard layout. The data
                                  that is being referenced is in
                                  EFI_DESCRIPTION_STRING_BUNDLE
                                  format.

  @param DescriptorCount  The number of Descriptor entries in
                          this layout.

  @param Descriptors  An array of key descriptors.

**/
typedef struct {
  UINT16              LayoutLength;
  EFI_GUID            Guid;
  RELOFST             LayoutDescriptorString;
  UINT8               DescriptorCount;
  // EFI_KEY_DESCRIPTOR   Descriptors[...];
} EFI_HII_KEYBOARD_LAYOUT;


/**
   
  @param Language   The language to associate with
                    DescriptionString.

  @param Space  A space (U-0x0020) character to force as a
                separator between the Language field and the
                formal description string.

  @param DescriptionString  A null-terminated description
                            string.

**/
typedef struct {
  CHAR16  Language[3];
  CHAR16  Space;
  CHAR16  DescriptionString[1];
} EFI_DESCRIPTION_STRING;

/**
   
  @param DescriptionCount   The number of description strings.

  @param DescriptionString  An array of language-specific
                            description strings.

**/
typedef struct {
  UINT16                  DescriptionCount;
  // EFI_DESCRIPTION_STRING   DescriptionString[];
} EFI_DESCRIPTION_STRING_BUNDLE;

/**

  See the figure below for which key corresponds to the values in
  the enumeration above. For example, EfiKeyLCtrl corresponds to
  the left control key in the lower-left corner of the keyboard,
  EfiKeyFour corresponds to the 4 key on the numeric keypad, and
  EfiKeySLck corresponds to the Scroll Lock key in the upper-right
  corner of the keyboard.

**/
typedef enum {
  EfiKeyLCtrl, EfiKeyA0, EfiKeyLAlt, EfiKeySpaceBar,
  EfiKeyA2, EfiKeyA3, EfiKeyA4, EfiKeyRCtrl, EfiKeyLeftArrow,
  EfiKeyDownArrow, EfiKeyRightArrow, EfiKeyZero,
  EfiKeyPeriod, EfiKeyEnter, EfiKeyLShift, EfiKeyB0,
  EfiKeyB1, EfiKeyB2, EfiKeyB3, EfiKeyB4, EfiKeyB5, EfiKeyB6,
  EfiKeyB7, EfiKeyB8, EfiKeyB9, EfiKeyB10, EfiKeyRshift,
  EfiKeyUpArrow, EfiKeyOne, EfiKeyTwo, EfiKeyThree,
  EfiKeyCapsLock, EfiKeyC1, EfiKeyC2, EfiKeyC3, EfiKeyC4,
  EfiKeyC5, EfiKeyC6, EfiKeyC7, EfiKeyC8, EfiKeyC9,
  EfiKeyC10, EfiKeyC11, EfiKeyC12, EfiKeyFour, EfiKeyFive,
  EfiKeySix, EfiKeyPlus, EfiKeyTab, EfiKeyD1, EfiKeyD2,
  EfiKeyD3, EfiKeyD4, EfiKeyD5, EfiKeyD6, EfiKeyD7, EfiKeyD8,
  EfiKeyD9, EfiKeyD10, EfiKeyD11, EfiKeyD12, EfiKeyD13,
  EfiKeyDel, EfiKeyEnd, EfiKeyPgDn, EfiKeySeven, EfiKeyEight,
  EfiKeyNine, EfiKeyE0, EfiKeyE1, EfiKeyE2, EfiKeyE3,
  EfiKeyE4, EfiKeyE5, EfiKeyE6, EfiKeyE7, EfiKeyE8, EfiKeyE9,
  EfiKeyE10, EfiKeyE11, EfiKeyE12, EfiKeyBackSpace,
  EfiKeyIns, EfiKeyHome, EfiKeyPgUp, EfiKeyNLck, EfiKeySlash,
  EfiKeyAsterisk, EfiKeyMinus, EfiKeyEsc, EfiKeyF1, EfiKeyF2,
  EfiKeyF3, EfiKeyF4, EfiKeyF5, EfiKeyF6, EfiKeyF7, EfiKeyF8,
  EfiKeyF9, EfiKeyF10, EfiKeyF11, EfiKeyF12, EfiKeyPrint,
  EfiKeySLck, EfiKeyPause
} EFI_KEY;

/**
   
  @param Key Used to describe a physical key on a keyboard.

  @param Unicode  Unicode value for the Key.

  @param ShiftedUnicode   Unicode value for the key with the
                          shift key being held down. 

  @param AltGrUnicode   Unicode value for the key with the
                        Alt-GR being held down.

  @param ShiftedAltGrUnicode  Unicode value for the key with the
                              Alt-GR and shift keys being held down.

  @param Modifier   Modifier keys are defined to allow for
                    special functionality that is not
                    necessarily accomplished by a printable
                    character. Many of these modifier keys are
                    flags to toggle certain state bits on and
                    off inside of a keyboard driver.
  
**/
typedef struct {
  EFI_KEY Key;
  CHAR16  Unicode;
  CHAR16  ShiftedUnicode;
  CHAR16  AltGrUnicode;
  CHAR16  ShiftedAltGrUnicode;
  UINT16  Modifier;
} EFI_KEY_DESCRIPTOR;


//
// Modifier values
//
#define EFI_NULL_MODIFIER                 0x0000
#define EFI_LEFT_CONTROL_MODIFIER         0x0001
#define EFI_RIGHT_CONTROL_MODIFIER        0x0002
#define EFI_LEFT_ALT_MODIFIER             0x0003
#define EFI_RIGHT_ALT_MODIFIER            0x0004
#define EFI_ALT_GR_MODIFIER               0x0005
#define EFI_INSERT_MODIFIER               0x0006
#define EFI_DELETE_MODIFIER               0x0007
#define EFI_PAGE_DOWN_MODIFIER            0x0008
#define EFI_PAGE_UP_MODIFIER              0x0009
#define EFI_HOME_MODIFIER                 0x000A
#define EFI_END_MODIFIER                  0x000B
#define EFI_LEFT_SHIFT_MODIFIER           0x000C
#define EFI_RIGHT_SHIFT_MODIFIER          0x000D
#define EFI_CAPS_LOCK_MODIFIER            0x000E
#define EFI_NUM_LOCK _MODIFIER            0x000F
#define EFI_LEFT_ARROW_MODIFIER           0x0010
#define EFI_RIGHT_ARROW_MODIFIER          0x0011
#define EFI_DOWN_ARROW_MODIFIER           0x0012
#define EFI_UP_ARROW_MODIFIER             0X0013
#define EFI_NS_KEY_MODIFIER               0x0014
#define EFI_NS_KEY_DEPENDENCY_MODIFIER    0x0015
#define EFI_FUNCTION_KEY_ONE_MODIFIER     0x0016
#define EFI_FUNCTION_KEY_TWO_MODIFIER     0x0017
#define EFI_FUNCTION_KEY_THREE_MODIFIER   0x0018
#define EFI_FUNCTION_KEY_FOUR_MODIFIER    0x0019
#define EFI_FUNCTION_KEY_FIVE_MODIFIER    0x001A
#define EFI_FUNCTION_KEY_SIX_MODIFIER     0x001B
#define EFI_FUNCTION_KEY_SEVEN_MODIFIER   0x001C
#define EFI_FUNCTION_KEY_EIGHT_MODIFIER   0x001D
#define EFI_FUNCTION_KEY_NINE_MODIFIER    0x001E
#define EFI_FUNCTION_KEY_TEN_MODIFIER     0x001F
#define EFI_FUNCTION_KEY_ELEVEN_MODIFIER  0x0020
#define EFI_FUNCTION_KEY_TWELVE_MODIFIER  0x0021
//
// Keys that have multiple control functions based on modifier
// settings are handled in the keyboard driver implementation.
// For instance PRINT_KEY might have a modifier held down and
// is still a nonprinting character, but might have an alternate
// control function like SYSREQUEST
//
#define EFI_PRINT_MODIFIER                0x0022
#define EFI_SYS_REQUEST_MODIFIER          0x0023
#define EFI_SCROLL_LOCK_MODIFIER          0x0024
#define EFI_PAUSE_MODIFIER                0x0025
#define EFI_BREAK_MODIFIER                0x0026



/**
   
  This routine retrieves an array of GUID values for each keyboard
  layout that was previously registered in the system.

  @param This   A pointer to the EFI_HII_PROTOCOL instance.

  @param KeyGuidBufferLength  On input, a pointer to the length
                              of the keyboard GUID buffer. On
                              output, the length of the handle
                              buffer that is required for the
                              handles found. KeyGuidBuffer An
                              array of keyboard layout GUID
                              instances returned.

  @retval EFI_SUCCESS   KeyGuidBuffer was updated successfully.
  
  @retval EFI_BUFFER_TOO_SMALL  The KeyGuidBufferLength
                                parameter indicates that
                                KeyGuidBuffer is too small to
                                support the number of GUIDs.
                                KeyGuidBufferLength is updated
                                with a value that will enable
                                the data to fit.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_FIND_KEYBOARD_LAYOUTS) (
  IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
  IN OUT    UINT16                    *KeyGuidBufferLength,
  OUT       EFI_GUID                  *KeyGuidBuffer
);


/**
   
  This routine retrieves the requested keyboard layout. The layout
  is a physical description of the keys on a keyboard and the
  character(s) that are associated with a particular set of key
  strokes.

  @param This   A pointer to the EFI_HII_PROTOCOL instance.
  
  @param KeyGuid  A pointer to the unique ID associated with a
                  given keyboard layout. If KeyGuid is NULL then
                  the current layout will be retrieved.
  
  @param KeyboardLayout A pointer to a buffer containing the
                        retrieved keyboard layout. below.
  
  @retval EFI_SUCCESS   The keyboard layout was retrieved
                        successfully.
  
  @retval EFI_NOT_FOUND   The requested keyboard layout was not
                          found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_KEYBOARD_LAYOUT) (
  IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
  IN CONST  EFI_GUID                  *KeyGuid,
  OUT       EFI_HII_KEYBOARD_LAYOUT   *KeyboardLayout
);

/**
   
  This routine sets the default keyboard layout to the one
  referenced by KeyGuid. When this routine is called, an event
  will be signaled of the EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID
  group type. This is so that agents which are sensitive to the
  current keyboard layout being changed can be notified of this
  change.

  @param This   A pointer to the EFI_HII_DATABASE_PROTOCOL
                instance.

  @param KeyGuid  A pointer to the unique ID associated with a
                  given keyboard layout.


  @retval EFI_SUCCESS   The current keyboard layout was
                        successfully set.
  
  @retval EFI_NOT_FOUND   The referenced keyboard layout was not
                          found, so action was taken.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_SET_KEYBOARD_LAYOUT) (
  IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
  IN CONST  EFI_GUID                  *KeyGuid
);

/**
   
  Return the EFI handle associated with a package list.
  
  @param This   A pointer to the EFI_HII_DATABASE_PROTOCOL
                instance.
  
  @param PackageListHandle  An EFI_HII_HANDLE that corresponds
                            to the desired package list in the
                            HIIdatabase.
  
  @param DriverHandle   On return, contains the EFI_HANDLE which
                        was registered with the package list in
                        NewPackageList().
  
  @retval EFI_SUCCESS   The DriverHandle was returned
                        successfully.
  
  @retval EFI_INVALID_PARAMETER   The PackageListHandle was not
                                  valid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_PACK_HANDLE) (
  IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
  IN CONST  EFI_HII_HANDLE            PackageListHandle,
  OUT       EFI_HANDLE                *DriverHandle
);

/**
   
  @param NewPackageList Add a new package list to the HII
                        database.

  @param RemovePackageList  Remove a package list from the HII
                            database.

  @param UpdatePackageList  Update a package list in the HII
                            database.

  @param ListPackageLists   List the handles of the package
                            lists within the HII database. 

  @param ExportPackageLists Export package lists from the HII
                            database.

  @param RegisterPackageNotify  Register notification when
                                packages of a certain type are
                                installed.

  @param UnregisterPackageNotify  Unregister notification of
                                  packages.

  @param FindKeyboardLayouts  Retrieves a list of the keyboard
                              layouts in the system.

  @param GetKeyboardLayout  Allows a program to extract the
                            current keyboard layout. See the
                            GetKeyboardLayout() function
                            description.

  @param SetKeyboardLayout  Changes the current keyboard layout.
                            See the SetKeyboardLayout() function


**/
struct _EFI_HII_DATABASE_PROTOCOL {
  EFI_HII_NEW_PACK                NewPackageList;
  EFI_HII_REMOVE_PACK             RemovePackageList;
  EFI_HII_UPDATE_PACK             UpdatePackageList;
  EFI_HII_LIST_PACKS              ListPackageLists;
  EFI_HII_EXPORT_PACKS            ExportPackageLists;
  EFI_HII_REGISTER_NOTIFY         RegisterPackageNotify;
  EFI_HII_UNREGISTER_NOTIFY       UnregisterPackageNotify;
  EFI_HII_FIND_KEYBOARD_LAYOUTS   FindKeyboardLayouts;
  EFI_HII_GET_KEYBOARD_LAYOUT     GetKeyboardLayout;
  EFI_HII_SET_KEYBOARD_LAYOUT     SetKeyboardLayout;
  EFI_HII_GET_PACK_HANDLE         GetPackageHandle;
};

extern EFI_GUID gEfiHiiDatabaseProtocolGuid;

#endif

