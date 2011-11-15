/*++

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GenFvImageLib.c

Abstract:

  This file contains functions required to generate a Firmware Volume.

--*/

//
// Include files
//
#include "GenFvImageLib.h"
#include "GenFvImageLibInternal.h"
#include <string.h>
#include EFI_GUID_DEFINITION (PeiPeCoffLoader)
#include "EfiFirmwareFileSystem.h"
#include "EfiWorkingBlockHeader.h"
#include "EfiVariable.h"
#include <io.h>
#include <assert.h>
#include "CommonLib.h"
#include "FvLib.h"
#include "EfiImage.h"
#include "crc32.h"
#include "EfiUtilityMsgs.h"
#include EFI_GUID_DEFINITION (FirmwareFileSystem)
#include EFI_GUID_DEFINITION (FirmwareFileSystem2)

//
// Define the PE/COFF loader
//
extern EFI_PEI_PE_COFF_LOADER_PROTOCOL  mPeCoffLoader;

//
// Local function prototypes
//
EFI_STATUS
GetPe32Info (
  IN UINT8                  *Pe32,
  OUT UINT32                *EntryPoint,
  OUT UINT32                *BaseOfCode,
  OUT UINT16                *MachineType
  );

//
// Local function implementations.
//
#if (PI_SPECIFICATION_VERSION < 0x00010000)
EFI_GUID  FfsGuid = EFI_FIRMWARE_FILE_SYSTEM_GUID;
#else
EFI_GUID  FfsGuid = EFI_FIRMWARE_FILE_SYSTEM2_GUID;
#endif

EFI_GUID  DefaultFvPadFileNameGuid = { 0x78f54d4, 0xcc22, 0x4048, 0x9e, 0x94, 0x87, 0x9c, 0x21, 0x4d, 0x56, 0x2f };

//
// This data array will be located at the base of the Firmware Volume Header (FVH)
// in the boot block.  It must not exceed 14 bytes of code.  The last 2 bytes
// will be used to keep the FVH checksum consistent.
// This code will be run in response to a starutp IPI for HT-enabled systems.
//
#define SIZEOF_STARTUP_DATA_ARRAY 0x10

UINT8                                   m128kRecoveryStartupApDataArray[SIZEOF_STARTUP_DATA_ARRAY] = {
  //
  // EA D0 FF 00 F0               ; far jmp F000:FFD0
  // 0, 0, 0, 0, 0, 0, 0, 0, 0,   ; Reserved bytes
  // 0, 0                         ; Checksum Padding
  //
  0xEA,
  0xD0,
  0xFF,
  0x0,
  0xF0,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00
};

UINT8                                   m64kRecoveryStartupApDataArray[SIZEOF_STARTUP_DATA_ARRAY] = {
  //
  // EB CE                               ; jmp short ($-0x30)
  // ; (from offset 0x0 to offset 0xFFD0)
  // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ; Reserved bytes
  // 0, 0                                ; Checksum Padding
  //
  0xEB,
  0xCE,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00
};

EFI_STATUS
ParseFvInf (
  IN MEMORY_FILE  *InfFile,
  IN FV_INFO      *FvInfo
  )
/*++

Routine Description:

  This function parses a FV.INF file and copies info into a FV_INFO structure.

Arguments:

  InfFile         Memory file image.
  FvInfo          Information read from INF file.

Returns:

  EFI_SUCCESS       INF file information successfully retrieved.
  EFI_ABORTED       INF file has an invalid format.
  EFI_NOT_FOUND     A required string was not found in the INF file.
--*/
{
  CHAR8       Value[_MAX_PATH];
  UINT64      Value64;
  UINTN       Index;
  EFI_STATUS  Status;

  //
  // Initialize FV info
  //
  memset (FvInfo, 0, sizeof (FV_INFO));

  //
  // Read the FV base address
  //
  Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_FV_BASE_ADDRESS_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Get the base address
    //
    Status = AsciiStringToUint64 (Value, FALSE, &Value64);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0, EFI_FV_BASE_ADDRESS_STRING, "invalid value");
      return EFI_ABORTED;
    }

    FvInfo->BaseAddress = Value64;
  } else {
    Error (NULL, 0, 0, EFI_FV_BASE_ADDRESS_STRING, "could not find value");
    return EFI_ABORTED;
  }
  //
  // Read the FV Guid
  //
  Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_FV_GUID_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Get the guid value
    //
    Status = StringToGuid (Value, &FvInfo->FvGuid);
    if (EFI_ERROR (Status)) {
      memcpy (&FvInfo->FvGuid, &FfsGuid, sizeof (EFI_GUID));
    }
  } else {
    memcpy (&FvInfo->FvGuid, &FfsGuid, sizeof (EFI_GUID));
  }
  //
  // Read the FV file name
  //
  Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_FV_FILE_NAME_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // copy the file name
    //
    strcpy (FvInfo->FvName, Value);
  } else {
    Error (NULL, 0, 0, EFI_FV_FILE_NAME_STRING, "value not specified");
    return EFI_ABORTED;
  }
  //
  // Read the Sym file name
  //
  Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_SYM_FILE_NAME_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // copy the file name
    //
    strcpy (FvInfo->SymName, Value);
  } else {
    //
    // Symbols not required, so init to NULL.
    //
    strcpy (FvInfo->SymName, "");
  }
  //
  // Read the read disabled capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_READ_DISABLED_CAP_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update the read disabled flag
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_READ_DISABLED_CAP;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_READ_DISABLED_CAP_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_READ_DISABLED_CAP_STRING, "value not specified");
    return Status;
  }
  //
  // Read the read enabled capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_READ_ENABLED_CAP_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update the read disabled flag
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_READ_ENABLED_CAP;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_READ_ENABLED_CAP_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_READ_ENABLED_CAP_STRING, "value not specified");
    return Status;
  }
  //
  // Read the read status attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_READ_STATUS_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update the read disabled flag
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_READ_STATUS;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_READ_STATUS_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_READ_STATUS_STRING, "value not specified");
    return Status;
  }
  //
  // Read the write disabled capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_WRITE_DISABLED_CAP_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update the write disabled flag
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_WRITE_DISABLED_CAP;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_WRITE_DISABLED_CAP_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_WRITE_DISABLED_CAP_STRING, "value not specified");
    return Status;
  }
  //
  // Read the write enabled capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_WRITE_ENABLED_CAP_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update the write disabled flag
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_WRITE_ENABLED_CAP;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_WRITE_ENABLED_CAP_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_WRITE_ENABLED_CAP_STRING, "value not specified");
    return Status;
  }
  //
  // Read the write status attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_WRITE_STATUS_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update the write disabled flag
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_WRITE_STATUS;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_WRITE_STATUS_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_WRITE_STATUS_STRING, "value not specified");
    return Status;
  }
  //
  // Read the lock capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_LOCK_CAP_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update the attribute flag
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_LOCK_CAP;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_LOCK_CAP_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_LOCK_CAP_STRING, "value not specified");
    return Status;
  }
  //
  // Read the lock status attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_LOCK_STATUS_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update the attribute flag
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_LOCK_STATUS;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_LOCK_STATUS_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_LOCK_STATUS_STRING, "value not specified");
    return Status;
  }
  //
  // Read the sticky write attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_STICKY_WRITE_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update the attribute flag
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_STICKY_WRITE;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_STICKY_WRITE_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_STICKY_WRITE_STRING, "value not specified");
    return Status;
  }
  //
  // Read the memory mapped attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_MEMORY_MAPPED_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update the attribute flag
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_MEMORY_MAPPED;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_MEMORY_MAPPED_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_MEMORY_MAPPED_STRING, "value not specified");
    return Status;
  }
  //
  // Read the erase polarity attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ERASE_POLARITY_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update the attribute flag
    //
    if (strcmp (Value, ONE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ERASE_POLARITY;
    } else if (strcmp (Value, ZERO_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ERASE_POLARITY_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ERASE_POLARITY_STRING, "value not specified");
    return Status;
  }

#if (PI_SPECIFICATION_VERSION >= 0x00010000)        
  //
  // Read the read lock capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_READ_LOCK_CAP_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_READ_LOCK_CAP;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_READ_LOCK_CAP_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_READ_LOCK_CAP_STRING, "value not specified");
    return Status;
  }

  //
  // Read the read lock status attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_READ_LOCK_STATUS_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_READ_LOCK_STATUS;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_READ_LOCK_STATUS_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_READ_LOCK_STATUS_STRING, "value not specified");
    return Status;
  }

  //
  // Read the write lock capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_WRITE_LOCK_CAP_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_WRITE_LOCK_CAP;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_WRITE_LOCK_CAP_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_WRITE_LOCK_CAP_STRING, "value not specified");
    return Status;
  }

  //
  // Read the write lock status attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_WRITE_LOCK_STATUS_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_WRITE_LOCK_STATUS;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_WRITE_LOCK_STATUS_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_WRITE_LOCK_STATUS_STRING, "value not specified");
    return Status;
  }
#endif

#if (PI_SPECIFICATION_VERSION < 0x00010000)     
  //
  // Read the alignment capabilities attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_CAP_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_CAP;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_CAP_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_CAP_STRING, "value not specified");
    return Status;
  }

  //
  // Read the word alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_2_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_2;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_2_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_2_STRING, "value not specified");
    return Status;
  }

  
  //
  // Read the dword alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_4_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_4;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_4_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_4_STRING, "value not specified");
    return Status;
  }
  //
  // Read the word alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_8_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_8;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_8_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_8_STRING, "value not specified");
    return Status;
  }
  //
  // Read the qword alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_16_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_16;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_16_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_16_STRING, "value not specified");
    return Status;
  }
  //
  // Read the 32 byte alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_32_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_32;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_32_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_32_STRING, "value not specified");
    return Status;
  }
  //
  // Read the 64 byte alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_64_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_64;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_64_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_64_STRING, "value not specified");
    return Status;
  }
  //
  // Read the 128 byte alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_128_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_128;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_128_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_128_STRING, "value not specified");
    return Status;
  }
  //
  // Read the 256 byte alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_256_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_256;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_256_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_256_STRING, "value not specified");
    return Status;
  }
  //
  // Read the 512 byte alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_512_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_512;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_512_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_512_STRING, "value not specified");
    return Status;
  }
  //
  // Read the 1K byte alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_1K_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_1K;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_1K_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_1K_STRING, "value not specified");
    return Status;
  }
  //
  // Read the 2K byte alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_2K_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_2K;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_2K_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_2K_STRING, "value not specified");
    return Status;
  }
  //
  // Read the 4K byte alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_4K_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_4K;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_4K_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_4K_STRING, "value not specified");
    return Status;
  }
  //
  // Read the 8K byte alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_8K_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_8K;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_8K_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_8K_STRING, "value not specified");
    return Status;
  }
  //
  // Read the 16K byte alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_16K_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_16K;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_16K_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_16K_STRING, "value not specified");
    return Status;
  }
  //
  // Read the 32K byte alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_32K_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_32K;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_32K_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_32K_STRING, "value not specified");
    return Status;
  }
  //
  // Read the 64K byte alignment capability attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB_ALIGNMENT_64K_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, TRUE_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB_ALIGNMENT_64K;
    } else if (strcmp (Value, FALSE_STRING) != 0) {
      Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_64K_STRING, "expected %s | %s", TRUE_STRING, FALSE_STRING);
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB_ALIGNMENT_64K_STRING, "value not specified");
    return Status;
  }

  if (!(FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_CAP) &&
      (
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_2) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_4) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_8) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_16) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_32) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_64) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_128) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_256) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_512) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_1K) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_2K) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_4K) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_8K) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_16K) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_32K) ||
        (FvInfo->FvAttributes & EFI_FVB_ALIGNMENT_64K)
      )
     ){
    Error (
      NULL,
      0,
      0,
      "illegal combination of alignment attributes",
      "if %s is not %s, no individual alignments can be %s",
      EFI_FVB_ALIGNMENT_CAP_STRING,
      TRUE_STRING,
      TRUE_STRING
      );
    return EFI_ABORTED;
  }
