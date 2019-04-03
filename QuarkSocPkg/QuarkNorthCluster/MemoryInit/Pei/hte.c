/** @file
HTE handling routines for MRC use.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "mrc.h"
#include "memory_options.h"
#include "io.h"

#include "hte.h"


#ifdef SIM
VOID delay_n(UINT32 nanoseconds);
#define MySimStall(a)   delay_n(a/1000)
#endif

STATIC VOID EnableAllHteErrors(
    UINT8 Mask)
/*++

 Routine Description:

 This function enables to HTE to detect all possible errors for
 the given training parameters (per-bit or full byte lane).

 Returns:

 None

 --*/
{
  isbW32m(HTE, 0x000200A2, 0xFFFFFFFF);
  isbW32m(HTE, 0x000200A3, 0x000000FF);
  isbW32m(HTE, 0x000200A4, 0x00000000);
}

STATIC UINT32 CheckHteErrors(
    VOID)
/*++

 Routine Description:

 This function goes and reads the HTE register in order to find any error

 Returns:

 The errors detected in the HTE status register

 --*/
{
  return isbR32m(HTE, 0x000200A7);
}

STATIC VOID WaitForHteComplete(
    VOID)
/*++

 Routine Description:

 This function waits until HTE finishes

 Returns:

 None

 --*/
{
  UINT32 Tmp;

  ENTERFN();

  //
  // Is the test done?
  //
  do
  {
#ifdef SIM
    MySimStall (35000); // 35 ns delay
#endif
  } while (0 != (isbR32m(HTE, 0x00020012) & BIT30));

  Tmp = isbR32m(HTE, 0x00020011);
  Tmp = Tmp | BIT9;
  Tmp = Tmp & ~(BIT13 | BIT12);
  isbW32m(HTE, 0x00020011, Tmp);

  LEAVEFN();
}

STATIC VOID ClearHteErrorRegisters(
    VOID)
/*++

 Routine Description:

 Clears registers related with errors in the HTE.

 Returns:

 None

 --*/
{
  UINT32 Tmp;

  //
  // Clear all HTE errors and enable error checking
  // for burst and chunk.
  //
  Tmp = isbR32m(HTE, 0x000200A1);
  Tmp |= BIT8;
  isbW32m(HTE, 0x000200A1, Tmp);
}

UINT32 HteMemInit(
    MRC_PARAMS *CurrentMrcData,
    UINT8 MemInitFlag,
    UINT8 HaltHteEngineOnError)

