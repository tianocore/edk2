/** @file
  Arrays defining DDR4 and DDR5 SPD EEPROM data

  Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPD_TEST_DATA_H_
#define SPD_TEST_DATA_H

#define DDR4_SPD_LEN 512
#define DDR5_SPD_LEN 1024

extern const UINT8                Ddr4DimmTestData1[];
extern const SMBIOS_TABLE_TYPE17  Ddr4DimmTestData1ExpectedResult;
extern const UINT8                Ddr4DimmTestData2[];
extern const SMBIOS_TABLE_TYPE17  Ddr4DimmTestData2ExpectedResult;

#endif /* SPD_TEST_DATA_H_ */