#else
  //
  // Read the PI1.0 FVB2 Alignment Capabilities Attribute
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FVB2_ALIGNMENT_STRING, 0, Value);

  if (Status == EFI_SUCCESS) {
    //
    // Update attribute
    //
    if (strcmp (Value, EFI_FVB2_ALIGNMENT_1_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_1;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_2_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_2;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_4_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_4;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_8_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_8;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_16_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_16;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_32_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_32;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_64_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_64;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_128_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_128;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_256_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_256;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_512_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_512;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_1K_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_1K;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_2K_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_2K;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_4K_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_4K;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_8K_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_8K;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_16K_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_16K;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_32K_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_32K;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_64K_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_64K;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_128K_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_128K;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_256K_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_256K;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_512K_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_512K;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_1M_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_1M;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_2M_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_2M;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_4M_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_4M;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_8M_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_8M;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_16M_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_16M;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_32M_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_32M;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_64M_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_64M;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_128M_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_128M;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_256M_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_256M;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_512M_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_512M;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_1G_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_1G;
    } else if (strcmp (Value, EFI_FVB2_ALIGNMENT_2G_STRING) == 0) {
      FvInfo->FvAttributes |= EFI_FVB2_ALIGNMENT_2G;
    } else {
      Error (NULL, 0, 0, EFI_FVB2_ALIGNMENT_STRING, "value not correct!");
      return EFI_ABORTED;
    }
  } else {
    Error (NULL, 0, 0, EFI_FVB2_ALIGNMENT_STRING, "value not specified");
    return Status;
  }

#endif  
  //
  // Read block maps
  //
  for (Index = 0; Index < MAX_NUMBER_OF_FV_BLOCKS; Index++) {
    //
    // Read the number of blocks
    //
    Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_NUM_BLOCKS_STRING, Index, Value);

    if (Status == EFI_SUCCESS) {
      if (strcmp (Value, AUTO_STRING) == 0) {
        Value64 = (UINT64) -1;
      } else {
        //
        // Update the number of blocks
        //
        Status = AsciiStringToUint64 (Value, FALSE, &Value64);
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 0, Value, "invalid value for %s", EFI_NUM_BLOCKS_STRING);
          return EFI_ABORTED;
        }
      }

      FvInfo->FvBlocks[Index].NumBlocks = (UINT32) Value64;
    } else {
      //
      // If there is no number of blocks, but there is a size, then we have a mismatched pair
      // and should return an error.
      //
      Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_BLOCK_SIZE_STRING, Index, Value);
      if (!EFI_ERROR (Status)) {
        Error (NULL, 0, 0, "must specify both", "%s and %s", EFI_NUM_BLOCKS_STRING, EFI_BLOCK_SIZE_STRING);
        return EFI_ABORTED;
      } else {
        //
        // We are done
        //
        break;
      }
    }
    //
    // Read the size of blocks
    //
    Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_BLOCK_SIZE_STRING, Index, Value);

    if (Status == EFI_SUCCESS) {
      //
      // Update the number of blocks
      //
      Status = AsciiStringToUint64 (Value, FALSE, &Value64);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 0, Value, "invalid value specified for %s", EFI_BLOCK_SIZE_STRING);
        return EFI_ABORTED;
      }

      FvInfo->FvBlocks[Index].BlockLength = (UINT32) Value64;
    } else {
      //
      // There is a number of blocks, but there is no size, so we have a mismatched pair
      // and should return an error.
      //
      Error (NULL, 0, 0, "must specify both", "%s and %s", EFI_NUM_BLOCKS_STRING, EFI_BLOCK_SIZE_STRING);
      return EFI_ABORTED;
    }
  }
  //
  // Read files
  //
  for (Index = 0; Index < MAX_NUMBER_OF_FILES_IN_FV; Index++) {
    //
    // Read the number of blocks
    //
    Status = FindToken (InfFile, FILES_SECTION_STRING, EFI_FILE_NAME_STRING, Index, Value);

    if (Status == EFI_SUCCESS) {
      //
      // Add the file
      //
      strcpy (FvInfo->FvFiles[Index], Value);
    } else {
      break;
    }
  }

  if (FindSection (InfFile, COMPONENT_SECTION_STRING)) {
    Index = 0;
    while (TRUE) {
      Status = FindTokenInstanceInSection (
                 InfFile,
                 COMPONENT_SECTION_STRING,
                 Index,
                 FvInfo->FvComponents[Index].ComponentName,
                 Value
                 );
      if (EFI_ERROR (Status)) {
        break;
      }
      Status = AsciiStringToUint64 (Value, FALSE, &Value64);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 0, Value, "not a valid integer");
        return EFI_ABORTED;
      }

      FvInfo->FvComponents[Index].Size = (UINTN) Value64;
      Index++;
    }
  }
  //
  // Compute size for easy access later
  //
  FvInfo->Size = 0;
  for (Index = 0; FvInfo->FvBlocks[Index].NumBlocks; Index++) {
    if ((FvInfo->Size == (UINTN) -1 && Index > 0) ||
        (FvInfo->FvBlocks[Index].NumBlocks == (UINT32) -1 && Index > 0)
        ) {
      //
      // Error 1. more pairs after AUTO
      // Error 2. AUTO appear in non-first position
      //
      Error (NULL, 0, 0, NULL, "cannot have more than one pair of %s and %s if %s is set to %s", 
        EFI_NUM_BLOCKS_STRING, EFI_BLOCK_SIZE_STRING,
        EFI_NUM_BLOCKS_STRING, AUTO_STRING
        );
      return EFI_ABORTED;
    }

    if (FvInfo->FvBlocks[Index].NumBlocks == (UINT32) -1) {
      FvInfo->Size = (UINTN) -1;
    } else {
      FvInfo->Size += FvInfo->FvBlocks[Index].NumBlocks * FvInfo->FvBlocks[Index].BlockLength;
    }
  }

  if (FvInfo->Size == (UINTN) -1 && FvInfo->FvFiles[0][0] == 0) {
    //
    // Non FFS FV cannot set block number to AUTO
    //
    Error (NULL, 0, 0, "non-FFS FV", "cannot set %s to %s",   EFI_NUM_BLOCKS_STRING, AUTO_STRING);
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

VOID
UpdateFfsFileState (
  IN EFI_FFS_FILE_HEADER          *FfsFile,
  IN EFI_FIRMWARE_VOLUME_HEADER   *FvHeader
  )
/*++

Routine Description:

  This function changes the FFS file attributes based on the erase polarity
  of the FV.

Arguments:

  FfsFile   File header.
  FvHeader  FV header.

Returns:

  None

--*/
{
  if (FvHeader->Attributes & EFI_FVB_ERASE_POLARITY) {
    FfsFile->State = (UINT8)~(FfsFile->State);
  }
}

EFI_STATUS
ReadFfsAlignment (
  IN EFI_FFS_FILE_HEADER    *FfsFile,
  IN OUT UINT32             *Alignment
  )
/*++

Routine Description:

  This function determines the alignment of the FFS input file from the file
  attributes.

Arguments:

  FfsFile       FFS file to parse
  Alignment     The minimum required alignment of the FFS file, in bytes

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  EFI_ABORTED              An error occurred.

--*/
{
  //
  // Verify input parameters.
  //
  if (FfsFile == NULL || Alignment == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch ((FfsFile->Attributes >> 3) & 0x07) {

  case 0:
    //
    // 1 byte alignment
    //
    *Alignment = (1 << 0);
    break;

  case 1:
    //
    // 16 byte alignment
    //
    *Alignment = (1 << 4);
    break;

  case 2:
    //
    // 128 byte alignment
    //
    *Alignment = (1 << 7);
    break;

  case 3:
    //
    // 512 byte alignment
    //
    *Alignment = (1 << 9);
    break;

  case 4:
    //
    // 1K byte alignment
    //
    *Alignment = (1 << 10);
    break;

  case 5:
    //
    // 4K byte alignment
    //
    *Alignment = (1 << 12);
    break;

  case 6:
    //
    // 32K byte alignment
    //
    *Alignment = (1 << 15);
    break;

  case 7:
    //
    // 64K byte alignment
    //
    *Alignment = (1 << 16);
    break;

  default:
    Error (NULL, 0, 0, "nvalid file attribute calculated, this is most likely a utility error", NULL);
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AddPadFile (
  IN OUT MEMORY_FILE  *FvImage,
  IN UINT32           DataAlignment
  )
/*++

Routine Description:

  This function adds a pad file to the FV image if it required to align the
  data of the next file.

Arguments:

  FvImage         The memory image of the FV to add it to.  The current offset
                  must be valid.
  DataAlignment   The data alignment of the next FFS file.

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  EFI_OUT_OF_RESOURCES     Insufficient resources exist in the FV to complete
                           the pad file add.

--*/
{
  EFI_FFS_FILE_HEADER *PadFile;
  UUID                PadFileGuid;
  UINTN               PadFileSize;

  //
  // Verify input parameters.
  //
  if (FvImage == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Basic assumption is we start from an 8 byte aligned address
  // and our file header is a multiple of 8 bytes
  //
  assert ((UINTN) FvImage->CurrentFilePointer % 8 == 0);
  assert (sizeof (EFI_FFS_FILE_HEADER) % 8 == 0);

  //
  // Check if a pad file is necessary
  //
  if (((UINTN) FvImage->CurrentFilePointer - (UINTN) FvImage->FileImage + sizeof (EFI_FFS_FILE_HEADER)) % DataAlignment == 0) {
    return EFI_SUCCESS;
  }
  //
  // Write pad file header
  //
  PadFile = (EFI_FFS_FILE_HEADER *) FvImage->CurrentFilePointer;

  //
  // Verify that we have enough space for the file header
  //
  if ((UINTN) (PadFile + sizeof (EFI_FFS_FILE_HEADER)) >= (UINTN) FvImage->Eof) {
    return EFI_OUT_OF_RESOURCES;
  }

  UuidCreate (&PadFileGuid);
  memset (PadFile, 0, sizeof (EFI_FFS_FILE_HEADER));
  memcpy (&PadFile->Name, &PadFileGuid, sizeof (EFI_GUID));
  PadFile->Type       = EFI_FV_FILETYPE_FFS_PAD;
  PadFile->Attributes = 0;

  //
  // Calculate the pad file size
  //
  //
  // This is the earliest possible valid offset (current plus pad file header
  // plus the next file header)
  //
  PadFileSize = (UINTN) FvImage->CurrentFilePointer - (UINTN) FvImage->FileImage + (sizeof (EFI_FFS_FILE_HEADER) * 2);

  //
  // Add whatever it takes to get to the next aligned address
  //
  while ((PadFileSize % DataAlignment) != 0) {
    PadFileSize++;
  }
  //
  // Subtract the next file header size
  //
  PadFileSize -= sizeof (EFI_FFS_FILE_HEADER);

  //
  // Subtract the starting offset to get size
  //
  PadFileSize -= (UINTN) FvImage->CurrentFilePointer - (UINTN) FvImage->FileImage;

  //
  // Write pad file size (calculated size minus next file header size)
  //
  PadFile->Size[0]  = (UINT8) (PadFileSize & 0xFF);
  PadFile->Size[1]  = (UINT8) ((PadFileSize >> 8) & 0xFF);
  PadFile->Size[2]  = (UINT8) ((PadFileSize >> 16) & 0xFF);

  //
  // Fill in checksums and state, they must be 0 for checksumming.
  //
  PadFile->IntegrityCheck.Checksum.Header = 0;
  PadFile->IntegrityCheck.Checksum.File   = 0;
  PadFile->State                          = 0;
  PadFile->IntegrityCheck.Checksum.Header = CalculateChecksum8 ((UINT8 *) PadFile, sizeof (EFI_FFS_FILE_HEADER));
  if (PadFile->Attributes & FFS_ATTRIB_CHECKSUM) {
#if (PI_SPECIFICATION_VERSION < 0x00010000)  
    PadFile->IntegrityCheck.Checksum.File = CalculateChecksum8 ((UINT8 *) PadFile, PadFileSize);
#else
    PadFile->IntegrityCheck.Checksum.File = CalculateChecksum8 ((UINT8 *) ((UINTN)PadFile + sizeof (EFI_FFS_FILE_HEADER)), PadFileSize - sizeof (EFI_FFS_FILE_HEADER));
#endif
  } else {
    PadFile->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
  }

  PadFile->State = EFI_FILE_HEADER_CONSTRUCTION | EFI_FILE_HEADER_VALID | EFI_FILE_DATA_VALID;
  UpdateFfsFileState (
    (EFI_FFS_FILE_HEADER *) PadFile,
    (EFI_FIRMWARE_VOLUME_HEADER *) FvImage->FileImage
    );

  //
  // Verify that we have enough space (including the padding
  //
  if ((UINTN) (PadFile + sizeof (EFI_FFS_FILE_HEADER)) >= (UINTN) FvImage->Eof) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Update the current FV pointer
  //
  FvImage->CurrentFilePointer += PadFileSize;

  return EFI_SUCCESS;
}

BOOLEAN
IsVtfFile (
  IN EFI_FFS_FILE_HEADER    *FileBuffer
  )
/*++

Routine Description:

  This function checks the header to validate if it is a VTF file

Arguments:

  FileBuffer     Buffer in which content of a file has been read.

Returns:

  TRUE    If this is a VTF file
  FALSE   If this is not a VTF file

--*/
{
  EFI_GUID  VtfGuid = EFI_FFS_VOLUME_TOP_FILE_GUID;
  if (!memcmp (&FileBuffer->Name, &VtfGuid, sizeof (EFI_GUID))) {
    return TRUE;
  } else {
    return FALSE;
  }
}

EFI_STATUS
FfsRebaseImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINT32  *ReadSize,
  OUT    VOID    *Buffer
  )
/*++

Routine Description:

  Support routine for the PE/COFF Loader that reads a buffer from a PE/COFF file

Arguments:

  FileHandle - The handle to the PE/COFF file

  FileOffset - The offset, in bytes, into the file to read

  ReadSize   - The number of bytes to read from the file starting at FileOffset

  Buffer     - A pointer to the buffer to read the data into.

Returns:

  EFI_SUCCESS - ReadSize bytes of data were read into Buffer from the PE/COFF file starting at FileOffset

--*/
{
  CHAR8   *Destination8;
  CHAR8   *Source8;
  UINT32  Length;

  Destination8  = Buffer;
  Source8       = (CHAR8 *) ((UINTN) FileHandle + FileOffset);
  Length        = *ReadSize;
  while (Length--) {
    *(Destination8++) = *(Source8++);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
RebaseFfsFile (
  IN OUT EFI_FFS_FILE_HEADER    *FfsFile,
  IN EFI_PHYSICAL_ADDRESS       BaseAddress
  )
/*++

Routine Description:

  This function determines if a file is XIP and should be rebased.  It will
  rebase any PE32 sections found in the file using the base address.

Arguments:

  FfsFile           A pointer to Ffs file image.
  BaseAddress       The base address to use for rebasing the file image.

Returns:

  EFI_SUCCESS             The image was properly rebased.
  EFI_INVALID_PARAMETER   An input parameter is invalid.
  EFI_ABORTED             An error occurred while rebasing the input file image.
  EFI_OUT_OF_RESOURCES    Could not allocate a required resource.

--*/
{
  EFI_STATUS                            Status;
  EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  UINTN                                 MemoryImagePointer;
  UINTN                                 MemoryImagePointerAligned;

  EFI_PHYSICAL_ADDRESS                  ImageAddress;
  UINT64                                ImageSize;
  EFI_PHYSICAL_ADDRESS                  EntryPoint;

  UINT32                                Pe32FileSize;
  UINT32                                NewPe32BaseAddress;

  UINTN                                 Index;
  EFI_FILE_SECTION_POINTER              CurrentPe32Section;
  UINT8                                 FileGuidString[80];

  //
  // Verify input parameters
  //
  if (FfsFile == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Convert the GUID to a string so we can at least report which file
  // if we find an error.
  //
  PrintGuidToBuffer (&FfsFile->Name, FileGuidString, sizeof (FileGuidString), TRUE);

  //
  // Do some nominal checks on the file, then check for XIP.
  //
  Status = VerifyFfsFile (FfsFile);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0, "invalid FFS file", FileGuidString);
    return EFI_INVALID_PARAMETER;
  }

  if (FfsFile->Type != EFI_FV_FILETYPE_SECURITY_CORE &&
      FfsFile->Type != EFI_FV_FILETYPE_PEI_CORE &&
      FfsFile->Type != EFI_FV_FILETYPE_PEIM
      ) {
    //
    // File is not XIP, so don't rebase
    //
    return EFI_SUCCESS;
  }
  //
  // Rebase each PE32 section
  //
  for (Index = 1;; Index++) {
    Status = GetSectionByType (FfsFile, EFI_SECTION_PE32, Index, &CurrentPe32Section);
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Calculate the PE32 base address, the FFS file base plus the offset of the PE32 section
    //
    NewPe32BaseAddress = ((UINT32) BaseAddress) + ((UINTN) CurrentPe32Section.Pe32Section - (UINTN) FfsFile);

    //
    // Initialize context
    //
    memset (&ImageContext, 0, sizeof (ImageContext));
    ImageContext.Handle     = (VOID *) ((UINTN) CurrentPe32Section.Pe32Section + sizeof (EFI_PE32_SECTION));
    ImageContext.ImageRead  = (EFI_PEI_PE_COFF_LOADER_READ_FILE) FfsRebaseImageRead;

    Status                  = mPeCoffLoader.GetImageInfo (&mPeCoffLoader, &ImageContext);

    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0, "GetImageInfo() failed", FileGuidString);
      return Status;
    }
    //
    // Allocate a buffer for the image to be loaded into.
    //
    Pe32FileSize              = GetLength (CurrentPe32Section.Pe32Section->CommonHeader.Size);
    MemoryImagePointer        = (UINTN) (malloc (Pe32FileSize + 0x1000));
    MemoryImagePointerAligned = (MemoryImagePointer + 0x0FFF) & (-1 << 12);
    if (MemoryImagePointerAligned == 0) {
      Error (NULL, 0, 0, "memory allocation failure", NULL);
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // bugbug
    //
    ImageContext.ImageAddress = MemoryImagePointerAligned;
    Status = mPeCoffLoader.LoadImage (&mPeCoffLoader, &ImageContext);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0, "LoadImage() failure", FileGuidString);
      free ((VOID *) MemoryImagePointer);
      return Status;
    }

    Status = mPeCoffLoader.RelocateImage (&mPeCoffLoader, &ImageContext);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0, "RelocateImage() failure", FileGuidString);
      free ((VOID *) MemoryImagePointer);
      return Status;
    }

    ImageAddress  = ImageContext.ImageAddress;
    ImageSize     = ImageContext.ImageSize;
    EntryPoint    = ImageContext.EntryPoint;

    if (ImageSize > Pe32FileSize) {
      Error (
        NULL,
        0,
        0,
        "rebased PE32 is larger than original PE32 image",
        "0x%X > 0x%X on file %s",
        ImageSize,
        Pe32FileSize,
        FileGuidString
        );
      free ((VOID *) MemoryImagePointer);
      return EFI_ABORTED;
    }

    memcpy (CurrentPe32Section.Pe32Section, (VOID *) MemoryImagePointerAligned, Pe32FileSize);

    free ((VOID *) MemoryImagePointer);
  }
  //
  // the above for loop will always exit with EFI_NOT_FOUND if it completes
  // normally.  If Index == 1 at exit, then no PE32 sections were found.  If it
  // exits with any other error code, then something broke...
  //
  if (Status != EFI_NOT_FOUND) {
    Error (NULL, 0, 0, "failed to parse PE32 section", FileGuidString);
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AddSymFile (
  IN UINT64               BaseAddress,
  IN EFI_FFS_FILE_HEADER  *FfsFile,
  IN OUT MEMORY_FILE      *SymImage,
  IN CHAR8                *SourceFileName
  )
/*++

Routine Description:

  This function adds the SYM tokens in the source file to the destination file.
  The SYM tokens are updated to reflect the base address.

Arguments:

  BaseAddress     The base address for the new SYM tokens.
  FfsFile         Pointer to the beginning of the FFS file in question.
  SymImage        The memory file to update with symbol information.
  SourceFileName  The source file.

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  EFI_ABORTED              An error occurred.

--*/
{
  FILE                      *SourceFile;

  CHAR8                     Buffer[_MAX_PATH];
  CHAR8                     Type[_MAX_PATH];
  CHAR8                     Address[_MAX_PATH];
  CHAR8                     Section[_MAX_PATH];
  CHAR8                     Token[_MAX_PATH];
  CHAR8                     SymFileName[_MAX_PATH];
  CHAR8                     CodeModuleName[_MAX_PATH];
  CHAR8                     *Ptr;

  UINT64                    TokenAddress;

  EFI_STATUS                Status;
  EFI_FILE_SECTION_POINTER  Pe32Section;
  UINT32                    EntryPoint;
  UINT32                    BaseOfCode;
  UINT16                    MachineType;

  //
  // Verify input parameters.
  //
  if (BaseAddress == 0 || FfsFile == NULL || SymImage == NULL || SourceFileName == NULL) {
    Error (NULL, 0, 0, "invalid parameter passed to AddSymFile()", NULL);
    return EFI_INVALID_PARAMETER;
  }
  //
  // Check if we want to add this file
  //
  //
  // Get the file name
  //
  strcpy (Buffer, SourceFileName);

  //
  // Copy the file name for the path of the sym file and truncate the name portion.
  //
  strcpy (SymFileName, Buffer);
  Ptr = strrchr (SymFileName, '\\');
  assert (Ptr);
  Ptr[0] = 0;

  //
  // Find the file extension and make it lower case
  //
  Ptr = strrchr (SymFileName, '.');
  if (Ptr != NULL) {
    _strlwr (Ptr);
  }
  //
  // Check if it is PEI file
  //
  if (strstr (Buffer, ".pei") != NULL) {
    //
    // Find the human readable portion
    //
    if (!strtok (Buffer, "-") ||
        !strtok (NULL, "-") ||
        !strtok (NULL, "-") ||
        !strtok (NULL, "-") ||
        !strtok (NULL, "-") ||
        !strcpy (Buffer, strtok (NULL, "."))
          ) {
      Error (NULL, 0, 0, "failed to find human readable portion of the file name in AddSymFile()", NULL);
      return EFI_ABORTED;
    }
    //
    // Save code module name
    //
    strcpy (CodeModuleName, Buffer);

    //
    // Add the symbol file name and extension to the file path.
    //
    strcat (Buffer, ".sym");
    strcat (SymFileName, "\\");
    strcat (SymFileName, Buffer);
  } else {
    //
    // Only handle PEIM files.
    //
    return EFI_SUCCESS;
  }
  //
  // Find PE32 section
  //
  Status = GetSectionByType (FfsFile, EFI_SECTION_PE32, 1, &Pe32Section);

  //
  // BUGBUG: Assume if no PE32 section it is PIC and hardcode base address
  //
  if (Status == EFI_NOT_FOUND) {
    Status = GetSectionByType (FfsFile, EFI_SECTION_TE, 1, &Pe32Section);
  }

  if (Status == EFI_SUCCESS) {
    Status = GetPe32Info (
               (VOID *) ((UINTN) Pe32Section.Pe32Section + sizeof (EFI_SECTION_PE32)),
               &EntryPoint,
               &BaseOfCode,
               &MachineType
               );
  } else {
    if (Status == EFI_NOT_FOUND) {
      BaseOfCode = 0x60;
      Status     = EFI_SUCCESS;
    } else {
      Error (NULL, 0, 0, "could not parse a PE32 section from the PEI file", NULL);
      return Status;
    }
  }

  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0, "GetPe32Info() could not get PE32 entry point for PEI file", NULL);
    return Status;
  }

  //
  // Open the source file
  //
  SourceFile = fopen (SymFileName, "r");
  if (SourceFile == NULL) {
    //
    // SYM files are not required.
    //
    return EFI_SUCCESS;
  }
  //
  // Read the first line
  //
  if (fgets (Buffer, _MAX_PATH, SourceFile) == NULL) {
    Buffer[0] = 0;
  }
  //
  // Make sure it matches the expected sym format
  //
  if (strcmp (Buffer, "TEXTSYM format | V1.0\n")) {
    fclose (SourceFile);
    Error (NULL, 0, 0, "AddSymFile() found unexpected sym format in input file", NULL);
    return EFI_ABORTED;
  }
  //
  // Read in the file
  //
  while (feof (SourceFile) == 0) {
    //
    // Read a line
    //
    if (fscanf (
          SourceFile,
          "%s | %s | %s | %s\n",
          Type,
          Address,
          Section,
          Token
          ) == 4) {
      //
      // If the token starts with "??" ignore it
      //
      if (Token[0] == '?' && Token[1] == '?') {
        continue;
      }
      //
      // Get the token address
      //
      AsciiStringToUint64 (Address, TRUE, &TokenAddress);

      //
      // Add the base address
      //
      TokenAddress += BaseAddress;

      //
      // If PE32 or TE section then find the start of code.  For PIC it is hardcoded.
      //
      if (Pe32Section.Pe32Section) {
        //
        // Add the offset of the PE32 section
        //
        TokenAddress += (UINTN) Pe32Section.Pe32Section - (UINTN) FfsFile;

        //
        // Add the size of the PE32 section header
        //
        TokenAddress += sizeof (EFI_PE32_SECTION);
      } else {
        //
        // BUGBUG: Don't know why this is 0x28 bytes.
        //
        TokenAddress += 0x28;
      }
      //
      // Add the beginning of the code
      //
      TokenAddress += BaseOfCode;

      sprintf (
        Buffer,
        "%s | %016I64X | %s | _%s%s\n",
        Type,
        TokenAddress,
        Section,
        CodeModuleName,
        Token
        );
      memcpy (SymImage->CurrentFilePointer, Buffer, strlen (Buffer) + 1);
      SymImage->CurrentFilePointer = (UINT8 *) (((UINTN) SymImage->CurrentFilePointer) + strlen (Buffer) + 1);
    }
  }

  fclose (SourceFile);
  return EFI_SUCCESS;
}

