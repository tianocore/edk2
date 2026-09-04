# Firmware Management Protocol (FMP) DXE

For an overview of how FmpDxe fits into the overall FmpDevicePkg architecture, including capsule authentication, capsule
versioning, device-specific dispatch, dependency evaluation, and update policy, see
[FmpDevicePkg/Docs/FmpDevicePkg_ReadMe.md](../Docs/FmpDevicePkg_ReadMe.md).

This document tracks impactful design changes to FmpDxe for the benefit of capsule implementers and platform integrators
to see when and why these changes were made.

---

## Design Changes

- **Date:** 07/20/2026
- **Description/Rationale:** PopulateDescriptor () previously read the FmpControllerState NV variable multiple times
per call through several getters such as GetVersionFromVariable () and GetLowestSupportedVersionFromVariable (),
each independently invoking GetFmpControllerState (). This caused redundant GetVariable () calls and repeated
failed-read debug messages when the variable did not yet exist.
- **Changes:** The getter functions were renamed (for example, to GetVersionFromFmpControllerState ()) and
simplified to pure field extractors that operate on a FMP_CONTROLLER_STATE pointer supplied by the caller instead of
independently reading the variable. PopulateDescriptor () now calls GetFmpControllerState () once, passes the result
to each getter, and frees it after the last use.
- **Impact/Mitigation:**
This is an internal implementation change local to FmpDxe. It does not modify the FmpDeviceLib, FmpDependencyLib,
FmpDependencyCheckLib, FmpDependencyDeviceLib, or CapsuleUpdatePolicyLib APIs, so existing library instances are
unaffected.

---

- **Date:** 09/24/2025
- **Description/Rationale:** XDR-encoded certificate list handling in FmpDxe assumed the PublicKeyDataXdr buffer
(populated from PcdFmpDevicePkcs7CertBufferXdr) is 4-byte aligned, which is not always the case. For example, if
PublicKeyDataXdr = 0x02 and a certificate's length is 0x05, the correct offset to the next certificate is
0x2 + align_up (0x5, 4) = 0xA, but the prior logic computed align_up (0x2 + 0x5, 4) = 0x8.
- **Changes:** The offset calculation used to walk the XDR-encoded certificate list was corrected to align up each
certificate's length independently rather than aligning the running (potentially already unaligned) offset.
- **Impact/Mitigation:**
Platforms that supply a PcdFmpDevicePkcs7CertBufferXdr value containing more than one certificate, where an earlier
certificate's length is not a multiple of 4 bytes, will see corrected certificate parsing behavior.

---

- **Date:** 01/20/2022
- **Description/Rationale:** CheckTheImageInternal () did not force ImageUpdatable to an invalid value when
FmpDeviceCheckImageWithStatus () returned an error but left ImageUpdatable set to IMAGE_UPDATABLE_VALID. In addition,
the FmpDeviceCheckImageWithStatus () LastAttemptStatus parameter description could be misread as meaning the value
is only inspected when the function returns an error.
- **Changes:** CheckTheImageInternal () now forces ImageUpdatable to IMAGE_UPDATABLE_INVALID if
FmpDeviceCheckImageWithStatus () returns an error, and only validates/converts the returned LastAttemptStatus value
when the image is not updatable so a valid LAST_ATTEMPT_STATUS_SUCCESS is not overwritten for an updatable image.
The API documentation for FmpDeviceCheckImageWithStatus () was updated to remove the sentence limiting
LastAttemptStatus inspection to error cases.
- **Impact/Mitigation:**
FmpDeviceLib instances should always set LastAttemptStatus to a value in the designated range (or
LAST_ATTEMPT_STATUS_SUCCESS) whenever FmpDeviceCheckImageWithStatus () is called, not only when it returns an error.

---

- **Date:** 10/26/2021
- **Description/Rationale:** FmpDxe used the deprecated EDKII_VARIABLE_LOCK_PROTOCOL (RequestToLock ()) to lock its
NV variables.
- **Changes:** FmpDxe was updated to lock its NV variables using EDKII_VARIABLE_POLICY_PROTOCOL and
RegisterBasicVariablePolicy () from the new VariablePolicyHelperLib dependency. The module Depex was intended to
change from gEdkiiVariableLockProtocolGuid to gEdkiiVariablePolicyProtocolGuid at the same time. This was missed and
corrected in a follow-up fix on 07/09/2024.
- **Impact/Mitigation:**
Platforms must produce gEdkiiVariablePolicyProtocolGuid for FmpDxe to start. Platforms that only produce the
deprecated gEdkiiVariableLockProtocolGuid protocol will need to update their variable services accordingly.

---

- **Date:** 06/15/2020
- **Description/Rationale:** Extending on the more granular LastAttemptStatus support added in FmpDeviceSetImage (),
FmpDeviceCheckImage () also has a LastAttemptStatus parameter added. An image check is always performed by a set
image operation. A more granular status code from the check image path greatly improves overall error isolation when
applying an image.
- **Changes:** This change allows the FmpDeviceLib implementation to return a last attempt status code in the range
LAST_ATTEMPT_STATUS_LIBRARY_ERROR_MIN_ERROR_CODE to LAST_ATTEMPT_STATUS_LIBRARY_ERROR_MAX_ERROR_CODE. Furthermore,
an internal wrapper for CheckTheImage () in FmpDxe was added called CheckTheImageInternal (). This function can return
a last attempt status code for an error in the driver prior to invoking FmpDeviceCheckImage (). These driver error
codes will be in the range of LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL_VENDOR_RANGE_MIN to
LAST_ATTEMPT_STATUS_DRIVER_ERROR_MAX_ERROR_CODE.
- **Impact/Mitigation:**
The change break the build for all FmpDeviceLib instances due to the API change. Each FmpDeviceLib should change to
the new API definition and implement support to return unique values for LastAttemptStatus when appropriate.

---

- **Date:** 10/07/2019
- **Description/Rationale:** Capsule update is the process where each OEM has a lot of interest. Especially when there
is capsule update failure, it is helpful to gather more information of the failure. With existing implementations, the
SetImage routine from FmpDxe driver, which performs most heavy lifting during capsule update, will only
populate LastAttemptStatus with limited pre-defined error codes which could be consumed/inspected by the OS when it
recovers and boots. Thus our proposal is to update the SetImage routine and leverage the
LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL_VENDOR_RANGE range newly defined in UEFI Spec 2.8 Section 23.4, so that the
error code will provide better granularity when viewing capsule update failure from OS device manager.
- **Changes:** A few error codes (128 total) are reserved from LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL_VENDOR_RANGE
range for FmpDxe driver usage, which ranges from thermal and power API failure to capsule payload header check failure.
Furthermore, *an output pointer of the LastAttemptStatus is added as an input argument for FmpDeviceSetImage function
in FmpDeviceLib to allow platform to provide their own platform specific error codes*.
(SPI write failure, SVN checking failure, and more).
- **Impact/Mitigation:**
The italic text above will cause a breaking change for all the FmpDeviceLib instances due to API being modified. This
is to provide better visibility to OEMs to decode capsule update failures more efficiently. Each FmpDeviceLib should
change to the new API definition and populate proper LastAttemptStatus values when applicable.