/*++

 Routine Description:

 Uses HW HTE engine to initialize or test all memory attached to a given DUNIT.
 If MemInitFlag is 1, this routine writes 0s to all memory locations to initialize
 ECC.
 If MemInitFlag is 0, this routine will send an 5AA55AA5 pattern to all memory
 locations on the RankMask and then read it back.  Then it sends an A55AA55A
 pattern to all memory locations on the RankMask and reads it back.

 Arguments:

 CurrentMrcData: Host struture for all MRC global data.
 MemInitFlag: 0 for memtest, 1 for meminit.
 HaltHteEngineOnError:  Halt the HTE engine on first error observed, or keep
 running to see how many errors are found.

 Returns:
 Errors register showing HTE failures.
 Also prints out which rank failed the HTE test if failure occurs.
 For rank detection to work, the address map must be left in its default
 state.  If MRC changes the address map, this function must be modified
 to change it back to default at the beginning, then restore it at the end.

 --*/
{
  UINT32 Offset;
  UINT8 TestNum;
  UINT8 i;

  //
  // Clear out the error registers at the start of each memory
  // init or memory test run.
  //
  ClearHteErrorRegisters();

  isbW32m(HTE, 0x00020062, 0x00000015);

  for (Offset = 0x80; Offset <= 0x8F; Offset++)
  {
    isbW32m(HTE, Offset, ((Offset & 1) ? 0xA55A : 0x5AA5));
  }

  isbW32m(HTE, 0x00020021, 0x00000000);
#ifdef QUICKSIM
  // Just do 4 cache lines for simulation memtest to save time.
  isbW32m(HTE, 0x00020022, 4-1);
#else
  isbW32m(HTE, 0x00020022, (CurrentMrcData->mem_size >> 6) - 1);
#endif

  isbW32m(HTE, 0x00020063, 0xAAAAAAAA);
  isbW32m(HTE, 0x00020064, 0xCCCCCCCC);
  isbW32m(HTE, 0x00020065, 0xF0F0F0F0);
  isbW32m(HTE, 0x00020066, 0x03000000);

  switch (MemInitFlag)
  {
  case MrcMemInit:
    TestNum = 1; // Only 1 write pass through memory is needed to initialize ECC.
    break;
  case MrcMemTest:
    TestNum = 4; // Write/read then write/read with inverted pattern.
    break;
  default:
    DPF(D_INFO, "Unknown parameter for MemInitFlag: %d\n", MemInitFlag);
    return 0xFFFFFFFF;
    break;
  }

  DPF(D_INFO, "HteMemInit");
  for (i = 0; i < TestNum; i++)
  {
    DPF(D_INFO, ".");

    if (i == 0)
    {
      isbW32m(HTE, 0x00020061, 0x00000000);
      isbW32m(HTE, 0x00020020, 0x00110010);
    }
    else if (i == 1)
    {
      isbW32m(HTE, 0x00020061, 0x00000000);
      isbW32m(HTE, 0x00020020, 0x00010010);
    }
    else if (i == 2)
    {
      isbW32m(HTE, 0x00020061, 0x00010100);
      isbW32m(HTE, 0x00020020, 0x00110010);
    }
    else
    {
      isbW32m(HTE, 0x00020061, 0x00010100);
      isbW32m(HTE, 0x00020020, 0x00010010);
    }

    isbW32m(HTE, 0x00020011, 0x00111000);
    isbW32m(HTE, 0x00020011, 0x00111100);

    WaitForHteComplete();

    //
    // If this is a READ pass, check for errors at the end.
    //
    if ((i % 2) == 1)
    {
      //
      // Return immediately if  error.
      //
      if (CheckHteErrors())
      {
        break;
      }
    }
  }

  DPF(D_INFO, "done\n", i);
  return CheckHteErrors();
}

STATIC UINT16 BasicDataCompareHte(
    MRC_PARAMS *CurrentMrcData,
    UINT32 Address,
    UINT8 FirstRun,
    UINT8 Mode)
/*++

 Routine Description:

 Execute basic single cache line memory write/read/verify test using simple constant
 pattern (different for READ_RAIN and WRITE_TRAIN modes.
 See BasicWriteReadHTE which is external visible wrapper.

 Arguments:

 CurrentMrcData: Host struture for all MRC global data.
 Address: memory adress being tested (must hit specific channel/rank)
 FirstRun: If set then hte registers are configured, otherwise
 it is assumed configuration is done and just re-run the test.
 Mode: READ_TRAIN or WRITE_TRAIN (the difference is in the pattern)

 Returns:
 Returns byte lane failure on each bit (for Quark only bit0 and bit1)

 --*/
{
  UINT32 Pattern;
  UINT32 Offset;

  if (FirstRun)
  {
    isbW32m(HTE, 0x00020020, 0x01B10021);
    isbW32m(HTE, 0x00020021, 0x06000000);
    isbW32m(HTE, 0x00020022, Address >> 6);
    isbW32m(HTE, 0x00020062, 0x00800015);
    isbW32m(HTE, 0x00020063, 0xAAAAAAAA);
    isbW32m(HTE, 0x00020064, 0xCCCCCCCC);
    isbW32m(HTE, 0x00020065, 0xF0F0F0F0);
    isbW32m(HTE, 0x00020061, 0x00030008);

    if (Mode == WRITE_TRAIN)
    {
      Pattern = 0xC33C0000;
    }
    else // READ_TRAIN
    {
      Pattern = 0xAA5555AA;
    }

    for (Offset = 0x80; Offset <= 0x8F; Offset++)
    {
      isbW32m(HTE, Offset, Pattern);
    }
  }

  isbW32m(HTE, 0x000200A1, 0xFFFF1000);

  isbW32m(HTE, 0x00020011, 0x00011000);
  isbW32m(HTE, 0x00020011, 0x00011100);

  WaitForHteComplete();

  //
  // Return bits 15:8 of HTE_CH0_ERR_XSTAT to check for any bytelane errors.
  //
  return ((CheckHteErrors() >> 8) & 0xFF);
}

