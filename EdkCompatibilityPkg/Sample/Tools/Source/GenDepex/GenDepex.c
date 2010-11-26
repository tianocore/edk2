/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GenDepex.c

Abstract:

  Generate Dependency Expression ("GenDepex")

  Infix to Postfix Algorithm

  This code has been scrubbed to be free of having any EFI core tree dependencies.
  It should build in any environment that supports a standard C-library w/ string
  operations and File I/O services.

  As an example of usage, consider the following:

  The input user file could be something like "Sample.DXS" whose contents are

    #include "Tiano.h"

    DEPENDENCY_START
      NOT (DISK_IO_PROTOCOL AND SIMPLE_FILE_SYSTEM_PROTOCOL) 
        OR EFI_PXE_BASE_CODE_PROTOCOL
    DEPENDENCY_END

  This file is then washed through the C-preprocessor, viz.,

    cl /EP Sample.DXS > Sample.TMP1

  This yields the following file "Sample.TMP1" whose contents are

    DEPENDENCY_START
      NOT ({ 0xce345171, 0xba0b, 0x11d2, 0x8e, 0x4f, 0x0, 0xa0, 0xc9, 0x69, 0x72,
        0x3b } AND { 0x964e5b22, 0x6459, 0x11d2, 0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69,
        0x72, 0x3b }) OR { 0x03c4e603, 0xac28, 0x11d3, 0x9a, 0x2d, 0x00, 0x90, 0x27,
        0x3f, 0xc1, 0x4d }
    DEPENDENCY_END

  This file, in turn, will be fed into the utility, viz.,

    GenDepex Sample.TMP1 Sample.TMP2

  With a file that is 55 bytes long:

     55 bytes for the grammar binary
        PUSH opcode         - 1  byte
        GUID Instance       - 16 bytes
        PUSH opcode         - 1  byte
        GUID Instance       - 16 bytes
        AND opcode          - 1  byte
        NOT opcode          - 1  byte
        PUSH opcode         - 1  byte
        GUID Instance       - 16 bytes
        OR opcode           - 1  byte
        END opcode          - 1  byte

  The file "Sample.TMP2" could be fed via a Section-builder utility 
  (GenSection) that would be used for the creation of a dependency
  section file (.DPX) which in turn would be used by a generate FFS
  utility (GenFfsFile) to produce a DXE driver/core (.DXE) or 
  a DXE application (.APP) file.

  Complies with Tiano C Coding Standards Document, version 0.31, 12 Dec 2000.

--*/

#include "GenDepex.h"

//
// Utility Name
//
#define UTILITY_NAME  "GenDepex"

//
// Utility version information
//
#define UTILITY_VERSION "v1.0"

extern
BOOLEAN
ParseDepex (
  IN      INT8      *Pbegin,
  IN      UINT32    length
  );

VOID
PrintGenDepexUtilityInfo (
  VOID
  )
/*++

Routine Description:

  Displays the standard utility information to SDTOUT.

Arguments:

  None

Returns:

  None

--*/
{
  int         Index;
  const char  *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel Generate Dependency Expression Utility",
    "  Copyright (C), 1996 - 2008 Intel Corporation",
    
#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif
    NULL
  };
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
}

VOID
PrintGenDepexUsageInfo (
  VOID
  )
/*++

Routine Description:

  Displays the utility usage syntax to STDOUT.

Arguments:

  None

Returns:

  None

--*/
{
  int         Index;
  const char  *Str[] = {
    "",
    "Usage:",
    "  "UTILITY_NAME" [OPTION]...",
    "Options:",
    "  -I INFILE    The input pre-processed dependency text files name",
    "  -O OUTFILE   The output binary dependency files name",
    "  -P BOUNDARY  The padding integer value to align the output file size",
    NULL
  };

  PrintGenDepexUtilityInfo ();
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
}

DEPENDENCY_OPCODE
PopOpCode (
  IN OUT VOID **Stack
  )
/*++

Routine Description:

  Pop an element from the Opcode stack.

Arguments:

  Stack               Current top of the OpCode stack location

Returns:

  DEPENDENCY_OPCODE   OpCode at the top of the OpCode stack.
  Stack               New top of the OpCode stack location


--*/
{
  DEPENDENCY_OPCODE *OpCodePtr;

  OpCodePtr = *Stack;
  OpCodePtr--;
  *Stack = OpCodePtr;
  return *OpCodePtr;
}