EFI_STATUS
ReallocateFvImage (
  IN OUT MEMORY_FILE         *FvImage,
  IN OUT FV_INFO             *FvInfo,
  IN OUT EFI_FFS_FILE_HEADER **VtfFileImage,
  IN OUT UINTN               *FvImageCapacity
  )
/*++
Routine Description:
  Increase the size of FV image by 1 block. The routine may reallocate memory
  depending on the capacity of the FV image.

Arguments:
  FvImage         The memory image of the FV to add it to.  The current offset
                  must be valid.
  FvInfo          Pointer to information about the FV.
  VtfFileImage    A pointer to the VTF file within the FvImage.  If this is equal
                  to the end of the FvImage then no VTF previously found.
  FvImageCapacity Capacity of image buffer for FV.

Returns:
  EFI_SUCCESS              The function completed successfully.
  EFI_OUT_OF_RESOURCES     Insufficient resources exist to complete the reallocation.

--*/
{
  CHAR8                      *FileImage;
  UINTN                      OldSize;
  UINTN                      IncreaseSize;
  EFI_FIRMWARE_VOLUME_HEADER *FvHeader;
  BOOLEAN                    AllocateNewMemory;
  EFI_FFS_FILE_HEADER        *NewVtfFileImage;
  UINT32                     VtfFileLength;
  UINT8                      TempByte;

  OldSize      = (UINTN) FvImage->Eof - (UINTN) FvImage->FileImage;
  IncreaseSize = FvInfo->FvBlocks[0].BlockLength;
  assert (OldSize == FvInfo->FvBlocks[0].NumBlocks * FvInfo->FvBlocks[0].BlockLength);

  //
  // Assume we have enough capacity
  //
  AllocateNewMemory = FALSE;
  

  if (OldSize + IncreaseSize > *FvImageCapacity) {
    AllocateNewMemory = TRUE;
    //
    // Increase capacity by one unit
    //
    *FvImageCapacity = OldSize + FV_CAPACITY_INCREASE_UNIT;
    FileImage = malloc (*FvImageCapacity);

    if (FileImage == NULL) {
      Error (NULL, 0, 0, "memory allocation failure", NULL);
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Initialize the content per FV polarity
    //
    if (FvInfo->FvAttributes & EFI_FVB_ERASE_POLARITY) {
      memset (FileImage, -1, *FvImageCapacity);
    } else {
      memset (FileImage, 0, *FvImageCapacity);
    }

    //
    // Copy the FV content before VTF
    //
    memcpy (FileImage, FvImage->FileImage, (UINTN) *VtfFileImage - (UINTN) FvImage->FileImage);
  } else {
    FileImage = FvImage->FileImage;
  }

  //
  // Move VTF if it exists
  //
  NewVtfFileImage = (EFI_FFS_FILE_HEADER *) (FileImage + ((UINTN) *VtfFileImage - (UINTN) FvImage->FileImage) + IncreaseSize);
  if ((UINTN) *VtfFileImage != (UINTN) FvImage->Eof) {
    //
    // Exchange the VTF buffer from end to start for two purpose:
    // 1. Exchange: Preserve the default value per FV polarity
    // 2. End->Start: Avoid destroying the VTF data during exchanging
    //
    VtfFileLength = GetLength ((*VtfFileImage)->Size);
    while (VtfFileLength-- != 0) {
      TempByte = ((UINT8 *) VtfFileImage)[VtfFileLength];
      ((UINT8 *) VtfFileImage)[VtfFileLength]    = ((UINT8 *) NewVtfFileImage)[VtfFileLength];
      ((UINT8 *) NewVtfFileImage)[VtfFileLength] = TempByte;
    }
  }

  //
  // Update VTF Pointer
  //
  *VtfFileImage = NewVtfFileImage;

  //
  // Update FvInfo
  //
  FvInfo->FvBlocks[0].NumBlocks ++;

  //
  // Update FV Header
  //
  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) FileImage;
  FvHeader->FvBlockMap[0].NumBlocks = FvInfo->FvBlocks[0].NumBlocks;
  FvHeader->FvLength                = OldSize + IncreaseSize;
  FvHeader->Checksum                = 0;
  FvHeader->Checksum                = CalculateChecksum16 ((UINT16 *) FvHeader, FvHeader->HeaderLength / sizeof (UINT16));
      
  //
  // Update FvImage
  //
  if (AllocateNewMemory) {
    free (FvImage->FileImage);
    FvImage->CurrentFilePointer = FileImage + (FvImage->CurrentFilePointer - FvImage->FileImage);    
    FvImage->FileImage          = FileImage;
  }
  FvImage->Eof                  = FvImage->FileImage + OldSize + IncreaseSize;

  InitializeFvLib (FvImage->FileImage, OldSize + IncreaseSize);
  return EFI_SUCCESS;
}

