/** @file
 The data structures in this code come from:
 OMAP35x Applications Processor Technical Reference Manual chapter 25
 OMAP34xx Multimedia Device Technical Reference Manual chapter 26.4.8.

 You should use the OMAP35x manual when possible. Some things, like SectionKey,
 are not defined in the OMAP35x manual and you have to use the OMAP34xx manual
 to find the data.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>



//TOC structure as defined by OMAP35XX TRM.
typedef struct {
  unsigned int  Start;
  unsigned int  Size;
  unsigned int  Reserved1;
  unsigned int  Reserved2;
  unsigned int  Reserved3;
  unsigned char Filename[12];
} TOC_DATA;

//NOTE: OMAP3430 TRM has CHSETTINGS and CHRAM structures.
typedef struct {
  unsigned int   SectionKey;
  unsigned char  Valid;
  unsigned char  Version;
  unsigned short Reserved;
  unsigned int   Flags;
  unsigned int   PRM_CLKSRC_CTRL;
  unsigned int   PRM_CLKSEL;
  unsigned int   CM_CLKSEL1_EMU;
  unsigned int   CM_CLKSEL_CORE;
  unsigned int   CM_CLKSEL_WKUP;
  unsigned int   CM_CLKEN_PLL_DPLL3;
  unsigned int   CM_AUTOIDLE_PLL_DPLL3;
  unsigned int   CM_CLKSEL1_PLL;
  unsigned int   CM_CLKEN_PLL_DPLL4;
  unsigned int   CM_AUTOIDLE_PLL_DPLL4;
  unsigned int   CM_CLKSEL2_PLL;
  unsigned int   CM_CLKSEL3_PLL;
  unsigned int   CM_CLKEN_PLL_MPU;
  unsigned int   CM_AUTOIDLE_PLL_MPU;
  unsigned int   CM_CLKSEL1_PLL_MPU;
  unsigned int   CM_CLKSEL2_PLL_MPU;
  unsigned int   CM_CLKSTCTRL_MPU;
} CHSETTINGS_DATA;

typedef struct {
  unsigned int   SectionKey;
  unsigned char  Valid;
  unsigned char  Reserved1;
  unsigned char  Reserved2;
  unsigned char  Reserved3;
  unsigned short SDRC_SYSCONFIG_LSB;
  unsigned short SDRC_CS_CFG_LSB;
  unsigned short SDRC_SHARING_LSB;
  unsigned short SDRC_ERR_TYPE_LSB;
  unsigned int   SDRC_DLLA_CTRL;
  unsigned short Reserved4;
  unsigned short Reserved5;
  unsigned int   SDRC_POWER;
  unsigned short MEMORY_TYPE_CS0;
  unsigned short Reserved6;
  unsigned int   SDRC_MCFG_0;
  unsigned short SDRC_MR_0_LSB;
  unsigned short SDRC_EMR1_0_LSB;
  unsigned short SDRC_EMR2_0_LSB;
  unsigned short SDRC_EMR3_0_LSB;
  unsigned int   SDRC_ACTIM_CTRLA_0;
  unsigned int   SDRC_ACTIM_CTRLB_0;
  unsigned int   SDRC_RFRCTRL_0;
  unsigned short MEMORY_TYPE_CS1;
  unsigned short Reserved7;
  unsigned int   SDRC_MCFG_1;
  unsigned short SDRC_MR_1_LSB;
  unsigned short SDRC_EMR1_1_LSB;
  unsigned short SDRC_EMR2_1_LSB;
  unsigned short SDRC_EMR3_1_LSB;
  unsigned int   SDRC_ACTIM_CTRLA_1;
  unsigned int   SDRC_ACTIM_CTRLB_1;
  unsigned int   SDRC_RFRCTRL_1;
  unsigned int   Reserved8;
  unsigned short Flags;
  unsigned short Reserved9;
} CHRAM_DATA;

#define CHSETTINGS_START      0xA0
#define CHSETTINGS_SIZE       0x50
#define CHRAM_START           0xF0
#define CHRAM_SIZE            0x5C
#define CLOSING_TOC_ITEM_SIZE 4

unsigned char gConfigurationHeader[512];
unsigned int  gImageExecutionAddress;
char          *gInputImageFile = NULL;
char          *gOutputImageFile = NULL;
char          *gDataFile = NULL;

static
void
PrintUsage (
  void
  )
{
  printf("Usage..\n");
}

static
void
PopulateCHSETTINGSData (
  FILE            *DataFile,
  CHSETTINGS_DATA *CHSETTINGSData
  )
{
  unsigned int Value;

  CHSETTINGSData->SectionKey            = 0xC0C0C0C1;
  CHSETTINGSData->Valid                 = 0x1;
  CHSETTINGSData->Version               = 0x1;
  CHSETTINGSData->Reserved              = 0x00;
  CHSETTINGSData->Flags                 = 0x050001FD;

  //General clock settings.
  fscanf(DataFile, "PRM_CLKSRC_CTRL=0x%08x\n", &Value);
  CHSETTINGSData->PRM_CLKSRC_CTRL = Value;
  fscanf(DataFile, "PRM_CLKSEL=0x%08x\n", &Value);
  CHSETTINGSData->PRM_CLKSEL = Value;
  fscanf(DataFile, "CM_CLKSEL1_EMU=0x%08x\n", &Value);
  CHSETTINGSData->CM_CLKSEL1_EMU = Value;

  //Clock configuration
  fscanf(DataFile, "CM_CLKSEL_CORE=0x%08x\n", &Value);
  CHSETTINGSData->CM_CLKSEL_CORE = Value;
  fscanf(DataFile, "CM_CLKSEL_WKUP=0x%08x\n", &Value);
  CHSETTINGSData->CM_CLKSEL_WKUP = Value;

  //DPLL3 (Core) settings
  fscanf(DataFile, "CM_CLKEN_PLL_DPLL3=0x%08x\n", &Value);
  CHSETTINGSData->CM_CLKEN_PLL_DPLL3 = Value;
  fscanf(DataFile, "CM_AUTOIDLE_PLL_DPLL3=0x%08x\n", &Value);
  CHSETTINGSData->CM_AUTOIDLE_PLL_DPLL3 = Value;
  fscanf(DataFile, "CM_CLKSEL1_PLL=0x%08x\n", &Value);
  CHSETTINGSData->CM_CLKSEL1_PLL = Value;

  //DPLL4 (Peripheral) settings
  fscanf(DataFile, "CM_CLKEN_PLL_DPLL4=0x%08x\n", &Value);
  CHSETTINGSData->CM_CLKEN_PLL_DPLL4 = Value;
  fscanf(DataFile, "CM_AUTOIDLE_PLL_DPLL4=0x%08x\n", &Value);
  CHSETTINGSData->CM_AUTOIDLE_PLL_DPLL4 = Value;
  fscanf(DataFile, "CM_CLKSEL2_PLL=0x%08x\n", &Value);
  CHSETTINGSData->CM_CLKSEL2_PLL = Value;
  fscanf(DataFile, "CM_CLKSEL3_PLL=0x%08x\n", &Value);
  CHSETTINGSData->CM_CLKSEL3_PLL = Value;

  //DPLL1 (MPU) settings
  fscanf(DataFile, "CM_CLKEN_PLL_MPU=0x%08x\n", &Value);
  CHSETTINGSData->CM_CLKEN_PLL_MPU = Value;
  fscanf(DataFile, "CM_AUTOIDLE_PLL_MPU=0x%08x\n", &Value);
  CHSETTINGSData->CM_AUTOIDLE_PLL_MPU = Value;
  fscanf(DataFile, "CM_CLKSEL1_PLL_MPU=0x%08x\n", &Value);
  CHSETTINGSData->CM_CLKSEL1_PLL_MPU = Value;
  fscanf(DataFile, "CM_CLKSEL2_PLL_MPU=0x%08x\n", &Value);
  CHSETTINGSData->CM_CLKSEL2_PLL_MPU = Value;
  fscanf(DataFile, "CM_CLKSTCTRL_MPU=0x%08x\n", &Value);
  CHSETTINGSData->CM_CLKSTCTRL_MPU = Value;
}

static
void
PopulateCHRAMData (
  FILE       *DataFile,
  CHRAM_DATA *CHRAMData
  )
{
  unsigned int Value;

  CHRAMData->SectionKey         = 0xC0C0C0C2;
  CHRAMData->Valid              = 0x1;

  fscanf(DataFile, "SDRC_SYSCONFIG_LSB=0x%04x\n", &Value);
  CHRAMData->SDRC_SYSCONFIG_LSB = Value;
  fscanf(DataFile, "SDRC_CS_CFG_LSB=0x%04x\n", &Value);
  CHRAMData->SDRC_CS_CFG_LSB    = Value;
  fscanf(DataFile, "SDRC_SHARING_LSB=0x%04x\n", &Value);
  CHRAMData->SDRC_SHARING_LSB   = Value;
  fscanf(DataFile, "SDRC_ERR_TYPE_LSB=0x%04x\n", &Value);
  CHRAMData->SDRC_ERR_TYPE_LSB  = Value;
  fscanf(DataFile, "SDRC_DLLA_CTRL=0x%08x\n", &Value);
  CHRAMData->SDRC_DLLA_CTRL     = Value;
  fscanf(DataFile, "SDRC_POWER=0x%08x\n", &Value);
  CHRAMData->SDRC_POWER         = Value;
  fscanf(DataFile, "MEMORY_TYPE_CS0=0x%04x\n", &Value);
  CHRAMData->MEMORY_TYPE_CS0    = Value;
  fscanf(DataFile, "SDRC_MCFG_0=0x%08x\n", &Value);
  CHRAMData->SDRC_MCFG_0        = Value;
  fscanf(DataFile, "SDRC_MR_0_LSB=0x%04x\n", &Value);
  CHRAMData->SDRC_MR_0_LSB      = Value;
  fscanf(DataFile, "SDRC_EMR1_0_LSB=0x%04x\n", &Value);
  CHRAMData->SDRC_EMR1_0_LSB    = Value;
  fscanf(DataFile, "SDRC_EMR2_0_LSB=0x%04x\n", &Value);
  CHRAMData->SDRC_EMR2_0_LSB    = Value;
  fscanf(DataFile, "SDRC_EMR3_0_LSB=0x%04x\n", &Value);
  CHRAMData->SDRC_EMR3_0_LSB    = Value;
  fscanf(DataFile, "SDRC_ACTIM_CTRLA_0=0x%08x\n", &Value);
  CHRAMData->SDRC_ACTIM_CTRLA_0 = Value;
  fscanf(DataFile, "SDRC_ACTIM_CTRLB_0=0x%08x\n", &Value);
  CHRAMData->SDRC_ACTIM_CTRLB_0 = Value;
  fscanf(DataFile, "SDRC_RFRCTRL_0=0x%08x\n", &Value);
  CHRAMData->SDRC_RFRCTRL_0     = Value;
  fscanf(DataFile, "MEMORY_TYPE_CS1=0x%04x\n", &Value);
  CHRAMData->MEMORY_TYPE_CS1    = Value;
  fscanf(DataFile, "SDRC_MCFG_1=0x%08x\n", &Value);
  CHRAMData->SDRC_MCFG_1        = Value;
  fscanf(DataFile, "SDRC_MR_1_LSB=0x%04x\n", &Value);
  CHRAMData->SDRC_MR_1_LSB      = Value;
  fscanf(DataFile, "SDRC_EMR1_1_LSB=0x%04x\n", &Value);
  CHRAMData->SDRC_EMR1_1_LSB    = Value;
  fscanf(DataFile, "SDRC_EMR2_1_LSB=0x%04x\n", &Value);
  CHRAMData->SDRC_EMR2_1_LSB    = Value;
  fscanf(DataFile, "SDRC_EMR3_1_LSB=0x%04x\n", &Value);
  CHRAMData->SDRC_EMR3_1_LSB    = Value;
  fscanf(DataFile, "SDRC_ACTIM_CTRLA_1=0x%08x\n", &Value);
  CHRAMData->SDRC_ACTIM_CTRLA_1 = Value;
  fscanf(DataFile, "SDRC_ACTIM_CTRLB_1=0x%08x\n", &Value);
  CHRAMData->SDRC_ACTIM_CTRLB_1 = Value;
  fscanf(DataFile, "SDRC_RFRCTRL_1=0x%08x\n", &Value);
  CHRAMData->SDRC_RFRCTRL_1     = Value;

  CHRAMData->Flags              = 0x0003;
}

static
void
PrepareConfigurationHeader (
  void
  )
{
  TOC_DATA        Toc;
  CHSETTINGS_DATA CHSETTINGSData;
  CHRAM_DATA      CHRAMData;
  unsigned int    ConfigurationHdrOffset = 0;
  FILE            *DataFile;

  // Open data file
  DataFile = fopen(gDataFile, "rb");
  if (DataFile == NULL) {
    fprintf(stderr, "Can't open data file %s.\n", gDataFile);
    exit(1);
  }

  //Initialize configuration header.
  memset(gConfigurationHeader, 0x00, sizeof(gConfigurationHeader));

  //CHSETTINGS TOC
  memset(&Toc, 0x00, sizeof(TOC_DATA));
  Toc.Start = CHSETTINGS_START;
  Toc.Size = CHSETTINGS_SIZE;
  strcpy((char *)Toc.Filename, (const char *)"CHSETTINGS");
  memcpy(gConfigurationHeader + ConfigurationHdrOffset, &Toc, sizeof(TOC_DATA));

  //Populate CHSETTINGS Data
  memset(&CHSETTINGSData, 0x00, sizeof(CHSETTINGS_DATA));
  PopulateCHSETTINGSData(DataFile, &CHSETTINGSData);
  memcpy(gConfigurationHeader + Toc.Start, &CHSETTINGSData, Toc.Size);

  //Adjust ConfigurationHdrOffset to point to next TOC
  ConfigurationHdrOffset += sizeof(TOC_DATA);

  //CHRAM TOC
  memset(&Toc, 0x00, sizeof(TOC_DATA));
  Toc.Start = CHRAM_START;
  Toc.Size = CHRAM_SIZE;
  strcpy((char *)Toc.Filename, (const char *)"CHRAM");
  memcpy(gConfigurationHeader + ConfigurationHdrOffset, &Toc, sizeof(TOC_DATA));

  //Populate CHRAM Data
  memset(&CHRAMData, 0x00, sizeof(CHRAM_DATA));
  PopulateCHRAMData(DataFile, &CHRAMData);
  memcpy(gConfigurationHeader + Toc.Start, &CHRAMData, Toc.Size);

  //Adjust ConfigurationHdrOffset to point to next TOC
  ConfigurationHdrOffset += sizeof(TOC_DATA);

  //Closing TOC item
  memset(gConfigurationHeader + ConfigurationHdrOffset, 0xFF, CLOSING_TOC_ITEM_SIZE);
  ConfigurationHdrOffset += CLOSING_TOC_ITEM_SIZE;

  // Close data file
  fclose(DataFile);
}

static
void
ConstructImage (
  void
  )
{
  FILE         *InputFile;
  FILE         *OutputFile;
  unsigned int InputImageFileSize;
  struct       stat FileStat;
  char         Ch;
  unsigned int i;

  InputFile = fopen(gInputImageFile, "rb");
  if (InputFile == NULL) {
    fprintf(stderr, "Can't open input file.\n");
    exit(0);
  }

  // Get the size of the input image.
  fstat(fileno(InputFile), &FileStat);
  InputImageFileSize = FileStat.st_size;

  OutputFile = fopen(gOutputImageFile, "wb");
  if (OutputFile == NULL) {
    fprintf(stderr, "Can't open output file %s.\n", gOutputImageFile);
    exit(0);
  }

  // Write Configuration header
  fwrite(gConfigurationHeader, 1, sizeof(gConfigurationHeader), OutputFile);

  // Write image header (Input image size, execution address)
  fwrite(&InputImageFileSize, 1, 4, OutputFile);
  fwrite(&gImageExecutionAddress, 1, 4, OutputFile);

  // Copy input image to the output file.
  for (i = 0; i < InputImageFileSize; i++) {
    fread(&Ch, 1, 1, InputFile);
    fwrite(&Ch, 1, 1, OutputFile);
  }

  fclose(InputFile);
  fclose(OutputFile);
}


int
main (
  int    argc,
  char** argv
  )
{
  char          Ch;
  unsigned char *ptr;
  int           i;
  int           TwoArg;

  if (argc == 1) {
    PrintUsage ();
    exit(1);
  }

  for (i=1; i < argc; i++) {
    if (argv[i][0] == '-') {
      // TwoArg TRUE -E 0x123, FALSE -E0x1234
      TwoArg = (argv[i][2] != ' ');
      switch (argv[i][1]) {
        case 'E': /* Image execution address */
          gImageExecutionAddress = strtoul (TwoArg ? argv[i+1] : &argv[i][2], (char **)&ptr, 16);
          break;

        case 'I': /* Input image file */
          gInputImageFile = TwoArg ? argv[i+1] : &argv[i][2];
          break;

        case 'O': /* Output image file */
          gOutputImageFile = TwoArg ? argv[i+1] : &argv[i][2];
          break;

        case 'D': /* Data file */
          gDataFile = TwoArg ? argv[i+1] : &argv[i][2];
          break;

        default:
          abort ();
      }
    }
  }


  //Prepare configuration header
  PrepareConfigurationHeader ();

  //Build image with configuration header + image header + image
  ConstructImage ();

  return 0;
}
