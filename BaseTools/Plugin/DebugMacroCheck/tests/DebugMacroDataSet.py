# @file DebugMacroDataSet.py
#
# Contains a debug macro test data set for verifying debug macros are
# recognized and parsed properly.
#
# This data is automatically converted into test cases. Just add the new
# data object here and run the tests.
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

from .MacroTest import (NoSpecifierNoArgumentMacroTest,
                        EqualSpecifierEqualArgumentMacroTest,
                        MoreSpecifiersThanArgumentsMacroTest,
                        LessSpecifiersThanArgumentsMacroTest,
                        IgnoredSpecifiersMacroTest,
                        SpecialParsingMacroTest,
                        CodeSnippetMacroTest)


# Ignore flake8 linter errors for lines that are too long (E501)
# flake8: noqa: E501

# Data Set of DEBUG macros and expected results.
# macro: A string representing a DEBUG macro.
# result: A tuple with the following value representations.
#         [0]: Count of total formatting errors
#         [1]: Count of print specifiers found
#         [2]: Count of macro arguments found
DEBUG_MACROS = [
    #####################################################################
    # Section: No Print Specifiers No Arguments
    #####################################################################
    NoSpecifierNoArgumentMacroTest(
        r'',
        (0, 0, 0)
    ),
    NoSpecifierNoArgumentMacroTest(
        r'DEBUG ((DEBUG_ERROR, "\\"));',
        (0, 0, 0)
    ),
    NoSpecifierNoArgumentMacroTest(
        r'DEBUG ((DEBUG_EVENT, ""));',
        (0, 0, 0)
    ),
    NoSpecifierNoArgumentMacroTest(
        r'DEBUG ((DEBUG_EVENT, "\n"));',
        (0, 0, 0)
    ),
    NoSpecifierNoArgumentMacroTest(
        r'DEBUG ((DEBUG_EVENT, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"));',
        (0, 0, 0)
    ),
    NoSpecifierNoArgumentMacroTest(
        r'DEBUG ((DEBUG_EVENT, "GCD:Initial GCD Memory Space Map\n"));',
        (0, 0, 0)
    ),
    NoSpecifierNoArgumentMacroTest(
        r'DEBUG ((DEBUG_GCD, "GCD:Initial GCD Memory Space Map\n"));',
        (0, 0, 0)
    ),
    NoSpecifierNoArgumentMacroTest(
        r'DEBUG ((DEBUG_INFO, "   Retuning TimerCnt Disabled\n"));',
        (0, 0, 0)
    ),

    #####################################################################
    # Section: Equal Print Specifiers to Arguments
    #####################################################################
    EqualSpecifierEqualArgumentMacroTest(
        r'DEBUG ((DEBUG_INFO, "%d", Number));',
        (0, 1, 1)
    ),
    EqualSpecifierEqualArgumentMacroTest(
        r'DEBUG ((DEBUG_BLKIO, "NorFlashBlockIoReset(MediaId=0x%x)\n", This->Media->MediaId));',
        (0, 1, 1)
    ),
    EqualSpecifierEqualArgumentMacroTest(
        r'DEBUG ((DEBUG_INFO, "   Retuning TimerCnt %dseconds\n", 2 * (Capability->TimerCount - 1)));',
        (0, 1, 1)
    ),
    EqualSpecifierEqualArgumentMacroTest(
        r'DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to reset port %d - %r\n", Port, Status));',
        (0, 2, 2)
    ),
    EqualSpecifierEqualArgumentMacroTest(
        r'DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to reset port %d - %r\n", Port, Status));',
        (0, 2, 2)
    ),
    EqualSpecifierEqualArgumentMacroTest(
        r'DEBUG ((DEBUG_INFO, "Find GPT Partition [0x%lx", PartitionEntryBuffer[Index].StartingLBA));',
        (0, 1, 1)
    ),
    EqualSpecifierEqualArgumentMacroTest(
        r'DEBUG ((DEBUG_ERROR, "Failed to locate gEdkiiBootLogo2ProtocolGuid Status = %r.  No Progress bar support. \n", Status));',
        (0, 1, 1)
    ),
    EqualSpecifierEqualArgumentMacroTest(
        r'DEBUG ((DEBUG_LOAD, " (%s)", Image->ExitData));',
        (0, 1, 1)
    ),
    EqualSpecifierEqualArgumentMacroTest(
        r'DEBUG ((DEBUG_DISPATCH, "%a%r%s%lx%p%c%g", Ascii, Status, Unicode, Hex, Pointer, Character, Guid));',
        (0, 7, 7)
    ),
    EqualSpecifierEqualArgumentMacroTest(
        r'DEBUG ((DEBUG_INFO, "LoadCapsuleOnDisk - LoadRecoveryCapsule (%d) - %r\n", CapsuleInstance, Status));',
        (0, 2, 2)
    ),
    EqualSpecifierEqualArgumentMacroTest(
        r'DEBUG ((DEBUG_DISPATCH, "%a%r%s%lx%p%c%g%a%r%s%lx%p%c%g%a%r%s%lx%p%c%g%a%r%s%lx%p%c%g", Ascii, Status, Unicode, Hex, Pointer, Character, Guid, Ascii, Status, Unicode, Hex, Pointer, Character, Guid, Ascii, Status, Unicode, Hex, Pointer, Character, Guid, Ascii, Status, Unicode, Hex, Pointer, Character, Guid));',
        (0, 28, 28)
    ),

    #####################################################################
    # Section: More Print Specifiers Than Arguments
    #####################################################################
    MoreSpecifiersThanArgumentsMacroTest(
        r'DEBUG ((DEBUG_BLKIO, "NorFlashBlockIoReadBlocks(MediaId=0x%x, Lba=%ld, BufferSize=0x%x bytes (%d kB), BufferPtr @ 0x%08x)\n", MediaId, Lba, BufferSizeInBytes, Buffer));',
        (1, 5, 4)
    ),
    MoreSpecifiersThanArgumentsMacroTest(
        r'DEBUG ((DEBUG_INFO, "%a: Request=%s\n", __func__));',
        (1, 2, 1)
    ),
    MoreSpecifiersThanArgumentsMacroTest(
        r'DEBUG ((DEBUG_ERROR, "%a: Invalid request format %d for %d\n", CertFormat, CertRequest));',
        (1, 3, 2)
    ),

    #####################################################################
    # Section: Less Print Specifiers Than Arguments
    #####################################################################
    LessSpecifiersThanArgumentsMacroTest(
        r'DEBUG ((DEBUG_INFO, "Find GPT Partition [0x%lx", PartitionEntryBuffer[Index].StartingLBA, BlockDevPtr->LastBlock));',
        (1, 1, 2)
    ),
    LessSpecifiersThanArgumentsMacroTest(
        r'DEBUG ((DEBUG_INFO, "   Retuning TimerCnt Disabled\n", 2 * (Capability->TimerCount - 1)));',
        (1, 0, 1)
    ),
    LessSpecifiersThanArgumentsMacroTest(
        r'DEBUG ((DEBUG_ERROR, "Failed to locate gEdkiiBootLogo2ProtocolGuid.  No Progress bar support. \n", Status));',
        (1, 0, 1)
    ),
    LessSpecifiersThanArgumentsMacroTest(
        r'DEBUG ((DEBUG_ERROR, "UsbEnumeratePort: Critical Over Current\n", Port));',
        (1, 0, 1)
    ),
    LessSpecifiersThanArgumentsMacroTest(
        r'DEBUG ((DEBUG_ERROR, "[TPM2] Submit PP Request failure! Sync PPRQ/PPRM with PP variable.\n", Status));',
        (1, 0, 1)
    ),
    LessSpecifiersThanArgumentsMacroTest(
        r'DEBUG ((DEBUG_ERROR, ": Failed to update debug log index file: %r !\n", __func__, Status));',
        (1, 1, 2)
    ),
    LessSpecifiersThanArgumentsMacroTest(
        r'DEBUG ((DEBUG_ERROR, "%a - Failed to extract nonce from policy blob with return status %r\n", __func__, gPolicyBlobFieldName[MFCI_POLICY_TARGET_NONCE], Status));',
        (1, 2, 3)
    ),

    #####################################################################
    # Section: Macros with Ignored Specifiers
    #####################################################################
    IgnoredSpecifiersMacroTest(
        r'DEBUG ((DEBUG_INIT, "%HEmuOpenBlock: opened %a%N\n", Private->Filename));',
        (0, 1, 1)
    ),
    IgnoredSpecifiersMacroTest(
        r'DEBUG ((DEBUG_LOAD, " (%hs)", Image->ExitData));',
        (0, 1, 1)
    ),
    IgnoredSpecifiersMacroTest(
        r'DEBUG ((DEBUG_LOAD, "%H%s%N: Unknown flag - ''%H%s%N''\r\n", String1, String2));',
        (0, 2, 2)
    ),

    #####################################################################
    # Section: Macros with Special Parsing Scenarios
    #####################################################################
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_INFO, " File Name: %a\n", "Document.txt"))',
        (0, 1, 1),
        "Malformatted Macro - Missing Semicolon"
    ),
    SpecialParsingMacroTest(
        r'DEBUG (DEBUG_INFO, " File Name: %a\n", "Document.txt");',
        (0, 0, 0),
        "Malformatted Macro - Missing Two Parentheses"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_INFO, "%a\n", "Removable Slot"));',
        (0, 1, 1),
        "Single String Argument in Quotes"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_INFO, "   SDR50 Tuning      %a\n", Capability->TuningSDR50 ? "TRUE" : "FALSE"));',
        (0, 1, 1),
        "Ternary Operator Present"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_INFO, "   SDR50 Tuning      %a\n", Capability->TuningSDR50 ? "TRUE" : "FALSE"));',
        (0, 1, 1),
        "Ternary Operator Present"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((DEBUG_ERROR, "\\"));
        DEBUG ((DEBUG_ERROR, "\\"));
        DEBUG ((DEBUG_ERROR, "\\"));
        DEBUG ((DEBUG_ERROR, "\\"));
        ''',
        (0, 0, 0),
        "Multiple Macros with an Escaped Character"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_INFO,
          "UsbEnumerateNewDev: device uses translator (%d, %d)\n",
          Child->Translator.TranslatorHubAddress,
          Child->Translator.TranslatorPortNumber
          ));
        ''',
        (0, 2, 2),
        "Multi-line Macro"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_INFO,
          "UsbEnumeratePort: port %d state - %02x, change - %02x on %p\n",
          Port,
          PortState.PortStatus,
          PortState.PortChangeStatus,
          HubIf
          ));
        ''',
        (0, 4, 4),
        "Multi-line Macro"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_ERROR,
          "%a:%a: failed to allocate reserved pages: "
          "BufferSize=%Lu LoadFile=\"%s\" FilePath=\"%s\"\n",
          gEfiCallerBaseName,
          __func__,
          (UINT64)BufferSize,
          LoadFileText,
          FileText
          ));
        ''',
        (0, 5, 5),
        "Multi-line Macro with Compiler String Concatenation"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: GTDT: GT Block Frame Info Structures %d and %d have the same " \
          "frame number: 0x%x.\n",
          Index1,
          Index2,
          FrameNumber1
          ));
        ''',
        (0, 3, 3),
        "Multi-line Macro with Backslash String Concatenation"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: PPTT: Too many private resources. Count = %d. " \
          "Maximum supported Processor Node size exceeded. " \
          "Token = %p. Status = %r\n",
          ProcInfoNode->NoOfPrivateResources,
          ProcInfoNode->ParentToken,
          Status
          ));
        ''',
        (0, 3, 3),
        "Multi-line Macro with Backslash String Concatenation"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_VERBOSE,
          "% 20a % 20a % 20a % 20a\n",
          "PhysicalStart(0x)",
          "PhysicalSize(0x)",
          "CpuStart(0x)",
          "RegionState(0x)"
          ));
        ''',
        (0, 4, 4),
        "Multi-line Macro with Quoted String Arguments"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_ERROR,
          "XenPvBlk: "
          "%a error %d on %a at sector %Lx, num bytes %Lx\n",
          Response->operation == BLKIF_OP_READ ? "read" : "write",
          Status,
          IoData->Dev->NodeName,
          (UINT64)IoData->Sector,
          (UINT64)IoData->Size
          ));
        ''',
        (0, 5, 5),
        "Multi-line Macro with Ternary Operator and Quoted String Arguments"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_ERROR,
          "%a: Label=\"%s\" OldParentNodeId=%Lu OldName=\"%a\" "
          "NewParentNodeId=%Lu NewName=\"%a\" Errno=%d\n",
          __func__,
          VirtioFs->Label,
          OldParentNodeId,
          OldName,
          NewParentNodeId,
          NewName,
          CommonResp.Error
          ));
        ''',
        (0, 7, 7),
        "Multi-line Macro with Escaped Quotes and String Concatenation"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((DEBUG_WARN, "Failed to retrieve Variable:\"MebxData\", Status = %r\n", Status));
        ''',
        (0, 1, 1),
        "Escaped Parentheses in Debug Message"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG((DEBUG_INFO, "%0d %s", XbB_ddr4[1][bankBit][xorBit], xorBit == (XaB_NUM_OF_BITS-1) ? "]": ", "));
        ''',
        (0, 2, 2),
        "Parentheses in Ternary Operator Expression"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_INFO | DEBUG_EVENT | DEBUG_WARN, "   %u\n", &Structure->Block.Value));',
        (0, 1, 1),
        "Multiple Print Specifier Levels Present"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, "   %s\n", ReturnString()));',
        (0, 1, 1),
        "Function Call Argument No Params"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, "   %s\n", ReturnString(&Param1)));',
        (0, 1, 1),
        "Function Call Argument 1 Param"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, "   %s\n", ReturnString(&Param1, Param2)));',
        (0, 1, 1),
        "Function Call Argument Multiple Params"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, "   %s\n", ReturnString(&Param1, ReturnParam())));',
        (0, 1, 1),
        "Function Call Argument 2-Level Depth No 2nd-Level Param"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, "   %s\n", ReturnString(&Param1, ReturnParam(*Param))));',
        (0, 1, 1),
        "Function Call Argument 2-Level Depth 1 2nd-Level Param"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, "   %s\n", ReturnString(&Param1, ReturnParam(*Param, &ParamNext))));',
        (0, 1, 1),
        "Function Call Argument 2-Level Depth Multiple 2nd-Level Param"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, "   %s\n", ReturnString(&Param1, ReturnParam(*Param, GetParam(1, 2, 3)))));',
        (0, 1, 1),
        "Function Call Argument 3-Level Depth Multiple Params"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, "   %s\n", ReturnString(&Param1, ReturnParam(*Param, GetParam(1, 2, 3), NextParam))));',
        (0, 1, 1),
        "Function Call Argument 3-Level Depth Multiple Params with Param After Function Call"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, "   %s-%a\n", ReturnString(&Param1), ReturnString2(&ParamN)));',
        (0, 2, 2),
        "Multiple Function Call Arguments"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, "   %s\n", ReturnString(&Param1), ReturnString2(&ParamN)));',
        (1, 1, 2),
        "Multiple Function Call Arguments with Imbalance"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, "   %s%s\n", (ReturnString(&Param1)), (ReturnString2(&ParamN))));',
        (0, 2, 2),
        "Multiple Function Call Arguments Surrounded with Parentheses"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, " %s\n", ((((ReturnString(&Param1)))))));',
        (0, 1, 1),
        "Multiple Function Call Arguments Surrounded with Many Parentheses"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, ""%B%08X%N: %-48a %V*%a*%N"", HexNumber, ReturnString(Array[Index]), &AsciiString[0]));',
        (0, 3, 3),
        "Complex String Print Specifier 1"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, "0x%-8x:%H%s%N % -64s(%73-.73s){%g}<%H% -70s%N>\n.   Size: 0x%-16x (%-,d) bytes.\n\n", HexNumber, GetUnicodeString (), &UnicodeString[4], UnicodeString2, &Guid, AnotherUnicodeString, Struct.SomeSize, CommaDecimalValue));',
        (0, 8, 8),
        "Multiple Complex Print Specifiers 1"
    ),
    SpecialParsingMacroTest(
        r'DEBUG ((DEBUG_WARN, "0x%-8x:%H%s%N % -64s(%73-.73s){%g}<%H% -70s%N%r>\n.   Size: 0x%-16x (%-,d) bytes.\n\n", HexNumber, GetUnicodeString (), &UnicodeString[4], UnicodeString2, &Guid, AnotherUnicodeString, Struct.SomeSize, CommaDecimalValue));',
        (1, 9, 8),
        "Multiple Complex Print Specifiers Imbalance 1"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_ERROR,
          ("%a: Label=\"%s\" CanonicalPathname=\"%a\" FileName=\"%s\" "
           "OpenMode=0x%Lx Attributes=0x%Lx: nonsensical request to possibly "
           "create a file marked read-only, for read-write access\n"),
          __func__,
          VirtioFs->Label,
          VirtioFsFile->CanonicalPathname,
          FileName,
          OpenMode,
          Attributes
          ));
        ''',
        (0, 6, 6),
        "Multi-Line with Parentheses Around Debug String Compiler String Concat"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG (
          (DEBUG_INFO,
          " %02x: %04x %02x/%02x/%02x %02x/%02x %04x %04x %04x:%04x\n",
          (UINTN)Index,
          (UINTN)LocalBbsTable[Index].BootPriority,
          (UINTN)LocalBbsTable[Index].Bus,
          (UINTN)LocalBbsTable[Index].Device,
          (UINTN)LocalBbsTable[Index].Function,
          (UINTN)LocalBbsTable[Index].Class,
          (UINTN)LocalBbsTable[Index].SubClass,
          (UINTN)LocalBbsTable[Index].DeviceType,
          (UINTN)*(UINT16 *)&LocalBbsTable[Index].StatusFlags,
          (UINTN)LocalBbsTable[Index].BootHandlerSegment,
          (UINTN)LocalBbsTable[Index].BootHandlerOffset,
          (UINTN)((LocalBbsTable[Index].MfgStringSegment << 4) + LocalBbsTable[Index].MfgStringOffset),
          (UINTN)((LocalBbsTable[Index].DescStringSegment << 4) + LocalBbsTable[Index].DescStringOffset))
          );
        ''',
        (1, 11, 13),
        "Multi-line Macro with Many Arguments And Multi-Line Parentheses"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_WARN,
          "0x%-8x:%H%s%N % -64s(%73-.73s){%g}<%H% -70s%N>\n.   Size: 0x%-16x (%-,d) bytes.\n\n",
          HexNumber,
          GetUnicodeString (InnerFunctionCall(Arg1, &Arg2)),
          &UnicodeString[4],
          UnicodeString2,
          &Guid,
          AnotherUnicodeString,
          Struct.SomeSize,
          CommaDecimalValue
          ));
        ''',
        (0, 8, 8),
        "Multi-line Macro with Multiple Complex Print Specifiers 1 and 2-Depth Function Calls"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG (
          (DEBUG_NET,
          "TcpFastRecover: enter fast retransmission for TCB %p, recover point is %d\n",
          Tcb,
          Tcb->Recover)
          );
        ''',
        (0, 2, 2),
        "Multi-line Macro with Parentheses Separated"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_VERBOSE,
          "%a: APIC ID " FMT_APIC_ID " was hot-plugged "
                                     "before; ignoring it\n",
          __func__,
          NewApicId
          ));
        ''',
        (1, 1, 2),
        "Multi-line Imbalanced Macro with Indented String Concatenation"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_VERBOSE,
          "%a: APIC ID was hot-plugged - %a",
          __func__,
          "String with , inside"
          ));
        ''',
        (0, 2, 2),
        "Multi-line with Quoted String Argument Containing Comma"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_VERBOSE,
          "%a: APIC ID was hot-plugged - %a",
          __func__,
          "St,ring, with , ins,ide"
          ));
        ''',
        (0, 2, 2),
        "Multi-line with Quoted String Argument Containing Multiple Commas"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((DEBUG_VERBOSE, "%a: APIC ID was hot-plugged, \"%a\"", __func__, "S\"t,\"ring, with , ins,i\"de"));
        ''',
        (0, 2, 2),
        "Quoted String Argument with Escaped Quotes and Multiple Commas"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_ERROR,
          "%a: AddProcessor(" FMT_APIC_ID "): %r\n",
          __func__,
          Status
          ));
        ''',
        (0, 2, 2),
        "Quoted Parenthesized String Inside Debug Message String"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((
          DEBUG_INFO,
          "%a: hot-added APIC ID " FMT_APIC_ID ", SMBASE 0x%Lx, "
                                               "EFI_SMM_CPU_SERVICE_PROTOCOL assigned number %Lu\n",
          __func__,
          (UINT64)mCpuHotPlugData->SmBase[NewSlot],
          (UINT64)NewProcessorNumberByProtocol
          ));
        ''',
        (0, 3, 3),
        "Quoted String with Concatenation Inside Debug Message String"
    ),
    SpecialParsingMacroTest(
        r'''
        DEBUG ((DEBUG_INFO, Index == COLUMN_SIZE/2 ? "0" : " %02x", (UINTN)Data[Index]));
        ''',
        (0, 1, 1),
        "Ternary Operating in Debug Message String"
    ),

    #####################################################################
    # Section: Code Snippet Tests
    #####################################################################
    CodeSnippetMacroTest(
        r'''
        /**
        Print the BBS Table.

        @param LocalBbsTable   The BBS table.
        @param BbsCount        The count of entry in BBS table.
        **/
        VOID
        LegacyBmPrintBbsTable (
        IN BBS_TABLE  *LocalBbsTable,
        IN UINT16     BbsCount
        )
        {
        UINT16  Index;

        DEBUG ((DEBUG_INFO, "\n"));
        DEBUG ((DEBUG_INFO, " NO  Prio bb/dd/ff cl/sc Type Stat segm:offs\n"));
        DEBUG ((DEBUG_INFO, "=============================================\n"));
        for (Index = 0; Index < BbsCount; Index++) {
            if (!LegacyBmValidBbsEntry (&LocalBbsTable[Index])) {
            continue;
            }

            DEBUG (
              (DEBUG_INFO,
              " %02x: %04x %02x/%02x/%02x %02x/%02x %04x %04x %04x:%04x\n",
              (UINTN)Index,
              (UINTN)LocalBbsTable[Index].BootPriority,
              (UINTN)LocalBbsTable[Index].Bus,
              (UINTN)LocalBbsTable[Index].Device,
              (UINTN)LocalBbsTable[Index].Function,
              (UINTN)LocalBbsTable[Index].Class,
              (UINTN)LocalBbsTable[Index].SubClass,
              (UINTN)LocalBbsTable[Index].DeviceType,
              (UINTN)*(UINT16 *)&LocalBbsTable[Index].StatusFlags,
              (UINTN)LocalBbsTable[Index].BootHandlerSegment,
              (UINTN)LocalBbsTable[Index].BootHandlerOffset,
              (UINTN)((LocalBbsTable[Index].MfgStringSegment << 4) + LocalBbsTable[Index].MfgStringOffset),
              (UINTN)((LocalBbsTable[Index].DescStringSegment << 4) + LocalBbsTable[Index].DescStringOffset))
              );
        }

        DEBUG ((DEBUG_INFO, "\n"));
        ''',
        (1, 0, 0),
        "Code Section with An Imbalanced Macro"
    ),
    CodeSnippetMacroTest(
        r'''
        if (*Buffer == AML_ROOT_CHAR) {
            //
            // RootChar
            //
            Buffer++;
            DEBUG ((DEBUG_ERROR, "\\"));
        } else if (*Buffer == AML_PARENT_PREFIX_CHAR) {
            //
            // ParentPrefixChar
            //
            do {
            Buffer++;
            DEBUG ((DEBUG_ERROR, "^"));
            } while (*Buffer == AML_PARENT_PREFIX_CHAR);
        }
        DEBUG ((DEBUG_WARN, "Failed to retrieve Variable:\"MebxData\", Status = %r\n", Status));
        ''',
        (0, 1, 1),
        "Code Section with Escaped Backslash and Escaped Quotes"
    ),
    CodeSnippetMacroTest(
        r'''
        if (EFI_ERROR (Status)) {
          UINTN  Offset;
          UINTN  Start;

          DEBUG ((
            DEBUG_INFO,
            "Variable FV header is not valid. It will be reinitialized.\n"
            ));

          //
          // Get FvbInfo to provide in FwhInstance.
          //
          Status = GetFvbInfo (Length, &GoodFwVolHeader);
          ASSERT (!EFI_ERROR (Status));
        }
        ''',
        (0, 0, 0),
        "Code Section with Multi-Line Macro with No Arguments"
    )
]
