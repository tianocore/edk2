/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "Edb.h"

//
// Debugger Command Table
//
EFI_DEBUGGER_COMMAND_SET  mDebuggerCommandSet[] = {
  //
  // Execution
  //
  {
    L"G",
    L"G/[F5]             - continue to run the program\n",
    L"The go command is used to cause the debugger to not interrupt execution of the EBC image. The debugger will only break execution of the interpreter if an exception is encountered (including an EBC breakpoint).\n\n",
    L"G [til <Address|Symbol>]\n"
    L"  (No Argument) - It means continue run the program.\n"
    L"  til           - It means continuing run the program till IP is the Address.\n"
    L"  <Address>     - The hexical address user want to break at.\n"
    L"  <Symbol>      - The symbol name for target address user want to break at. It has following format [MapFileName:]SymbolName\n",
    L"Execution:\n",
    {SCAN_F5, CHAR_NULL},
    DebuggerGo
  },
  {
    L"T",
    L"T/[F8]             - step into\n",
    L"The step into command will cause the EBC debugger to step a single instruction. If the instruction is a call to internal code (CALL), then the debugger will break at the new function CALL.\n\n",
    L"T\n"
    L"  (No Argument)\n",
    L"",
    {SCAN_F8, CHAR_NULL},
    DebuggerStepInto
  },
  {
    L"P",
    L"P/[F10]            - step over\n",
    L"The step over command will cause the EBC debugger to step a single instruction. If the instruction is a call to internal code (CALL), then the external call will be made and the debugger will break at the instruction following the CALL.\n\n",
    L"P\n"
    L"  (No Argument)\n",
    L"",
    {SCAN_F10, CHAR_NULL},
    DebuggerStepOver
  },
  {
    L"O",
    L"O/[F11]            - step out\n",
    L"The step out command causes the EBC debugger to step out function calls. The function will be executed, but the debugger will stop after the called function returns.\n\n",
    L"O\n"
    L"  (No Argument)\n",
    L"",
    {SCAN_F11, CHAR_NULL},
    DebuggerStepOut
  },
  {
    L"Q",
    L"Q                  - reset the debugger to default value and go\n",
    L"The quit command will reset the debugger to default value and go.\n\n",
    L"Q\n"
    L"  (No Argument)\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerQuit
  },
  //
  // Break
  //
  {
    L"BOC",
    L"BO[C|CX|R|E|T|K]   - break on CALL/CALLEX/RET/Entrypoint/Native Thunk/Key\n",
    L"Enabling break-on-call will cause the debugger to halt execution and display the debugger prompt prior to executing any EBC CALL (to EBC) instructions.\n\n",
    L"BOC [on|off]\n"
    L"  (No Argument) - show current state\n"
    L"  on            - enable break-on-call\n"
    L"  off           - disable break-on-call\n",
    L"Break:\n",
    {SCAN_NULL, CHAR_NULL},
    DebuggerBreakOnCALL
  },
  {
    L"BOCX",
    L"",
    L"Enabling break-on-callex will cause the debugger to halt execution and display the debugger prompt prior to executing EBC CALLEX (thunk out) instructions.\n\n",
    L"BOCX [on|off]\n"
    L"  (No Argument) - show current state\n"
    L"  on            - enable break-on-callex\n"
    L"  off           - disable break-on-callex\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerBreakOnCALLEX
  },
  {
    L"BOR",
    L"",
    L"Enabling break-on-return will cause the debugger to halt execution and display the debugger prompt prior to executing EBC RET instructions.\n\n",
    L"BOR [on|off]\n"
    L"  (No Argument) - show current state\n"
    L"  on            - enable break-on-return\n"
    L"  off           - disable break-on-return\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerBreakOnRET
  },
  {
    L"BOE",
    L"",
    L"Enabling break-on-entrypoint will cause the debugger to halt execution and display the debugger prompt prior to start a driver entry point. (Default is on)\n\n",
    L"BOE [on|off]\n"
    L"  (No Argument) - show current state\n"
    L"  on            - enable break-on-entrypoint\n"
    L"  off           - disable break-on-entrypoint\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerBreakOnEntrypoint
  },
  {
    L"BOT",
    L"",
    L"Enabling break-on-thunk will cause the debugger to halt execution and display the debugger prompt prior to start native call EBC thunk. (Default is on)\n\n",
    L"BOT [on|off]\n"
    L"  (No Argument) - show current state\n"
    L"  on            - enable break-on-thunk\n"
    L"  off           - disable break-on-thunk\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerBreakOnThunk
  },
  {
    L"BOK",
    L"",
    L"Enabling break-on-key will cause the debugger to halt execution and display the debugger prompt after press any key.\n\n",
    L"BOK [on|off]\n"
    L"  (No Argument) - show current state\n"
    L"  on            - enable break-on-key\n"
    L"  off           - disable break-on-key\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerBreakOnKey
  },
  {
    L"BL",
    L"B[L|P|C|D|E]       - breakpoint list/set/clear/disable/enable\n",
    L"List Breakpoint\n\n",
    L"BL\n"
    L"  (No Argument) - show the state for current breakpoint\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerBreakpointList
  },
  {
    L"BP",
    L"",
    L"Set Breakpoint\n\n",
    L"BP <Address|Symbol>\n"
    L"  <Address> - Hexical breakpoint address\n"
    L"  <Symbol>  - Symbol name for breakpoint address. It has following format [MapFileName:]SymbolName.\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerBreakpointSet
  },
  {
    L"BC",
    L"",
    L"Clear Breakpoint\n\n",
    L"BC <Index>|*\n"
    L"  <Index>   - Decimal breakpoint index, which can be got from BL command\n"
    L"  *         - For all the breakpoint\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerBreakpointClear
  },
  {
    L"BD",
    L"",
    L"Disable Breakpoint\n\n",
    L"BD <Index>|*\n"
    L"  <Index>   - Decimal breakpoint index, which can be got from BL command\n"
    L"  *         - For all the breakpoint\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerBreakpointDisable
  },
  {
    L"BE",
    L"",
    L"Enable Breakpoint\n\n",
    L"BE <Index>|*\n"
    L"  <Index>   - Decimal breakpoint index, which can be got from BL command\n"
    L"  *         - For all the breakpoint\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerBreakpointEnable
  },
  //
  // Information
  //
  {
    L"K",
    L"K                  - show/clear call-stack\n",
    L"The call-stack command will show or clear the current call-stack.\n\n",
    L"K [p [<ParameterNum>]|c]\n"
    L"  (No Argument) - Show current call-stack\n"
    L"  p             - Show current call-stack with parameters\n"
    L"  ParameterNum  - Decimal call-stack parameters number, 8 by default, 16 as max\n"
    L"  c             - Clear current call-stack\n",
    L"Information:\n",
    {SCAN_NULL, CHAR_NULL},
    DebuggerCallStack
  },
  {
    L"TRACE",
    L"TRACE              - show/clear trace instruction branch\n",
    L"The trace command will show or clear the latest instruction branch.\n\n",
    L"TRACE [c]\n"
    L"  (No Argument) - Show current instrcution branch\n"
    L"  c             - Clear current instruction branch\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerInstructionBranch
  },
  {
    L"R",
    L"R/[F2]             - display/modify register\n",
    L"The register command is used to display or modify the contents of EBC VM registers. (R0~R7, Flags, IP)\n\n",
    L"R [<Register> <Value>]\n"
    L"  (No Argument) - Display all registers\n"
    L"  <Register>    - EBC VM register name (R0~R7, Flags, ControlFlags, and IP\n"
    L"  <Value>       - The Hexical value of register\n",
    L"",
    {SCAN_F2, CHAR_NULL},
    DebuggerRegister
  },
  {
    L"L",
    L"L/[F4]             - show/load instruction assembly count\n",
    L"The list assembly command will disassemble instructions starting with the current EBC VM instruction pointer. (by default 5 instructions)\n\n",
    L"L [<Count>]\n"
    L"  (No Argument) - List current assembly code\n"
    L"  Count         - The decimal instruction assembly count\n",
    L"",
    {SCAN_F4, CHAR_NULL},
    DebuggerList
  },
  {
    L"SCOPE",
    L"SCOPE              - load scope address\n",
    L"The scope command will disassemble instructions starting with the Scope. (by default current EBC VM IP)\n\n",
    L"SCOPE <Address|Symbol>\n"
    L"  <Address> - The Hexical address where user wants to see the assembly code\n"
    L"  <Symbol>  - Symbol name for scope address. It has following format [MapFileName:]SymbolName.\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerScope
  },
  {
    L"DB",
    L"[D|E][B|W|D|Q]     - display/modify memory\n",
    L"Display BYTES Memory\n\n",
    L"DB <Address|Symbol> [<Count>]\n"
    L"  <Address> - The hexical memory address\n"
    L"  <Symbol>  - Symbol name for memory address. It has following format [MapFileName:]SymbolName.\n"
    L"  <Count>   - The hexical memory count (not set means 1)\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerMemoryDB
  },
  {
    L"DW",
    L"",
    L"Display WORDS Memory\n\n",
    L"DW <Address|Symbol> [<Count>]\n"
    L"  <Address> - The hexical memory address\n"
    L"  <Symbol>  - Symbol name for memory address. It has following format [MapFileName:]SymbolName.\n"
    L"  <Count>   - The hexical memory count (not set means 1)\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerMemoryDW
  },
  {
    L"DD",
    L"",
    L"Display DWORDS Memory\n\n",
    L"DD <Address|Symbol> [<Count>]\n"
    L"  <Address> - The hexical memory address\n"
    L"  <Symbol>  - Symbol name for memory address. It has following format [MapFileName:]SymbolName.\n"
    L"  <Count>   - The hexical memory count (not set means 1)\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerMemoryDD
  },
  {
    L"DQ",
    L"",
    L"Display QWORDS Memory\n\n",
    L"DQ <Address|Symbol> [<Count>]\n"
    L"  <Address> - The hexical memory address\n"
    L"  <Symbol>  - Symbol name for memory address. It has following format [MapFileName:]SymbolName.\n"
    L"  <Count>   - The hexical memory count (not set means 1)\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerMemoryDQ
  },
  {
    L"EB",
    L"",
    L"Enter BYTES Memory\n\n",
    L"EB <Address|Symbol> <Value>\n"
    L"  <Address> - The hexical memory address\n"
    L"  <Symbol>  - Symbol name for memory address. It has following format [MapFileName:]SymbolName.\n"
    L"  <Value>   - The hexical memory value\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerMemoryEB
  },
  {
    L"EW",
    L"",
    L"Enter WORDS Memory\n\n",
    L"EW <Address|Symbol> <Value>\n"
    L"  <Address> - The hexical memory address\n"
    L"  <Symbol>  - Symbol name for memory address. It has following format [MapFileName:]SymbolName.\n"
    L"  <Value>   - The hexical memory value\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerMemoryEW
  },
  {
    L"ED",
    L"",
    L"Enter DWORDS Memory\n\n",
    L"ED <Address|Symbol> <Value>\n"
    L"  <Address> - The hexical memory address\n"
    L"  <Symbol>  - Symbol name for memory address. It has following format [MapFileName:]SymbolName.\n"
    L"  <Value>   - The hexical memory value\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerMemoryED
  },
  {
    L"EQ",
    L"",
    L"Enter QWORDS Memory\n\n",
    L"EQ <Address|Symbol> <Value>\n"
    L"  <Address> - The hexical memory address\n"
    L"  <Symbol>  - Symbol name for memory address. It has following format [MapFileName:]SymbolName.\n"
    L"  <Value>   - The hexical memory value\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerMemoryEQ
  },
  //
  // Symbol
  //
  {
    L"LN",
    L"LN                 - list the symbol\n",
    L"The show symbol command will list all the current symbol. It can list the symbol in one symbol file, or list the same symbol in all the files. It can also list the symbol according to nearest address.\n\n",
    L"LN [[F <SymbolFile>] [S <Symbol>]] | <Address>\n"
    L"  (No Argument)  - List all the symbol\n"
    L"  F <SymbolFile> - List the symbol in this symbol file only\n"
    L"  S <Symbol>     - List this symbol only\n"
    L"  <Address>      - The hexical memory address, which user want to find the symbol for.\n",
    L"Symbol:\n",
    {SCAN_NULL, CHAR_NULL},
    DebuggerListSymbol
  },
  {
    L"LOADSYMBOL",
    L"[UN]LOADSYMBOL     - load/unload the symbol file\n",
    L"The load symbol command will load the ebc map file. Then it parses the function name and global variable, and the print real name when do the disassembly. (Symbol file name should be XXX.MAP)\n\n",
    L"LOADSYMBOL <SymbolFile> [a]\n"
    L"  SymbolFile - The EBC symbol file (Its name should be XXX.MAP)\n"
    L"  a          - Automatically load code files in the same dir\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerLoadSymbol
  },
  {
    L"UNLOADSYMBOL",
    L"",
    L"The unload symbol command will unload the ebc map and cod file. After that the name will not be print.\n\n",
    L"UNLOADSYMBOL <SymbolFile>\n"
    L"  SymbolFile - The EBC symbol file (Its name should be XXX.MAP)\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerUnloadSymbol
  },
  {
    L"LOADCODE",
    L"[UN]LOADCODE       - load/unload the code file\n",
    L"The load code command will load the ebc cod file. Then it parses the cod file, and the print source code when do the disassembly. (Code file name should be XXX.COD)\n\n",
    L"LOADCODE <CodeFile> <SymbolFile>\n"
    L"  CodeFile   - The EBC code file (Its name should be XXX.COD)\n"
    L"  SymbolFile - The EBC symbol file (Its name should be XXX.MAP)\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerLoadCode
  },
  {
    L"UNLOADCODE",
    L"",
    L"The unload code command will unload the ebc cod file. After that the source code will not be print.\n\n",
    L"UNLOADCODE <CodeFile> <SymbolFile>\n"
    L"  CodeFile   - The EBC code file (Its name should be XXX.COD)\n"
    L"  SymbolFile - The EBC symbol file (Its name should be XXX.MAP)\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerUnloadCode
  },
  {
    L"DISPLAYSYMBOL",
    L"DISPLAYSYMBOL/[F3] - disable/enable the symbol output\n",
    L"",
    L"The display symbol command will configure the symbol show or not-show when disassembly.\n\n"
    L"DISPLAYSYMBOL [on|off]\n"
    L"  (No Argument) - swtich symbol output state to another one\n"
    L"  on            - enable symbol output\n"
    L"  off           - disable symbol output\n",
    L"",
    {SCAN_F3, CHAR_NULL},
    DebuggerDisplaySymbol
  },
  {
    L"DISPLAYCODE",
    L"DISPLAYCODE/[F6]   - disable/enable the source code only output\n",
    L"",
    L"The display code command will configure the source code only show or misc source code with assembly.\n\n"
    L"DISPLAYCODE [on|off]\n"
    L"  (No Argument) - swtich source only output state to another one\n"
    L"  on            - enable source only output\n"
    L"  off           - disable source only output\n",
    L"",
    {SCAN_F6, CHAR_NULL},
    DebuggerDisplayCode
  },
  //
  // Other
  //
  {
    L"H",
    L"",
    L"The help command will print help information for each command\n\n",
    L"H [<Command>]\n",
    L"",
    {SCAN_F1, CHAR_NULL},
    DebuggerHelp
  },
/*
  //
  // Extended
  //
  {
    L"!IB",
    L"![I|O][B|W|D]      - display/modify IO\n",
    L"",
    L"!IB <Address>\n",
    L"Extended:\n",
    {SCAN_NULL, CHAR_NULL},
    DebuggerExtIoIB
  },
  {
    L"!IW",
    L"",
    L"",
    L"!IW <Address>\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerExtIoIW
  },
  {
    L"!ID",
    L"",
    L"",
    L"!ID <Address>\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerExtIoID
  },
  {
    L"!OB",
    L"",
    L"",
    L"!OB <Address> <Value>\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerExtIoOB
  },
  {
    L"!OW",
    L"",
    L"",
    L"!OW <Address> <Value>\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerExtIoOW
  },
  {
    L"!OD",
    L"",
    L"",
    L"!OD <Address> <Value>\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerExtIoOD
  },
  {
    L"!PCIL",
    L"!PCIL              - list PCI device, with BAR\n",
    L"",
    L"!PCIL [B]\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerExtPciPCIL
  },
  {
    L"!PCID",
    L"!PCID              - show PCI space\n",
    L"",
    L"!PCID Bus Device Function [H|B|E]\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerExtPciPCID
  },
  {
    L"!CFGB",
    L"!CFG[B|W|D]        - show/modify PCI space",
    L"",
    L"!CFGB <Address> [<Value>]\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerExtPciCFGB
  },
  {
    L"!CFGW",
    L"",
    L"",
    L"!CFGW <Address> [<Value>]\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerExtPciCFGW
  },
  {
    L"!CFGD",
    L"",
    L"",
    L"!CFGD <Address> [<Value>]\n",
    L"",
    {SCAN_NULL, CHAR_NULL},
    DebuggerExtPciCFGD
  },
*/
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    {SCAN_NULL, CHAR_NULL},
    NULL
  },
};

