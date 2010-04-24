/** @file
  Language Library implementation that provides functions for language conversion
  between ISO 639-2 and RFC 4646 language codes.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/LanguageLib.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

//
// Lookup table of ISO639-2 3 character language codes to ISO 639-1 2 character language codes
// Each entry is 5 CHAR8 values long.  The first 3 CHAR8 values are the ISO 639-2 code.
// The last 2 CHAR8 values are the ISO 639-1 code.
//
// ISO 639-2 B codes and deprecated ISO 639-1 codes are not supported.
//
// Commonly used language codes such as English and French are put in the front of the table for quick match.
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 mIso639ToRfc4646ConversionTable[] =
"\
engen\
frafr\
aaraa\
abkab\
aveae\
afraf\
akaak\
amham\
argan\
araar\
asmas\
avaav\
aymay\
azeaz\
bakba\
belbe\
bulbg\
bihbh\
bisbi\
bambm\
benbn\
bodbo\
brebr\
bosbs\
catca\
chece\
chach\
cosco\
crecr\
cescs\
chucu\
chvcv\
cymcy\
danda\
deude\
divdv\
dzodz\
eweee\
ellel\
epoeo\
spaes\
estet\
euseu\
fasfa\
fulff\
finfi\
fijfj\
faofo\
fryfy\
glega\
glagd\
glggl\
grngn\
gujgu\
glvgv\
hauha\
hebhe\
hinhi\
hmoho\
hrvhr\
hatht\
hunhu\
hyehy\
herhz\
inaia\
indid\
ileie\
iboig\
iiiii\
ipkik\
idoio\
islis\
itait\
ikuiu\
jpnja\
javjv\
katka\
konkg\
kikki\
kuakj\
kazkk\
kalkl\
khmkm\
kankn\
korko\
kaukr\
kasks\
kurku\
komkv\
corkw\
kirky\
latla\
ltzlb\
luglg\
limli\
linln\
laolo\
litlt\
lublu\
lavlv\
mlgmg\
mahmh\
mrimi\
mkdmk\
malml\
monmn\
marmr\
msams\
mltmt\
myamy\
nauna\
nobnb\
ndend\
nepne\
ndong\
nldnl\
nnonn\
norno\
nblnr\
navnv\
nyany\
ocioc\
ojioj\
ormom\
orior\
ossos\
panpa\
plipi\
polpl\
pusps\
porpt\
quequ\
rohrm\
runrn\
ronro\
rusru\
kinrw\
sansa\
srdsc\
sndsd\
smese\
sagsg\
sinsi\
slksk\
slvsl\
smosm\
snasn\
somso\
sqisq\
srpsr\
sswss\
sotst\
sunsu\
swesv\
swasw\
tamta\
telte\
tgktg\
thath\
tirti\
tuktk\
tgltl\
tsntn\
tonto\
turtr\
tsots\
tattt\
twitw\
tahty\
uigug\
ukruk\
urdur\
uzbuz\
venve\
vievi\
volvo\
wlnwa\
wolwo\
xhoxh\
yidyi\
yoryo\
zhaza\
zhozh\
zulzu\
";

/**
  Converts upper case ASCII characters in an ASCII string to lower case ASCII 
  characters in an ASCII string.

  If a an ASCII character in Source is in the range 'A'..'Z', then it is converted 
  to an ASCII character in the range 'a'..'z' in Destination.  Otherwise, no 
  conversion is performed.  Length ASCII characters from Source are convertered and
  stored in Destination.

  @param  Destination  An ASCII string to store the results of the conversion.
  @param  Source       The source ASCII string of the conversion.
  @param  Length       The number of ASCII characters to convert.

**/
VOID
EFIAPI
InternalLanguageLibToLower (
  OUT CHAR8        *Destination,
  IN  CONST CHAR8  *Source,
  IN  UINTN        Length
  )
{
  for (; Length > 0; Length--, Destination++, Source++) {
    *Destination = (CHAR8)((*Source >= 'A' && *Source <= 'Z') ? *Source + ('a' - 'A') : *Source);
  }
}

