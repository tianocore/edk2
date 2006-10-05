/*++

Copyright (c)  2004-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Microcode.h

Abstract:

  Header file for flash management utility in the Intel Platform 
  Innovation Framework for EFI build environment.

--*/

#ifndef _MICROCODE_H_
#define _MICROCODE_H_

void
MicrocodeConstructor (
  void
  );
/*++

Routine Description:

  Constructor of module Microcode

Arguments:

  None

Returns:

  None

--*/

void
MicrocodeDestructor (
  void
  );
/*++

Routine Description:

  Destructor of module Microcode

Arguments:

  None

Returns:

  None

--*/

STATUS
MicrocodeParseFile (
  char  *InFileName,
  char  *OutFileName
  );
/*++

Routine Description:
  Parse a microcode text file, and write the binary results to an output file.

Arguments:
  InFileName  - input text file to parse
  OutFileName - output file to write raw binary data from parsed input file

Returns:
  STATUS_SUCCESS    - no errors or warnings
  STATUS_ERROR      - errors were encountered

--*/


#endif // #ifndef _MICROCODE_H_