EFI_STATUS
AddFile (
  IN OUT MEMORY_FILE          *FvImage,
  IN FV_INFO                  *FvInfo,
  IN UINTN                    Index,
  IN OUT EFI_FFS_FILE_HEADER  **VtfFileImage,
  IN OUT MEMORY_FILE          *SymImage,
  IN OUT UINTN                *FvImageCapacity
  )
/*++

Routine Description:

  This function adds a file to the FV image.  The file will pad to the
  appropriate alignment if required.

Arguments:

  FvImage         The memory image of the FV to add it to.  The current offset
                  must be valid.
  FvInfo          Pointer to information about the FV.
  Index           The file in the FvInfo file list to add.
  VtfFileImage    A pointer to the VTF file within the FvImage.  If this is equal
                  to the end of the FvImage then no VTF previously found.
  SymImage        The memory image of the Sym file to update if symbols are present.
                  The current offset must be valid.
  FvImageCapacity Capacity of image buffer for FV.

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  EFI_ABORTED              An error occurred.
  EFI_OUT_OF_RESOURCES     Insufficient resources exist to complete the add.

--*/
{
  FILE                  *NewFile;
  UINTN                 FileSize;
  UINT8                 *FileBuffer;
  UINTN                 NumBytesRead;
  UINT32                CurrentFileAlignment;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  CurrentFileBaseAddress;
  UINT8                 VtfHeaderChecksum;
  UINT8                 VtfFileChecksum;
  UINT8                 FileState;
  UINT32                TailSize;
#if (PI_SPECIFICATION_VERSION < 0x00010000)
  EFI_FFS_FILE_TAIL     TailValue;
#endif
  //
  // Verify input parameters.
  //
  if (FvImage == NULL || FvInfo == NULL || FvInfo->FvFiles[Index][0] == 0 || VtfFileImage == NULL || SymImage == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Read the file to add
  //
  NewFile = fopen (FvInfo->FvFiles[Index], "rb");

  if (NewFile == NULL) {
    Error (NULL, 0, 0, FvInfo->FvFiles[Index], "failed to open file for reading");
    return EFI_ABORTED;
  }
  //
  // Get the file size
  //
  FileSize = _filelength (_fileno (NewFile));

  //
  // Read the file into a buffer
  //
  FileBuffer = malloc (FileSize);
  if (FileBuffer == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  NumBytesRead = fread (FileBuffer, sizeof (UINT8), FileSize, NewFile);

  //
  // Done with the file, from this point on we will just use the buffer read.
  //
  fclose (NewFile);

  //
  // Verify read successful
  //
  if (NumBytesRead != sizeof (UINT8) * FileSize) {
    Error (NULL, 0, 0, FvInfo->FvFiles[Index], "failed to read input file contents");
    Status = EFI_ABORTED;
    goto Exit;
  }
  //
  // Verify space exists to add the file
  //
  while (FileSize > (UINTN) ((UINTN) *VtfFileImage - (UINTN) FvImage->CurrentFilePointer)) {
    if (FvInfo->Size != (UINTN) -1) {
      Error (NULL, 0, 0, FvInfo->FvFiles[Index], "insufficient space remains to add the file");
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    } else {
      //
      // FV Size is AUTO, increase by one block
      //
      Status = ReallocateFvImage (FvImage, FvInfo, VtfFileImage, FvImageCapacity);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 0, FvInfo->FvFiles[Index], "insufficient resources to add the file");
        goto Exit;
      }
    }
  }
  //
  // Update the file state based on polarity of the FV.
  //
  UpdateFfsFileState (
    (EFI_FFS_FILE_HEADER *) FileBuffer,
    (EFI_FIRMWARE_VOLUME_HEADER *) FvImage->FileImage
    );

  //
  // If we have a VTF file, add it at the top.
  //
  if (IsVtfFile ((EFI_FFS_FILE_HEADER *) FileBuffer)) {
    if ((UINTN) *VtfFileImage == (UINTN) FvImage->Eof) {
      //
      // No previous VTF, add this one.
      //
      *VtfFileImage = (EFI_FFS_FILE_HEADER *) (UINTN) ((UINTN) FvImage->FileImage + FvInfo->Size - FileSize);
      //
      // Sanity check. The file MUST align appropriately
      //
      if ((((UINTN) *VtfFileImage) & 0x07) != 0) {
        Error (NULL, 0, 0, "VTF file does not align on 8-byte boundary", NULL);
      }
      //
      // copy VTF File Header
      //
      memcpy (*VtfFileImage, FileBuffer, sizeof (EFI_FFS_FILE_HEADER));

      //
      // Copy VTF body
      //
      memcpy (
        (UINT8 *) *VtfFileImage + sizeof (EFI_FFS_FILE_HEADER),
        FileBuffer + sizeof (EFI_FFS_FILE_HEADER),
        FileSize - sizeof (EFI_FFS_FILE_HEADER)
        );

      //
      // re-calculate the VTF File Header
      //
      FileState = (*VtfFileImage)->State;
      (*VtfFileImage)->State = 0;
      *(UINT32 *) ((*VtfFileImage)->Size) = FileSize;
      (*VtfFileImage)->IntegrityCheck.Checksum.Header = 0;
      (*VtfFileImage)->IntegrityCheck.Checksum.File = 0;

      VtfHeaderChecksum = CalculateChecksum8 ((UINT8 *) *VtfFileImage, sizeof (EFI_FFS_FILE_HEADER));
      (*VtfFileImage)->IntegrityCheck.Checksum.Header = VtfHeaderChecksum;
      //
      // Determine if it has a tail
      //
      if ((*VtfFileImage)->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
        TailSize = sizeof (EFI_FFS_FILE_TAIL);
      } else {
        TailSize = 0;
      }

      if ((*VtfFileImage)->Attributes & FFS_ATTRIB_CHECKSUM) {
    #if (PI_SPECIFICATION_VERSION < 0x00010000)
        VtfFileChecksum = CalculateChecksum8 ((UINT8 *) *VtfFileImage, FileSize - TailSize);
    #else
        VtfFileChecksum = CalculateChecksum8 ((UINT8 *) ((UINTN)*VtfFileImage + sizeof (EFI_FFS_FILE_HEADER)), FileSize - TailSize - sizeof(EFI_FFS_FILE_HEADER));
    #endif
        (*VtfFileImage)->IntegrityCheck.Checksum.File = VtfFileChecksum;
      } else {
        (*VtfFileImage)->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
      }
    #if (PI_SPECIFICATION_VERSION < 0x00010000)
      //
      // If it has a file tail, update it
      //
      if ((*VtfFileImage)->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
        TailValue = (EFI_FFS_FILE_TAIL) (~((*VtfFileImage)->IntegrityCheck.TailReference));
        *(EFI_FFS_FILE_TAIL *) (((UINTN) (*VtfFileImage) + GetLength ((*VtfFileImage)->Size) - sizeof (EFI_FFS_FILE_TAIL))) = TailValue;
      }
    #endif  
      (*VtfFileImage)->State = FileState;
      Status = EFI_SUCCESS;
      goto Exit;
    } else {
      //
      // Already found a VTF file.
      //
      Error (NULL, 0, 0, "multiple VTF files are illegal in a single FV", NULL);
      Status = EFI_ABORTED;
      goto Exit;
    }
  }
  //
  // Check if alignment is required
  //
  Status = ReadFfsAlignment ((EFI_FFS_FILE_HEADER *) FileBuffer, &CurrentFileAlignment);
  if (EFI_ERROR (Status)) {
    printf ("ERROR: Could not determine alignment of file %s.\n", FvInfo->FvFiles[Index]);
    Status = EFI_ABORTED;
    goto Exit;
  }
  //
  // Add pad file if necessary
  //
  while (EFI_ERROR (AddPadFile (FvImage, CurrentFileAlignment))) {
    if (FvInfo->Size != (UINTN) -1) {
      printf ("ERROR: Could not align the file data properly.\n");
      Status = EFI_ABORTED;
      goto Exit;
    } else {    
      //
      // FV Size is AUTO, increase by one block
      //
      Status = ReallocateFvImage (FvImage, FvInfo, VtfFileImage, FvImageCapacity);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 0, FvInfo->FvFiles[Index], "insufficient resources to add the file");
        goto Exit;
      }      
    }
  }

  //
  // Add file
  //
  while (FileSize > (UINTN) ((UINTN) *VtfFileImage - (UINTN) FvImage->CurrentFilePointer)) {
    if (FvInfo->Size != (UINTN) -1) {
      printf ("ERROR: The firmware volume is out of space, could not add file %s.\n", FvInfo->FvFiles[Index]);
      Status = EFI_ABORTED;
      goto Exit;
    } else {      
      //
      // FV Size is AUTO, increase by one one block
      //
      Status = ReallocateFvImage (FvImage, FvInfo, VtfFileImage, FvImageCapacity);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 0, FvInfo->FvFiles[Index], "insufficient resources to add the file");
        goto Exit;
      }
    }
  }

  //
  // Copy the file
  //
  memcpy (FvImage->CurrentFilePointer, FileBuffer, FileSize);

  //
  // If the file is XIP, rebase
  //
  CurrentFileBaseAddress = FvInfo->BaseAddress + ((UINTN) FvImage->CurrentFilePointer - (UINTN) FvImage->FileImage);
  //
  //    Status = RebaseFfsFile ((EFI_FFS_FILE_HEADER*) FvImage->CurrentFilePointer, CurrentFileBaseAddress);
  //    if (EFI_ERROR(Status)) {
  //      printf ("ERROR: Could not rebase the file %s.\n", FvInfo->FvFiles[Index]);
  //      return EFI_ABORTED;
  //    }
  //
  // Update Symbol file
  //
  Status = AddSymFile (
            CurrentFileBaseAddress,
            (EFI_FFS_FILE_HEADER *) FvImage->CurrentFilePointer,
            SymImage,
            FvInfo->FvFiles[Index]
            );
  assert (!EFI_ERROR (Status));

  //
  // Update the current pointer in the FV image
  //
  FvImage->CurrentFilePointer += FileSize;

  //
  // Make next file start at QWord Boundry
  //
  while (((UINTN) FvImage->CurrentFilePointer & 0x07) != 0) {
    FvImage->CurrentFilePointer++;
  }

