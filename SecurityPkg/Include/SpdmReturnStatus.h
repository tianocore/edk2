/** @file
  EDKII Device Security library for SPDM device.
  It follows the SPDM Specification.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPDM_RETURN_STATUS_H_
#define SPDM_RETURN_STATUS_H_

/* The layout of SPDM_RETURN is
 * [31:28] - Severity
 * [27:24] - Reserved
 * [23:16] - Source
 * [15:00] - Code
 */
typedef UINT32 SPDM_RETURN;

/* Returns 1 if severity is SPDM_SEVERITY_SUCCESS else it returns 0. */
#define SPDM_STATUS_IS_SUCCESS(status) \
    (SPDM_STATUS_SEVERITY(status) == SPDM_SEVERITY_SUCCESS)

/* Returns 1 if severity is SPDM_SEVERITY_ERROR else it returns 0. */
#define SPDM_STATUS_IS_ERROR(status) \
    (SPDM_STATUS_SEVERITY(status) == SPDM_SEVERITY_ERROR)

/* Returns the severity of the status. */
#define SPDM_STATUS_SEVERITY(status)  (((status) >> 28) & 0xf)

/* Returns the source of the status. */
#define SPDM_STATUS_SOURCE(status)  (((status) >> 16) & 0xff)

#define SPDM_SEVERITY_SUCCESS  0x0
#define SPDM_SEVERITY_ERROR    0x8

#define SPDM_SOURCE_SUCCESS       0x00
#define SPDM_SOURCE_CORE          0x01
#define SPDM_SOURCE_CRYPTO        0x02
#define SPDM_SOURCE_CERT_PARSE    0x03
#define SPDM_SOURCE_TRANSPORT     0x04
#define SPDM_SOURCE_MEAS_COLLECT  0x05
#define SPDM_SOURCE_RNG           0x06

#define SPDM_STATUS_CONSTRUCT(severity, source, code) \
    ((SPDM_RETURN)(((severity) << 28) | ((source) << 16) | (code)))

/* Success status is always 0x00000000. */
#define SPDM_STATUS_SUCCESS \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_SUCCESS, SPDM_SOURCE_SUCCESS, 0x0000)

#define SPDM_RET_ON_ERR(status) \
    do { \
        if (SPDM_STATUS_IS_ERROR(status)) { \
            return (status); \
        } \
    } \
    while (0)

/* - Core Errors - */

/* The function input parameter is invalid. */
#define SPDM_STATUS_INVALID_PARAMETER \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x0001)

/* Unable to complete operation due to unsupported capabilities by either the caller, the peer,
 * or both. */
#define SPDM_STATUS_UNSUPPORTED_CAP \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x0002)

/* Unable to complete operation due to caller's state. */
#define SPDM_STATUS_INVALID_STATE_LOCAL \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x0003)

/* Unable to complete operation due to peer's state. */
#define SPDM_STATUS_INVALID_STATE_PEER \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x0004)

/* The received message contains one or more invalid message fields. */
#define SPDM_STATUS_INVALID_MSG_FIELD \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x0005)

/* The received message's size is invalid. */
#define SPDM_STATUS_INVALID_MSG_SIZE \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x0006)

/* Unable to derive a common set of versions, algorithms, etc. */
#define SPDM_STATUS_NEGOTIATION_FAIL \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x0007)

/* Received a Busy error message. */
#define SPDM_STATUS_BUSY_PEER \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x0008)

/* Received a NotReady error message. */
#define SPDM_STATUS_NOT_READY_PEER \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x0009)

/* Received an unexpected error message. */
#define SPDM_STATUS_ERROR_PEER \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x000a)

/* Received a RequestResynch error message. */
#define SPDM_STATUS_RESYNCH_PEER \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x000b)

/* Unable to append new data to buffer due to resource exhaustion. */
#define SPDM_STATUS_BUFFER_FULL \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x000c)

/* Unable to return data because caller does not provide big enough buffer. */
#define SPDM_STATUS_BUFFER_TOO_SMALL \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x000d)

/* Unable to allocate more session. */
#define SPDM_STATUS_SESSION_NUMBER_EXCEED \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x000e)

/* Decrypt error from peer. */
#define SPDM_STATUS_SESSION_MSG_ERROR \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CORE, 0x000f)

/* - Cryptography Errors - */

/* Generic failure originating from the cryptography module. */
#define SPDM_STATUS_CRYPTO_ERROR \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CRYPTO, 0x0000)

/* Verification of the provided signature digest, signature, or AEAD tag failed. */
#define SPDM_STATUS_VERIF_FAIL \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CRYPTO, 0x0001)

/* AEAD sequence number overflow. */
#define SPDM_STATUS_SEQUENCE_NUMBER_OVERFLOW \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CRYPTO, 0x0002)

/* - Certificate Parsing Errors - */

/* Certificate is malformed or does not comply to x.509 standard. */
#define SPDM_STATUS_INVALID_CERT \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_CERT_PARSE, 0x0000)

/* - Transport Errors - */

/* Unable to send message to peer. */
#define SPDM_STATUS_SEND_FAIL \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_TRANSPORT, 0x0000)

/* Unable to receive message from peer. */
#define SPDM_STATUS_RECEIVE_FAIL \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_TRANSPORT, 0x0001)

/* - Measurement Collection Errors - */

/* Unable to collect measurement because of invalid index. */
#define SPDM_STATUS_MEAS_INVALID_INDEX \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_MEAS_COLLECT, 0x0000)

/* Unable to collect measurement because of internal error. */
#define SPDM_STATUS_MEAS_INTERNAL_ERROR \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_MEAS_COLLECT, 0x0001)

/* - Random Number Generation Errors - */

/* Unable to produce random number due to lack of entropy. */
#define SPDM_STATUS_LOW_ENTROPY \
    SPDM_STATUS_CONSTRUCT(SPDM_SEVERITY_ERROR, SPDM_SOURCE_RNG, 0x0000)

#endif
