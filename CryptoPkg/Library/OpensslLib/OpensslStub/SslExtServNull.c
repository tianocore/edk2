/** @file
  Null implementation of SslExtServ functions called by TlsLib.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <openssl/ocsp.h>
#include "../ssl_local.h"
#include "statem_local.h"
#include "internal/cryptlib.h"

int
tls_parse_ctos_renegotiate (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return -1;
}

int
tls_parse_ctos_server_name (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_maxfragmentlen (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

#ifndef OPENSSL_NO_SRP
int
tls_parse_ctos_srp (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

#endif

int
tls_parse_ctos_ec_pt_formats (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_session_ticket (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_sig_algs_cert (
  SSL_CONNECTION            *s,
  PACKET                    *pkt,
  ossl_unused unsigned int  context,
  ossl_unused X509          *x,
  ossl_unused size_t        chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_sig_algs (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

#ifndef OPENSSL_NO_OCSP
int
tls_parse_ctos_status_request (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

#endif

#ifndef OPENSSL_NO_NEXTPROTONEG
int
tls_parse_ctos_npn (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

#endif

/*
 * Save the ALPN extension in a ClientHello.|pkt| holds the contents of the ALPN
 * extension, not including type and length. Returns: 1 on success, 0 on error.
 */
int
tls_parse_ctos_alpn (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

#ifndef OPENSSL_NO_SRTP
int
tls_parse_ctos_use_srtp (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

#endif

int
tls_parse_ctos_etm (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

/*
 * Process a psk_kex_modes extension received in the ClientHello. |pkt| contains
 * the raw PACKET data for the extension. Returns 1 on success or 0 on failure.
 */
int
tls_parse_ctos_psk_kex_modes (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

/*
 * Process a key_share extension received in the ClientHello. |pkt| contains
 * the raw PACKET data for the extension. Returns 1 on success or 0 on failure.
 */
int
tls_parse_ctos_key_share (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_cookie (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_supported_groups (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_ems (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_early_data (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_psk (
  SSL_CONNECTION  *s,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_post_handshake_auth (
  SSL_CONNECTION            *s,
  PACKET                    *pkt,
  ossl_unused unsigned int  context,
  ossl_unused X509          *x,
  ossl_unused size_t        chainidx
  )
{
  return 0;
}

/*
 * Add the server's renegotiation binding
 */
EXT_RETURN
tls_construct_stoc_renegotiate (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_server_name (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

/* Add/include the server's max fragment len extension into ServerHello */
EXT_RETURN
tls_construct_stoc_maxfragmentlen (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_ec_pt_formats (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_supported_groups (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_session_ticket (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

#ifndef OPENSSL_NO_OCSP
EXT_RETURN
tls_construct_stoc_status_request (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

#endif

#ifndef OPENSSL_NO_NEXTPROTONEG
EXT_RETURN
tls_construct_stoc_next_proto_neg (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

#endif

EXT_RETURN
tls_construct_stoc_alpn (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

#ifndef OPENSSL_NO_SRTP
EXT_RETURN
tls_construct_stoc_use_srtp (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

#endif

EXT_RETURN
tls_construct_stoc_etm (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_ems (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_supported_versions (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_key_share (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_cookie (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_cryptopro_bug (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_early_data (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_psk (
  SSL_CONNECTION  *s,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_client_cert_type (
  SSL_CONNECTION  *sc,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

int
tls_parse_ctos_client_cert_type (
  SSL_CONNECTION  *sc,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}

EXT_RETURN
tls_construct_stoc_server_cert_type (
  SSL_CONNECTION  *sc,
  WPACKET         *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return EXT_RETURN_FAIL;
}

int
tls_parse_ctos_server_cert_type (
  SSL_CONNECTION  *sc,
  PACKET          *pkt,
  unsigned int    context,
  X509            *x,
  size_t          chainidx
  )
{
  return 0;
}