STATIC UINT16 ReadWriteDataCompareHte(
    MRC_PARAMS *CurrentMrcData,
    UINT32 Address,
    UINT8 LoopCount,
    UINT32 LfsrSeedVictim,
    UINT32 LfsrSeedAggressor,
    UINT8 VictimBit,
    UINT8 FirstRun)
/*++

 Routine Description:

 Examines single cache line memory with write/read/verify test using
 multiple data patterns (victim-aggressor algorithm).
 See WriteStressBitLanesHTE which is external visible wrapper.

 Arguments:

 CurrentMrcData: host struture for all MRC global data.
 Address: memory adress being tested (must hit specific channel/rank)
 LoopCount: number of test iterations
 LfsrSeedXxx: victim aggressor data pattern seed
 VictimBit: should be 0 as auto rotate feature is in use.
 FirstRun: If set then hte registers are configured, otherwise
 it is assumed configuration is done and just re-run the test.

 Returns:
 Returns byte lane failure on each bit (for Quark only bit0 and bit1)

 --*/
{
  UINT32 Offset;
  UINT32 Tmp;

  if (FirstRun)
  {
    isbW32m(HTE, 0x00020020, 0x00910024);
    isbW32m(HTE, 0x00020023, 0x00810024);
    isbW32m(HTE, 0x00020021, 0x06070000);
    isbW32m(HTE, 0x00020024, 0x06070000);
    isbW32m(HTE, 0x00020022, Address >> 6);
    isbW32m(HTE, 0x00020025, Address >> 6);
    isbW32m(HTE, 0x00020062, 0x0000002A);
    isbW32m(HTE, 0x00020063, LfsrSeedVictim);
    isbW32m(HTE, 0x00020064, LfsrSeedAggressor);
    isbW32m(HTE, 0x00020065, LfsrSeedVictim);

    //
    // Write the pattern buffers to select the victim bit. Start with bit0.
    //
    for (Offset = 0x80; Offset <= 0x8F; Offset++)
    {
      if ((Offset % 8) == VictimBit)
      {
        isbW32m(HTE, Offset, 0x55555555);
      }
      else
      {
        isbW32m(HTE, Offset, 0xCCCCCCCC);
      }
    }

    isbW32m(HTE, 0x00020061, 0x00000000);
    isbW32m(HTE, 0x00020066, 0x03440000);
    isbW32m(HTE, 0x000200A1, 0xFFFF1000);
  }

  Tmp = 0x10001000 | (LoopCount << 16);
  isbW32m(HTE, 0x00020011, Tmp);
  isbW32m(HTE, 0x00020011, Tmp | BIT8);

  WaitForHteComplete();

  return (CheckHteErrors() >> 8) & 0xFF;
}

UINT16 BasicWriteReadHTE(
    MRC_PARAMS *CurrentMrcData,
    UINT32 Address,
    UINT8 FirstRun,
    UINT8 Mode)
