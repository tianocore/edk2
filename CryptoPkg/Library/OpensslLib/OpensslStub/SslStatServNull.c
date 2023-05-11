/** @file
  Null implementation of SslStatServ functions called by TlsLib.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "../ssl_local.h"
#include "statem_local.h"
#include "internal/constant_time.h"
#include "internal/cryptlib.h"
#include <openssl/core_names.h>
#include <openssl/asn1t.h>

int
ossl_statem_server_read_transition (
  SSL  *s,
  int  mt
  )
{
  return 0;
}

/*
 * Should we send a CertificateRequest message?
 *
 * Valid return values are:
 *   1: Yes
 *   0: No
 */
int
send_certificate_request (
  SSL  *s
  )
{
  return 0;
}

/*
 * ossl_statem_server_write_transition() works out what handshake state to move
 * to next when the server is writing messages to be sent to the client.
 */
WRITE_TRAN
ossl_statem_server_write_transition (
  SSL  *s
  )
{
  return WRITE_TRAN_ERROR;
}

WORK_STATE
ossl_statem_server_pre_work (
  SSL         *s,
  WORK_STATE  wst
  )
{
  return WORK_ERROR;
}

/*
 * Perform any work that needs to be done after sending a message from the
 * server to the client.
 */
WORK_STATE
ossl_statem_server_post_work (
  SSL         *s,
  WORK_STATE  wst
  )
{
  return WORK_ERROR;
}

/*
 * Get the message construction function and message type for sending from the
 * server
 *
 * Valid return values are:
 *   1: Success
 *   0: Error
 */
int
ossl_statem_server_construct_message (
  SSL        *s,
  WPACKET    *pkt,
  confunc_f  *confunc,
  int        *mt
  )
{
  return 0;
}

/*
 * Returns the maximum allowed length for the current message that we are
 * reading. Excludes the message header.
 */
size_t
ossl_statem_server_max_message_size (
  SSL  *s
  )
{
  return 0;
}

/*
 * Process a message that the server has received from the client.
 */
MSG_PROCESS_RETURN
ossl_statem_server_process_message (
  SSL     *s,
  PACKET  *pkt
  )
{
  return MSG_PROCESS_ERROR;
}

/*
 * Perform any further processing required following the receipt of a message
 * from the client
 */
WORK_STATE
ossl_statem_server_post_process_message (
  SSL         *s,
  WORK_STATE  wst
  )
{
  return WORK_ERROR;
}

int
dtls_raw_hello_verify_request (
  WPACKET        *pkt,
  unsigned char  *cookie,
  size_t         cookie_len
  )
{
  return 0;
}

int
dtls_construct_hello_verify_request (
  SSL      *s,
  WPACKET  *pkt
  )
{
  return 0;
}

MSG_PROCESS_RETURN
tls_process_client_hello (
  SSL     *s,
  PACKET  *pkt
  )
{
  return MSG_PROCESS_ERROR;
}

/*
 * Call the alpn_select callback if needed. Upon success, returns 1.
 * Upon failure, returns 0.
 */
int
tls_handle_alpn (
  SSL  *s
  )
{
  return 0;
}

WORK_STATE
tls_post_process_client_hello (
  SSL         *s,
  WORK_STATE  wst
  )
{
  return WORK_ERROR;
}

int
tls_construct_server_hello (
  SSL      *s,
  WPACKET  *pkt
  )
{
  return 0;
}

int
tls_construct_server_done (
  SSL      *s,
  WPACKET  *pkt
  )
{
  return 0;
}

int
tls_construct_server_key_exchange (
  SSL      *s,
  WPACKET  *pkt
  )
{
  return 0;
}

int
tls_construct_certificate_request (
  SSL      *s,
  WPACKET  *pkt
  )
{
  return 0;
}

MSG_PROCESS_RETURN
tls_process_client_key_exchange (
  SSL     *s,
  PACKET  *pkt
  )
{
  return MSG_PROCESS_ERROR;
}

WORK_STATE
tls_post_process_client_key_exchange (
  SSL         *s,
  WORK_STATE  wst
  )
{
  return WORK_ERROR;
}

MSG_PROCESS_RETURN
tls_process_client_certificate (
  SSL     *s,
  PACKET  *pkt
  )
{
  return MSG_PROCESS_ERROR;
}

int
tls_construct_server_certificate (
  SSL      *s,
  WPACKET  *pkt
  )
{
  return 0;
}

int
tls_construct_new_session_ticket (
  SSL      *s,
  WPACKET  *pkt
  )
{
  return 0;
}

/*
 * In TLSv1.3 this is called from the extensions code, otherwise it is used to
 * create a separate message. Returns 1 on success or 0 on failure.
 */
int
tls_construct_cert_status_body (
  SSL      *s,
  WPACKET  *pkt
  )
{
  return 0;
}

int
tls_construct_cert_status (
  SSL      *s,
  WPACKET  *pkt
  )
{
  return 0;
}

#ifndef OPENSSL_NO_NEXTPROTONEG

/*
 * tls_process_next_proto reads a Next Protocol Negotiation handshake message.
 * It sets the next_proto member in s if found
 */
MSG_PROCESS_RETURN
tls_process_next_proto (
  SSL     *s,
  PACKET  *pkt
  )
{
  return MSG_PROCESS_ERROR;
}

#endif

MSG_PROCESS_RETURN
tls_process_end_of_early_data (
  SSL     *s,
  PACKET  *pkt
  )
{
  return MSG_PROCESS_ERROR;
}