Exit:
  //
  // Free allocated memory.
  //
  free (FileBuffer);
  return Status;
}

EFI_STATUS
AddVariableBlock (
  IN UINT8                    *FvImage,
  IN UINTN                    Size,
  IN FV_INFO                  *FvInfo
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  VARIABLE_STORE_HEADER       *VarStoreHeader;
  //
  // Variable block should exclude FvHeader. Since the length of
  // FvHeader depends on the block map, which is variable length,
  // we could only decide the actual variable block length here.
  //
  FvHeader                  = (EFI_FIRMWARE_VOLUME_HEADER *) FvImage;
  FvImage                   = FvImage + FvHeader->HeaderLength;

  VarStoreHeader            = (VARIABLE_STORE_HEADER *) FvImage;

  VarStoreHeader->Signature = VARIABLE_STORE_SIGNATURE;
  VarStoreHeader->Size      = Size - FvHeader->HeaderLength;
  VarStoreHeader->Format    = VARIABLE_STORE_FORMATTED;
  VarStoreHeader->State     = VARIABLE_STORE_HEALTHY;
  VarStoreHeader->Reserved  = 0;
  VarStoreHeader->Reserved1 = 0;

  return EFI_SUCCESS;
}

EFI_STATUS
AddEventLogBlock (
  IN UINT8                    *FvImage,
  IN UINTN                    Size,
  IN FV_INFO                  *FvInfo
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
AddFTWWorkingBlock (
  IN UINT8                    *FvImage,
  IN UINTN                    Size,
  IN FV_INFO                  *FvInfo
  )
{
  EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *FTWHeader;
  UINT32                                  Crc32;

  Crc32     = 0;
  FTWHeader = (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *) FvImage;
  memcpy (&FTWHeader->Signature, &(FvInfo->FvGuid), sizeof (EFI_GUID));
  FTWHeader->WriteQueueSize = Size - sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER);
  CalculateCrc32 (FvImage, sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER), &Crc32);
  FTWHeader->Crc = Crc32;
  if (FvInfo->FvAttributes & EFI_FVB_ERASE_POLARITY) {
    FTWHeader->WorkingBlockValid    = 0;
    FTWHeader->WorkingBlockInvalid  = 1;
  } else {
    FTWHeader->WorkingBlockValid    = 1;
    FTWHeader->WorkingBlockInvalid  = 0;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AddFTWSpareBlock (
  IN UINT8                    *FvImage,
  IN UINTN                    Size,
  IN FV_INFO                  *FvInfo
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
GenNonFFSFv (
  IN UINT8                    *FvImage,
  IN FV_INFO                  *FvInfo
  )
/*++

Routine Description:

  This function generate the non FFS FV image, such as the working block
  and spare block. How each component of the FV is built is component
  specific.

Arguments:

  FvImage       The memory image of the FV to add it to.  The current offset
                must be valid.
  FvInfo        Pointer to information about the FV.

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  EFI_ABORTED              An error occurred.
  EFI_OUT_OF_RESOURCES     Insufficient resources exist to complete the add.

--*/
{
  UINTN                       Index;
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  UINT64                      TotalSize;

  FvHeader  = (EFI_FIRMWARE_VOLUME_HEADER *) FvImage;
  TotalSize = 0;

  for (Index = 0; FvInfo->FvComponents[Index].Size != 0; Index++) {
    if (_stricmp (FvInfo->FvComponents[Index].ComponentName, EFI_NV_VARIABLE_STRING) == 0) {
      AddVariableBlock (FvImage, FvInfo->FvComponents[Index].Size, FvInfo);
    } else if (_stricmp (FvInfo->FvComponents[Index].ComponentName, EFI_NV_EVENT_LOG_STRING) == 0) {
      AddEventLogBlock (FvImage, FvInfo->FvComponents[Index].Size, FvInfo);
    } else if (_stricmp (FvInfo->FvComponents[Index].ComponentName, EFI_NV_FTW_WORKING_STRING) == 0) {
      AddFTWWorkingBlock (FvImage, FvInfo->FvComponents[Index].Size, FvInfo);
    } else if (_stricmp (FvInfo->FvComponents[Index].ComponentName, EFI_NV_FTW_SPARE_STRING) == 0) {
      AddFTWSpareBlock (FvImage, FvInfo->FvComponents[Index].Size, FvInfo);
    } else {
      printf ("Warning: Unknown Non-FFS block %s \n", FvInfo->FvComponents[Index].ComponentName);
    }

    FvImage   = FvImage + FvInfo->FvComponents[Index].Size;
    TotalSize = TotalSize + FvInfo->FvComponents[Index].Size;
  }
  //
  // Index and TotalSize is zero mean there's no component, so this is an empty fv
  //
  if ((Index != 0 || TotalSize != 0) && TotalSize != FvInfo->Size) {
    printf ("Error. Component size does not sum up to FV size.\n");
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PadFvImage (
  IN MEMORY_FILE          *FvImage,
  IN EFI_FFS_FILE_HEADER  *VtfFileImage
  )
/*++

Routine Description:

  This function places a pad file between the last file in the FV and the VTF
  file if the VTF file exists.

Arguments:

  FvImage       Memory file for the FV memory image
  VtfFileImage  The address of the VTF file.  If this is the end of the FV
                image, no VTF exists and no pad file is needed.

Returns:

  EFI_SUCCESS             Completed successfully.
  EFI_INVALID_PARAMETER   One of the input parameters was NULL.

--*/
{
  EFI_FFS_FILE_HEADER *PadFile;
  UINTN               FileSize;

  //
  // If there is no VTF or the VTF naturally follows the previous file without a
  // pad file, then there's nothing to do
  //
  if ((UINTN) VtfFileImage == (UINTN) FvImage->Eof || (void *) FvImage->CurrentFilePointer == (void *) VtfFileImage) {
    return EFI_SUCCESS;
  }
  //
  // Pad file starts at beginning of free space
  //
  PadFile = (EFI_FFS_FILE_HEADER *) FvImage->CurrentFilePointer;

  //
  // write header
  //
  memset (PadFile, 0, sizeof (EFI_FFS_FILE_HEADER));
  memcpy (&PadFile->Name, &DefaultFvPadFileNameGuid, sizeof (EFI_GUID));
  PadFile->Type       = EFI_FV_FILETYPE_FFS_PAD;
  PadFile->Attributes = 0;

  //
  // FileSize includes the EFI_FFS_FILE_HEADER
  //
  FileSize          = (UINTN) VtfFileImage - (UINTN) FvImage->CurrentFilePointer;
  PadFile->Size[0]  = (UINT8) (FileSize & 0x000000FF);
  PadFile->Size[1]  = (UINT8) ((FileSize & 0x0000FF00) >> 8);
  PadFile->Size[2]  = (UINT8) ((FileSize & 0x00FF0000) >> 16);

  //
  // Fill in checksums and state, must be zero during checksum calculation.
  //
  PadFile->IntegrityCheck.Checksum.Header = 0;
  PadFile->IntegrityCheck.Checksum.File   = 0;
  PadFile->State                          = 0;
  PadFile->IntegrityCheck.Checksum.Header = CalculateChecksum8 ((UINT8 *) PadFile, sizeof (EFI_FFS_FILE_HEADER));
  if (PadFile->Attributes & FFS_ATTRIB_CHECKSUM) {
#if (PI_SPECIFICATION_VERSION < 0x00010000)
    PadFile->IntegrityCheck.Checksum.File = CalculateChecksum8 ((UINT8 *) PadFile, FileSize);
#else
    PadFile->IntegrityCheck.Checksum.File = CalculateChecksum8 ((UINT8 *) ((UINTN) PadFile + sizeof (EFI_FFS_FILE_HEADER)), FileSize - sizeof (EFI_FFS_FILE_HEADER));
#endif
  } else {
    PadFile->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
  }

  PadFile->State = EFI_FILE_HEADER_CONSTRUCTION | EFI_FILE_HEADER_VALID | EFI_FILE_DATA_VALID;

  UpdateFfsFileState (
    (EFI_FFS_FILE_HEADER *) PadFile,
    (EFI_FIRMWARE_VOLUME_HEADER *) FvImage->FileImage
    );
  //
  // Update the current FV pointer
  //
  FvImage->CurrentFilePointer = FvImage->Eof;

  return EFI_SUCCESS;
}

EFI_STATUS
UpdateResetVector (
  IN MEMORY_FILE            *FvImage,
  IN FV_INFO                *FvInfo,
  IN EFI_FFS_FILE_HEADER    *VtfFile
  )
/*++

Routine Description:

  This parses the FV looking for the PEI core and then plugs the address into
  the SALE_ENTRY point of the BSF/VTF for IPF and does BUGBUG TBD action to
  complete an IA32 Bootstrap FV.

Arguments:

  FvImage       Memory file for the FV memory image
  FvInfo        Information read from INF file.
  VtfFile       Pointer to the VTF file in the FV image.

Returns:

  EFI_SUCCESS             Function Completed successfully.
  EFI_ABORTED             Error encountered.
  EFI_INVALID_PARAMETER   A required parameter was NULL.
  EFI_NOT_FOUND           PEI Core file not found.

--*/
{
  EFI_FFS_FILE_HEADER       *PeiCoreFile;
  EFI_FFS_FILE_HEADER       *SecCoreFile;
  EFI_STATUS                Status;
  EFI_FILE_SECTION_POINTER  Pe32Section;
  UINT32                    EntryPoint;
  UINT32                    BaseOfCode;
  UINT16                    MachineType;
  EFI_PHYSICAL_ADDRESS      PeiCorePhysicalAddress;
  EFI_PHYSICAL_ADDRESS      SecCorePhysicalAddress;
  EFI_PHYSICAL_ADDRESS      *SecCoreEntryAddressPtr;
  UINT32                    *Ia32ResetAddressPtr;
  UINT8                     *BytePointer;
  UINT8                     *BytePointer2;
  UINT16                    *WordPointer;
  UINT16                    CheckSum;
  UINTN                     Index;
  EFI_FFS_FILE_STATE        SavedState;
  UINT32                    TailSize;
  UINT64                    FitAddress;
  FIT_TABLE                 *FitTablePtr;
#if (PI_SPECIFICATION_VERSION < 0x00010000)
  EFI_FFS_FILE_TAIL         TailValue;
#endif
  //
  // Verify input parameters
  //
  if (FvImage == NULL || FvInfo == NULL || VtfFile == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Initialize FV library
  //
  InitializeFvLib (FvImage->FileImage, (UINTN) FvImage->Eof - (UINTN) FvImage->FileImage);

  //
  // Verify VTF file
  //
  Status = VerifyFfsFile (VtfFile);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Find the PEI Core
  //
  Status = GetFileByType (EFI_FV_FILETYPE_PEI_CORE, 1, &PeiCoreFile);
  if (EFI_ERROR (Status) || PeiCoreFile == NULL) {
    Error (NULL, 0, 0, "could not find the PEI core in the FV", NULL);
    return EFI_ABORTED;
  }
  //
  // PEI Core found, now find PE32 or TE section
  //
  Status = GetSectionByType (PeiCoreFile, EFI_SECTION_PE32, 1, &Pe32Section);
  if (Status == EFI_NOT_FOUND) {
    Status = GetSectionByType (PeiCoreFile, EFI_SECTION_TE, 1, &Pe32Section);
  }

  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0, "could not find PE32 or TE section in PEI core file", NULL);
    return EFI_ABORTED;
  }

  Status = GetPe32Info (
            (VOID *) ((UINTN) Pe32Section.Pe32Section + sizeof (EFI_SECTION_PE32)),
            &EntryPoint,
            &BaseOfCode,
            &MachineType
            );

  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0, "could not get PE32 entry point for PEI core", NULL);
    return EFI_ABORTED;
  }
  //
  // Physical address is FV base + offset of PE32 + offset of the entry point
  //
  PeiCorePhysicalAddress = FvInfo->BaseAddress;
  PeiCorePhysicalAddress += (UINTN) Pe32Section.Pe32Section + sizeof (EFI_SECTION_PE32) - (UINTN) FvImage->FileImage;
  PeiCorePhysicalAddress += EntryPoint;

  if (MachineType == EFI_IMAGE_MACHINE_IA64) {
    //
    // Update PEI_CORE address
    //
    //
    // Set the uncached attribute bit in the physical address
    //
    PeiCorePhysicalAddress |= 0x8000000000000000;

    //
    // Check if address is aligned on a 16 byte boundary
    //
    if (PeiCorePhysicalAddress & 0xF) {
      printf (
        "ERROR: PEI_CORE entry point is not aligned on a 16 byte boundary, address specified is %Xh.\n",
        PeiCorePhysicalAddress
        );
      return EFI_ABORTED;
    }
    //
    // First Get the FIT table address
    //
    FitAddress  = (*(UINT64 *) (FvImage->Eof - IPF_FIT_ADDRESS_OFFSET)) & 0xFFFFFFFF;

    FitTablePtr = (FIT_TABLE *) (FvImage->FileImage + (FitAddress - FvInfo->BaseAddress));

    Status      = UpdatePeiCoreEntryInFit (FitTablePtr, PeiCorePhysicalAddress);

    if (!EFI_ERROR (Status)) {
      UpdateFitCheckSum (FitTablePtr);
    }
    //
    // Find the Sec Core
    //
    Status = GetFileByType (EFI_FV_FILETYPE_SECURITY_CORE, 1, &SecCoreFile);
    if (EFI_ERROR (Status) || SecCoreFile == NULL) {
      Error (NULL, 0, 0, "could not find the Sec core in the FV", NULL);
      return EFI_ABORTED;
    }
    //
    // Sec Core found, now find PE32 section
    //
    Status = GetSectionByType (SecCoreFile, EFI_SECTION_PE32, 1, &Pe32Section);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0, "could not find PE32 section in SEC core file", NULL);
      return EFI_ABORTED;
    }

    Status = GetPe32Info (
              (VOID *) ((UINTN) Pe32Section.Pe32Section + sizeof (EFI_SECTION_PE32)),
              &EntryPoint,
              &BaseOfCode,
              &MachineType
              );
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0, "could not get PE32 entry point for SEC core", NULL);
      return EFI_ABORTED;
    }
    //
    // Physical address is FV base + offset of PE32 + offset of the entry point
    //
    SecCorePhysicalAddress = FvInfo->BaseAddress;
    SecCorePhysicalAddress += (UINTN) Pe32Section.Pe32Section + sizeof (EFI_SECTION_PE32) - (UINTN) FvImage->FileImage;
    SecCorePhysicalAddress += EntryPoint;

    //
    // Update SEC_CORE address
    //
    //
    // Set the uncached attribute bit in the physical address
    //
    SecCorePhysicalAddress |= 0x8000000000000000;

    //
    // Update the address
    //
    SecCoreEntryAddressPtr  = (EFI_PHYSICAL_ADDRESS *) ((UINTN) FvImage->Eof - IPF_SALE_ENTRY_ADDRESS_OFFSET);
    *SecCoreEntryAddressPtr = SecCorePhysicalAddress;

    //
    // Check if address is aligned on a 16 byte boundary
    //
    if (SecCorePhysicalAddress & 0xF) {
      printf (
        "ERROR: SALE_ENTRY entry point is not aligned on a 16 byte boundary, address specified is %Xh.\n",
        SecCorePhysicalAddress
        );
      return EFI_ABORTED;
    }
  } else if ((MachineType == EFI_IMAGE_MACHINE_IA32) || 
             (MachineType == EFI_IMAGE_MACHINE_X64)) {
    //
    // Get the location to update
    //
    Ia32ResetAddressPtr = (UINT32 *) ((UINTN) FvImage->Eof - IA32_PEI_CORE_ENTRY_OFFSET);

    //
    // Write lower 32 bits of physical address
    //
    *Ia32ResetAddressPtr = (UINT32) PeiCorePhysicalAddress;

    //
    // Update the BFV base address
    //
    Ia32ResetAddressPtr   = (UINT32 *) ((UINTN) FvImage->Eof - 4);
    *Ia32ResetAddressPtr  = (UINT32) (FvInfo->BaseAddress);

    CheckSum              = 0x0000;

    //
    // Update the Startup AP in the FVH header block ZeroVector region.
    //
    BytePointer   = (UINT8 *) ((UINTN) FvImage->FileImage);
    BytePointer2  = (FvInfo->Size == 0x10000) ? m64kRecoveryStartupApDataArray : m128kRecoveryStartupApDataArray;
    for (Index = 0; Index < SIZEOF_STARTUP_DATA_ARRAY; Index++) {
      *BytePointer++ = *BytePointer2++;
    }
    //
    // Calculate the checksum
    //
    WordPointer = (UINT16 *) ((UINTN) FvImage->FileImage);
    for (Index = 0; Index < SIZEOF_STARTUP_DATA_ARRAY / 2; Index++) {
      CheckSum = (UINT16) (CheckSum + ((UINT16) *WordPointer));
      WordPointer++;
    }
    //
    // Update the checksum field
    //
    BytePointer = (UINT8 *) ((UINTN) FvImage->FileImage);
    BytePointer += (SIZEOF_STARTUP_DATA_ARRAY - 2);
    WordPointer   = (UINT16 *) BytePointer;
    *WordPointer  = (UINT16) (0x10000 - (UINT32) CheckSum);
  } else {
    Error (NULL, 0, 0, "invalid machine type in PEI core", "machine type=0x%X", (UINT32) MachineType);
    return EFI_ABORTED;
  }
  //
  // Determine if it has an FFS file tail.
  //
  if (VtfFile->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
    TailSize = sizeof (EFI_FFS_FILE_TAIL);
  } else {
    TailSize = 0;
  }
  //
  // Now update file checksum
  //
  SavedState  = VtfFile->State;
  VtfFile->IntegrityCheck.Checksum.File = 0;
  VtfFile->State                        = 0;
  if (VtfFile->Attributes & FFS_ATTRIB_CHECKSUM) {
#if (PI_SPECIFICATION_VERSION < 0x00010000)
    VtfFile->IntegrityCheck.Checksum.File = CalculateChecksum8 (
                                              (UINT8 *) VtfFile,
                                              GetLength (VtfFile->Size) - TailSize
                                              );

#else
    VtfFile->IntegrityCheck.Checksum.File = CalculateChecksum8 (
                                              (UINT8 *) ((UINTN)VtfFile + sizeof (EFI_FFS_FILE_HEADER)),
                                              GetLength (VtfFile->Size) - TailSize - sizeof (EFI_FFS_FILE_HEADER)
                                              );
#endif
  } else {
    VtfFile->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
  }

  VtfFile->State = SavedState;

