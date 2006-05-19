/*++

Copyright (c)  2003 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  CreateGuid.c  

Abstract:

  Library routine to create a GUID

--*/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void
CreateGuid (
  GUID *Guid
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Guid  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  CoCreateGuid (Guid);
}
