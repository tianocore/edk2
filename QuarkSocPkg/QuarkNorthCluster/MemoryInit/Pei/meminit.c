/************************************************************************
 *
 * Copyright (c) 2013-2015 Intel Corporation.
 *
* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * This file contains all of the Cat Mountain Memory Reference Code (MRC).
 *
 * These functions are generic and should work for any Cat Mountain config.
 *
 * MRC requires two data structures to be passed in which are initialised by "PreMemInit()".
 *
 * The basic flow is as follows:
 * 01) Check for supported DDR speed configuration
 * 02) Set up MEMORY_MANAGER buffer as pass-through (POR)
 * 03) Set Channel Interleaving Mode and Channel Stride to the most aggressive setting possible
 * 04) Set up the MCU logic
 * 05) Set up the DDR_PHY logic
 * 06) Initialise the DRAMs (JEDEC)
 * 07) Perform the Receive Enable Calibration algorithm
 * 08) Perform the Write Leveling algorithm
 * 09) Perform the Read Training algorithm (includes internal Vref)
 * 10) Perform the Write Training algorithm
 * 11) Set Channel Interleaving Mode and Channel Stride to the desired settings
 *
 * Dunit configuration based on Valleyview MRC.
 *
 ***************************************************************************/

#include "mrc.h"
#include "memory_options.h"

#include "meminit.h"
#include "meminit_utils.h"
#include "hte.h"
#include "io.h"

// Override ODT to off state if requested
#define DRMC_DEFAULT    (mrc_params->rd_odt_value==0?BIT12:0)


// tRFC values (in picoseconds) per density
const uint32_t tRFC[5] =
{
    90000,  // 512Mb
    110000, // 1Gb
    160000, // 2Gb
    300000, // 4Gb
    350000, // 8Gb
    };

// tCK clock period in picoseconds per speed index 800, 1066, 1333
const uint32_t tCK[3] =
{
    2500,
    1875,
    1500
};

#ifdef SIM
// Select static timings specific to simulation environment
#define PLATFORM_ID    0
#else
// Select static timings specific to ClantonPeek platform
#define PLATFORM_ID    1
#endif


// Global variables
const uint16_t ddr_wclk[] =
    {193, 158};

const uint16_t ddr_wctl[] =
    {  1, 217};

const uint16_t ddr_wcmd[] =
    {  1, 220};


#ifdef BACKUP_RCVN
const uint16_t ddr_rcvn[] =
    {129, 498};
#endif // BACKUP_RCVN

#ifdef BACKUP_WDQS
const uint16_t ddr_wdqs[] =
    { 65, 289};
#endif // BACKUP_WDQS

#ifdef BACKUP_RDQS
const uint8_t ddr_rdqs[] =
    { 32,  24};
#endif // BACKUP_RDQS

#ifdef BACKUP_WDQ
const uint16_t ddr_wdq[] =
    { 32, 257};
#endif // BACKUP_WDQ



// Select MEMORY_MANAGER as the source for PRI interface
static void select_memory_manager(
    MRCParams_t *mrc_params)
{
  RegDCO Dco;

  ENTERFN();

  Dco.raw = isbR32m(MCU, DCO);
  Dco.field.PMICTL = 0;          //0 - PRI owned by MEMORY_MANAGER
  isbW32m(MCU, DCO, Dco.raw);

  LEAVEFN();
}

// Select HTE as the source for PRI interface
void select_hte(
    MRCParams_t *mrc_params)
{
  RegDCO Dco;

  ENTERFN();

  Dco.raw = isbR32m(MCU, DCO);
  Dco.field.PMICTL = 1;          //1 - PRI owned by HTE
  isbW32m(MCU, DCO, Dco.raw);

  LEAVEFN();
}

// Send DRAM command, data should be formated
// using DCMD_Xxxx macro or emrsXCommand structure.
static void dram_init_command(
    uint32_t data)
{
  Wr32(DCMD, 0, data);
}

// Send DRAM wake command using special MCU side-band WAKE opcode
static void dram_wake_command(
    void)
{
  ENTERFN();

  Wr32(MMIO, PCIADDR(0,0,0,SB_PACKET_REG),
      (uint32_t) SB_COMMAND(SB_WAKE_CMND_OPCODE, MCU, 0));

  LEAVEFN();
}

// Stop self refresh driven by MCU
static void clear_self_refresh(
    MRCParams_t *mrc_params)
{
  ENTERFN();

  // clear the PMSTS Channel Self Refresh bits
  isbM32m(MCU, PMSTS, BIT0, BIT0);

  LEAVEFN();
}

// Configure MCU before jedec init sequence
static void prog_decode_before_jedec(
    MRCParams_t *mrc_params)
{
  RegDRP Drp;
  RegDRCF Drfc;
  RegDCAL Dcal;
  RegDSCH Dsch;
  RegDPMC0 Dpmc0;

  ENTERFN();

  // Disable power saving features
  Dpmc0.raw = isbR32m(MCU, DPMC0);
  Dpmc0.field.CLKGTDIS = 1;
  Dpmc0.field.DISPWRDN = 1;
  Dpmc0.field.DYNSREN = 0;
  Dpmc0.field.PCLSTO = 0;
  isbW32m(MCU, DPMC0, Dpmc0.raw);

  // Disable out of order transactions
  Dsch.raw = isbR32m(MCU, DSCH);
  Dsch.field.OOODIS = 1;
  Dsch.field.NEWBYPDIS = 1;
  isbW32m(MCU, DSCH, Dsch.raw);

  // Disable issuing the REF command
  Drfc.raw = isbR32m(MCU, DRFC);
  Drfc.field.tREFI = 0;
  isbW32m(MCU, DRFC, Drfc.raw);

  // Disable ZQ calibration short
  Dcal.raw = isbR32m(MCU, DCAL);
  Dcal.field.ZQCINT = 0;
  Dcal.field.SRXZQCL = 0;
  isbW32m(MCU, DCAL, Dcal.raw);

  // Training performed in address mode 0, rank population has limited impact, however
  // simulator complains if enabled non-existing rank.
  Drp.raw = 0;
  if (mrc_params->rank_enables & 1)
    Drp.field.rank0Enabled = 1;
  if (mrc_params->rank_enables & 2)
    Drp.field.rank1Enabled = 1;
  isbW32m(MCU, DRP, Drp.raw);

  LEAVEFN();
}

// After Cold Reset, BIOS should set COLDWAKE bit to 1 before
// sending the WAKE message to the Dunit.
// For Standby Exit, or any other mode in which the DRAM is in
// SR, this bit must be set to 0.
static void perform_ddr_reset(
    MRCParams_t *mrc_params)
{
  ENTERFN();

  // Set COLDWAKE bit before sending the WAKE message
  isbM32m(MCU, DRMC, BIT16, BIT16);

  // Send wake command to DUNIT (MUST be done before JEDEC)
  dram_wake_command();

  // Set default value
  isbW32m(MCU, DRMC, DRMC_DEFAULT);

  LEAVEFN();
}

// Dunit Initialisation Complete.
// Indicates that initialisation of the Dunit has completed.
// Memory accesses are permitted and maintenance operation
// begins. Until this bit is set to a 1, the memory controller will
// not accept DRAM requests from the MEMORY_MANAGER or HTE.
static void set_ddr_init_complete(
    MRCParams_t *mrc_params)
{
  RegDCO Dco;

  ENTERFN();

  Dco.raw = isbR32m(MCU, DCO);
  Dco.field.PMICTL = 0;          //0 - PRI owned by MEMORY_MANAGER
  Dco.field.IC = 1;              //1 - initialisation complete
  isbW32m(MCU, DCO, Dco.raw);

  LEAVEFN();
}

static void prog_page_ctrl(
    MRCParams_t *mrc_params)
{
  RegDPMC0 Dpmc0;

  ENTERFN();

  Dpmc0.raw = isbR32m(MCU, DPMC0);

  Dpmc0.field.PCLSTO = 0x4;
  Dpmc0.field.PREAPWDEN = 1;

  isbW32m(MCU, DPMC0, Dpmc0.raw);
}

// Configure MCU Power Management Control Register
// and Scheduler Control Register.
static void prog_ddr_control(
    MRCParams_t *mrc_params)
{
  RegDSCH Dsch;
  RegDPMC0 Dpmc0;

  ENTERFN();

  Dpmc0.raw = isbR32m(MCU, DPMC0);
  Dsch.raw = isbR32m(MCU, DSCH);

  Dpmc0.field.DISPWRDN = mrc_params->power_down_disable;
  Dpmc0.field.CLKGTDIS = 0;
  Dpmc0.field.PCLSTO = 4;
  Dpmc0.field.PREAPWDEN = 1;

  Dsch.field.OOODIS = 0;
  Dsch.field.OOOST3DIS = 0;
  Dsch.field.NEWBYPDIS = 0;

  isbW32m(MCU, DSCH, Dsch.raw);
  isbW32m(MCU, DPMC0, Dpmc0.raw);

  // CMDTRIST = 2h - CMD/ADDR are tristated when no valid command
  isbM32m(MCU, DPMC1, 2 << 4, BIT5|BIT4);

  LEAVEFN();
}

// After training complete configure MCU Rank Population Register
// specifying: ranks enabled, device width, density, address mode.
static void prog_dra_drb(
    MRCParams_t *mrc_params)
{
  RegDRP Drp;
  RegDCO Dco;

  ENTERFN();

  Dco.raw = isbR32m(MCU, DCO);
  Dco.field.IC = 0;
  isbW32m(MCU, DCO, Dco.raw);

  Drp.raw = 0;
  if (mrc_params->rank_enables & 1)
    Drp.field.rank0Enabled = 1;
  if (mrc_params->rank_enables & 2)
    Drp.field.rank1Enabled = 1;
  if (mrc_params->dram_width == x16)
  {
    Drp.field.dimm0DevWidth = 1;
    Drp.field.dimm1DevWidth = 1;
  }
  // Density encoding in DRAMParams_t 0=512Mb, 1=Gb, 2=2Gb, 3=4Gb
  // has to be mapped RANKDENSx encoding (0=1Gb)
  Drp.field.dimm0DevDensity = mrc_params->params.DENSITY - 1;
  Drp.field.dimm1DevDensity = mrc_params->params.DENSITY - 1;

  // Address mode can be overwritten if ECC enabled
  Drp.field.addressMap = mrc_params->address_mode;

  isbW32m(MCU, DRP, Drp.raw);

  Dco.field.PMICTL = 0;          //0 - PRI owned by MEMORY_MANAGER
  Dco.field.IC = 1;              //1 - initialisation complete
  isbW32m(MCU, DCO, Dco.raw);

  LEAVEFN();
}

// Configure refresh rate and short ZQ calibration interval.
// Activate dynamic self refresh.
static void change_refresh_period(
    MRCParams_t *mrc_params)
{
  RegDRCF Drfc;
  RegDCAL Dcal;
  RegDPMC0 Dpmc0;

  ENTERFN();

  Drfc.raw = isbR32m(MCU, DRFC);
  Drfc.field.tREFI = mrc_params->refresh_rate;
  Drfc.field.REFDBTCLR = 1;
  isbW32m(MCU, DRFC, Drfc.raw);

  Dcal.raw = isbR32m(MCU, DCAL);
  Dcal.field.ZQCINT = 3; // 63ms
  isbW32m(MCU, DCAL, Dcal.raw);

  Dpmc0.raw = isbR32m(MCU, DPMC0);
  Dpmc0.field.ENPHYCLKGATE = 1;
  Dpmc0.field.DYNSREN = 1;
  isbW32m(MCU, DPMC0, Dpmc0.raw);

  LEAVEFN();
}

// Send DRAM wake command
static void perform_wake(
    MRCParams_t *mrc_params)
{
  ENTERFN();

  dram_wake_command();

  LEAVEFN();
}

// prog_ddr_timing_control (aka mcu_init):
// POST_CODE[major] == 0x02
//
// It will initialise timing registers in the MCU (DTR0..DTR4).
static void prog_ddr_timing_control(
    MRCParams_t *mrc_params)
{
  uint8_t TCL, WL;
  uint8_t TRP, TRCD, TRAS, TWR, TWTR, TRRD, TRTP, TFAW;
  uint32_t TCK;

  RegDTR0 Dtr0;
  RegDTR1 Dtr1;
  RegDTR2 Dtr2;
  RegDTR3 Dtr3;
  RegDTR4 Dtr4;

  ENTERFN();

  // mcu_init starts
  post_code(0x02, 0x00);

  Dtr0.raw = isbR32m(MCU, DTR0);
  Dtr1.raw = isbR32m(MCU, DTR1);
  Dtr2.raw = isbR32m(MCU, DTR2);
  Dtr3.raw = isbR32m(MCU, DTR3);
  Dtr4.raw = isbR32m(MCU, DTR4);

  TCK = tCK[mrc_params->ddr_speed];  // Clock in picoseconds
  TCL = mrc_params->params.tCL;      // CAS latency in clocks
  TRP = TCL;  // Per CAT MRC
  TRCD = TCL;  // Per CAT MRC
  TRAS = MCEIL(mrc_params->params.tRAS, TCK);
  TWR = MCEIL(15000, TCK);   // Per JEDEC: tWR=15000ps DDR2/3 from 800-1600

  TWTR = MCEIL(mrc_params->params.tWTR, TCK);
  TRRD = MCEIL(mrc_params->params.tRRD, TCK);
  TRTP = 4;  // Valid for 800 and 1066, use 5 for 1333
  TFAW = MCEIL(mrc_params->params.tFAW, TCK);

  WL = 5 + mrc_params->ddr_speed;

  Dtr0.field.dramFrequency = mrc_params->ddr_speed;

  Dtr0.field.tCL = TCL - 5;            //Convert from TCL (DRAM clocks) to VLV indx
  Dtr0.field.tRP = TRP - 5;            //5 bit DRAM Clock
  Dtr0.field.tRCD = TRCD - 5;          //5 bit DRAM Clock

  Dtr1.field.tWCL = WL - 3;            //Convert from WL (DRAM clocks)  to VLV indx
  Dtr1.field.tWTP = WL + 4 + TWR - 14;  //Change to tWTP
  Dtr1.field.tRTP = MMAX(TRTP, 4) - 3;  //4 bit DRAM Clock
  Dtr1.field.tRRD = TRRD - 4;        //4 bit DRAM Clock
  Dtr1.field.tCMD = 1;             //2N
  Dtr1.field.tRAS = TRAS - 14;      //6 bit DRAM Clock

  Dtr1.field.tFAW = ((TFAW + 1) >> 1) - 5;    //4 bit DRAM Clock
  Dtr1.field.tCCD = 0;                        //Set 4 Clock CAS to CAS delay (multi-burst)
  Dtr2.field.tRRDR = 1;
  Dtr2.field.tWWDR = 2;
  Dtr2.field.tRWDR = 2;
  Dtr3.field.tWRDR = 2;
  Dtr3.field.tWRDD = 2;

  if (mrc_params->ddr_speed == DDRFREQ_800)
  {
     // Extended RW delay (+1)
     Dtr3.field.tRWSR = TCL - 5 + 1;
  }
  else if(mrc_params->ddr_speed == DDRFREQ_1066)
  {
     // Extended RW delay (+1)
     Dtr3.field.tRWSR = TCL - 5 + 1;
  }

  Dtr3.field.tWRSR = 4 + WL + TWTR - 11;

  if (mrc_params->ddr_speed == DDRFREQ_800)
  {
    Dtr3.field.tXP = MMAX(0, 1 - Dtr1.field.tCMD);
  }
  else
  {
    Dtr3.field.tXP = MMAX(0, 2 - Dtr1.field.tCMD);
  }

  Dtr4.field.WRODTSTRT = Dtr1.field.tCMD;
  Dtr4.field.WRODTSTOP = Dtr1.field.tCMD;
  Dtr4.field.RDODTSTRT = Dtr1.field.tCMD + Dtr0.field.tCL - Dtr1.field.tWCL + 2; //Convert from WL (DRAM clocks)  to VLV indx
  Dtr4.field.RDODTSTOP = Dtr1.field.tCMD + Dtr0.field.tCL - Dtr1.field.tWCL + 2;
  Dtr4.field.TRGSTRDIS = 0;
  Dtr4.field.ODTDIS = 0;

  isbW32m(MCU, DTR0, Dtr0.raw);
  isbW32m(MCU, DTR1, Dtr1.raw);
  isbW32m(MCU, DTR2, Dtr2.raw);
  isbW32m(MCU, DTR3, Dtr3.raw);
  isbW32m(MCU, DTR4, Dtr4.raw);

  LEAVEFN();
}