VOID
PushOpCode (
  IN OUT  VOID                **Stack,
  IN      DEPENDENCY_OPCODE   OpCode
  )
/*++

Routine Description:

  Push an element onto the Opcode Stack

Arguments:

  Stack     Current top of the OpCode stack location
  OpCode    OpCode to push onto the stack

Returns:

  Stack     New top of the OpCode stack location

--*/
{
  DEPENDENCY_OPCODE *OpCodePtr;

  OpCodePtr   = *Stack;
  *OpCodePtr  = OpCode;
  OpCodePtr++;
  *Stack = OpCodePtr;
}

EFI_STATUS
GenerateDependencyExpression (
  IN     FILE           *InFile,
  IN OUT FILE           *OutFile,
  IN     UINT8          Padding  OPTIONAL
  )
/*++

Routine Description:

  This takes the pre-compiled dependency text file and 
  converts it into a binary dependency file.

  The BNF for the dependency expression is as follows 
  (from the DXE 1.0 Draft specification).

  The inputted BNF grammar is thus:
    <depex> ::= sor <dep> | 
                before GUID <dep> | 
                after GUID <dep> | 
                <bool>

    <dep> ::=   <bool> |

    <bool> ::=  <bool> and <term> | 
                <bool> or <term> | 
                <term>

    <term> ::=  not <factor> | 
                <factor>

    <factor> ::= ( <bool> ) | 
                 <term> <term> | 
                 GUID | 
                 <boolval>

    <boolval> ::= true | 
                  false

  The outputed binary grammer is thus:
    <depex> ::= sor <dep> | 
                before <depinst> <dep> | 
                after <depinst> <dep> | 
                <bool>

    <dep> ::=   <bool> |

    <bool> ::=  <bool> and <term> | 
                <bool> or <term> | <term>

    <term> ::=  not <factor> | 
                <factor>

    <factor> ::= ( <bool> ) | 
                 <term> <term> | 
                 <boolval> | 
                 <depinst> | 
                 <termval>

    <boolval> ::= true | 
                  false

    <depinst> ::= push GUID

    <termval> ::= end

  BugBug: A correct grammer is parsed correctly. A file that violates the
          grammer may parse when it should generate an error. There is some
          error checking and it covers most of the case when it's an include
          of definition issue. An ill formed expresion may not be detected.

Arguments:

  InFile -  Input pre-compiled text file of the dependency expression.
            This needs to be in ASCII.
            The file pointer can not be NULL.

  OutFile - Binary dependency file.
            The file pointer can not be NULL.

  Padding - OPTIONAL integer value to pad the output file to.


Returns:

  EFI_SUCCESS             The function completed successfully.
  EFI_INVALID_PARAMETER   One of the parameters in the text file was invalid.
  EFI_OUT_OF_RESOURCES    Unable to allocate memory.
  EFI_ABORTED             An misc error occurred.

--*/
{
  INT8              *Ptrx;
  INT8              *Pend;
  INT8              *EvaluationStack;
  INT8              *StackPtr;
  INT8              *Buffer;
  INT8              Line[LINESIZE];
  UINTN             Index;
  UINTN             OutFileSize;
  UINTN             FileSize;
  UINTN             Results;
  BOOLEAN           NotDone;
  BOOLEAN           Before_Flag;
  BOOLEAN           After_Flag;
  BOOLEAN           Dep_Flag;
  BOOLEAN           SOR_Flag;
  EFI_GUID          Guid;
  UINTN             ArgCountParsed;
  DEPENDENCY_OPCODE Opcode;

  Before_Flag = FALSE;
  After_Flag  = FALSE;
  Dep_Flag    = FALSE;
  SOR_Flag    = FALSE;

  memset (Line, 0, LINESIZE);

  OutFileSize     = 0;

  EvaluationStack = (INT8 *) malloc (EVAL_STACK_SIZE);

  if (EvaluationStack != NULL) {
    StackPtr = EvaluationStack;
  } else {
    printf ("Unable to allocate memory to EvaluationStack - Out of resources\n");
    return EFI_OUT_OF_RESOURCES;
  }

  Results = (UINTN) fseek (InFile, 0, SEEK_END);

  if (Results != 0) {
    printf ("FSEEK failed - Aborted\n");
    return EFI_ABORTED;
  }

  FileSize = ftell (InFile);

  if (FileSize == -1L) {
    printf ("FTELL failed - Aborted\n");
    return EFI_ABORTED;
  }

  Buffer = (INT8 *) malloc (FileSize + BUFFER_SIZE);

  if (Buffer == NULL) {
    printf ("Unable to allocate memory to Buffer - Out of resources\n");
    free (EvaluationStack);

    Results = (UINTN) fclose (InFile);
    if (Results != 0) {
      printf ("FCLOSE failed\n");
    }

    Results = (UINTN) fclose (OutFile);
    if (Results != 0) {
      printf ("FCLOSE failed\n");
    }

    return EFI_OUT_OF_RESOURCES;
  }

  Results = (UINTN) fseek (InFile, 0, SEEK_SET);

  if (Results != 0) {
    printf ("FSEEK failed - Aborted\n");
    return EFI_ABORTED;
  }

  fread (Buffer, FileSize, 1, InFile);

  Ptrx    = Buffer;
  Pend    = Ptrx + FileSize - strlen (DEPENDENCY_END);
  Index   = FileSize;

  NotDone = TRUE;
  while ((Index--) && NotDone) {

    if (strncmp (Pend, DEPENDENCY_END, strlen (DEPENDENCY_END)) == 0) {
      NotDone = FALSE;
    } else {
      Pend--;
    }
  }

  if (NotDone) {
    printf ("Couldn't find end string %s\n", DEPENDENCY_END);

    Results = (UINTN) fclose (InFile);
    if (Results != 0) {
      printf ("FCLOSE failed\n");
    }

    Results = (UINTN) fclose (OutFile);
    if (Results != 0) {
      printf ("FCLOSE failed\n");
    }

    free (Buffer);
    free (EvaluationStack);

    return EFI_INVALID_PARAMETER;
  }

  Index   = FileSize;

  NotDone = TRUE;
  while ((Index--) && NotDone) {

    if (strncmp (Ptrx, DEPENDENCY_START, strlen (DEPENDENCY_START)) == 0) {
      Ptrx += strlen (DEPENDENCY_START);
      NotDone = FALSE;
      //
      // BUGBUG -- should Index be decremented by sizeof(DEPENDENCY_START)?
      //
    } else {
      Ptrx++;
    }
  }

  if (NotDone) {
    printf ("Couldn't find start string %s\n", DEPENDENCY_START);

    Results = (UINTN) fclose (InFile);
    if (Results != 0) {
      printf ("FCLOSE failed\n");
    }

    Results = (UINTN) fclose (OutFile);
    if (Results != 0) {
      printf ("FCLOSE failed\n");
    }

    free (Buffer);
    free (EvaluationStack);

    return EFI_INVALID_PARAMETER;
  }
  //
  //  validate the syntax of expression
  //
  if (!ParseDepex (Ptrx, Pend - Ptrx - 1)) {
    printf ("The syntax of expression is wrong\n");

    Results = (UINTN) fclose (InFile);
    if (Results != 0) {
      printf ("FCLOSE failed\n");
    }

    Results = (UINTN) fclose (OutFile);
    if (Results != 0) {
      printf ("FCLOSE failed\n");
    }

    free (Buffer);
    free (EvaluationStack);

    return EFI_INVALID_PARAMETER;
  }

  NotDone = TRUE;

  while ((Index--) && NotDone) {

    if (*Ptrx == ' ') {
      Ptrx++;
    } else if (*Ptrx == '\n' || *Ptrx == '\r') {
      Ptrx++;
    } else if (strncmp (Ptrx, OPERATOR_SOR, strlen (OPERATOR_SOR)) == 0) {
      //
      //  Checks for some invalid dependencies
      //
      if (Before_Flag) {

        printf ("A BEFORE operator was detected.\n");
        printf ("There can only be one SOR or one AFTER or one BEFORE operator\n");
        return EFI_INVALID_PARAMETER;

      } else if (After_Flag) {

        printf ("An AFTER operator was detected.\n");
        printf ("There can only be one SOR or one AFTER or one BEFORE operator\n");
        return EFI_INVALID_PARAMETER;

      } else if (SOR_Flag) {

        printf ("Another SOR operator was detected.\n");
        printf ("There can only be one SOR or one AFTER or one BEFORE operator\n");
        return EFI_INVALID_PARAMETER;

      } else if (Dep_Flag) {

        printf ("The Schedule On Request - SOR operator must be the first operator following DEPENDENCY_START\n");
        return EFI_INVALID_PARAMETER;

      } else {
        //
        //  BUGBUG - This was not in the spec but is in the CORE code
        //  An OPERATOR_SOR has to be first - following the DEPENDENCY_START
        //
        fputc (EFI_DEP_SOR, OutFile);
        OutFileSize++;
        Ptrx += strlen (OPERATOR_SOR);
        SOR_Flag = TRUE;

      }
    } else if (strncmp (Ptrx, OPERATOR_BEFORE, strlen (OPERATOR_BEFORE)) == 0) {
      //
      //  Checks for some invalid dependencies
      //
      if (Before_Flag) {

        printf ("Another BEFORE operator was detected.\n");
        printf ("There can only be one SOR or one AFTER or one BEFORE operator\n");
        return EFI_INVALID_PARAMETER;

      } else if (After_Flag) {

        printf ("An AFTER operator was detected.\n");
        printf ("There can only be one SOR or one AFTER or one BEFORE operator\n");
        return EFI_INVALID_PARAMETER;

      } else if (SOR_Flag) {

        printf ("A SOR operator was detected.\n");
        printf ("There can only be one SOR or one AFTER or one BEFORE operator\n");
        return EFI_INVALID_PARAMETER;

      } else if (Dep_Flag) {

        printf ("The BEFORE operator must be the first operator following DEPENDENCY_START\n");
        return EFI_INVALID_PARAMETER;

      } else {
        fputc (EFI_DEP_BEFORE, OutFile);
        OutFileSize++;
        Ptrx += strlen (OPERATOR_BEFORE);
        Before_Flag = TRUE;
      }
    } else if (strncmp (Ptrx, OPERATOR_AFTER, strlen (OPERATOR_AFTER)) == 0) {
      //
      //  Checks for some invalid dependencies
      //
      if (Before_Flag) {

        printf ("A BEFORE operator was detected.\n");
        printf ("There can only be one SOR or one AFTER or one BEFORE operator\n");
        return EFI_INVALID_PARAMETER;

      } else if (After_Flag) {

        printf ("Another AFTER operator was detected.\n");
        printf ("There can only be one SOR or one AFTER or one BEFORE operator\n");
        return EFI_INVALID_PARAMETER;

      } else if (SOR_Flag) {

        printf ("A SOR operator was detected.\n");
        printf ("There can only be one SOR or one AFTER or one BEFORE operator\n");
        return EFI_INVALID_PARAMETER;

      } else if (Dep_Flag) {

        printf ("The AFTER operator must be the first operator following DEPENDENCY_START\n");
        return EFI_INVALID_PARAMETER;

      } else {
        fputc (EFI_DEP_AFTER, OutFile);
        OutFileSize++;
        Ptrx += strlen (OPERATOR_AFTER);
        Dep_Flag    = TRUE;
        After_Flag  = TRUE;
      }
    } else if (strncmp (Ptrx, OPERATOR_AND, strlen (OPERATOR_AND)) == 0) {
      while (StackPtr != EvaluationStack) {
        Opcode = PopOpCode ((VOID **) &StackPtr);
        if (Opcode != DXE_DEP_LEFT_PARENTHESIS) {
          fputc (Opcode, OutFile);
          OutFileSize++;
        } else {
          PushOpCode ((VOID **) &StackPtr, DXE_DEP_LEFT_PARENTHESIS);
          break;
        }
      }

      PushOpCode ((VOID **) &StackPtr, EFI_DEP_AND);
      Ptrx += strlen (OPERATOR_AND);
      Dep_Flag = TRUE;

    } else if (strncmp (Ptrx, OPERATOR_OR, strlen (OPERATOR_OR)) == 0) {
      while (StackPtr != EvaluationStack) {
        Opcode = PopOpCode ((VOID **) &StackPtr);
        if (Opcode != DXE_DEP_LEFT_PARENTHESIS) {
          fputc (Opcode, OutFile);
          OutFileSize++;
        } else {
          PushOpCode ((VOID **) &StackPtr, DXE_DEP_LEFT_PARENTHESIS);
          break;
        }
      }

      PushOpCode ((VOID **) &StackPtr, EFI_DEP_OR);
      Ptrx += strlen (OPERATOR_OR);
      Dep_Flag = TRUE;

    } else if (strncmp (Ptrx, OPERATOR_NOT, strlen (OPERATOR_NOT)) == 0) {
      while (StackPtr != EvaluationStack) {
        Opcode = PopOpCode ((VOID **) &StackPtr);
        if (Opcode != DXE_DEP_LEFT_PARENTHESIS) {
          fputc (Opcode, OutFile);
          OutFileSize++;
        } else {
          PushOpCode ((VOID **) &StackPtr, DXE_DEP_LEFT_PARENTHESIS);
          break;
        }
      }

      PushOpCode ((VOID **) &StackPtr, EFI_DEP_NOT);
      Ptrx += strlen (OPERATOR_NOT);
      Dep_Flag = TRUE;

    } else if (*Ptrx == '\t') {

      printf ("File contains tabs. This violates the coding standard\n");
      return EFI_INVALID_PARAMETER;

    } else if (*Ptrx == '\n') {
      //
      // Skip the newline character in the file
      //
      Ptrx++;

    } else if (strncmp (Ptrx, OPERATOR_LEFT_PARENTHESIS, strlen (OPERATOR_LEFT_PARENTHESIS)) == 0) {
      PushOpCode ((VOID **) &StackPtr, DXE_DEP_LEFT_PARENTHESIS);

      Ptrx += strlen (OPERATOR_LEFT_PARENTHESIS);
      Dep_Flag = TRUE;

    } else if (strncmp (Ptrx, OPERATOR_RIGHT_PARENTHESIS, strlen (OPERATOR_RIGHT_PARENTHESIS)) == 0) {
      while (StackPtr != EvaluationStack) {
        Opcode = PopOpCode ((VOID **) &StackPtr);
        if (Opcode != DXE_DEP_LEFT_PARENTHESIS) {
          fputc (Opcode, OutFile);
          OutFileSize++;
        } else {
          break;
        }
      }

      Ptrx += strlen (OPERATOR_RIGHT_PARENTHESIS);
      Dep_Flag = TRUE;

    } else if (strncmp (Ptrx, OPERATOR_TRUE, strlen (OPERATOR_TRUE)) == 0) {

      fputc (EFI_DEP_TRUE, OutFile);

      OutFileSize++;

      //
      // OutFileSize += sizeof (EFI_DEP_TRUE);
      //
      Dep_Flag = TRUE;

      Ptrx += strlen (OPERATOR_TRUE);

    } else if (strncmp (Ptrx, OPERATOR_FALSE, strlen (OPERATOR_FALSE)) == 0) {

      fputc (EFI_DEP_FALSE, OutFile);

      OutFileSize++;

      //
      // OutFileSize += sizeof (EFI_DEP_FALSE);
      //
      Dep_Flag = TRUE;

      Ptrx += strlen (OPERATOR_FALSE);

    } else if (*Ptrx == '{') {
      Ptrx++;

      if (*Ptrx == ' ') {
        Ptrx++;
      }

      ArgCountParsed = sscanf (
                        Ptrx,
                        "%x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x",
                        &Guid.Data1,
                        &Guid.Data2,
                        &Guid.Data3,
                        &Guid.Data4[0],
                        &Guid.Data4[1],
                        &Guid.Data4[2],
                        &Guid.Data4[3],
                        &Guid.Data4[4],
                        &Guid.Data4[5],
                        &Guid.Data4[6],
                        &Guid.Data4[7]
                        );

      if (ArgCountParsed != 11) {
        printf ("We have found an illegal GUID\n");
        printf ("Fix your depex\n");
        exit (-1);
      }

      while (*Ptrx != '}') {
        Ptrx++;
      }
      //
      // Absorb the closing }
      //
      Ptrx++;

      //
      // Don't provide a PUSH Opcode for the Before and After case
      //
      if ((!Before_Flag) && (!After_Flag)) {
        fputc (EFI_DEP_PUSH, OutFile);
        OutFileSize++;
      }

      fwrite (&Guid, sizeof (EFI_GUID), 1, OutFile);

      OutFileSize += sizeof (EFI_GUID);
      Dep_Flag = TRUE;

    } else if (strncmp (Ptrx, DEPENDENCY_END, strlen (DEPENDENCY_END)) == 0) {
      NotDone = FALSE;
    } else {
      //
      // Not a valid construct. Null terminate somewhere out there and
      // print an error message.
      //
      *(Ptrx + 20) = 0;
      printf (UTILITY_NAME" ERROR: Unrecognized input at: \"%s\"...\n", Ptrx);
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  //  DRAIN();
  //
  while (StackPtr != EvaluationStack) {
    fputc (PopOpCode ((VOID **) &StackPtr), OutFile);
    OutFileSize++;
  }

  if (OutFileSize == 0) {
    printf ("Grammer contains no operators or constants\n");
    return EFI_INVALID_PARAMETER;
  }

  fputc (EFI_DEP_END, OutFile);

  OutFileSize++;

  //
  //  Checks for invalid padding values
  //
  if (Padding < 0) {

    printf ("The inputted padding value was %d\n", Padding);
    printf ("The optional padding value can not be less than ZERO\n");
    return EFI_INVALID_PARAMETER;

  } else if (Padding > 0) {

    while ((OutFileSize % Padding) != 0) {

      fputc (' ', OutFile);
      OutFileSize++;
    }
  }

  Results = (UINTN) fclose (InFile);
  if (Results != 0) {
    printf ("FCLOSE failed\n");
  }

  Results = (UINTN) fclose (OutFile);
  if (Results != 0) {
    printf ("FCLOSE failed\n");
  }

  free (Buffer);
  free (EvaluationStack);

  return EFI_SUCCESS;
} // End GenerateDependencyExpression function

EFI_STATUS
main (
  IN UINTN argc,
  IN CHAR8 *argv[]
  )
/*++

Routine Description:

  Parse user entries.  Print some rudimentary help

Arguments:

  argc    The count of input arguments
  argv    The input arguments string array

Returns:

  EFI_SUCCESS             The function completed successfully.
  EFI_INVALID_PARAMETER   One of the input parameters was invalid or one of the parameters in the text file was invalid.
  EFI_OUT_OF_RESOURCES    Unable to allocate memory.
  EFI_ABORTED             Unable to open/create a file or a misc error.

--*/
// TODO:    ] - add argument and description to function comment
{
  FILE    *OutFile;
  FILE    *InFile;
  UINT8   Padding;
  UINTN   Index;
  BOOLEAN Input_Flag;
  BOOLEAN Output_Flag;
  BOOLEAN Pad_Flag;

  InFile      = NULL;
  OutFile     = NULL;
  Padding     = 0;
  Input_Flag  = FALSE;
  Output_Flag = FALSE;
  Pad_Flag    = FALSE;

  //
  //  Output the calling arguments
  //
  //printf ("\n\n");
  //for (Index = 0; Index < argc; Index++) {
  //  printf ("%s ", argv[Index]);
  //}
  //
  //printf ("\n\n");

  if (argc < 5) {
    printf ("Not enough arguments\n");
    PrintGenDepexUsageInfo ();
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 1; Index < argc - 1; Index++) {

    if ((strcmp (argv[Index], "-I") == 0) || (strcmp (argv[Index], "-i") == 0)) {

      if (!Input_Flag) {

        InFile      = fopen (argv[Index + 1], "rb");
        Input_Flag  = TRUE;

      } else {
        printf ("GenDepex only allows one INPUT (-I) argument\n");
        return EFI_INVALID_PARAMETER;
      }

    } else if ((strcmp (argv[Index], "-O") == 0) || (strcmp (argv[Index], "-o") == 0)) {

      if (!Output_Flag) {

        OutFile     = fopen (argv[Index + 1], "wb");
        Output_Flag = TRUE;

      } else {
        printf ("GenDepex only allows one OUTPUT (-O) argument\n");
        return EFI_INVALID_PARAMETER;
      }

    } else if ((strcmp (argv[Index], "-P") == 0) || (strcmp (argv[Index], "-p") == 0)) {

      if (!Pad_Flag) {

        Padding   = (UINT8) atoi (argv[Index + 1]);
        Pad_Flag  = TRUE;

      } else {
        printf ("GenDepex only allows one PADDING (-P) argument\n");
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  PrintGenDepexUtilityInfo ();

  if (InFile == NULL) {
    printf ("Can not open <INFILE> for reading.\n");
    PrintGenDepexUsageInfo ();
    return EFI_ABORTED;
  }

  if (OutFile == NULL) {
    printf ("Can not open <OUTFILE> for writting.\n");
    PrintGenDepexUsageInfo ();
    return EFI_ABORTED;
  }

  return GenerateDependencyExpression (InFile, OutFile, Padding);
}
