/** @file
  Write to an Interactive I/O Output device.

  The functions assume that isatty() is TRUE at the time they are called.
  Since the UEFI console is a WIDE character device, these functions do all
  processing using wide characters.

  It is the responsibility of the caller, or higher level function, to perform
  any necessary translation between wide and narrow characters.

  Copyright (c) 2012 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>

#include  <LibConfig.h>

#include  <assert.h>
#include  <errno.h>
#include  <sys/termios.h>
#include  <Device/IIO.h>

static wchar_t  Spaces[] = L"                ";   // Spaces for expanding TABs

#define MAX_TAB_WIDTH     ((int)(sizeof(Spaces) / sizeof(wchar_t)) - 1)

#define MAX_EXPANSION     3

/** Process and buffer one character for output.

    @param[in]    filp      Pointer to a file descriptor structure.
    @param[out]   OBuf      Pointer to the Output Buffer FIFO.
    @param[in]    InCh      The wide character to process.

    @retval   <0    An error occurred.  Reason is in errno.
                      * EINVAL  The pointer to the IIO object is NULL.
                      * ENOSPC  The OBuf FIFO is full.

    @retval    0    A character was input but not placed in the output buffer.

    @retval   >0    The number of characters buffered.  Normally 1, or 2.
                    If a character is discarded because of flag settings, a
                    1 will be returned.
**/
ssize_t
IIO_WriteOne(struct __filedes *filp, cFIFO *OBuf, wchar_t InCh)
{
  cIIO               *This;
  struct termios     *Termio;
  tcflag_t            OFlag;
  ssize_t             RetVal;
  wchar_t             wc[MAX_EXPANSION];        // Sub-buffer for conversions
  wchar_t            *wcb;          // Pointer to either wc or spaces
  int                 numW    = 0;  // Wide characters placed in OBuf
  INT32               TabWidth;     // Each TAB expands into this number of spaces
  UINT32              CurColumn;    // Current cursor column on the screen
  UINT32              CurRow;       // Current cursor row on the screen
  UINT32              PrevColumn;   // Previous column.  Used to detect wrapping.
  UINT32              AdjColumn;    // Current cursor column on the screen

  RetVal    = -1;
  wcb       = wc;
  This      = filp->devdata;
  if((This != NULL) && (OBuf->FreeSpace(OBuf, AsElements) >= MAX_EXPANSION)) {
    Termio    = &This->Termio;
    OFlag     = Termio->c_oflag;
    TabWidth  = (INT32)This->Termio.c_cc[VTABLEN];
    if(TabWidth > MAX_TAB_WIDTH) {
      TabWidth = MAX_TAB_WIDTH;
    }
    CurColumn = This->CurrentXY.Column;
    CurRow    = This->CurrentXY.Row;

    numW      = 1;          // The majority of characters buffer one character
    AdjColumn = 0;
    if(OFlag & OPOST) {
      /* Perform output processing */
      switch(InCh) {
        case CHAR_TAB:                //{{
          if(OFlag & OXTABS) {
            if(TabWidth > 0) {
              int   SpaceIndex;

              SpaceIndex = CurColumn % TabWidth;    // Number of spaces after a Tab Stop
              numW = TabWidth - SpaceIndex;         // Number of spaces to the next Tab Stop
              SpaceIndex = MAX_TAB_WIDTH - numW;    // Index into the Spaces array
              wcb = &Spaces[SpaceIndex];            // Point to the appropriate number of spaces
            }
            else {
              wc[0] = L' ';
            }
            AdjColumn = numW;
          }
          else {
            wc[0] = InCh;     // Send the TAB itself - assumes that it does not move cursor.
          }
          break;                      //}}

        case CHAR_CARRIAGE_RETURN:    //{{
          if((OFlag & OCRNL) == 0) {
            if((OFlag & ONLRET) == 0) {
              numW = 0;   /* Discard the CR */
              // Cursor doesn't move
            }
            else {
              wc[0]     = CHAR_CARRIAGE_RETURN;
              CurColumn = 0;
            }
            break;
          }
          else {
            InCh = CHAR_LINEFEED;
          }                           //}}
          // Fall through to the NL case
        case CHAR_LINEFEED:           //{{
          if(OFlag & ONLCR) {
            wc[0] = CHAR_CARRIAGE_RETURN;
            wc[1] = CHAR_LINEFEED;
            numW  = 2;
            CurColumn = 0;
          }
          break;                      //}}

        case CHAR_BACKSPACE:          //{{
          if(CurColumn > 0) {
            wc[0] = CHAR_BACKSPACE;
            CurColumn = (UINT32)ModuloDecrement(CurColumn, (UINT32)This->MaxColumn);
          }
          else {
            numW = 0;   // Discard the backspace if in column 0
          }
          break;                      //}}

        case CHAR_EOT:                //{{
          if(OFlag & ONOEOT) {
            numW = 0;             // Discard the EOT character
            // Cursor doesn't move
            break;
          }                           //}}
          // Fall through to default in order to potentially output "^D"
        default:                      //{{
          if((InCh >= 0) && (InCh < L' ')) {
            // InCh contains a control character
            if(OFlag & OCTRL) {
              wc[1]     = InCh + L'@';
              wc[0]     = L'^';
              numW      = 2;
              AdjColumn = 2;
            }
            else {
              numW = 0;   // Discard.  Not a UEFI supported control character.
            }
          }
          else {
            // Regular printing character
            wc[0]     = InCh;
            AdjColumn = 1;
          }
          break;                      //}}
      }
      if(numW < MAX_EXPANSION) {
        wc[numW] = 0;             // Terminate the sub-buffer
      }
      if(AdjColumn != 0) {
        // Adjust the cursor position
        PrevColumn = CurColumn;
        CurColumn = ModuloAdd(PrevColumn, AdjColumn, (UINT32)This->MaxColumn);
        if(CurColumn < PrevColumn) {
          // We must have wrapped, so we are on the next Row
          ++CurRow;
          if(CurRow >= This->MaxRow) {
            // The screen has scrolled so need to adjust Initial location.
            --This->InitialXY.Row;        // Initial row has moved up one
            CurRow = (UINT32)(This->MaxRow - 1);    // We stay on the bottom row
          }
        }
      }
      This->CurrentXY.Column  = CurColumn;
      This->CurrentXY.Row     = CurRow;
    }
    else {
      // Output processing disabled -- RAW output mode
      wc[0] = InCh;
      wc[1] = 0;
    }
    // Put the character(s) into the output buffer
    if(numW > 0) {
      (void)OBuf->Write(OBuf, (const void *)wcb, (size_t)numW);
    }
    RetVal = numW;
  }
  else {
    if(This == NULL) {
      errno = EINVAL;
    }
    else {
      errno = ENOSPC;
    }
  }
  return RetVal;
}
