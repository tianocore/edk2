/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  VfrServices.h

Abstract:

  Prototypes and defines for routines and classes used by the
  EFI VFR compiler.
  
--*/

#ifndef _VFR_SERVICES_H_
#define _VFR_SERVICES_H_

class VfrOpcodeHandler
{
public:
  VfrOpcodeHandler (
    VOID
    )
  /*++

Routine Description:
  Constructor for the VFR opcode handling class.
  
Arguments:
  None

Returns:
  None

--*/
  ;
  ~VfrOpcodeHandler (
    VOID
    )
  /*++

Routine Description:
  Destructor for the VFR opcode handler. Free up memory allocated
  while parsing the VFR script.
  
Arguments:
  None

Returns:
  None

--*/
  ;
  void
  WriteIfrBytes (
    VOID
    )
  /*++

Routine Description:
  This function is invoked at the end of parsing. Its purpose
  is to write out all the IFR bytes that were queued up while
  parsing.
  
Arguments:
  None

Returns:
  None

--*/
  ;
  int
  AddOpcodeByte (
    UINT8  OpcodeByte,
    UINT32 LineNum
    )
  /*++

Routine Description:
  This function is invoked by the parser when a new IFR
  opcode should be emitted.
  
Arguments:
  OpcodeByte  - the IFR opcode
  LineNum     - the line number from the source file that resulted
                in the opcode being emitted.

Returns:
  0 always

--*/
  ;
  void
  AddByte (
    UINT8 ByteVal,
    UINT8 KeyByte
    )
  /*++

Routine Description:
  This function is invoked by the parser when it determines
  that more raw IFR bytes should be emitted to the output stream.
  Here we just queue them up into an output buffer.
  
Arguments:
  ByteVal   - the raw byte to emit to the output IFR stream
  KeyByte   - a value that can be used for debug. 

Returns:
  None

--*/
  ;
  void
  SetVarStoreId (
    UINT16 VarStoreId
    )
  /*++

Routine Description:
  This function is invoked by the parser when a variable is referenced in the 
  VFR. Save the variable store (and set a flag) so that we can later determine 
  if we need to emit a varstore-select or varstore-select-pair opcode.
  
Arguments:
  VarStoreId - ID of the variable store referenced in the VFR

Returns:
  None

--*/
  ;
  void
  SetSecondaryVarStoreId (
    UINT16 VarStoreId
    )
  /*++

Routine Description:
  This function is invoked by the parser when a secondary variable is 
  referenced in the VFR. Save the variable store (and set a flag) so 
  that we can later determine if we need to emit a varstore-select or 
  varstore-pair opcode.
  
Arguments:
  VarStoreId - ID of the variable store referenced in the VFR

Returns:
  None

--*/
  ;

/* */
private:
  int
  FlushQueue (
    VOID
    )
  /*++

Routine Description:
  This function is invoked to flush the internal IFR buffer.
  
Arguments:
  None

Returns:
  0 always

--*/
  ;
  int
  IAddByte (
    UINT8  ByteVal,
    UINT8  KeyByte,
    UINT32 LineNum
    )
  /*++

Routine Description:
  This internal function is used to add actual IFR bytes to
  the output stream. Most other functions queue up the bytes
  in an internal buffer. Once they come here, there's no
  going back.

  
Arguments:
  ByteVal   - value to write to output 
  KeyByte   - key value tied to the byte -- useful for debug
  LineNum   - line number from source file the byte resulted from

Returns:
  0 - if successful
  1 - failed due to memory allocation failure

--*/
  ;

/* */
private:
  IFR_BYTE  *mIfrBytes;
  IFR_BYTE  *mLastIfrByte;
  UINT32    mQueuedByteCount;
  UINT32    mBytesWritten;
  UINT32    mQueuedLineNum;
  UINT8     mQueuedBytes[MAX_QUEUE_COUNT];
  UINT8     mQueuedKeyBytes[MAX_QUEUE_COUNT];
  UINT8     mQueuedOpcodeByte;
  UINT32    mQueuedOpcodeByteValid;
  UINT16    mPrimaryVarStoreId;
  UINT8     mPrimaryVarStoreIdSet;
  UINT16    mSecondaryVarStoreId;
  UINT8     mSecondaryVarStoreIdSet;
  UINT16    mDefaultVarStoreId;
};

#endif // #ifndef _VFR_SERVICES_H_
