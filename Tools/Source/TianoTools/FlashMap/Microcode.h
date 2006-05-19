/*++

Copyright (c)  2004 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

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