#if (PI_SPECIFICATION_VERSION < 0x00010000)
  //
  // Update tail if present
  //
  if (VtfFile->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
    TailValue = (EFI_FFS_FILE_TAIL) (~(VtfFile->IntegrityCheck.TailReference));
    *(EFI_FFS_FILE_TAIL *) (((UINTN) (VtfFile) + GetLength (VtfFile->Size) - sizeof (EFI_FFS_FILE_TAIL))) = TailValue;
  }
#endif
  return EFI_SUCCESS;
}

EFI_STATUS
GetPe32Info (
  IN UINT8                  *Pe32,
  OUT UINT32                *EntryPoint,
  OUT UINT32                *BaseOfCode,
  OUT UINT16                *MachineType
  )
/*++

Routine Description:

  Retrieves the PE32 entry point offset and machine type from PE image or TE image.
  See EfiImage.h for machine types.  The entry point offset is from the beginning
  of the PE32 buffer passed in.

Arguments:

  Pe32          Beginning of the PE32.
  EntryPoint    Offset from the beginning of the PE32 to the image entry point.
  BaseOfCode    Base address of code.
  MachineType   Magic number for the machine type.

Returns:

  EFI_SUCCESS             Function completed successfully.
  EFI_ABORTED             Error encountered.
  EFI_INVALID_PARAMETER   A required parameter was NULL.
  EFI_UNSUPPORTED         The operation is unsupported.

--*/
{
  EFI_IMAGE_DOS_HEADER  *DosHeader;
  EFI_IMAGE_NT_HEADERS  *NtHeader;
  EFI_TE_IMAGE_HEADER   *TeHeader;

  //
  // Verify input parameters
  //
  if (Pe32 == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // First check whether it is one TE Image.
  //
  TeHeader = (EFI_TE_IMAGE_HEADER *) Pe32;
  if (TeHeader->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    //
    // By TeImage Header to get output
    //
    *EntryPoint  = TeHeader->AddressOfEntryPoint + sizeof (EFI_TE_IMAGE_HEADER) - TeHeader->StrippedSize;
    *BaseOfCode  = TeHeader->BaseOfCode + sizeof (EFI_TE_IMAGE_HEADER) - TeHeader->StrippedSize;
    *MachineType = TeHeader->Machine;
  } else {
    //
    // Then check whether
    // is the DOS header
    //
    DosHeader = (EFI_IMAGE_DOS_HEADER *) Pe32;

    //
    // Verify DOS header is expected
    //
    if (DosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
      printf ("ERROR: Unknown magic number in the DOS header, 0x%04X.\n", DosHeader->e_magic);
      return EFI_UNSUPPORTED;
    }
    //
    // Immediately following is the NT header.
    //
    NtHeader = (EFI_IMAGE_NT_HEADERS *) ((UINTN) Pe32 + DosHeader->e_lfanew);
    
    //
    // Verify NT header is expected
    //
    if (NtHeader->Signature != EFI_IMAGE_NT_SIGNATURE) {
      printf ("ERROR: Unrecognized image signature 0x%08X.\n", NtHeader->Signature);
      return EFI_UNSUPPORTED;
    }
    //
    // Get output
    //
    *EntryPoint   = NtHeader->OptionalHeader.AddressOfEntryPoint;
    *BaseOfCode   = NtHeader->OptionalHeader.BaseOfCode;
    *MachineType  = NtHeader->FileHeader.Machine;
  }

  //
  // Verify machine type is supported
  //
  if (*MachineType != EFI_IMAGE_MACHINE_IA32 && 
      *MachineType != EFI_IMAGE_MACHINE_IA64 &&
      *MachineType != EFI_IMAGE_MACHINE_X64) {
    printf ("ERROR: Unrecognized machine type in the PE32 file.\n");
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}
//
// Exposed function implementations (prototypes are defined in GenFvImageLib.h)
//
EFI_STATUS
GenerateFvImage (
  IN CHAR8    *InfFileImage,
  IN UINTN    InfFileSize,
  OUT UINT8   **FvImage,
  OUT UINTN   *FvImageSize,
  OUT CHAR8   **FvFileName,
  OUT UINT8   **SymImage,
  OUT UINTN   *SymImageSize,
  OUT CHAR8   **SymFileName
  )
/*++

Routine Description:

  This is the main function which will be called from application.

Arguments:

  InfFileImage  Buffer containing the INF file contents.
  InfFileSize   Size of the contents of the InfFileImage buffer.
  FvImage       Pointer to the FV image created.
  FvImageSize   Size of the FV image created and pointed to by FvImage.
  FvFileName    Requested name for the FV file.
  SymImage      Pointer to the Sym image created.
  SymImageSize  Size of the Sym image created and pointed to by SymImage.
  SymFileName   Requested name for the Sym file.

Returns:

  EFI_SUCCESS             Function completed successfully.
  EFI_OUT_OF_RESOURCES    Could not allocate required resources.
  EFI_ABORTED             Error encountered.
  EFI_INVALID_PARAMETER   A required parameter was NULL.

--*/
{
  EFI_STATUS                  Status;
  MEMORY_FILE                 InfMemoryFile;
  MEMORY_FILE                 FvImageMemoryFile;
  MEMORY_FILE                 SymImageMemoryFile;
  FV_INFO                     FvInfo;
  UINTN                       Index;
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  EFI_FFS_FILE_HEADER         *VtfFileImage;
  UINTN                       FvImageCapacity;

  //
  // Check for invalid parameter
  //
  if (InfFileImage == NULL || FvImage == NULL || FvImageSize == NULL || FvFileName == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Initialize file structures
  //
  InfMemoryFile.FileImage           = InfFileImage;
  InfMemoryFile.CurrentFilePointer  = InfFileImage;
  InfMemoryFile.Eof                 = InfFileImage + InfFileSize;

  //
  // Parse the FV inf file for header information
  //
  Status = ParseFvInf (&InfMemoryFile, &FvInfo);
  if (EFI_ERROR (Status)) {
    printf ("ERROR: Could not parse the input INF file.\n");
    return EFI_ABORTED;
  }
  //
  // Update the file name return values
  //
  strcpy (*FvFileName, FvInfo.FvName);
  strcpy (*SymFileName, FvInfo.SymName);

  //
  // Calculate the FV size
  //
  if (FvInfo.Size != (UINTN) -1) {
    *FvImageSize    = FvInfo.Size;
    FvImageCapacity = FvInfo.Size;
  } else {
    //
    // For auto size, set default as one block
    //
    FvInfo.FvBlocks[0].NumBlocks = 1;
    *FvImageSize    = FvInfo.FvBlocks[0].BlockLength;
    FvImageCapacity = FV_CAPACITY_INCREASE_UNIT;
  }

  //
  // Allocate the FV
  //
  *FvImage = malloc (FvImageCapacity);
  if (*FvImage == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Allocate space for symbol file storage
  //
  *SymImage = malloc (SYMBOL_FILE_SIZE);
  if (*SymImage == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Initialize the FV to the erase polarity
  //
  if (FvInfo.FvAttributes & EFI_FVB_ERASE_POLARITY) {
    memset (*FvImage, -1, FvImageCapacity);
  } else {
    memset (*FvImage, 0, FvImageCapacity);
  }
  //
  // Initialize FV header
  //
  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) *FvImage;

  //
  // Initialize the zero vector to all zeros.
  //
  memset (FvHeader->ZeroVector, 0, 16);

  //
  // Copy the FFS GUID
  //
  memcpy (&FvHeader->FileSystemGuid, &FvInfo.FvGuid, sizeof (EFI_GUID));

  FvHeader->FvLength    = *FvImageSize;
  FvHeader->Signature   = EFI_FVH_SIGNATURE;
  FvHeader->Attributes  = FvInfo.FvAttributes;
#if (PI_SPECIFICATION_VERSION < 0x00010000)
  FvHeader->Revision    = EFI_FVH_REVISION;
  FvHeader->Reserved[0] = 0;
  FvHeader->Reserved[1] = 0;
  FvHeader->Reserved[2] = 0;
#else
  FvHeader->Revision    = EFI_FVH_PI_REVISION;
  FvHeader->ExtHeaderOffset = 0;
  FvHeader->Reserved[0] = 0;
#endif
  //
  // Copy firmware block map
  //
  for (Index = 0; FvInfo.FvBlocks[Index].NumBlocks != 0; Index++) {
    FvHeader->FvBlockMap[Index].NumBlocks   = FvInfo.FvBlocks[Index].NumBlocks;
    FvHeader->FvBlockMap[Index].BlockLength = FvInfo.FvBlocks[Index].BlockLength;
  }
  //
  // Add block map terminator
  //
  FvHeader->FvBlockMap[Index].NumBlocks   = 0;
  FvHeader->FvBlockMap[Index].BlockLength = 0;

  //
  // Complete the header
  //
  FvHeader->HeaderLength  = (UINT16) (((UINTN) &(FvHeader->FvBlockMap[Index + 1])) - (UINTN) *FvImage);
  FvHeader->Checksum      = 0;
  FvHeader->Checksum      = CalculateChecksum16 ((UINT16 *) FvHeader, FvHeader->HeaderLength / sizeof (UINT16));

  //
  // If there is no FFS file, find and generate each components of the FV
  //
  if (FvInfo.FvFiles[0][0] == 0) {
    Status = GenNonFFSFv (*FvImage, &FvInfo);
    if (EFI_ERROR (Status)) {
      printf ("ERROR: Could not generate NonFFS FV.\n");
      free (*FvImage);
      return EFI_ABORTED;
    }

    return EFI_SUCCESS;
  }
  //
  // Initialize our "file" view of the buffer
  //
  FvImageMemoryFile.FileImage           = *FvImage;
  FvImageMemoryFile.CurrentFilePointer  = *FvImage + FvHeader->HeaderLength;
  FvImageMemoryFile.Eof                 = *FvImage +*FvImageSize;

  //
  // Initialize our "file" view of the symbol file.
  //
  SymImageMemoryFile.FileImage          = *SymImage;
  SymImageMemoryFile.CurrentFilePointer = *SymImage;
  SymImageMemoryFile.Eof                = *FvImage + SYMBOL_FILE_SIZE;

  //
  // Initialize the FV library.
  //
  InitializeFvLib (FvImageMemoryFile.FileImage, *FvImageSize);

  //
  // Files start on 8 byte alignments, so move to the next 8 byte aligned
  // address.  For now, just assert if it isn't.  Currently FV header is
  // always a multiple of 8 bytes.
  // BUGBUG: Handle this better
  //
  assert ((((UINTN) FvImageMemoryFile.CurrentFilePointer) % 8) == 0);

  //
  // Initialize the VTF file address.
  //
  VtfFileImage = (EFI_FFS_FILE_HEADER *) FvImageMemoryFile.Eof;

  //
  // Add files to FV
  //
  for (Index = 0; FvInfo.FvFiles[Index][0] != 0; Index++) {
    //
    // Add the file
    //
    Status = AddFile (&FvImageMemoryFile, &FvInfo, Index, &VtfFileImage, &SymImageMemoryFile, &FvImageCapacity);

    //
    // Update FvImageSize and FvImage as they may be changed in AddFile routine
    //
    if (FvInfo.Size == (UINTN) -1) {
      *FvImageSize = FvInfo.FvBlocks[0].NumBlocks * FvInfo.FvBlocks[0].BlockLength;
      *FvImage     = FvImageMemoryFile.FileImage;
    }

    //
    // Exit if error detected while adding the file
    //
    if (EFI_ERROR (Status)) {
      printf ("ERROR: Could not add file %s.\n", FvInfo.FvFiles[Index]);
      free (*FvImage);
      return EFI_ABORTED;
    }
  }
  //
  // If there is a VTF file, some special actions need to occur.
  //
  if ((UINTN) VtfFileImage != (UINTN) FvImageMemoryFile.Eof) {
    //
    // Pad from the end of the last file to the beginning of the VTF file.
    //
    Status = PadFvImage (&FvImageMemoryFile, VtfFileImage);
    if (EFI_ERROR (Status)) {
      printf ("ERROR: Could not create the pad file between the last file and the VTF file.\n");
      free (*FvImage);
      return EFI_ABORTED;
    }
    //
    // Update reset vector (SALE_ENTRY for IPF)
    // Now for IA32 and IA64 platform, the fv which has bsf file must have the 
    // EndAddress of 0xFFFFFFFF. Thus, only this type fv needs to update the   
    // reset vector. If the PEI Core is found, the VTF file will probably get  
    // corrupted by updating the entry point.                                  
    //
    if ((FvInfo.BaseAddress + *FvImageSize) == FV_IMAGES_TOP_ADDRESS) {       
      Status = UpdateResetVector (&FvImageMemoryFile, &FvInfo, VtfFileImage);
      if (EFI_ERROR(Status)) {                                               
        printf ("ERROR: Could not update the reset vector.\n");              
        free (*FvImage);                                                     
        return EFI_ABORTED;                                                  
      }                                                                      
    }
  }
  //
  // Determine final Sym file size
  //
  *SymImageSize = SymImageMemoryFile.CurrentFilePointer - SymImageMemoryFile.FileImage;

  return EFI_SUCCESS;
}

EFI_STATUS
UpdatePeiCoreEntryInFit (
  IN FIT_TABLE     *FitTablePtr,
  IN UINT64        PeiCorePhysicalAddress
  )
/*++

Routine Description:

  This function is used to update the Pei Core address in FIT, this can be used by Sec core to pass control from
  Sec to Pei Core

Arguments:

  FitTablePtr             - The pointer of FIT_TABLE.
  PeiCorePhysicalAddress  - The address of Pei Core entry.

Returns:

  EFI_SUCCESS             - The PEI_CORE FIT entry was updated successfully.
  EFI_NOT_FOUND           - Not found the PEI_CORE FIT entry.

--*/
{
  FIT_TABLE *TmpFitPtr;
  UINTN     Index;
  UINTN     NumFitComponents;

  TmpFitPtr         = FitTablePtr;
  NumFitComponents  = TmpFitPtr->CompSize;

  for (Index = 0; Index < NumFitComponents; Index++) {
    if ((TmpFitPtr->CvAndType & FIT_TYPE_MASK) == COMP_TYPE_FIT_PEICORE) {
      TmpFitPtr->CompAddress = PeiCorePhysicalAddress;
      return EFI_SUCCESS;
    }

    TmpFitPtr++;
  }

  return EFI_NOT_FOUND;
}

VOID
UpdateFitCheckSum (
  IN FIT_TABLE   *FitTablePtr
  )
/*++

Routine Description:

  This function is used to update the checksum for FIT.


Arguments:

  FitTablePtr             - The pointer of FIT_TABLE.

Returns:

  None.

--*/
{
  if ((FitTablePtr->CvAndType & CHECKSUM_BIT_MASK) >> 7) {
    FitTablePtr->CheckSum = 0;
    FitTablePtr->CheckSum = CalculateChecksum8 ((UINT8 *) FitTablePtr, FitTablePtr->CompSize * 16);
  }
}
