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
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return -1;
}

int
tls_parse_ctos_server_name (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_maxfragmentlen (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

#ifndef OPENSSL_NO_SRP
int
tls_parse_ctos_srp (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

#endif

int
tls_parse_ctos_ec_pt_formats (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_session_ticket (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_sig_algs_cert (
  SSL                       *s,
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
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

#ifndef OPENSSL_NO_OCSP
int
tls_parse_ctos_status_request (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

#endif

#ifndef OPENSSL_NO_NEXTPROTONEG
int
tls_parse_ctos_npn (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
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
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

#ifndef OPENSSL_NO_SRTP
int
tls_parse_ctos_use_srtp (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

#endif

int
tls_parse_ctos_etm (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
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
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
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
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_cookie (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_supported_groups (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_ems (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_early_data (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_psk (
  SSL           *s,
  PACKET        *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return 0;
}

int
tls_parse_ctos_post_handshake_auth (
  SSL                       *s,
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
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_server_name (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

/* Add/include the server's max fragment len extension into ServerHello */
EXT_RETURN
tls_construct_stoc_maxfragmentlen (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_ec_pt_formats (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_supported_groups (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_session_ticket (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

#ifndef OPENSSL_NO_OCSP
EXT_RETURN
tls_construct_stoc_status_request (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

#endif

#ifndef OPENSSL_NO_NEXTPROTONEG
EXT_RETURN
tls_construct_stoc_next_proto_neg (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

#endif

EXT_RETURN
tls_construct_stoc_alpn (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

#ifndef OPENSSL_NO_SRTP
EXT_RETURN
tls_construct_stoc_use_srtp (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

#endif

EXT_RETURN
tls_construct_stoc_etm (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_ems (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_supported_versions (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_key_share (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_cookie (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_cryptopro_bug (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_early_data (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}

EXT_RETURN
tls_construct_stoc_psk (
  SSL           *s,
  WPACKET       *pkt,
  unsigned int  context,
  X509          *x,
  size_t        chainidx
  )
{
  return EXT_RETURN_FAIL;
}
