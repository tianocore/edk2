/** @file

**/

#ifndef _TEST_PCD_H_
#define _TEST_PCD_H_

#define MAX_TEST_Dance     16
#define MAX_TEST_Tango 16

typedef struct {
    UINT8 Dance[MAX_TEST_Dance];
} TEST_Tango;

typedef struct {
  TEST_Tango Tango[MAX_TEST_Tango];
} WALTZ_TEST_MAP;


/**
  Get TEST Mapping from Tango and DanceID

  @param[in] Tango     - Tango Id - 0 based
  @param[in] DanceId   - Waltzory
  @param[in] TESTValue - TEST Value

  @retval EFI_SUCCESS   - Value found
  @retval EFI_NOT_FOUND - Value not found

**/
EFI_STATUS
EFIAPI
GetTestMap (
  IN UINT8 Tango,
  IN UINT8 DanceId,
  IN UINT8 *TESTValue
  );

/**
  Set TEST Mapping for given Tango and DanceID

  @param[in] Tango     - Tango Id - 0 based
  @param[in] DanceId   - Waltz
  @param[in] TESTValue - TEST Value

  @retval EFI_SUCCESS   - Value set successfully
  @retval EFI_NOT_FOUND - Value not set

**/
EFI_STATUS
EFIAPI
SetTestMap (
  IN UINT8 Tango,
  IN UINT8 DanceId,
  IN UINT8 TESTValue
  );

#endif //_TEST_PCD_H_