/*++

 Routine Description:

 Execute basic single cache line memory write/read/verify test using simple constant
 pattern (different for READ_RAIN and WRITE_TRAIN modes.

 Arguments:

 CurrentMrcData: Host struture for all MRC global data.
 Address: memory adress being tested (must hit specific channel/rank)
 FirstRun: If set then hte registers are configured, otherwise
 it is assumed configuration is done and just re-run the test.
 Mode: READ_TRAIN or WRITE_TRAIN (the difference is in the pattern)

 Returns:
 Returns byte lane failure on each bit (for Quark only bit0 and bit1)

 --*/
{
  UINT16 ByteLaneErrors;

  ENTERFN();

  //
  // Enable all error reporting in preparation for HTE test.
  //
  EnableAllHteErrors(0xFF);
  ClearHteErrorRegisters();

  ByteLaneErrors = BasicDataCompareHte(CurrentMrcData, Address, FirstRun,
      Mode);

  LEAVEFN();
  return ByteLaneErrors;
}

UINT16 WriteStressBitLanesHTE(
    MRC_PARAMS *CurrentMrcData,
    UINT32 Address,
    UINT8 FirstRun)
/*++

 Routine Description:

 Examines single cache line memory with write/read/verify test using
 multiple data patterns (victim-aggressor algorithm).

 Arguments:

 CurrentMrcData: host struture for all MRC global data.
 Address: memory adress being tested (must hit specific channel/rank)
 FirstRun: If set then hte registers are configured, otherwise
 it is assumed configuration is done and just re-run the test.

 Returns:
 Returns byte lane failure on each bit (for Quark only bit0 and bit1)

 --*/
{
  UINT16 ByteLaneErrors;
  UINT8 VictimBit = 0;

  ENTERFN();

  //
  // Enable all error reporting in preparation for HTE test.
  //
  EnableAllHteErrors(0xFF);
  ClearHteErrorRegisters();

  //
  // Loop through each bit in the bytelane.  Each pass creates a victim bit
  // while keeping all other bits the same - as aggressors.
  // AVN HTE adds an auto-rotate feature which allows us to program the entire victim/aggressor
  // sequence in 1 step. The victim bit rotates on each pass so no need to have software implement
  // a victim bit loop like on VLV.
  //
  ByteLaneErrors = ReadWriteDataCompareHte(CurrentMrcData, Address,
      HTE_LOOP_CNT, HTE_LFSR_VICTIM_SEED, HTE_LFSR_AGRESSOR_SEED, VictimBit,
      FirstRun);

  LEAVEFN();
  return ByteLaneErrors;
}

VOID HteMemOp(
    UINT32 Address,
    UINT8 FirstRun,
    UINT8 IsWrite)
/*++

 Routine Description:

 Execute basic single cache line memory write or read.
 This is just for receive enable / fine write levelling purpose.

 Arguments:

 CurrentMrcData: Host structure for all MRC global data.
 Address: memory address used (must hit specific channel/rank)
 FirstRun: If set then hte registers are configured, otherwise
 it is assumed configuration is done and just re-run the test.
 IsWrite: When non-zero memory write operation executed, otherwise read

 Returns:
 None

 --*/
{
  UINT32 Offset;
  UINT32 Tmp;

  EnableAllHteErrors(0xFF);
  ClearHteErrorRegisters();

  if (FirstRun)
  {
    Tmp = IsWrite ? 0x01110021 : 0x01010021;
    isbW32m(HTE, 0x00020020, Tmp);

    isbW32m(HTE, 0x00020021, 0x06000000);
    isbW32m(HTE, 0x00020022, Address >> 6);
    isbW32m(HTE, 0x00020062, 0x00800015);
    isbW32m(HTE, 0x00020063, 0xAAAAAAAA);
    isbW32m(HTE, 0x00020064, 0xCCCCCCCC);
    isbW32m(HTE, 0x00020065, 0xF0F0F0F0);
    isbW32m(HTE, 0x00020061, 0x00030008);

    for (Offset = 0x80; Offset <= 0x8F; Offset++)
    {
      isbW32m(HTE, Offset, 0xC33C0000);
    }
  }

  isbW32m(HTE, 0x000200A1, 0xFFFF1000);
  isbW32m(HTE, 0x00020011, 0x00011000);
  isbW32m(HTE, 0x00020011, 0x00011100);

  WaitForHteComplete();
}