/**
  Convert an ISO 639-2 language code to a RFC 4646 language code.
  If the ISO 639-2 language code has a corresponding ISO 639-1 code, then the ISO 639-1
  code is returned. Else the original ISO 639-2 code is returned. The returned RFC 4646
  language code is composed of only a primary language subtag.

  If Iso639Language is NULL, then ASSERT.
  If Rfc4646Language is NULL, then ASSERT.

  @param[out] Rfc4646Language  Pointers to a buffer large enough for an ASCII string
                               which reprsents a RFC 4646 language code containging only
                               either a ISO 639-1 or ISO 639-2 primary language subtag.
                               This string is Null-terminated.
  @param[in]  Iso639Language   Pointer to a 3-letter ASCII string which represents
                               an ISO 639-2 language code. This string is not required
                               to be Null-terminated.

  @retval TRUE                 The ISO 639-2 language code was converted to a ISO 639-1 code.
  @retval FALSE                The language code does not have corresponding ISO 639-1 code.

**/
BOOLEAN
EFIAPI
ConvertIso639ToRfc4646 (
  OUT CHAR8        *Rfc4646Language,
  IN  CONST CHAR8  *Iso639Language
  )
{
  CONST CHAR8  *Match;
  
  ASSERT (Iso639Language != NULL);
  ASSERT (Rfc4646Language != NULL);

  //
  // Convert first 3 characters of Iso639Language to lower case ASCII characters in Rfc4646Language
  //
  InternalLanguageLibToLower (Rfc4646Language, Iso639Language, 3);
  Rfc4646Language[3] = '\0';

  Match = mIso639ToRfc4646ConversionTable;
  do {
    Match = AsciiStrStr (Match, Rfc4646Language);
    if (Match == NULL) {
      return FALSE;
    }
    if (((Match - mIso639ToRfc4646ConversionTable) % 5) == 0) {
      break;
    }
    ++Match;
  } while (TRUE);
  Rfc4646Language[0] = Match[3];
  Rfc4646Language[1] = Match[4];
  Rfc4646Language[2] = '\0';
  return TRUE;
}

/**
  Convert a RFC 4646 language code to an ISO 639-2 language code. The primary language
  subtag of the RFC 4646 code must be either an ISO 639-1 or 639-2 code. If the primary
  language subtag is an ISO 639-1 code, then it is converted to its corresponding ISO 639-2
  code (T code if applies). Else the ISO 639-2 code is returned.

  If Rfc4646Language is NULL, then ASSERT.
  If Iso639Language is NULL, then ASSERT.

  @param[out] Iso639Language   Pointers to a buffer large enough for a 3-letter ASCII string
                               which reprsents an ISO 639-2 language code. The string is Null-terminated.
  @param[in]  Rfc4646Language  Pointer to a RFC 4646 language code string. This string is terminated
                               by a NULL or a ';' character.

  @retval TRUE                 Language code converted successfully.
  @retval FALSE                The RFC 4646 language code is invalid or unsupported.

**/
BOOLEAN
EFIAPI
ConvertRfc4646ToIso639 (
  OUT CHAR8        *Iso639Language,
  IN  CONST CHAR8  *Rfc4646Language
  )
{
  CONST CHAR8 *Match;
  
  ASSERT (Rfc4646Language != NULL);
  ASSERT (Iso639Language != NULL);

  //
  // RFC 4646 language code check before determining 
  // if the primary language subtag is ISO 639-1 or 639-2 code
  //
  if (Rfc4646Language[0] == '\0' || Rfc4646Language[1] == '\0') {
    return FALSE;
  }
  
  //
  // Check if the primary language subtag is ISO 639-1 code
  //
  if (Rfc4646Language[2] == ';' || Rfc4646Language[2] == '-' || Rfc4646Language[2] == '\0') {
    //
    // Convert first 2 characters of Rfc4646Language to lower case ASCII characters in Iso639Language
    //
    InternalLanguageLibToLower (Iso639Language, Rfc4646Language, 2);
    //
    // Convert ISO 639-1 code to ISO 639-2 code
    //
    Iso639Language[2] = '\0';
    Match = mIso639ToRfc4646ConversionTable;
    do {
      Match = AsciiStrStr (Match, Iso639Language);
      if (Match == NULL) {
        return FALSE;
      }
      if (((Match - mIso639ToRfc4646ConversionTable) % 5) == 3) {
        break;
      }
      ++Match;
    } while (TRUE);
    Rfc4646Language = Match - 3;
  } else if (!(Rfc4646Language[3] == ';' || Rfc4646Language[3] == '-' || Rfc4646Language[3] == '\0')) {
    return FALSE;
  }
  Iso639Language[0] = Rfc4646Language[0];
  Iso639Language[1] = Rfc4646Language[1];
  Iso639Language[2] = Rfc4646Language[2];
  Iso639Language[3] = '\0';
  return TRUE;  
}