/**

  Find the command according to name.

  @param  CommandName   - Command Name
  @param  CommandArg    - Command Argument

  @return Not NULL        - The DebuggerCommand is found successfully
  @return NULL            - not found

**/
EFI_DEBUGGER_COMMAND
MatchDebuggerCommand (
  IN CHAR16    *CommandName,
  IN CHAR16    **CommandArg
  )
{
  UINTN  Index;
  CHAR16 *Temp;

  //
  // Get Command Name
  //
  Temp = StrGetNewTokenLine (CommandName, L" ");
  CommandName = Temp;
  //
  // Get Command Argument
  //
  Temp = StrGetNextTokenLine (L" ");
  *CommandArg = Temp;

  if (CommandName == NULL) {
    return NULL;
  }

  //
  // Go through each command, check the CommandName
  //
  for (Index = 0; mDebuggerCommandSet[Index].CommandName != NULL; Index++) {
    if (StriCmp (CommandName, mDebuggerCommandSet[Index].CommandName) == 0) {
      //
      // Found
      //
      return mDebuggerCommandSet[Index].CommandFunc;
    }
  }

  //
  // Not found
  //
  return NULL;
}

/**

  Find the command name according to the function key.

  @param  CommandKey    - Command Function Key

  @return Not NULL        - The DebuggerName is found successfully
  @return NULL            - not found

**/
CHAR16 *
GetCommandNameByKey (
  IN EFI_INPUT_KEY CommandKey
  )
{
  UINTN  Index;

  //
  // Go through each command, check the CommandKey
  //
  for (Index = 0; mDebuggerCommandSet[Index].CommandName != NULL; Index++) {
    if ((mDebuggerCommandSet[Index].CommandKey.UnicodeChar == CommandKey.UnicodeChar) &&
        (mDebuggerCommandSet[Index].CommandKey.ScanCode    == CommandKey.ScanCode)) {
      //
      // Found
      //
      return mDebuggerCommandSet[Index].CommandName;
    }
  }

  //
  // Not found
  //
  return NULL;
}