// ddrphy_init:
// POST_CODE[major] == 0x03
//
// This function performs some initialisation on the DDRIO unit.
// This function is dependent on BOARD_ID, DDR_SPEED, and CHANNEL_ENABLES.
static void ddrphy_init(MRCParams_t *mrc_params)
{
  uint32_t tempD; // temporary DWORD
  uint8_t channel_i; // channel counter
  uint8_t rank_i; // rank counter
  uint8_t bl_grp_i; // byte lane group counter (2 BLs per module)

  uint8_t bl_divisor = /*(mrc_params->channel_width==x16)?2:*/1; // byte lane divisor
  uint8_t speed = mrc_params->ddr_speed & (BIT1|BIT0); // For DDR3 --> 0 == 800, 1 == 1066, 2 == 1333
  uint8_t tCAS;
  uint8_t tCWL;

  ENTERFN();

  tCAS = mrc_params->params.tCL;
  tCWL = 5 + mrc_params->ddr_speed;

  // ddrphy_init starts
  post_code(0x03, 0x00);

  // HSD#231531
  // Make sure IOBUFACT is deasserted before initialising the DDR PHY.
  // HSD#234845
  // Make sure WRPTRENABLE is deasserted before initialising the DDR PHY.
  for (channel_i=0; channel_i<NUM_CHANNELS; channel_i++) {
    if (mrc_params->channel_enables & (1<<channel_i)) {
      // Deassert DDRPHY Initialisation Complete
      isbM32m(DDRPHY, (CMDPMCONFIG0 + (channel_i * DDRIOCCC_CH_OFFSET)), ~BIT20, BIT20); // SPID_INIT_COMPLETE=0
      // Deassert IOBUFACT
      isbM32m(DDRPHY, (CMDCFGREG0 + (channel_i * DDRIOCCC_CH_OFFSET)), ~BIT2, BIT2); // IOBUFACTRST_N=0
      // Disable WRPTR
      isbM32m(DDRPHY, (CMDPTRREG + (channel_i * DDRIOCCC_CH_OFFSET)), ~BIT0, BIT0); // WRPTRENABLE=0
    } // if channel enabled
  } // channel_i loop

  // Put PHY in reset
  isbM32m(DDRPHY, MASTERRSTN, 0, BIT0); // PHYRSTN=0

  // Initialise DQ01,DQ23,CMD,CLK-CTL,COMP modules
  // STEP0:
  post_code(0x03, 0x10);
  for (channel_i=0; channel_i<NUM_CHANNELS; channel_i++) {
    if (mrc_params->channel_enables & (1<<channel_i)) {

      // DQ01-DQ23
      for (bl_grp_i=0; bl_grp_i<((NUM_BYTE_LANES/bl_divisor)/2); bl_grp_i++) {
        isbM32m(DDRPHY, (DQOBSCKEBBCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), ((bl_grp_i) ? (0x00) : (BIT22)), (BIT22)); // Analog MUX select - IO2xCLKSEL

        // ODT Strength
        switch (mrc_params->rd_odt_value) {
          case 1: tempD = 0x3; break; // 60 ohm
          case 2: tempD = 0x3; break; // 120 ohm
          case 3: tempD = 0x3; break; // 180 ohm
          default: tempD = 0x3; break; // 120 ohm
        }
        isbM32m(DDRPHY, (B0RXIOBUFCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), (tempD<<5), (BIT6|BIT5)); // ODT strength
        isbM32m(DDRPHY, (B1RXIOBUFCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), (tempD<<5), (BIT6|BIT5)); // ODT strength
        // Dynamic ODT/DIFFAMP
        tempD = (((tCAS)<<24)|((tCAS)<<16)|((tCAS)<<8)|((tCAS)<<0));
        switch (speed) {
          case 0: tempD -= 0x01010101; break; // 800
          case 1: tempD -= 0x02020202; break; // 1066
          case 2: tempD -= 0x03030303; break; // 1333
          case 3: tempD -= 0x04040404; break; // 1600
        }
        isbM32m(DDRPHY, (B01LATCTL1 + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), tempD, ((BIT28|BIT27|BIT26|BIT25|BIT24)|(BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT12|BIT11|BIT10|BIT9|BIT8)|(BIT4|BIT3|BIT2|BIT1|BIT0))); // Launch Time: ODT, DIFFAMP, ODT, DIFFAMP
        switch (speed) {
          // HSD#234715
          case 0: tempD = ((0x06<<16)|(0x07<<8)); break; // 800
          case 1: tempD = ((0x07<<16)|(0x08<<8)); break; // 1066
          case 2: tempD = ((0x09<<16)|(0x0A<<8)); break; // 1333
          case 3: tempD = ((0x0A<<16)|(0x0B<<8)); break; // 1600
        }
        isbM32m(DDRPHY, (B0ONDURCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), tempD, ((BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT13|BIT12|BIT11|BIT10|BIT9|BIT8))); // On Duration: ODT, DIFFAMP
        isbM32m(DDRPHY, (B1ONDURCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), tempD, ((BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT13|BIT12|BIT11|BIT10|BIT9|BIT8))); // On Duration: ODT, DIFFAMP

        switch (mrc_params->rd_odt_value) {
          case 0:  tempD = ((0x3F<<16)|(0x3f<<10)); break; // override DIFFAMP=on, ODT=off
          default: tempD = ((0x3F<<16)|(0x2A<<10)); break; // override DIFFAMP=on, ODT=on
        }
        isbM32m(DDRPHY, (B0OVRCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), tempD, ((BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT15|BIT14|BIT13|BIT12|BIT11|BIT10))); // Override: DIFFAMP, ODT
        isbM32m(DDRPHY, (B1OVRCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), tempD, ((BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT15|BIT14|BIT13|BIT12|BIT11|BIT10))); // Override: DIFFAMP, ODT

        // DLL Setup
        // 1xCLK Domain Timings: tEDP,RCVEN,WDQS (PO)
        isbM32m(DDRPHY, (B0LATCTL0 + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), (((tCAS+7)<<16)|((tCAS-4)<<8)|((tCWL-2)<<0)), ((BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT12|BIT11|BIT10|BIT9|BIT8)|(BIT4|BIT3|BIT2|BIT1|BIT0))); // 1xCLK: tEDP, RCVEN, WDQS
        isbM32m(DDRPHY, (B1LATCTL0 + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), (((tCAS+7)<<16)|((tCAS-4)<<8)|((tCWL-2)<<0)), ((BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT12|BIT11|BIT10|BIT9|BIT8)|(BIT4|BIT3|BIT2|BIT1|BIT0))); // 1xCLK: tEDP, RCVEN, WDQS

        // RCVEN Bypass (PO)
        isbM32m(DDRPHY, (B0RXIOBUFCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), ((0x0<<7)|(0x0<<0)), (BIT7|BIT0)); // AFE Bypass, RCVEN DIFFAMP
        isbM32m(DDRPHY, (B1RXIOBUFCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), ((0x0<<7)|(0x0<<0)), (BIT7|BIT0)); // AFE Bypass, RCVEN DIFFAMP
        // TX
        isbM32m(DDRPHY, (DQCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), (BIT16), (BIT16)); // 0 means driving DQ during DQS-preamble
        isbM32m(DDRPHY, (B01PTRCTL1 + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), (BIT8), (BIT8)); // WR_LVL mode disable
        // RX (PO)
        isbM32m(DDRPHY, (B0VREFCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), ((0x03<<2)|(0x0<<1)|(0x0<<0)), ((BIT7|BIT6|BIT5|BIT4|BIT3|BIT2)|BIT1|BIT0)); // Internal Vref Code, Enable#, Ext_or_Int (1=Ext)
        isbM32m(DDRPHY, (B1VREFCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), ((0x03<<2)|(0x0<<1)|(0x0<<0)), ((BIT7|BIT6|BIT5|BIT4|BIT3|BIT2)|BIT1|BIT0)); // Internal Vref Code, Enable#, Ext_or_Int (1=Ext)
        isbM32m(DDRPHY, (B0RXIOBUFCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), (0), (BIT4)); // Per-Bit De-Skew Enable
        isbM32m(DDRPHY, (B1RXIOBUFCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), (0), (BIT4)); // Per-Bit De-Skew Enable
      }
      // CLKEBB
      isbM32m(DDRPHY, (CMDOBSCKEBBCTL + (channel_i * DDRIOCCC_CH_OFFSET)), 0, (BIT23));

      // Enable tristate control of cmd/address bus
      isbM32m(DDRPHY, (CMDCFGREG0 + (channel_i * DDRIOCCC_CH_OFFSET)), 0, (BIT1|BIT0));

      // ODT RCOMP
      isbM32m(DDRPHY, (CMDRCOMPODT + (channel_i * DDRIOCCC_CH_OFFSET)), ((0x03<<5)|(0x03<<0)), ((BIT9|BIT8|BIT7|BIT6|BIT5)|(BIT4|BIT3|BIT2|BIT1|BIT0)));

      // CMDPM* registers must be programmed in this order...
      isbM32m(DDRPHY, (CMDPMDLYREG4 + (channel_i * DDRIOCCC_CH_OFFSET)), ((0xFFFFU<<16)|(0xFFFF<<0)), ((BIT31|BIT30|BIT29|BIT28|BIT27|BIT26|BIT25|BIT24|BIT23|BIT22|BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT15|BIT14|BIT13|BIT12|BIT11|BIT10|BIT9|BIT8|BIT7|BIT6|BIT5|BIT4|BIT3|BIT2|BIT1|BIT0))); // Turn On Delays: SFR (regulator), MPLL
      isbM32m(DDRPHY, (CMDPMDLYREG3 + (channel_i * DDRIOCCC_CH_OFFSET)), ((0xFU<<28)|(0xFFF<<16)|(0xF<<12)|(0x616<<0)), ((BIT31|BIT30|BIT29|BIT28)|(BIT27|BIT26|BIT25|BIT24|BIT23|BIT22|BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT15|BIT14|BIT13|BIT12)|(BIT11|BIT10|BIT9|BIT8|BIT7|BIT6|BIT5|BIT4|BIT3|BIT2|BIT1|BIT0))); // Delays: ASSERT_IOBUFACT_to_ALLON0_for_PM_MSG_3, VREG (MDLL) Turn On, ALLON0_to_DEASSERT_IOBUFACT_for_PM_MSG_gt0, MDLL Turn On
      isbM32m(DDRPHY, (CMDPMDLYREG2 + (channel_i * DDRIOCCC_CH_OFFSET)), ((0xFFU<<24)|(0xFF<<16)|(0xFF<<8)|(0xFF<<0)), ((BIT31|BIT30|BIT29|BIT28|BIT27|BIT26|BIT25|BIT24)|(BIT23|BIT22|BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT15|BIT14|BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)|(BIT7|BIT6|BIT5|BIT4|BIT3|BIT2|BIT1|BIT0))); // MPLL Divider Reset Delays
      isbM32m(DDRPHY, (CMDPMDLYREG1 + (channel_i * DDRIOCCC_CH_OFFSET)), ((0xFFU<<24)|(0xFF<<16)|(0xFF<<8)|(0xFF<<0)), ((BIT31|BIT30|BIT29|BIT28|BIT27|BIT26|BIT25|BIT24)|(BIT23|BIT22|BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT15|BIT14|BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)|(BIT7|BIT6|BIT5|BIT4|BIT3|BIT2|BIT1|BIT0))); // Turn Off Delays: VREG, Staggered MDLL, MDLL, PI
      isbM32m(DDRPHY, (CMDPMDLYREG0 + (channel_i * DDRIOCCC_CH_OFFSET)), ((0xFFU<<24)|(0xFF<<16)|(0xFF<<8)|(0xFF<<0)), ((BIT31|BIT30|BIT29|BIT28|BIT27|BIT26|BIT25|BIT24)|(BIT23|BIT22|BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT15|BIT14|BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)|(BIT7|BIT6|BIT5|BIT4|BIT3|BIT2|BIT1|BIT0))); // Turn On Delays: MPLL, Staggered MDLL, PI, IOBUFACT
      isbM32m(DDRPHY, (CMDPMCONFIG0 + (channel_i * DDRIOCCC_CH_OFFSET)), ((0x6<<8)|BIT6|(0x4<<0)), (BIT31|BIT30|BIT29|BIT28|BIT27|BIT26|BIT25|BIT24|BIT23|BIT22|BIT21|(BIT11|BIT10|BIT9|BIT8)|BIT6|(BIT3|BIT2|BIT1|BIT0))); // Allow PUnit signals
      isbM32m(DDRPHY, (CMDMDLLCTL +   (channel_i * DDRIOCCC_CH_OFFSET)), ((0x3<<4)|(0x7<<0)), ((BIT6|BIT5|BIT4)|(BIT3|BIT2|BIT1|BIT0))); // DLL_VREG Bias Trim, VREF Tuning for DLL_VREG
      // CLK-CTL
      isbM32m(DDRPHY, (CCOBSCKEBBCTL + (channel_i * DDRIOCCC_CH_OFFSET)), 0, (BIT24)); // CLKEBB
      isbM32m(DDRPHY, (CCCFGREG0 +     (channel_i * DDRIOCCC_CH_OFFSET)), ((0x0<<16)|(0x0<<12)|(0x0<<8)|(0xF<<4)|BIT0), ((BIT19|BIT18|BIT17|BIT16)|(BIT15|BIT14|BIT13|BIT12)|(BIT11|BIT10|BIT9|BIT8)|(BIT7|BIT6|BIT5|BIT4)|BIT0)); // Buffer Enable: CS,CKE,ODT,CLK
      isbM32m(DDRPHY, (CCRCOMPODT +    (channel_i * DDRIOCCC_CH_OFFSET)), ((0x03<<8)|(0x03<<0)), ((BIT12|BIT11|BIT10|BIT9|BIT8)|(BIT4|BIT3|BIT2|BIT1|BIT0))); // ODT RCOMP
      isbM32m(DDRPHY, (CCMDLLCTL +     (channel_i * DDRIOCCC_CH_OFFSET)), ((0x3<<4)|(0x7<<0)), ((BIT6|BIT5|BIT4)|(BIT3|BIT2|BIT1|BIT0))); // DLL_VREG Bias Trim, VREF Tuning for DLL_VREG

      // COMP (RON channel specific)
      // - DQ/DQS/DM RON: 32 Ohm
      // - CTRL/CMD RON: 27 Ohm
      // - CLK RON: 26 Ohm
      isbM32m(DDRPHY, (DQVREFCH0 +  (channel_i * DDRCOMP_CH_OFFSET)), ((0x08<<24)|(0x03<<16)), ((BIT29|BIT28|BIT27|BIT26|BIT25|BIT24)|(BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)));  // RCOMP Vref PU/PD
      isbM32m(DDRPHY, (CMDVREFCH0 + (channel_i * DDRCOMP_CH_OFFSET)), ((0x0C<<24)|(0x03<<16)), ((BIT29|BIT28|BIT27|BIT26|BIT25|BIT24)|(BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)));  // RCOMP Vref PU/PD
      isbM32m(DDRPHY, (CLKVREFCH0 + (channel_i * DDRCOMP_CH_OFFSET)), ((0x0F<<24)|(0x03<<16)), ((BIT29|BIT28|BIT27|BIT26|BIT25|BIT24)|(BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)));  // RCOMP Vref PU/PD
      isbM32m(DDRPHY, (DQSVREFCH0 + (channel_i * DDRCOMP_CH_OFFSET)), ((0x08<<24)|(0x03<<16)), ((BIT29|BIT28|BIT27|BIT26|BIT25|BIT24)|(BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)));  // RCOMP Vref PU/PD
      isbM32m(DDRPHY, (CTLVREFCH0 + (channel_i * DDRCOMP_CH_OFFSET)), ((0x0C<<24)|(0x03<<16)), ((BIT29|BIT28|BIT27|BIT26|BIT25|BIT24)|(BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)));  // RCOMP Vref PU/PD

      // DQS Swapped Input Enable
      isbM32m(DDRPHY, (COMPEN1CH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT19|BIT17),           ((BIT31|BIT30)|BIT19|BIT17|(BIT15|BIT14)));

      // ODT VREF = 1.5 x 274/360+274 = 0.65V (code of ~50)
      isbM32m(DDRPHY, (DQVREFCH0 +  (channel_i * DDRCOMP_CH_OFFSET)), ((0x32<<8)|(0x03<<0)),   ((BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)|(BIT5|BIT4|BIT3|BIT2|BIT1|BIT0))); // ODT Vref PU/PD
      isbM32m(DDRPHY, (DQSVREFCH0 + (channel_i * DDRCOMP_CH_OFFSET)), ((0x32<<8)|(0x03<<0)),   ((BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)|(BIT5|BIT4|BIT3|BIT2|BIT1|BIT0))); // ODT Vref PU/PD
      isbM32m(DDRPHY, (CLKVREFCH0 + (channel_i * DDRCOMP_CH_OFFSET)), ((0x0E<<8)|(0x05<<0)),   ((BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)|(BIT5|BIT4|BIT3|BIT2|BIT1|BIT0))); // ODT Vref PU/PD

      // Slew rate settings are frequency specific, numbers below are for 800Mhz (speed == 0)
      // - DQ/DQS/DM/CLK SR: 4V/ns,
      // - CTRL/CMD SR: 1.5V/ns
      tempD = (0x0E<<16)|(0x0E<<12)|(0x08<<8)|(0x0B<<4)|(0x0B<<0);
      isbM32m(DDRPHY, (DLYSELCH0 +   (channel_i * DDRCOMP_CH_OFFSET)), (tempD), ((BIT19|BIT18|BIT17|BIT16)|(BIT15|BIT14|BIT13|BIT12)|(BIT11|BIT10|BIT9|BIT8)|(BIT7|BIT6|BIT5|BIT4)|(BIT3|BIT2|BIT1|BIT0))); // DCOMP Delay Select: CTL,CMD,CLK,DQS,DQ
      isbM32m(DDRPHY, (TCOVREFCH0 +  (channel_i * DDRCOMP_CH_OFFSET)), ((0x05<<16)|(0x05<<8)|(0x05<<0)), ((BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)|(BIT5|BIT4|BIT3|BIT2|BIT1|BIT0))); // TCO Vref CLK,DQS,DQ
      isbM32m(DDRPHY, (CCBUFODTCH0 + (channel_i * DDRCOMP_CH_OFFSET)), ((0x03<<8)|(0x03<<0)), ((BIT12|BIT11|BIT10|BIT9|BIT8)|(BIT4|BIT3|BIT2|BIT1|BIT0))); // ODTCOMP CMD/CTL PU/PD
      isbM32m(DDRPHY, (COMPEN0CH0 +  (channel_i * DDRCOMP_CH_OFFSET)), (0), ((BIT31|BIT30)|BIT8)); // COMP

      #ifdef BACKUP_COMPS
      // DQ COMP Overrides
      isbM32m(DDRPHY, (DQDRVPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0A<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // RCOMP PU
      isbM32m(DDRPHY, (DQDRVPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0A<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // RCOMP PD
      isbM32m(DDRPHY, (DQDLYPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x10<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // DCOMP PU
      isbM32m(DDRPHY, (DQDLYPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x10<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // DCOMP PD
      isbM32m(DDRPHY, (DQODTPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0B<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // ODTCOMP PU
      isbM32m(DDRPHY, (DQODTPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0B<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // ODTCOMP PD
      isbM32m(DDRPHY, (DQTCOPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31), (BIT31)); // TCOCOMP PU
      isbM32m(DDRPHY, (DQTCOPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31), (BIT31)); // TCOCOMP PD
      // DQS COMP Overrides
      isbM32m(DDRPHY, (DQSDRVPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0A<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // RCOMP PU
      isbM32m(DDRPHY, (DQSDRVPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0A<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // RCOMP PD
      isbM32m(DDRPHY, (DQSDLYPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x10<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // DCOMP PU
      isbM32m(DDRPHY, (DQSDLYPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x10<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // DCOMP PD
      isbM32m(DDRPHY, (DQSODTPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0B<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // ODTCOMP PU
      isbM32m(DDRPHY, (DQSODTPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0B<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // ODTCOMP PD
      isbM32m(DDRPHY, (DQSTCOPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31), (BIT31)); // TCOCOMP PU
      isbM32m(DDRPHY, (DQSTCOPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31), (BIT31)); // TCOCOMP PD
      // CLK COMP Overrides
      isbM32m(DDRPHY, (CLKDRVPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0C<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // RCOMP PU
      isbM32m(DDRPHY, (CLKDRVPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0C<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // RCOMP PD
      isbM32m(DDRPHY, (CLKDLYPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x07<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // DCOMP PU
      isbM32m(DDRPHY, (CLKDLYPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x07<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // DCOMP PD
      isbM32m(DDRPHY, (CLKODTPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0B<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // ODTCOMP PU
      isbM32m(DDRPHY, (CLKODTPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0B<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // ODTCOMP PD
      isbM32m(DDRPHY, (CLKTCOPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31), (BIT31)); // TCOCOMP PU
      isbM32m(DDRPHY, (CLKTCOPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31), (BIT31)); // TCOCOMP PD
      // CMD COMP Overrides
      isbM32m(DDRPHY, (CMDDRVPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0D<<16)), (BIT31|(BIT21|BIT20|BIT19|BIT18|BIT17|BIT16))); // RCOMP PU
      isbM32m(DDRPHY, (CMDDRVPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0D<<16)), (BIT31|(BIT21|BIT20|BIT19|BIT18|BIT17|BIT16))); // RCOMP PD
      isbM32m(DDRPHY, (CMDDLYPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0A<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // DCOMP PU
      isbM32m(DDRPHY, (CMDDLYPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0A<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // DCOMP PD
      // CTL COMP Overrides
      isbM32m(DDRPHY, (CTLDRVPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0D<<16)), (BIT31|(BIT21|BIT20|BIT19|BIT18|BIT17|BIT16))); // RCOMP PU
      isbM32m(DDRPHY, (CTLDRVPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0D<<16)), (BIT31|(BIT21|BIT20|BIT19|BIT18|BIT17|BIT16))); // RCOMP PD
      isbM32m(DDRPHY, (CTLDLYPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0A<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // DCOMP PU
      isbM32m(DDRPHY, (CTLDLYPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x0A<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // DCOMP PD
      #else
      // DQ TCOCOMP Overrides
      isbM32m(DDRPHY, (DQTCOPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x1F<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // TCOCOMP PU
      isbM32m(DDRPHY, (DQTCOPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x1F<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // TCOCOMP PD
      // DQS TCOCOMP Overrides
      isbM32m(DDRPHY, (DQSTCOPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x1F<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // TCOCOMP PU
      isbM32m(DDRPHY, (DQSTCOPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x1F<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // TCOCOMP PD
      // CLK TCOCOMP Overrides
      isbM32m(DDRPHY, (CLKTCOPUCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x1F<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // TCOCOMP PU
      isbM32m(DDRPHY, (CLKTCOPDCTLCH0 + (channel_i * DDRCOMP_CH_OFFSET)), (BIT31|(0x1F<<16)), (BIT31|(BIT20|BIT19|BIT18|BIT17|BIT16))); // TCOCOMP PD
      #endif // BACKUP_COMPS
      // program STATIC delays
      #ifdef BACKUP_WCMD
      set_wcmd(channel_i, ddr_wcmd[PLATFORM_ID]);
      #else
      set_wcmd(channel_i, ddr_wclk[PLATFORM_ID] + HALF_CLK);
      #endif // BACKUP_WCMD
      for (rank_i=0; rank_i<NUM_RANKS; rank_i++) {
        if (mrc_params->rank_enables & (1<<rank_i)) {
          set_wclk(channel_i, rank_i, ddr_wclk[PLATFORM_ID]);
          #ifdef BACKUP_WCTL
          set_wctl(channel_i, rank_i, ddr_wctl[PLATFORM_ID]);
          #else
          set_wctl(channel_i, rank_i, ddr_wclk[PLATFORM_ID] + HALF_CLK);
          #endif // BACKUP_WCTL
        }
      }
    }
  }
  // COMP (non channel specific)
  //isbM32m(DDRPHY, (), (), ());
  isbM32m(DDRPHY, (DQANADRVPUCTL), (BIT30), (BIT30)); // RCOMP: Dither PU Enable
  isbM32m(DDRPHY, (DQANADRVPDCTL), (BIT30), (BIT30)); // RCOMP: Dither PD Enable
  isbM32m(DDRPHY, (CMDANADRVPUCTL), (BIT30), (BIT30)); // RCOMP: Dither PU Enable
  isbM32m(DDRPHY, (CMDANADRVPDCTL), (BIT30), (BIT30)); // RCOMP: Dither PD Enable
  isbM32m(DDRPHY, (CLKANADRVPUCTL), (BIT30), (BIT30)); // RCOMP: Dither PU Enable
  isbM32m(DDRPHY, (CLKANADRVPDCTL), (BIT30), (BIT30)); // RCOMP: Dither PD Enable
  isbM32m(DDRPHY, (DQSANADRVPUCTL), (BIT30), (BIT30)); // RCOMP: Dither PU Enable
  isbM32m(DDRPHY, (DQSANADRVPDCTL), (BIT30), (BIT30)); // RCOMP: Dither PD Enable
  isbM32m(DDRPHY, (CTLANADRVPUCTL), (BIT30), (BIT30)); // RCOMP: Dither PU Enable
  isbM32m(DDRPHY, (CTLANADRVPDCTL), (BIT30), (BIT30)); // RCOMP: Dither PD Enable
  isbM32m(DDRPHY, (DQANAODTPUCTL), (BIT30), (BIT30)); // ODT: Dither PU Enable
  isbM32m(DDRPHY, (DQANAODTPDCTL), (BIT30), (BIT30)); // ODT: Dither PD Enable
  isbM32m(DDRPHY, (CLKANAODTPUCTL), (BIT30), (BIT30)); // ODT: Dither PU Enable
  isbM32m(DDRPHY, (CLKANAODTPDCTL), (BIT30), (BIT30)); // ODT: Dither PD Enable
  isbM32m(DDRPHY, (DQSANAODTPUCTL), (BIT30), (BIT30)); // ODT: Dither PU Enable
  isbM32m(DDRPHY, (DQSANAODTPDCTL), (BIT30), (BIT30)); // ODT: Dither PD Enable
  isbM32m(DDRPHY, (DQANADLYPUCTL), (BIT30), (BIT30)); // DCOMP: Dither PU Enable
  isbM32m(DDRPHY, (DQANADLYPDCTL), (BIT30), (BIT30)); // DCOMP: Dither PD Enable
  isbM32m(DDRPHY, (CMDANADLYPUCTL), (BIT30), (BIT30)); // DCOMP: Dither PU Enable
  isbM32m(DDRPHY, (CMDANADLYPDCTL), (BIT30), (BIT30)); // DCOMP: Dither PD Enable
  isbM32m(DDRPHY, (CLKANADLYPUCTL), (BIT30), (BIT30)); // DCOMP: Dither PU Enable
  isbM32m(DDRPHY, (CLKANADLYPDCTL), (BIT30), (BIT30)); // DCOMP: Dither PD Enable
  isbM32m(DDRPHY, (DQSANADLYPUCTL), (BIT30), (BIT30)); // DCOMP: Dither PU Enable
  isbM32m(DDRPHY, (DQSANADLYPDCTL), (BIT30), (BIT30)); // DCOMP: Dither PD Enable
  isbM32m(DDRPHY, (CTLANADLYPUCTL), (BIT30), (BIT30)); // DCOMP: Dither PU Enable
  isbM32m(DDRPHY, (CTLANADLYPDCTL), (BIT30), (BIT30)); // DCOMP: Dither PD Enable
  isbM32m(DDRPHY, (DQANATCOPUCTL), (BIT30), (BIT30)); // TCO: Dither PU Enable
  isbM32m(DDRPHY, (DQANATCOPDCTL), (BIT30), (BIT30)); // TCO: Dither PD Enable
  isbM32m(DDRPHY, (CLKANATCOPUCTL), (BIT30), (BIT30)); // TCO: Dither PU Enable
  isbM32m(DDRPHY, (CLKANATCOPDCTL), (BIT30), (BIT30)); // TCO: Dither PD Enable
  isbM32m(DDRPHY, (DQSANATCOPUCTL), (BIT30), (BIT30)); // TCO: Dither PU Enable
  isbM32m(DDRPHY, (DQSANATCOPDCTL), (BIT30), (BIT30)); // TCO: Dither PD Enable
  isbM32m(DDRPHY, (TCOCNTCTRL), (0x1<<0), (BIT1|BIT0)); // TCOCOMP: Pulse Count
  isbM32m(DDRPHY, (CHNLBUFSTATIC), ((0x03<<24)|(0x03<<16)), ((BIT28|BIT27|BIT26|BIT25|BIT24)|(BIT20|BIT19|BIT18|BIT17|BIT16))); // ODT: CMD/CTL PD/PU
  isbM32m(DDRPHY, (MSCNTR), (0x64<<0), (BIT7|BIT6|BIT5|BIT4|BIT3|BIT2|BIT1|BIT0)); // Set 1us counter
  isbM32m(DDRPHY, (LATCH1CTL), (0x1<<28), (BIT30|BIT29|BIT28)); // ???

  // Release PHY from reset
  isbM32m(DDRPHY, MASTERRSTN, BIT0, BIT0); // PHYRSTN=1

  // STEP1:
  post_code(0x03, 0x11);
  for (channel_i=0; channel_i<NUM_CHANNELS; channel_i++) {
    if (mrc_params->channel_enables & (1<<channel_i)) {
      // DQ01-DQ23
      for (bl_grp_i=0; bl_grp_i<((NUM_BYTE_LANES/bl_divisor)/2); bl_grp_i++) {
        isbM32m(DDRPHY, (DQMDLLCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), (BIT13), (BIT13)); // Enable VREG
        delay_n(3);
      }
      // ECC
      isbM32m(DDRPHY, (ECCMDLLCTL), (BIT13), (BIT13)); // Enable VREG
      delay_n(3);
      // CMD
      isbM32m(DDRPHY, (CMDMDLLCTL + (channel_i * DDRIOCCC_CH_OFFSET)), (BIT13), (BIT13)); // Enable VREG
      delay_n(3);
      // CLK-CTL
      isbM32m(DDRPHY, (CCMDLLCTL + (channel_i * DDRIOCCC_CH_OFFSET)), (BIT13), (BIT13)); // Enable VREG
      delay_n(3);
    }
  }

  // STEP2:
  post_code(0x03, 0x12);
  delay_n(200);
  for (channel_i=0; channel_i<NUM_CHANNELS; channel_i++) {
    if (mrc_params->channel_enables & (1<<channel_i)) {
      // DQ01-DQ23
      for (bl_grp_i=0; bl_grp_i<((NUM_BYTE_LANES/bl_divisor)/2); bl_grp_i++) {
        isbM32m(DDRPHY, (DQMDLLCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), (BIT17), (BIT17)); // Enable MCDLL
        delay_n(50);
      }
      // ECC
      isbM32m(DDRPHY, (ECCMDLLCTL), (BIT17), (BIT17)); // Enable MCDLL
      delay_n(50);
      // CMD
      isbM32m(DDRPHY, (CMDMDLLCTL + (channel_i * DDRIOCCC_CH_OFFSET)), (BIT18), (BIT18)); // Enable MCDLL
      delay_n(50);
      // CLK-CTL
      isbM32m(DDRPHY, (CCMDLLCTL + (channel_i * DDRIOCCC_CH_OFFSET)), (BIT18), (BIT18)); // Enable MCDLL
      delay_n(50);
    }
  }

  // STEP3:
  post_code(0x03, 0x13);
  delay_n(100);
  for (channel_i=0; channel_i<NUM_CHANNELS; channel_i++) {
    if (mrc_params->channel_enables & (1<<channel_i)) {
      // DQ01-DQ23
      for (bl_grp_i=0; bl_grp_i<((NUM_BYTE_LANES/bl_divisor)/2); bl_grp_i++) {
#ifdef FORCE_16BIT_DDRIO
        tempD = ((bl_grp_i) && (mrc_params->channel_width == x16)) ? ((0x1<<12)|(0x1<<8)|(0xF<<4)|(0xF<<0)) : ((0xF<<12)|(0xF<<8)|(0xF<<4)|(0xF<<0));
#else
        tempD = ((0xF<<12)|(0xF<<8)|(0xF<<4)|(0xF<<0));
#endif
        isbM32m(DDRPHY, (DQDLLTXCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), (tempD), ((BIT15|BIT14|BIT13|BIT12)|(BIT11|BIT10|BIT9|BIT8)|(BIT7|BIT6|BIT5|BIT4)|(BIT3|BIT2|BIT1|BIT0))); // Enable TXDLL
        delay_n(3);
        isbM32m(DDRPHY, (DQDLLRXCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), (BIT3|BIT2|BIT1|BIT0), (BIT3|BIT2|BIT1|BIT0)); // Enable RXDLL
        delay_n(3);
        isbM32m(DDRPHY, (B0OVRCTL + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), (BIT3|BIT2|BIT1|BIT0), (BIT3|BIT2|BIT1|BIT0)); // Enable RXDLL Overrides BL0
      }

      // ECC
      tempD = ((0xF<<12)|(0xF<<8)|(0xF<<4)|(0xF<<0));
      isbM32m(DDRPHY, (ECCDLLTXCTL), (tempD), ((BIT15|BIT14|BIT13|BIT12)|(BIT11|BIT10|BIT9|BIT8)|(BIT7|BIT6|BIT5|BIT4)|(BIT3|BIT2|BIT1|BIT0))); // Enable TXDLL
      delay_n(3);

      // CMD (PO)
      isbM32m(DDRPHY, (CMDDLLTXCTL + (channel_i * DDRIOCCC_CH_OFFSET)), ((0xF<<12)|(0xF<<8)|(0xF<<4)|(0xF<<0)), ((BIT15|BIT14|BIT13|BIT12)|(BIT11|BIT10|BIT9|BIT8)|(BIT7|BIT6|BIT5|BIT4)|(BIT3|BIT2|BIT1|BIT0))); // Enable TXDLL
      delay_n(3);
    }
  }


  // STEP4:
  post_code(0x03, 0x14);
  for (channel_i=0; channel_i<NUM_CHANNELS; channel_i++) {
    if (mrc_params->channel_enables & (1<<channel_i)) {
      // Host To Memory Clock Alignment (HMC) for 800/1066
      for (bl_grp_i=0; bl_grp_i<((NUM_BYTE_LANES/bl_divisor)/2); bl_grp_i++) {
        isbM32m(DDRPHY, (DQCLKALIGNREG2 + (bl_grp_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), ((bl_grp_i)?(0x3):(0x1)), (BIT3|BIT2|BIT1|BIT0)); // CLK_ALIGN_MOD_ID
      }
      isbM32m(DDRPHY, (ECCCLKALIGNREG2 + (channel_i * DDRIODQ_CH_OFFSET)), 0x2, (BIT3|BIT2|BIT1|BIT0)); // CLK_ALIGN_MOD_ID
      isbM32m(DDRPHY, (CMDCLKALIGNREG2 + (channel_i * DDRIODQ_CH_OFFSET)), 0x0, (BIT3|BIT2|BIT1|BIT0)); // CLK_ALIGN_MOD_ID
      isbM32m(DDRPHY, (CCCLKALIGNREG2 + (channel_i * DDRIODQ_CH_OFFSET)), 0x2, (BIT3|BIT2|BIT1|BIT0)); // CLK_ALIGN_MOD_ID
      isbM32m(DDRPHY, (CMDCLKALIGNREG0 + (channel_i * DDRIOCCC_CH_OFFSET)), (0x2<<4), (BIT5|BIT4)); // CLK_ALIGN_MODE
      isbM32m(DDRPHY, (CMDCLKALIGNREG1 + (channel_i * DDRIOCCC_CH_OFFSET)), ((0x18<<16)|(0x10<<8)|(0x8<<2)|(0x1<<0)), ((BIT22|BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT14|BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)|(BIT7|BIT6|BIT5|BIT4|BIT3|BIT2)|(BIT1|BIT0))); // NUM_SAMPLES, MAX_SAMPLES, MACRO_PI_STEP, MICRO_PI_STEP
      isbM32m(DDRPHY, (CMDCLKALIGNREG2 + (channel_i * DDRIOCCC_CH_OFFSET)), ((0x10<<16)|(0x4<<8)|(0x2<<4)), ((BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT11|BIT10|BIT9|BIT8)|(BIT7|BIT6|BIT5|BIT4))); // ???, TOTAL_NUM_MODULES, FIRST_U_PARTITION
      #ifdef HMC_TEST
      isbM32m(DDRPHY, (CMDCLKALIGNREG0 + (channel_i * DDRIOCCC_CH_OFFSET)), BIT24, BIT24); // START_CLK_ALIGN=1
      while (isbR32m(DDRPHY, (CMDCLKALIGNREG0 + (channel_i * DDRIOCCC_CH_OFFSET))) & BIT24); // wait for START_CLK_ALIGN=0
      #endif // HMC_TEST

      // Set RD/WR Pointer Seperation & COUNTEN & FIFOPTREN
      isbM32m(DDRPHY, (CMDPTRREG + (channel_i * DDRIOCCC_CH_OFFSET)), BIT0, BIT0); // WRPTRENABLE=1


#ifdef SIM
      // comp is not working on simulator
#else
      // COMP initial
      isbM32m(DDRPHY, (COMPEN0CH0 + (channel_i * DDRCOMP_CH_OFFSET)), BIT5, BIT5); // enable bypass for CLK buffer (PO)
      isbM32m(DDRPHY, (CMPCTRL), (BIT0), (BIT0)); // Initial COMP Enable
      while (isbR32m(DDRPHY, (CMPCTRL)) & BIT0); // wait for Initial COMP Enable = 0
      isbM32m(DDRPHY, (COMPEN0CH0 + (channel_i * DDRCOMP_CH_OFFSET)), ~BIT5, BIT5); // disable bypass for CLK buffer (PO)
#endif

      // IOBUFACT
      // STEP4a
      isbM32m(DDRPHY, (CMDCFGREG0 + (channel_i * DDRIOCCC_CH_OFFSET)), BIT2, BIT2); // IOBUFACTRST_N=1

      // DDRPHY initialisation complete
      isbM32m(DDRPHY, (CMDPMCONFIG0 + (channel_i * DDRIOCCC_CH_OFFSET)), BIT20, BIT20); // SPID_INIT_COMPLETE=1
    }
  }

  LEAVEFN();
  return;
}

// jedec_init (aka PerformJedecInit):
// This function performs JEDEC initialisation on all enabled channels.
static void jedec_init(
    MRCParams_t *mrc_params,
    uint32_t silent)
{
  uint8_t TWR, WL, Rank;
  uint32_t TCK;

  RegDTR0 DTR0reg;

  DramInitDDR3MRS0 mrs0Command;
  DramInitDDR3EMR1 emrs1Command;
  DramInitDDR3EMR2 emrs2Command;
  DramInitDDR3EMR3 emrs3Command;

  ENTERFN();

  // jedec_init starts
  if (!silent)
  {
    post_code(0x04, 0x00);
  }

  // Assert RESET# for 200us
  isbM32m(DDRPHY, CCDDR3RESETCTL, BIT1, (BIT8|BIT1)); // DDR3_RESET_SET=0, DDR3_RESET_RESET=1
#ifdef QUICKSIM
      // Don't waste time during simulation
      delay_u(2);
#else
  delay_u(200);
#endif
  isbM32m(DDRPHY, CCDDR3RESETCTL, BIT8, (BIT8|BIT1)); // DDR3_RESET_SET=1, DDR3_RESET_RESET=0

  DTR0reg.raw = isbR32m(MCU, DTR0);

  // Set CKEVAL for populated ranks
  // then send NOP to each rank (#4550197)
  {
    uint32_t DRPbuffer;
    uint32_t DRMCbuffer;

    DRPbuffer = isbR32m(MCU, DRP);
    DRPbuffer &= 0x3;
    DRMCbuffer = isbR32m(MCU, DRMC);
    DRMCbuffer &= 0xFFFFFFFC;
    DRMCbuffer |= (BIT4 | DRPbuffer);

    isbW32m(MCU, DRMC, DRMCbuffer);

    for (Rank = 0; Rank < NUM_RANKS; Rank++)
    {
      // Skip to next populated rank
      if ((mrc_params->rank_enables & (1 << Rank)) == 0)
      {
        continue;
      }

      dram_init_command(DCMD_NOP(Rank));
    }

    isbW32m(MCU, DRMC, DRMC_DEFAULT);
  }

  // setup for emrs 2
  // BIT[15:11] --> Always "0"
  // BIT[10:09] --> Rtt_WR: want "Dynamic ODT Off" (0)
  // BIT[08]    --> Always "0"
  // BIT[07]    --> SRT: use sr_temp_range
  // BIT[06]    --> ASR: want "Manual SR Reference" (0)
  // BIT[05:03] --> CWL: use oem_tCWL
  // BIT[02:00] --> PASR: want "Full Array" (0)
  emrs2Command.raw = 0;
  emrs2Command.field.bankAddress = 2;

  WL = 5 + mrc_params->ddr_speed;
  emrs2Command.field.CWL = WL - 5;
  emrs2Command.field.SRT = mrc_params->sr_temp_range;

  // setup for emrs 3
  // BIT[15:03] --> Always "0"
  // BIT[02]    --> MPR: want "Normal Operation" (0)
  // BIT[01:00] --> MPR_Loc: want "Predefined Pattern" (0)
  emrs3Command.raw = 0;
  emrs3Command.field.bankAddress = 3;

  // setup for emrs 1
  // BIT[15:13]     --> Always "0"
  // BIT[12:12]     --> Qoff: want "Output Buffer Enabled" (0)
  // BIT[11:11]     --> TDQS: want "Disabled" (0)
  // BIT[10:10]     --> Always "0"
  // BIT[09,06,02]  --> Rtt_nom: use rtt_nom_value
  // BIT[08]        --> Always "0"
  // BIT[07]        --> WR_LVL: want "Disabled" (0)
  // BIT[05,01]     --> DIC: use ron_value
  // BIT[04:03]     --> AL: additive latency want "0" (0)
  // BIT[00]        --> DLL: want "Enable" (0)
  //
  // (BIT5|BIT1) set Ron value
  // 00 --> RZQ/6 (40ohm)
  // 01 --> RZQ/7 (34ohm)
  // 1* --> RESERVED
  //
  // (BIT9|BIT6|BIT2) set Rtt_nom value
  // 000 --> Disabled
  // 001 --> RZQ/4 ( 60ohm)
  // 010 --> RZQ/2 (120ohm)
  // 011 --> RZQ/6 ( 40ohm)
  // 1** --> RESERVED
  emrs1Command.raw = 0;
  emrs1Command.field.bankAddress = 1;
  emrs1Command.field.dllEnabled = 0; // 0 = Enable , 1 = Disable

  if (mrc_params->ron_value == 0)
  {
    emrs1Command.field.DIC0 = DDR3_EMRS1_DIC_34;
  }
  else
  {
    emrs1Command.field.DIC0 = DDR3_EMRS1_DIC_40;
  }


  if (mrc_params->rtt_nom_value == 0)
  {
    emrs1Command.raw |= (DDR3_EMRS1_RTTNOM_40 << 6);
  }
  else if (mrc_params->rtt_nom_value == 1)
  {
    emrs1Command.raw |= (DDR3_EMRS1_RTTNOM_60 << 6);
  }
  else if (mrc_params->rtt_nom_value == 2)
  {
    emrs1Command.raw |= (DDR3_EMRS1_RTTNOM_120 << 6);
  }

  // save MRS1 value (excluding control fields)
  mrc_params->mrs1 = emrs1Command.raw >> 6;

  // setup for mrs 0
  // BIT[15:13]     --> Always "0"
  // BIT[12]        --> PPD: for Quark (1)
  // BIT[11:09]     --> WR: use oem_tWR
  // BIT[08]        --> DLL: want "Reset" (1, self clearing)
  // BIT[07]        --> MODE: want "Normal" (0)
  // BIT[06:04,02]  --> CL: use oem_tCAS
  // BIT[03]        --> RD_BURST_TYPE: want "Interleave" (1)
  // BIT[01:00]     --> BL: want "8 Fixed" (0)
  // WR:
  // 0 --> 16
  // 1 --> 5
  // 2 --> 6
  // 3 --> 7
  // 4 --> 8
  // 5 --> 10
  // 6 --> 12
  // 7 --> 14
  // CL:
  // BIT[02:02] "0" if oem_tCAS <= 11 (1866?)
  // BIT[06:04] use oem_tCAS-4
  mrs0Command.raw = 0;
  mrs0Command.field.bankAddress = 0;
  mrs0Command.field.dllReset = 1;
  mrs0Command.field.BL = 0;
  mrs0Command.field.PPD = 1;
  mrs0Command.field.casLatency = DTR0reg.field.tCL + 1;

  TCK = tCK[mrc_params->ddr_speed];
  TWR = MCEIL(15000, TCK);   // Per JEDEC: tWR=15000ps DDR2/3 from 800-1600
  mrs0Command.field.writeRecovery = TWR - 4;

  for (Rank = 0; Rank < NUM_RANKS; Rank++)
  {
    // Skip to next populated rank
    if ((mrc_params->rank_enables & (1 << Rank)) == 0)
    {
      continue;
    }

    emrs2Command.field.rankSelect = Rank;
    dram_init_command(emrs2Command.raw);

    emrs3Command.field.rankSelect = Rank;
    dram_init_command(emrs3Command.raw);

    emrs1Command.field.rankSelect = Rank;
    dram_init_command(emrs1Command.raw);

    mrs0Command.field.rankSelect = Rank;
    dram_init_command(mrs0Command.raw);

    dram_init_command(DCMD_ZQCL(Rank));
  }

  LEAVEFN();
  return;
}

// rcvn_cal:
// POST_CODE[major] == 0x05
//
// This function will perform our RCVEN Calibration Algorithm.
// We will only use the 2xCLK domain timings to perform RCVEN Calibration.
// All byte lanes will be calibrated "simultaneously" per channel per rank.
static void rcvn_cal(
    MRCParams_t *mrc_params)
{
  uint8_t channel_i; // channel counter
  uint8_t rank_i; // rank counter
  uint8_t bl_i; // byte lane counter
  uint8_t bl_divisor = (mrc_params->channel_width == x16) ? 2 : 1; // byte lane divisor

#ifdef R2R_SHARING
  uint32_t final_delay[NUM_CHANNELS][NUM_BYTE_LANES]; // used to find placement for rank2rank sharing configs
#ifndef BACKUP_RCVN
  uint32_t num_ranks_enabled = 0; // used to find placement for rank2rank sharing configs
#endif // BACKUP_RCVN
#endif // R2R_SHARING

#ifdef BACKUP_RCVN
#else
  uint32_t tempD; // temporary DWORD
  uint32_t delay[NUM_BYTE_LANES]; // absolute PI value to be programmed on the byte lane
  RegDTR1 dtr1;
  RegDTR1 dtr1save;
#endif // BACKUP_RCVN
  ENTERFN();

  // rcvn_cal starts
  post_code(0x05, 0x00);

#ifndef BACKUP_RCVN
  // need separate burst to sample DQS preamble
  dtr1.raw = dtr1save.raw = isbR32m(MCU, DTR1);
  dtr1.field.tCCD = 1;
  isbW32m(MCU, DTR1, dtr1.raw);
#endif

#ifdef R2R_SHARING
  // need to set "final_delay[][]" elements to "0"
  memset((void *) (final_delay), 0x00, (size_t) sizeof(final_delay));
#endif // R2R_SHARING

  // loop through each enabled channel
  for (channel_i = 0; channel_i < NUM_CHANNELS; channel_i++)
  {
    if (mrc_params->channel_enables & (1 << channel_i))
    {
      // perform RCVEN Calibration on a per rank basis
      for (rank_i = 0; rank_i < NUM_RANKS; rank_i++)
      {
        if (mrc_params->rank_enables & (1 << rank_i))
        {
          // POST_CODE here indicates the current channel and rank being calibrated
          post_code(0x05, (0x10 + ((channel_i << 4) | rank_i)));

#ifdef BACKUP_RCVN
          // set hard-coded timing values
          for (bl_i=0; bl_i<(NUM_BYTE_LANES/bl_divisor); bl_i++)
          {
            set_rcvn(channel_i, rank_i, bl_i, ddr_rcvn[PLATFORM_ID]);
          }
#else
          // enable FIFORST
          for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i += 2)
          {
            isbM32m(DDRPHY, (B01PTRCTL1 + ((bl_i >> 1) * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), 0,
                BIT8); // 0 is enabled
          } // bl_i loop
          // initialise the starting delay to 128 PI (tCAS +1 CLK)
          for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
          {
#ifdef SIM
            // Original value was late at the end of DQS sequence
            delay[bl_i] = 3 * FULL_CLK;
#else
            delay[bl_i] = (4 + 1) * FULL_CLK; // 1x CLK domain timing is tCAS-4
#endif

            set_rcvn(channel_i, rank_i, bl_i, delay[bl_i]);
          } // bl_i loop

          // now find the rising edge
          find_rising_edge(mrc_params, delay, channel_i, rank_i, true);
          // Now increase delay by 32 PI (1/4 CLK) to place in center of high pulse.
          for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
          {
            delay[bl_i] += QRTR_CLK;
            set_rcvn(channel_i, rank_i, bl_i, delay[bl_i]);
          } // bl_i loop
          // Now decrement delay by 128 PI (1 CLK) until we sample a "0"
          do
          {

            tempD = sample_dqs(mrc_params, channel_i, rank_i, true);
            for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
            {
              if (tempD & (1 << bl_i))
              {
                if (delay[bl_i] >= FULL_CLK)
                {
                  delay[bl_i] -= FULL_CLK;
                  set_rcvn(channel_i, rank_i, bl_i, delay[bl_i]);
                }
                else
                {
                  // not enough delay
                  training_message(channel_i, rank_i, bl_i);
                  post_code(0xEE, 0x50);
                }
              }
            } // bl_i loop
          } while (tempD & 0xFF);

#ifdef R2R_SHARING
          // increment "num_ranks_enabled"
          num_ranks_enabled++;
          // Finally increment delay by 32 PI (1/4 CLK) to place in center of preamble.
          for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
          {
            delay[bl_i] += QRTR_CLK;
            // add "delay[]" values to "final_delay[][]" for rolling average
            final_delay[channel_i][bl_i] += delay[bl_i];
            // set timing based on rolling average values
            set_rcvn(channel_i, rank_i, bl_i, ((final_delay[channel_i][bl_i]) / num_ranks_enabled));
          } // bl_i loop
#else
          // Finally increment delay by 32 PI (1/4 CLK) to place in center of preamble.
          for (bl_i=0; bl_i<(NUM_BYTE_LANES/bl_divisor); bl_i++)
          {
            delay[bl_i] += QRTR_CLK;
            set_rcvn(channel_i, rank_i, bl_i, delay[bl_i]);
          } // bl_i loop

#endif // R2R_SHARING

          // disable FIFORST
          for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i += 2)
          {
            isbM32m(DDRPHY, (B01PTRCTL1 + ((bl_i >> 1) * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)), BIT8,
                BIT8); // 1 is disabled
          } // bl_i loop

#endif // BACKUP_RCVN

        } // if rank is enabled
      } // rank_i loop
    } // if channel is enabled
  } // channel_i loop

#ifndef BACKUP_RCVN
  // restore original
  isbW32m(MCU, DTR1, dtr1save.raw);
#endif

#ifdef MRC_SV
  if (mrc_params->tune_rcvn)
  {
    uint32_t rcven, val;
    uint32_t rdcmd2rcven;

    /*
     Formulas for RDCMD2DATAVALID & DIFFAMP dynamic timings

     1. Set after RCVEN training

     //Tune RDCMD2DATAVALID

     x80/x84[21:16]
     MAX OF 2 RANKS : round up (rdcmd2rcven (rcven 1x) + 2x x 2 + PI/128) + 5

     //rdcmd2rcven x80/84[12:8]
     //rcven 2x x70[23:20] & [11:8]

     //Tune DIFFAMP Timings

     //diffampen launch x88[20:16] & [4:0]  -- B01LATCTL1
     MIN OF 2 RANKS : round down (rcven 1x + 2x x 2 + PI/128) - 1

     //diffampen length x8C/x90 [13:8]   -- B0ONDURCTL B1ONDURCTL
     MAX OF 2 RANKS : roundup (rcven 1x + 2x x 2 + PI/128) + 5


     2. need to do a fiforst after settings these values
    */

    DPF(D_INFO, "BEFORE\n");
    DPF(D_INFO, "### %x\n", isbR32m(DDRPHY, B0LATCTL0));
    DPF(D_INFO, "### %x\n", isbR32m(DDRPHY, B01LATCTL1));
    DPF(D_INFO, "### %x\n", isbR32m(DDRPHY, B0ONDURCTL));

    DPF(D_INFO, "### %x\n", isbR32m(DDRPHY, B1LATCTL0));
    DPF(D_INFO, "### %x\n", isbR32m(DDRPHY, B1ONDURCTL));

    rcven = get_rcvn(0, 0, 0) / 128;
    rdcmd2rcven = (isbR32m(DDRPHY, B0LATCTL0) >> 8) & 0x1F;
    val = rdcmd2rcven + rcven + 6;
    isbM32m(DDRPHY, B0LATCTL0, val << 16, (BIT21|BIT20|BIT19|BIT18|BIT17|BIT16));

    val = rdcmd2rcven + rcven - 1;
    isbM32m(DDRPHY, B01LATCTL1, val << 0, (BIT4|BIT3|BIT2|BIT1|BIT0));

    val = rdcmd2rcven + rcven + 5;
    isbM32m(DDRPHY, B0ONDURCTL, val << 8, (BIT13|BIT12|BIT11|BIT10|BIT9|BIT8));

    rcven = get_rcvn(0, 0, 1) / 128;
    rdcmd2rcven = (isbR32m(DDRPHY, B1LATCTL0) >> 8) & 0x1F;
    val = rdcmd2rcven + rcven + 6;
    isbM32m(DDRPHY, B1LATCTL0, val << 16, (BIT21|BIT20|BIT19|BIT18|BIT17|BIT16));

    val = rdcmd2rcven + rcven - 1;
    isbM32m(DDRPHY, B01LATCTL1, val << 16, (BIT20|BIT19|BIT18|BIT17|BIT16));

    val = rdcmd2rcven + rcven + 5;
    isbM32m(DDRPHY, B1ONDURCTL, val << 8, (BIT13|BIT12|BIT11|BIT10|BIT9|BIT8));

    DPF(D_INFO, "AFTER\n");
    DPF(D_INFO, "### %x\n", isbR32m(DDRPHY, B0LATCTL0));
    DPF(D_INFO, "### %x\n", isbR32m(DDRPHY, B01LATCTL1));
    DPF(D_INFO, "### %x\n", isbR32m(DDRPHY, B0ONDURCTL));

    DPF(D_INFO, "### %x\n", isbR32m(DDRPHY, B1LATCTL0));
    DPF(D_INFO, "### %x\n", isbR32m(DDRPHY, B1ONDURCTL));

    DPF(D_INFO, "\nPress a key\n");
    mgetc();

    // fifo reset
    isbM32m(DDRPHY, B01PTRCTL1, 0, BIT8); // 0 is enabled
    delay_n(3);
    isbM32m(DDRPHY, B01PTRCTL1, BIT8, BIT8); // 1 is disabled
  }
#endif

  LEAVEFN();
  return;
}

// Check memory executing write/read/verify of many data patterns
// at the specified address. Bits in the result indicate failure
// on specific byte lane.
static uint32_t check_bls_ex(
    MRCParams_t *mrc_params,
    uint32_t address)
{
  uint32_t result;
  uint8_t first_run = 0;

  if (mrc_params->hte_setup)
  {
    mrc_params->hte_setup = 0;

    first_run = 1;
    select_hte(mrc_params);
  }

  result = WriteStressBitLanesHTE(mrc_params, address, first_run);

  DPF(D_TRN, "check_bls_ex result is %x\n", result);
  return result;
}

// Check memory executing simple write/read/verify at
// the specified address. Bits in the result indicate failure
// on specific byte lane.
static uint32_t check_rw_coarse(
    MRCParams_t *mrc_params,
    uint32_t address)
{
  uint32_t result = 0;
  uint8_t first_run = 0;

  if (mrc_params->hte_setup)
  {
    mrc_params->hte_setup = 0;

    first_run = 1;
    select_hte(mrc_params);
  }

  result = BasicWriteReadHTE(mrc_params, address, first_run, WRITE_TRAIN);

  DPF(D_TRN, "check_rw_coarse result is %x\n", result);
  return result;
}

// wr_level:
// POST_CODE[major] == 0x06
//
// This function will perform the Write Levelling algorithm (align WCLK and WDQS).
// This algorithm will act on each rank in each channel separately.
static void wr_level(
    MRCParams_t *mrc_params)
{
  uint8_t channel_i; // channel counter
  uint8_t rank_i; // rank counter
  uint8_t bl_i; // byte lane counter
  uint8_t bl_divisor = (mrc_params->channel_width == x16) ? 2 : 1; // byte lane divisor

#ifdef R2R_SHARING
  uint32_t final_delay[NUM_CHANNELS][NUM_BYTE_LANES]; // used to find placement for rank2rank sharing configs
#ifndef BACKUP_WDQS
  uint32_t num_ranks_enabled = 0; // used to find placement for rank2rank sharing configs
#endif // BACKUP_WDQS
#endif // R2R_SHARING

#ifdef BACKUP_WDQS
#else
  bool all_edges_found; // determines stop condition for CRS_WR_LVL
  uint32_t delay[NUM_BYTE_LANES]; // absolute PI value to be programmed on the byte lane
  // static makes it so the data is loaded in the heap once by shadow(), where
  // non-static copies the data onto the stack every time this function is called.

  uint32_t address; // address to be checked during COARSE_WR_LVL
  RegDTR4 dtr4;
  RegDTR4 dtr4save;
#endif // BACKUP_WDQS

  ENTERFN();

  // wr_level starts
  post_code(0x06, 0x00);

#ifdef R2R_SHARING
  // need to set "final_delay[][]" elements to "0"
  memset((void *) (final_delay), 0x00, (size_t) sizeof(final_delay));
#endif // R2R_SHARING
  // loop through each enabled channel
  for (channel_i = 0; channel_i < NUM_CHANNELS; channel_i++)
  {
    if (mrc_params->channel_enables & (1 << channel_i))
    {
      // perform WRITE LEVELING algorithm on a per rank basis
      for (rank_i = 0; rank_i < NUM_RANKS; rank_i++)
      {
        if (mrc_params->rank_enables & (1 << rank_i))
        {
          // POST_CODE here indicates the current rank and channel being calibrated
          post_code(0x06, (0x10 + ((channel_i << 4) | rank_i)));

#ifdef BACKUP_WDQS
          for (bl_i=0; bl_i<(NUM_BYTE_LANES/bl_divisor); bl_i++)
          {
            set_wdqs(channel_i, rank_i, bl_i, ddr_wdqs[PLATFORM_ID]);
            set_wdq(channel_i, rank_i, bl_i, (ddr_wdqs[PLATFORM_ID] - QRTR_CLK));
          }
#else

          { // Begin product specific code

            // perform a single PRECHARGE_ALL command to make DRAM state machine go to IDLE state
            dram_init_command(DCMD_PREA(rank_i));

            // enable Write Levelling Mode (EMRS1 w/ Write Levelling Mode Enable)
            dram_init_command(DCMD_MRS1(rank_i,0x0082));

            // set ODT DRAM Full Time Termination disable in MCU
            dtr4.raw = dtr4save.raw = isbR32m(MCU, DTR4);
            dtr4.field.ODTDIS = 1;
            isbW32m(MCU, DTR4, dtr4.raw);

            for (bl_i = 0; bl_i < ((NUM_BYTE_LANES / bl_divisor) / 2); bl_i++)
            {
              isbM32m(DDRPHY, DQCTL + (DDRIODQ_BL_OFFSET * bl_i) + (DDRIODQ_CH_OFFSET * channel_i),
                  (BIT28 | (0x1 << 8) | (0x1 << 6) | (0x1 << 4) | (0x1 << 2)),
                  (BIT28 | (BIT9|BIT8) | (BIT7|BIT6) | (BIT5|BIT4) | (BIT3|BIT2))); // Enable Sandy Bridge Mode (WDQ Tri-State) & Ensure 5 WDQS pulses during Write Leveling
            }

            isbM32m(DDRPHY, CCDDR3RESETCTL + (DDRIOCCC_CH_OFFSET * channel_i), (BIT16), (BIT16)); // Write Leveling Mode enabled in IO
          } // End product specific code
          // Initialise the starting delay to WCLK
          for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
          {
            { // Begin product specific code
              // CLK0 --> RK0
              // CLK1 --> RK1
              delay[bl_i] = get_wclk(channel_i, rank_i);
            } // End product specific code
            set_wdqs(channel_i, rank_i, bl_i, delay[bl_i]);
          } // bl_i loop
          // now find the rising edge
          find_rising_edge(mrc_params, delay, channel_i, rank_i, false);
          { // Begin product specific code
            // disable Write Levelling Mode
            isbM32m(DDRPHY, CCDDR3RESETCTL + (DDRIOCCC_CH_OFFSET * channel_i), (0), (BIT16)); // Write Leveling Mode disabled in IO

            for (bl_i = 0; bl_i < ((NUM_BYTE_LANES / bl_divisor) / 2); bl_i++)
            {
              isbM32m(DDRPHY, DQCTL + (DDRIODQ_BL_OFFSET * bl_i) + (DDRIODQ_CH_OFFSET * channel_i),
                  ((0x1 << 8) | (0x1 << 6) | (0x1 << 4) | (0x1 << 2)),
                  (BIT28 | (BIT9|BIT8) | (BIT7|BIT6) | (BIT5|BIT4) | (BIT3|BIT2))); // Disable Sandy Bridge Mode & Ensure 4 WDQS pulses during normal operation
            } // bl_i loop

            // restore original DTR4
            isbW32m(MCU, DTR4, dtr4save.raw);

            // restore original value (Write Levelling Mode Disable)
            dram_init_command(DCMD_MRS1(rank_i, mrc_params->mrs1));

            // perform a single PRECHARGE_ALL command to make DRAM state machine go to IDLE state
            dram_init_command(DCMD_PREA(rank_i));
          } // End product specific code

          post_code(0x06, (0x30 + ((channel_i << 4) | rank_i)));

          // COARSE WRITE LEVEL:
          // check that we're on the correct clock edge

          // hte reconfiguration request
          mrc_params->hte_setup = 1;

          // start CRS_WR_LVL with WDQS = WDQS + 128 PI
          for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
          {
            delay[bl_i] = get_wdqs(channel_i, rank_i, bl_i) + FULL_CLK;
            set_wdqs(channel_i, rank_i, bl_i, delay[bl_i]);
            // program WDQ timings based on WDQS (WDQ = WDQS - 32 PI)
            set_wdq(channel_i, rank_i, bl_i, (delay[bl_i] - QRTR_CLK));
          } // bl_i loop

          // get an address in the targeted channel/rank
          address = get_addr(mrc_params, channel_i, rank_i);
          do
          {
            uint32_t coarse_result = 0x00;
            uint32_t coarse_result_mask = byte_lane_mask(mrc_params);
            all_edges_found = true; // assume pass

#ifdef SIM
            // need restore memory to idle state as write can be in bad sync
            dram_init_command (DCMD_PREA(rank_i));
#endif

            mrc_params->hte_setup = 1;
            coarse_result = check_rw_coarse(mrc_params, address);

            // check for failures and margin the byte lane back 128 PI (1 CLK)
            for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
            {
              if (coarse_result & (coarse_result_mask << bl_i))
              {
                all_edges_found = false;
                delay[bl_i] -= FULL_CLK;
                set_wdqs(channel_i, rank_i, bl_i, delay[bl_i]);
                // program WDQ timings based on WDQS (WDQ = WDQS - 32 PI)
                set_wdq(channel_i, rank_i, bl_i, (delay[bl_i] - QRTR_CLK));
              }
            } // bl_i loop

          } while (!all_edges_found);

#ifdef R2R_SHARING
          // increment "num_ranks_enabled"
          num_ranks_enabled++;
          // accumulate "final_delay[][]" values from "delay[]" values for rolling average
          for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
          {
            final_delay[channel_i][bl_i] += delay[bl_i];
            set_wdqs(channel_i, rank_i, bl_i, ((final_delay[channel_i][bl_i]) / num_ranks_enabled));
            // program WDQ timings based on WDQS (WDQ = WDQS - 32 PI)
            set_wdq(channel_i, rank_i, bl_i, ((final_delay[channel_i][bl_i]) / num_ranks_enabled) - QRTR_CLK);
          } // bl_i loop
#endif // R2R_SHARING
#endif // BACKUP_WDQS

        } // if rank is enabled
      } // rank_i loop
    } // if channel is enabled
  } // channel_i loop

  LEAVEFN();
  return;
}

// rd_train:
// POST_CODE[major] == 0x07
//
// This function will perform the READ TRAINING Algorithm on all channels/ranks/byte_lanes simultaneously to minimize execution time.
// The idea here is to train the VREF and RDQS (and eventually RDQ) values to achieve maximum READ margins.
// The algorithm will first determine the X coordinate (RDQS setting).
// This is done by collapsing the VREF eye until we find a minimum required RDQS eye for VREF_MIN and VREF_MAX.
// Then we take the averages of the RDQS eye at VREF_MIN and VREF_MAX, then average those; this will be the final X coordinate.
// The algorithm will then determine the Y coordinate (VREF setting).
// This is done by collapsing the RDQS eye until we find a minimum required VREF eye for RDQS_MIN and RDQS_MAX.
// Then we take the averages of the VREF eye at RDQS_MIN and RDQS_MAX, then average those; this will be the final Y coordinate.
// NOTE: this algorithm assumes the eye curves have a one-to-one relationship, meaning for each X the curve has only one Y and vice-a-versa.
static void rd_train(
    MRCParams_t *mrc_params)
{

#define MIN_RDQS_EYE 10 // in PI Codes
#define MIN_VREF_EYE 10 // in VREF Codes
#define RDQS_STEP 1     // how many RDQS codes to jump while margining
#define VREF_STEP 1     // how many VREF codes to jump while margining
#define VREF_MIN (0x00) // offset into "vref_codes[]" for minimum allowed VREF setting
#define VREF_MAX (0x3F) // offset into "vref_codes[]" for maximum allowed VREF setting
#define RDQS_MIN (0x00) // minimum RDQS delay value
#define RDQS_MAX (0x3F) // maximum RDQS delay value
#define B 0 // BOTTOM VREF
#define T 1 // TOP VREF
#define L 0 // LEFT RDQS
#define R 1 // RIGHT RDQS

  uint8_t channel_i; // channel counter
  uint8_t rank_i; // rank counter
  uint8_t bl_i; // byte lane counter
  uint8_t bl_divisor = (mrc_params->channel_width == x16) ? 2 : 1; // byte lane divisor
#ifdef BACKUP_RDQS
#else
  uint8_t side_x; // tracks LEFT/RIGHT approach vectors
  uint8_t side_y; // tracks BOTTOM/TOP approach vectors
  uint8_t x_coordinate[2/*side_x*/][2/*side_y*/][NUM_CHANNELS][NUM_RANKS][NUM_BYTE_LANES]; // X coordinate data (passing RDQS values) for approach vectors
  uint8_t y_coordinate[2/*side_x*/][2/*side_y*/][NUM_CHANNELS][NUM_BYTE_LANES]; // Y coordinate data (passing VREF values) for approach vectors
  uint8_t x_center[NUM_CHANNELS][NUM_RANKS][NUM_BYTE_LANES]; // centered X (RDQS)
  uint8_t y_center[NUM_CHANNELS][NUM_BYTE_LANES]; // centered Y (VREF)
  uint32_t address; // target address for "check_bls_ex()"
  uint32_t result; // result of "check_bls_ex()"
  uint32_t bl_mask; // byte lane mask for "result" checking
#ifdef R2R_SHARING
  uint32_t final_delay[NUM_CHANNELS][NUM_BYTE_LANES]; // used to find placement for rank2rank sharing configs
  uint32_t num_ranks_enabled = 0; // used to find placement for rank2rank sharing configs
#endif // R2R_SHARING
#endif // BACKUP_RDQS
  // rd_train starts
  post_code(0x07, 0x00);

  ENTERFN();

#ifdef BACKUP_RDQS
  for (channel_i=0; channel_i<NUM_CHANNELS; channel_i++)
  {
    if (mrc_params->channel_enables & (1<<channel_i))
    {
      for (rank_i=0; rank_i<NUM_RANKS; rank_i++)
      {
        if (mrc_params->rank_enables & (1<<rank_i))
        {
          for (bl_i=0; bl_i<(NUM_BYTE_LANES/bl_divisor); bl_i++)
          {
            set_rdqs(channel_i, rank_i, bl_i, ddr_rdqs[PLATFORM_ID]);
          } // bl_i loop
        } // if rank is enabled
      } // rank_i loop
    } // if channel is enabled
  } // channel_i loop
#else
  // initialise x/y_coordinate arrays
  for (channel_i = 0; channel_i < NUM_CHANNELS; channel_i++)
  {
    if (mrc_params->channel_enables & (1 << channel_i))
    {
      for (rank_i = 0; rank_i < NUM_RANKS; rank_i++)
      {
        if (mrc_params->rank_enables & (1 << rank_i))
        {
          for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
          {
            // x_coordinate:
            x_coordinate[L][B][channel_i][rank_i][bl_i] = RDQS_MIN;
            x_coordinate[R][B][channel_i][rank_i][bl_i] = RDQS_MAX;
            x_coordinate[L][T][channel_i][rank_i][bl_i] = RDQS_MIN;
            x_coordinate[R][T][channel_i][rank_i][bl_i] = RDQS_MAX;
            // y_coordinate:
            y_coordinate[L][B][channel_i][bl_i] = VREF_MIN;
            y_coordinate[R][B][channel_i][bl_i] = VREF_MIN;
            y_coordinate[L][T][channel_i][bl_i] = VREF_MAX;
            y_coordinate[R][T][channel_i][bl_i] = VREF_MAX;
          } // bl_i loop
        } // if rank is enabled
      } // rank_i loop
    } // if channel is enabled
  } // channel_i loop

  // initialise other variables
  bl_mask = byte_lane_mask(mrc_params);
  address = get_addr(mrc_params, 0, 0);

#ifdef R2R_SHARING
  // need to set "final_delay[][]" elements to "0"
  memset((void *) (final_delay), 0x00, (size_t) sizeof(final_delay));
#endif // R2R_SHARING

  // look for passing coordinates
  for (side_y = B; side_y <= T; side_y++)
  {
    for (side_x = L; side_x <= R; side_x++)
    {

      post_code(0x07, (0x10 + (side_y * 2) + (side_x)));

      // find passing values
      for (channel_i = 0; channel_i < NUM_CHANNELS; channel_i++)
      {
        if (mrc_params->channel_enables & (0x1 << channel_i))
        {
          for (rank_i = 0; rank_i < NUM_RANKS; rank_i++)
          {

            if (mrc_params->rank_enables & (0x1 << rank_i))
            {
              // set x/y_coordinate search starting settings
              for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
              {
                set_rdqs(channel_i, rank_i, bl_i, x_coordinate[side_x][side_y][channel_i][rank_i][bl_i]);
                set_vref(channel_i, bl_i, y_coordinate[side_x][side_y][channel_i][bl_i]);
              } // bl_i loop
              // get an address in the target channel/rank
              address = get_addr(mrc_params, channel_i, rank_i);

              // request HTE reconfiguration
              mrc_params->hte_setup = 1;

              // test the settings
              do
              {

                // result[07:00] == failing byte lane (MAX 8)
                result = check_bls_ex( mrc_params, address);

                // check for failures
                if (result & 0xFF)
                {
                  // at least 1 byte lane failed
                  for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
                  {
                    if (result & (bl_mask << bl_i))
                    {
                      // adjust the RDQS values accordingly
                      if (side_x == L)
                      {
                        x_coordinate[L][side_y][channel_i][rank_i][bl_i] += RDQS_STEP;
                      }
                      else
                      {
                        x_coordinate[R][side_y][channel_i][rank_i][bl_i] -= RDQS_STEP;
                      }
                      // check that we haven't closed the RDQS_EYE too much
                      if ((x_coordinate[L][side_y][channel_i][rank_i][bl_i] > (RDQS_MAX - MIN_RDQS_EYE)) ||
                          (x_coordinate[R][side_y][channel_i][rank_i][bl_i] < (RDQS_MIN + MIN_RDQS_EYE))
                          ||
                          (x_coordinate[L][side_y][channel_i][rank_i][bl_i]
                              == x_coordinate[R][side_y][channel_i][rank_i][bl_i]))
                      {
                        // not enough RDQS margin available at this VREF
                        // update VREF values accordingly
                        if (side_y == B)
                        {
                          y_coordinate[side_x][B][channel_i][bl_i] += VREF_STEP;
                        }
                        else
                        {
                          y_coordinate[side_x][T][channel_i][bl_i] -= VREF_STEP;
                        }
                        // check that we haven't closed the VREF_EYE too much
                        if ((y_coordinate[side_x][B][channel_i][bl_i] > (VREF_MAX - MIN_VREF_EYE)) ||
                            (y_coordinate[side_x][T][channel_i][bl_i] < (VREF_MIN + MIN_VREF_EYE)) ||
                            (y_coordinate[side_x][B][channel_i][bl_i] == y_coordinate[side_x][T][channel_i][bl_i]))
                        {
                          // VREF_EYE collapsed below MIN_VREF_EYE
                          training_message(channel_i, rank_i, bl_i);
                          post_code(0xEE, (0x70 + (side_y * 2) + (side_x)));
                        }
                        else
                        {
                          // update the VREF setting
                          set_vref(channel_i, bl_i, y_coordinate[side_x][side_y][channel_i][bl_i]);
                          // reset the X coordinate to begin the search at the new VREF
                          x_coordinate[side_x][side_y][channel_i][rank_i][bl_i] =
                              (side_x == L) ? (RDQS_MIN) : (RDQS_MAX);
                        }
                      }
                      // update the RDQS setting
                      set_rdqs(channel_i, rank_i, bl_i, x_coordinate[side_x][side_y][channel_i][rank_i][bl_i]);
                    } // if bl_i failed
                  } // bl_i loop
                } // at least 1 byte lane failed
              } while (result & 0xFF);
            } // if rank is enabled
          } // rank_i loop
        } // if channel is enabled
      } // channel_i loop
    } // side_x loop
  } // side_y loop

  post_code(0x07, 0x20);

  // find final RDQS (X coordinate) & final VREF (Y coordinate)
  for (channel_i = 0; channel_i < NUM_CHANNELS; channel_i++)
  {
    if (mrc_params->channel_enables & (1 << channel_i))
    {
      for (rank_i = 0; rank_i < NUM_RANKS; rank_i++)
      {
        if (mrc_params->rank_enables & (1 << rank_i))
        {
          for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
          {
            uint32_t tempD1;
            uint32_t tempD2;

            // x_coordinate:
            DPF(D_INFO, "RDQS T/B eye rank%d lane%d : %d-%d %d-%d\n", rank_i, bl_i,
                x_coordinate[L][T][channel_i][rank_i][bl_i],
                x_coordinate[R][T][channel_i][rank_i][bl_i],
                x_coordinate[L][B][channel_i][rank_i][bl_i],
                x_coordinate[R][B][channel_i][rank_i][bl_i]);

            tempD1 = (x_coordinate[R][T][channel_i][rank_i][bl_i] + x_coordinate[L][T][channel_i][rank_i][bl_i]) / 2; // average the TOP side LEFT & RIGHT values
            tempD2 = (x_coordinate[R][B][channel_i][rank_i][bl_i] + x_coordinate[L][B][channel_i][rank_i][bl_i]) / 2; // average the BOTTOM side LEFT & RIGHT values
            x_center[channel_i][rank_i][bl_i] = (uint8_t) ((tempD1 + tempD2) / 2); // average the above averages

            // y_coordinate:
            DPF(D_INFO, "VREF R/L eye lane%d : %d-%d %d-%d\n", bl_i,
                y_coordinate[R][B][channel_i][bl_i],
                y_coordinate[R][T][channel_i][bl_i],
                y_coordinate[L][B][channel_i][bl_i],
                y_coordinate[L][T][channel_i][bl_i]);

            tempD1 = (y_coordinate[R][T][channel_i][bl_i] + y_coordinate[R][B][channel_i][bl_i]) / 2; // average the RIGHT side TOP & BOTTOM values
            tempD2 = (y_coordinate[L][T][channel_i][bl_i] + y_coordinate[L][B][channel_i][bl_i]) / 2; // average the LEFT side TOP & BOTTOM values
            y_center[channel_i][bl_i] = (uint8_t) ((tempD1 + tempD2) / 2); // average the above averages
          } // bl_i loop
        } // if rank is enabled
      } // rank_i loop
    } // if channel is enabled
  } // channel_i loop

#ifdef RX_EYE_CHECK
  // perform an eye check
  for (side_y=B; side_y<=T; side_y++)
  {
    for (side_x=L; side_x<=R; side_x++)
    {

      post_code(0x07, (0x30 + (side_y * 2) + (side_x)));

      // update the settings for the eye check
      for (channel_i=0; channel_i<NUM_CHANNELS; channel_i++)
      {
        if (mrc_params->channel_enables & (1<<channel_i))
        {
          for (rank_i=0; rank_i<NUM_RANKS; rank_i++)
          {
            if (mrc_params->rank_enables & (1<<rank_i))
            {
              for (bl_i=0; bl_i<(NUM_BYTE_LANES/bl_divisor); bl_i++)
              {
                if (side_x == L)
                {
                  set_rdqs(channel_i, rank_i, bl_i, (x_center[channel_i][rank_i][bl_i] - (MIN_RDQS_EYE / 2)));
                }
                else
                {
                  set_rdqs(channel_i, rank_i, bl_i, (x_center[channel_i][rank_i][bl_i] + (MIN_RDQS_EYE / 2)));
                }
                if (side_y == B)
                {
                  set_vref(channel_i, bl_i, (y_center[channel_i][bl_i] - (MIN_VREF_EYE / 2)));
                }
                else
                {
                  set_vref(channel_i, bl_i, (y_center[channel_i][bl_i] + (MIN_VREF_EYE / 2)));
                }
              } // bl_i loop
            } // if rank is enabled
          } // rank_i loop
        } // if channel is enabled
      } // channel_i loop

      // request HTE reconfiguration
      mrc_params->hte_setup = 1;

      // check the eye
      if (check_bls_ex( mrc_params, address) & 0xFF)
      {
        // one or more byte lanes failed
        post_code(0xEE, (0x74 + (side_x * 2) + (side_y)));
      }
    } // side_x loop
  } // side_y loop
#endif // RX_EYE_CHECK

  post_code(0x07, 0x40);

  // set final placements
  for (channel_i = 0; channel_i < NUM_CHANNELS; channel_i++)
  {
    if (mrc_params->channel_enables & (1 << channel_i))
    {
      for (rank_i = 0; rank_i < NUM_RANKS; rank_i++)
      {
        if (mrc_params->rank_enables & (1 << rank_i))
        {
#ifdef R2R_SHARING
          // increment "num_ranks_enabled"
          num_ranks_enabled++;
#endif // R2R_SHARING
          for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
          {
            // x_coordinate:
#ifdef R2R_SHARING
            final_delay[channel_i][bl_i] += x_center[channel_i][rank_i][bl_i];
            set_rdqs(channel_i, rank_i, bl_i, ((final_delay[channel_i][bl_i]) / num_ranks_enabled));
#else
            set_rdqs(channel_i, rank_i, bl_i, x_center[channel_i][rank_i][bl_i]);
#endif // R2R_SHARING
            // y_coordinate:
            set_vref(channel_i, bl_i, y_center[channel_i][bl_i]);
          } // bl_i loop
        } // if rank is enabled
      } // rank_i loop
    } // if channel is enabled
  } // channel_i loop
#endif // BACKUP_RDQS
  LEAVEFN();
  return;
}

// wr_train:
// POST_CODE[major] == 0x08
//
// This function will perform the WRITE TRAINING Algorithm on all channels/ranks/byte_lanes simultaneously to minimize execution time.
// The idea here is to train the WDQ timings to achieve maximum WRITE margins.
// The algorithm will start with WDQ at the current WDQ setting (tracks WDQS in WR_LVL) +/- 32 PIs (+/- 1/4 CLK) and collapse the eye until all data patterns pass.
// This is because WDQS will be aligned to WCLK by the Write Leveling algorithm and WDQ will only ever have a 1/2 CLK window of validity.
static void wr_train(
    MRCParams_t *mrc_params)
{

#define WDQ_STEP 1 // how many WDQ codes to jump while margining
#define L 0 // LEFT side loop value definition
#define R 1 // RIGHT side loop value definition

  uint8_t channel_i; // channel counter
  uint8_t rank_i; // rank counter
  uint8_t bl_i; // byte lane counter
  uint8_t bl_divisor = (mrc_params->channel_width == x16) ? 2 : 1; // byte lane divisor
#ifdef BACKUP_WDQ
#else
  uint8_t side_i; // LEFT/RIGHT side indicator (0=L, 1=R)
  uint32_t tempD; // temporary DWORD
  uint32_t delay[2/*side_i*/][NUM_CHANNELS][NUM_RANKS][NUM_BYTE_LANES]; // 2 arrays, for L & R side passing delays
  uint32_t address; // target address for "check_bls_ex()"
  uint32_t result; // result of "check_bls_ex()"
  uint32_t bl_mask; // byte lane mask for "result" checking
#ifdef R2R_SHARING
  uint32_t final_delay[NUM_CHANNELS][NUM_BYTE_LANES]; // used to find placement for rank2rank sharing configs
  uint32_t num_ranks_enabled = 0; // used to find placement for rank2rank sharing configs
#endif // R2R_SHARING
#endif // BACKUP_WDQ

  // wr_train starts
  post_code(0x08, 0x00);

  ENTERFN();

#ifdef BACKUP_WDQ
  for (channel_i=0; channel_i<NUM_CHANNELS; channel_i++)
  {
    if (mrc_params->channel_enables & (1<<channel_i))
    {
      for (rank_i=0; rank_i<NUM_RANKS; rank_i++)
      {
        if (mrc_params->rank_enables & (1<<rank_i))
        {
          for (bl_i=0; bl_i<(NUM_BYTE_LANES/bl_divisor); bl_i++)
          {
            set_wdq(channel_i, rank_i, bl_i, ddr_wdq[PLATFORM_ID]);
          } // bl_i loop
        } // if rank is enabled
      } // rank_i loop
    } // if channel is enabled
  } // channel_i loop
#else
  // initialise "delay"
  for (channel_i = 0; channel_i < NUM_CHANNELS; channel_i++)
  {
    if (mrc_params->channel_enables & (1 << channel_i))
    {
      for (rank_i = 0; rank_i < NUM_RANKS; rank_i++)
      {
        if (mrc_params->rank_enables & (1 << rank_i))
        {
          for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
          {
            // want to start with WDQ = (WDQS - QRTR_CLK) +/- QRTR_CLK
            tempD = get_wdqs(channel_i, rank_i, bl_i) - QRTR_CLK;
            delay[L][channel_i][rank_i][bl_i] = tempD - QRTR_CLK;
            delay[R][channel_i][rank_i][bl_i] = tempD + QRTR_CLK;
          } // bl_i loop
        } // if rank is enabled
      } // rank_i loop
    } // if channel is enabled
  } // channel_i loop

  // initialise other variables
  bl_mask = byte_lane_mask(mrc_params);
  address = get_addr(mrc_params, 0, 0);

#ifdef R2R_SHARING
  // need to set "final_delay[][]" elements to "0"
  memset((void *) (final_delay), 0x00, (size_t) sizeof(final_delay));
#endif // R2R_SHARING

  // start algorithm on the LEFT side and train each channel/bl until no failures are observed, then repeat for the RIGHT side.
  for (side_i = L; side_i <= R; side_i++)
  {
    post_code(0x08, (0x10 + (side_i)));

    // set starting values
    for (channel_i = 0; channel_i < NUM_CHANNELS; channel_i++)
    {
      if (mrc_params->channel_enables & (1 << channel_i))
      {
        for (rank_i = 0; rank_i < NUM_RANKS; rank_i++)
        {
          if (mrc_params->rank_enables & (1 << rank_i))
          {
            for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
            {
              set_wdq(channel_i, rank_i, bl_i, delay[side_i][channel_i][rank_i][bl_i]);
            } // bl_i loop
          } // if rank is enabled
        } // rank_i loop
      } // if channel is enabled
    } // channel_i loop

    // find passing values
    for (channel_i = 0; channel_i < NUM_CHANNELS; channel_i++)
    {
      if (mrc_params->channel_enables & (0x1 << channel_i))
      {
        for (rank_i = 0; rank_i < NUM_RANKS; rank_i++)
        {
          if (mrc_params->rank_enables & (0x1 << rank_i))
          {
            // get an address in the target channel/rank
            address = get_addr(mrc_params, channel_i, rank_i);

            // request HTE reconfiguration
            mrc_params->hte_setup = 1;

            // check the settings
            do
            {

#ifdef SIM
              // need restore memory to idle state as write can be in bad sync
              dram_init_command (DCMD_PREA(rank_i));
#endif

              // result[07:00] == failing byte lane (MAX 8)
              result = check_bls_ex( mrc_params, address);
              // check for failures
              if (result & 0xFF)
              {
                // at least 1 byte lane failed
                for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
                {
                  if (result & (bl_mask << bl_i))
                  {
                    if (side_i == L)
                    {
                      delay[L][channel_i][rank_i][bl_i] += WDQ_STEP;
                    }
                    else
                    {
                      delay[R][channel_i][rank_i][bl_i] -= WDQ_STEP;
                    }
                    // check for algorithm failure
                    if (delay[L][channel_i][rank_i][bl_i] != delay[R][channel_i][rank_i][bl_i])
                    {
                      // margin available, update delay setting
                      set_wdq(channel_i, rank_i, bl_i, delay[side_i][channel_i][rank_i][bl_i]);
                    }
                    else
                    {
                      // no margin available, notify the user and halt
                      training_message(channel_i, rank_i, bl_i);
                      post_code(0xEE, (0x80 + side_i));
                    }
                  } // if bl_i failed
                } // bl_i loop
              } // at least 1 byte lane failed
            } while (result & 0xFF); // stop when all byte lanes pass
          } // if rank is enabled
        } // rank_i loop
      } // if channel is enabled
    } // channel_i loop
  } // side_i loop

  // program WDQ to the middle of passing window
  for (channel_i = 0; channel_i < NUM_CHANNELS; channel_i++)
  {
    if (mrc_params->channel_enables & (1 << channel_i))
    {
      for (rank_i = 0; rank_i < NUM_RANKS; rank_i++)
      {
        if (mrc_params->rank_enables & (1 << rank_i))
        {
#ifdef R2R_SHARING
          // increment "num_ranks_enabled"
          num_ranks_enabled++;
#endif // R2R_SHARING
          for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
          {

            DPF(D_INFO, "WDQ eye rank%d lane%d : %d-%d\n", rank_i, bl_i,
                delay[L][channel_i][rank_i][bl_i],
                delay[R][channel_i][rank_i][bl_i]);

            tempD = (delay[R][channel_i][rank_i][bl_i] + delay[L][channel_i][rank_i][bl_i]) / 2;

#ifdef R2R_SHARING
            final_delay[channel_i][bl_i] += tempD;
            set_wdq(channel_i, rank_i, bl_i, ((final_delay[channel_i][bl_i]) / num_ranks_enabled));
#else
            set_wdq(channel_i, rank_i, bl_i, tempD);
#endif // R2R_SHARING

          } // bl_i loop
        } // if rank is enabled
      } // rank_i loop
    } // if channel is enabled
  } // channel_i loop
#endif // BACKUP_WDQ
  LEAVEFN();
  return;
}

// Wrapper for jedec initialisation routine
static void perform_jedec_init(
    MRCParams_t *mrc_params)
{
  jedec_init(mrc_params, 0);
}

// Configure DDRPHY for Auto-Refresh, Periodic Compensations,
// Dynamic Diff-Amp, ZQSPERIOD, Auto-Precharge, CKE Power-Down
static void set_auto_refresh(
    MRCParams_t *mrc_params)
{
  uint32_t channel_i;
  uint32_t rank_i;
  uint32_t bl_i;
  uint32_t bl_divisor = /*(mrc_params->channel_width==x16)?2:*/1;
  uint32_t tempD;

  ENTERFN();

  // enable Auto-Refresh, Periodic Compensations, Dynamic Diff-Amp, ZQSPERIOD, Auto-Precharge, CKE Power-Down
  for (channel_i = 0; channel_i < NUM_CHANNELS; channel_i++)
  {
    if (mrc_params->channel_enables & (1 << channel_i))
    {
      // Enable Periodic RCOMPS
      isbM32m(DDRPHY, CMPCTRL, (BIT1), (BIT1));


      // Enable Dynamic DiffAmp & Set Read ODT Value
      switch (mrc_params->rd_odt_value)
      {
        case 0: tempD = 0x3F; break;  // OFF
        default: tempD = 0x00; break; // Auto
      } // rd_odt_value switch

      for (bl_i=0; bl_i<((NUM_BYTE_LANES/bl_divisor)/2); bl_i++)
      {
        isbM32m(DDRPHY, (B0OVRCTL + (bl_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)),
            ((0x00<<16)|(tempD<<10)),
            ((BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT15|BIT14|BIT13|BIT12|BIT11|BIT10))); // Override: DIFFAMP, ODT

        isbM32m(DDRPHY, (B1OVRCTL + (bl_i * DDRIODQ_BL_OFFSET) + (channel_i * DDRIODQ_CH_OFFSET)),
            ((0x00<<16)|(tempD<<10)),
            ((BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)|(BIT15|BIT14|BIT13|BIT12|BIT11|BIT10)));// Override: DIFFAMP, ODT
      } // bl_i loop

      // Issue ZQCS command
      for (rank_i = 0; rank_i < NUM_RANKS; rank_i++)
      {
        if (mrc_params->rank_enables & (1 << rank_i))
        {
          dram_init_command(DCMD_ZQCS(rank_i));
        } // if rank_i enabled
      } // rank_i loop

    } // if channel_i enabled
  } // channel_i loop

  clear_pointers();

  LEAVEFN();
  return;
}

// Depending on configuration enables ECC support.
// Available memory size is decresed, and updated with 0s
// in order to clear error status. Address mode 2 forced.
static void ecc_enable(
    MRCParams_t *mrc_params)
{
  RegDRP Drp;
  RegDSCH Dsch;
  RegDECCCTRL Ctr;

  if (mrc_params->ecc_enables == 0) return;

  ENTERFN();

  // Configuration required in ECC mode
  Drp.raw = isbR32m(MCU, DRP);
  Drp.field.addressMap = 2;
  Drp.field.split64 = 1;
  isbW32m(MCU, DRP, Drp.raw);

  // Disable new request bypass
  Dsch.raw = isbR32m(MCU, DSCH);
  Dsch.field.NEWBYPDIS = 1;
  isbW32m(MCU, DSCH, Dsch.raw);

  // Enable ECC
  Ctr.raw = 0;
  Ctr.field.SBEEN = 1;
  Ctr.field.DBEEN = 1;
  Ctr.field.ENCBGEN = 1;
  isbW32m(MCU, DECCCTRL, Ctr.raw);

#ifdef SIM
  // Read back to be sure writing took place
  Ctr.raw = isbR32m(MCU, DECCCTRL);
#endif

  // Assume 8 bank memory, one bank is gone for ECC
  mrc_params->mem_size -= mrc_params->mem_size / 8;

  // For S3 resume memory content has to be preserved
  if (mrc_params->boot_mode != bmS3)
  {
    select_hte(mrc_params);
    HteMemInit(mrc_params, MrcMemInit, MrcHaltHteEngineOnError);
    select_memory_manager(mrc_params);
  }

  LEAVEFN();
  return;
}

// Lock MCU registers at the end of initialisation sequence.
static void lock_registers(
    MRCParams_t *mrc_params)
{
  RegDCO Dco;

  ENTERFN();

  Dco.raw = isbR32m(MCU, DCO);
  Dco.field.PMIDIS = 0;          //0 - PRI enabled
  Dco.field.PMICTL = 0;          //0 - PRI owned by MEMORY_MANAGER
  Dco.field.DRPLOCK = 1;
  Dco.field.REUTLOCK = 1;
  isbW32m(MCU, DCO, Dco.raw);

  LEAVEFN();

}

#ifdef MRC_SV

// cache write back invalidate
static void asm_wbinvd(void)
{
#if defined (SIM) || defined (GCC)
  asm(
    "wbinvd;"
  );
#else
  __asm wbinvd;
#endif
}

// cache invalidate
static void asm_invd(void)
{
#if defined (SIM) || defined (GCC)
  asm(
      "invd;"
  );
#else
  __asm invd;
#endif
}


static void cpu_read(void)
{
  uint32_t adr, dat, limit;

  asm_invd();

  limit = 8 * 1024;
  for (adr = 0; adr < limit; adr += 4)
  {
    dat = *(uint32_t*) adr;
    if ((adr & 0x0F) == 0)
    {
      DPF(D_INFO, "\n%x : ", adr);
    }
    DPF(D_INFO, "%x ", dat);
  }
  DPF(D_INFO, "\n");

  DPF(D_INFO, "CPU read done\n");
}


static void cpu_write(void)
{
  uint32_t adr, limit;

  limit = 8 * 1024;
  for (adr = 0; adr < limit; adr += 4)
  {
    *(uint32_t*) adr = 0xDEAD0000 + adr;
  }

  asm_wbinvd();

  DPF(D_INFO, "CPU write done\n");
}


static void cpu_memory_test(
    MRCParams_t *mrc_params)
{
  uint32_t result = 0;
  uint32_t val, dat, adr, adr0, step, limit;
  uint64_t my_tsc;

  ENTERFN();

  asm_invd();

  adr0 = 1 * 1024 * 1024;
  limit = 256 * 1024 * 1024;

  for (step = 0; step <= 4; step++)
  {
    DPF(D_INFO, "Mem test step %d starting from %xh\n", step, adr0);

    my_tsc = read_tsc();
    for (adr = adr0; adr < limit; adr += sizeof(uint32_t))
    {
      if (step == 0)      dat = adr;
      else if (step == 1) dat = (1 << ((adr >> 2) & 0x1f));
      else if (step == 2) dat = ~(1 << ((adr >> 2) & 0x1f));
      else if (step == 3) dat = 0x5555AAAA;
      else if (step == 4) dat = 0xAAAA5555;

      *(uint32_t*) adr = dat;
    }
    DPF(D_INFO, "Write time %llXh\n", read_tsc() - my_tsc);

    my_tsc = read_tsc();
    for (adr = adr0; adr < limit; adr += sizeof(uint32_t))
    {
      if (step == 0)      dat = adr;
      else if (step == 1) dat = (1 << ((adr >> 2) & 0x1f));
      else if (step == 2) dat = ~(1 << ((adr >> 2) & 0x1f));
      else if (step == 3) dat = 0x5555AAAA;
      else if (step == 4) dat = 0xAAAA5555;

      val = *(uint32_t*) adr;

      if (val != dat)
      {
        DPF(D_INFO, "%x vs. %x@%x\n", dat, val, adr);
        result = adr|BIT31;
      }
    }
    DPF(D_INFO, "Read time %llXh\n", read_tsc() - my_tsc);
  }

  DPF( D_INFO, "Memory test result %x\n", result);
  LEAVEFN();
}
#endif // MRC_SV


// Execute memory test, if error dtected it is
// indicated in mrc_params->status.
static void memory_test(
  MRCParams_t *mrc_params)
{
  uint32_t result = 0;

  ENTERFN();

  select_hte(mrc_params);
  result = HteMemInit(mrc_params, MrcMemTest, MrcHaltHteEngineOnError);
  select_memory_manager(mrc_params);

  DPF(D_INFO, "Memory test result %x\n", result);
  mrc_params->status = ((result == 0) ? MRC_SUCCESS : MRC_E_MEMTEST);
  LEAVEFN();
}


// Force same timings as with backup settings
static void static_timings(
  MRCParams_t *mrc_params)

{
  uint8_t ch, rk, bl;

  for (ch = 0; ch < NUM_CHANNELS; ch++)
  {
    for (rk = 0; rk < NUM_RANKS; rk++)
    {
      for (bl = 0; bl < NUM_BYTE_LANES; bl++)
      {
        set_rcvn(ch, rk, bl, 498);  // RCVN
        set_rdqs(ch, rk, bl,  24);  // RDQS
        set_wdqs(ch, rk, bl, 292);  // WDQS
        set_wdq( ch, rk, bl, 260);  // WDQ
        if (rk == 0)
        {
          set_vref(ch, bl, 32); // VREF (RANK0 only)
        }
      }
      set_wctl(ch, rk, 217); // WCTL
    }
    set_wcmd(ch, 220); // WCMD
  }

  return;
}

//
// Initialise system memory.
//
void MemInit(
  MRCParams_t *mrc_params)
{
  static const MemInit_t init[] =
  {
    { 0x0101, bmCold|bmFast|bmWarm|bmS3, clear_self_refresh       }, //0
    { 0x0200, bmCold|bmFast|bmWarm|bmS3, prog_ddr_timing_control  }, //1  initialise the MCU
    { 0x0103, bmCold|bmFast            , prog_decode_before_jedec }, //2
    { 0x0104, bmCold|bmFast            , perform_ddr_reset        }, //3
    { 0x0300, bmCold|bmFast       |bmS3, ddrphy_init              }, //4  initialise the DDRPHY
    { 0x0400, bmCold|bmFast            , perform_jedec_init       }, //5  perform JEDEC initialisation of DRAMs
    { 0x0105, bmCold|bmFast            , set_ddr_init_complete    }, //6
    { 0x0106,        bmFast|bmWarm|bmS3, restore_timings          }, //7
    { 0x0106, bmCold                   , default_timings          }, //8
    { 0x0500, bmCold                   , rcvn_cal                 }, //9  perform RCVN_CAL algorithm
    { 0x0600, bmCold                   , wr_level                 }, //10  perform WR_LEVEL algorithm
    { 0x0120, bmCold                   , prog_page_ctrl           }, //11
    { 0x0700, bmCold                   , rd_train                 }, //12  perform RD_TRAIN algorithm
    { 0x0800, bmCold                   , wr_train                 }, //13  perform WR_TRAIN algorithm
    { 0x010B, bmCold                   , store_timings            }, //14
    { 0x010C, bmCold|bmFast|bmWarm|bmS3, enable_scrambling        }, //15
    { 0x010D, bmCold|bmFast|bmWarm|bmS3, prog_ddr_control         }, //16
    { 0x010E, bmCold|bmFast|bmWarm|bmS3, prog_dra_drb             }, //17
    { 0x010F,               bmWarm|bmS3, perform_wake             }, //18
    { 0x0110, bmCold|bmFast|bmWarm|bmS3, change_refresh_period    }, //19
    { 0x0111, bmCold|bmFast|bmWarm|bmS3, set_auto_refresh         }, //20
    { 0x0112, bmCold|bmFast|bmWarm|bmS3, ecc_enable               }, //21
    { 0x0113, bmCold|bmFast            , memory_test              }, //22
    { 0x0114, bmCold|bmFast|bmWarm|bmS3, lock_registers           }  //23 set init done
  };

  uint32_t i;

  ENTERFN();

  DPF(D_INFO, "Meminit build %s %s\n", __DATE__, __TIME__);

  // MRC started
  post_code(0x01, 0x00);

  if (mrc_params->boot_mode != bmCold)
  {
    if (mrc_params->ddr_speed != mrc_params->timings.ddr_speed)
    {
      // full training required as frequency changed
      mrc_params->boot_mode = bmCold;
    }
  }

  for (i = 0; i < MCOUNT(init); i++)
  {
    uint64_t my_tsc;

#ifdef MRC_SV
    if (mrc_params->menu_after_mrc && i > 14)
    {
      uint8_t ch;

      mylop:

      DPF(D_INFO, "-- c - continue --\n");
      DPF(D_INFO, "-- j - move to jedec init --\n");
      DPF(D_INFO, "-- m - memory test --\n");
      DPF(D_INFO, "-- r - cpu read --\n");
      DPF(D_INFO, "-- w - cpu write --\n");
      DPF(D_INFO, "-- b - hte base test --\n");
      DPF(D_INFO, "-- g - hte extended test --\n");

      ch = mgetc();
      switch (ch)
      {
      case 'c':
        break;
      case 'j':  //move to jedec init
        i = 5;
        break;

      case 'M':
      case 'N':
        {
    uint32_t n, res, cnt=0;

    for(n=0; mgetch()==0; n++)
    {
      if( ch == 'M' || n % 256 == 0)
      {
        DPF(D_INFO, "n=%d e=%d\n", n, cnt);
      }

      res = 0;

      if( ch == 'M')
      {
        memory_test(mrc_params);
        res |= mrc_params->status;
            }

      mrc_params->hte_setup = 1;
            res |= check_bls_ex(mrc_params, 0x00000000);
            res |= check_bls_ex(mrc_params, 0x00000000);
            res |= check_bls_ex(mrc_params, 0x00000000);
            res |= check_bls_ex(mrc_params, 0x00000000);

      if( mrc_params->rank_enables & 2)
      {
        mrc_params->hte_setup = 1;
              res |= check_bls_ex(mrc_params, 0x40000000);
              res |= check_bls_ex(mrc_params, 0x40000000);
              res |= check_bls_ex(mrc_params, 0x40000000);
              res |= check_bls_ex(mrc_params, 0x40000000);
      }

      if( res != 0)
      {
              DPF(D_INFO, "###########\n");
              DPF(D_INFO, "#\n");
              DPF(D_INFO, "# Error count %d\n", ++cnt);
              DPF(D_INFO, "#\n");
              DPF(D_INFO, "###########\n");
      }

    } // for

          select_memory_manager(mrc_params);
  }
        goto mylop;
      case 'm':
        memory_test(mrc_params);
        goto mylop;
      case 'n':
        cpu_memory_test(mrc_params);
        goto mylop;

      case 'l':
        ch = mgetc();
        if (ch <= '9') DpfPrintMask ^= (ch - '0') << 3;
        DPF(D_INFO, "Log mask %x\n", DpfPrintMask);
        goto mylop;
      case 'p':
        print_timings(mrc_params);
        goto mylop;
      case 'R':
        rd_train(mrc_params);
        goto mylop;
      case 'W':
        wr_train(mrc_params);
        goto mylop;

      case 'r':
        cpu_read();
        goto mylop;
      case 'w':
        cpu_write();
        goto mylop;

      case 'g':
        {
        uint32_t result;
        select_hte(mrc_params);
        mrc_params->hte_setup = 1;
        result = check_bls_ex(mrc_params, 0);
        DPF(D_INFO, "Extended test result %x\n", result);
        select_memory_manager(mrc_params);
        }
        goto mylop;
      case 'b':
        {
        uint32_t result;
        select_hte(mrc_params);
        mrc_params->hte_setup = 1;
        result = check_rw_coarse(mrc_params, 0);
        DPF(D_INFO, "Base test result %x\n", result);
        select_memory_manager(mrc_params);
        }
        goto mylop;
      case 'B':
        select_hte(mrc_params);
        HteMemOp(0x2340, 1, 1);
        select_memory_manager(mrc_params);
        goto mylop;

      case '3':
        {
        RegDPMC0 DPMC0reg;

        DPF( D_INFO, "===>> Start suspend\n");
        isbR32m(MCU, DSTAT);

        DPMC0reg.raw = isbR32m(MCU, DPMC0);
        DPMC0reg.field.DYNSREN = 0;
        DPMC0reg.field.powerModeOpCode = 0x05;    // Disable Master DLL
        isbW32m(MCU, DPMC0, DPMC0reg.raw);

        // Should be off for negative test case verification
        #if 1
        Wr32(MMIO, PCIADDR(0,0,0,SB_PACKET_REG),
            (uint32_t)SB_COMMAND(SB_SUSPEND_CMND_OPCODE, MCU, 0));
        #endif

        DPF( D_INFO, "press key\n");
        mgetc();
        DPF( D_INFO, "===>> Start resume\n");
        isbR32m(MCU, DSTAT);

        mrc_params->boot_mode = bmS3;
        i = 0;
        }

      } // switch

    } // if( menu
#endif //MRC_SV

    if (mrc_params->boot_mode & init[i].boot_path)
    {
      uint8_t major = init[i].post_code >> 8 & 0xFF;
      uint8_t minor = init[i].post_code >> 0 & 0xFF;
      post_code(major, minor);

      my_tsc = read_tsc();
      init[i].init_fn(mrc_params);
      DPF(D_TIME, "Execution time %llX", read_tsc() - my_tsc);
    }
  }

  // display the timings
  print_timings(mrc_params);

  // MRC is complete.
  post_code(0x01, 0xFF);

  LEAVEFN();
  return;
}