/**
  Convert ISO 639-2 language codes to RFC 4646 codes and return the converted codes.
  Caller is responsible for freeing the allocated buffer.

  If Iso639Languages is NULL, then ASSERT.

  @param[in] Iso639Languages  Pointers to a Null-terminated ISO 639-2 language codes string containing
                              one or more ISO 639-2 3-letter language codes.
  
  @retval NULL                Invalid ISO 639-2 language code found.
  @retval NULL                Out of memory.
  @return                     Pointer to the allocate buffer containing the Null-terminated converted language codes string.
                              This string is composed of one or more RFC4646 language codes each of which has only
                              ISO 639-1 2-letter primary language subtag.

**/
CHAR8 *
EFIAPI
ConvertLanguagesIso639ToRfc4646 (
  IN CONST CHAR8  *Iso639Languages
  )
{
  UINTN  Length;
  UINTN  Iso639Index;
  UINTN  Rfc4646Index;
  CHAR8  *Rfc4646Languages;
  
  ASSERT (Iso639Languages != NULL);
  
  //
  // The length of ISO 639-2 lanugage codes string must be multiple of 3
  //
  Length = AsciiStrLen (Iso639Languages);
  if (Length % 3 != 0) {
    return NULL;
  }
  
  //
  // Allocate buffer for RFC 4646 language codes string
  //
  Rfc4646Languages = AllocatePool (Length + (Length / 3));
  if (Rfc4646Languages == NULL) {
    return NULL;
  }

  for (Iso639Index = 0, Rfc4646Index = 0; Iso639Languages[Iso639Index] != '\0'; Iso639Index += 3) {
    if (ConvertIso639ToRfc4646 (&Rfc4646Languages[Rfc4646Index], &Iso639Languages[Iso639Index])) {
      Rfc4646Index += 2;
    } else {
      Rfc4646Index += 3;
    }
    Rfc4646Languages[Rfc4646Index++] = ';';
  }
  Rfc4646Languages[Rfc4646Index - 1] = '\0';
  return Rfc4646Languages;
}

/**
  Convert RFC 4646 language codes to ISO 639-2 codes and return the converted codes.
  The primary language subtag of the RFC 4646 code must be either an ISO 639-1 or 639-2 code.
  Caller is responsible for freeing the allocated buffer.

  If Rfc4646Languages is NULL, then ASSERT.

  @param[in] Rfc4646Languages  Pointers to a Null-terminated RFC 4646 language codes string containing
                               one or more RFC 4646 language codes.
  
  @retval NULL                 Invalid or unsupported RFC 4646 language code found.
  @retval NULL                 Out of memory.
  @return                      Pointer to the allocate buffer containing the Null-terminated converted language codes string.
                               This string is composed of one or more ISO 639-2 language codes.

**/
CHAR8 *
EFIAPI
ConvertLanguagesRfc4646ToIso639 (
  IN CONST CHAR8  *Rfc4646Languages
  )
{
  UINTN  NumLanguages;
  UINTN  Iso639Index;
  UINTN  Rfc4646Index;
  CHAR8  *Iso639Languages;

  ASSERT (Rfc4646Languages != NULL);

  //
  // Determine the number of languages in the RFC 4646 language codes string
  //
  for (Rfc4646Index = 0, NumLanguages = 1; Rfc4646Languages[Rfc4646Index] != '\0'; Rfc4646Index++) {
    if (Rfc4646Languages[Rfc4646Index] == ';') {
      NumLanguages++;
    }
  }
  
  //
  // Allocate buffer for ISO 639-2 language codes string
  //
  Iso639Languages = AllocateZeroPool (NumLanguages * 3 + 1);
  if (Iso639Languages == NULL) {
    return NULL;
  }

  //
  // Do the conversion for each RFC 4646 language code
  //
  for (Rfc4646Index = 0, Iso639Index = 0; Rfc4646Languages[Rfc4646Index] != '\0';) {
    if (ConvertRfc4646ToIso639 (&Iso639Languages[Iso639Index], &Rfc4646Languages[Rfc4646Index])) {
      Iso639Index += 3;
    } else {
      FreePool (Iso639Languages);
      return NULL;
    }
    //
    // Locate next language code
    //
    while (Rfc4646Languages[Rfc4646Index] != ';' && Rfc4646Languages[Rfc4646Index] != '\0') {
      Rfc4646Index++;
    }
    if (Rfc4646Languages[Rfc4646Index] == ';') {
      Rfc4646Index++;
    }
  }
  Iso639Languages[Iso639Index] = '\0';
  return Iso639Languages;
}
