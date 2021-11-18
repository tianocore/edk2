/** @file

  Copyright (c) 2017-2018, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  System Control and Management Interface V1.0
    http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/
    DEN0056A_System_Control_and_Management_Interface.pdf
**/

#ifndef ARM_SCMI_CLOCK_PROTOCOL_PRIVATE_H_
#define ARM_SCMI_CLOCK_PROTOCOL_PRIVATE_H_

#pragma pack(1)

// Clock rate in two 32bit words.
typedef struct {
  UINT32    Low;
  UINT32    High;
} CLOCK_RATE_DWORD;

// Format of the returned rate array. Linear or Non-linear,.RatesFlag Bit[12]
#define RATE_FORMAT_SHIFT  12
#define RATE_FORMAT_MASK   0x0001
#define RATE_FORMAT(RatesFlags)  ((RatesFlags >> RATE_FORMAT_SHIFT)     \
                                     & RATE_FORMAT_MASK)

// Number of remaining rates after a call to the SCP, RatesFlag Bits[31:16]
#define NUM_REMAIN_RATES_SHIFT  16
#define NUM_REMAIN_RATES(RatesFlags)  ((RatesFlags >> NUM_REMAIN_RATES_SHIFT))

// Number of rates that are returned by a call.to the SCP, RatesFlag Bits[11:0]
#define NUM_RATES_MASK  0x0FFF
#define NUM_RATES(RatesFlags)  (RatesFlags & NUM_RATES_MASK)

// Return values for the CLOCK_DESCRIBER_RATE command.
typedef struct {
  UINT32              NumRatesFlags;

  // NOTE: Since EDK2 does not allow flexible array member [] we declare
  // here array of 1 element length. However below is used as a variable
  // length array.
  CLOCK_RATE_DWORD    Rates[1];
} CLOCK_DESCRIBE_RATES;

#define CLOCK_SET_DEFAULT_FLAGS  0

// Message parameters for CLOCK_RATE_SET command.
typedef struct {
  UINT32              Flags;
  UINT32              ClockId;
  CLOCK_RATE_DWORD    Rate;
} CLOCK_RATE_SET_ATTRIBUTES;

// Message parameters for CLOCK_CONFIG_SET command.
typedef struct {
  UINT32    ClockId;
  UINT32    Attributes;
} CLOCK_CONFIG_SET_ATTRIBUTES;

//  if ClockAttr Bit[0] is set then clock device is enabled.
#define CLOCK_ENABLE_MASK  0x1
#define CLOCK_ENABLED(ClockAttr)  ((ClockAttr & CLOCK_ENABLE_MASK) == 1)

typedef struct {
  UINT32    Attributes;
  UINT8     ClockName[SCMI_MAX_STR_LEN];
} CLOCK_ATTRIBUTES;

#pragma pack()

/** Initialize clock management protocol and install protocol on a given handle.

  @param[in] Handle              Handle to install clock management protocol.

  @retval EFI_SUCCESS            Clock protocol interface installed successfully.
**/
EFI_STATUS
ScmiClockProtocolInit (
  IN EFI_HANDLE  *Handle
  );

#endif /* ARM_SCMI_CLOCK_PROTOCOL_PRIVATE_H_ */
