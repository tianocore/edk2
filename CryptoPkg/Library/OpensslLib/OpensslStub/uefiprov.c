/** @file
  UEFI Openssl provider implementation.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <string.h>
#include <stdio.h>
#include <openssl/opensslconf.h>
#include <openssl/core.h>
#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>
#include <openssl/params.h>
#include "prov/bio.h"
#include "prov/provider_ctx.h"
#include "prov/providercommon.h"
#include "prov/implementations.h"
#include "prov/names.h"
#include "prov/provider_util.h"
#include "prov/seeding.h"
#include "internal/nelem.h"
#include "provider_local.h"

OSSL_provider_init_fn ossl_uefi_provider_init;
const OSSL_PROVIDER_INFO ossl_predefined_providers[] = {
    { "default", NULL, ossl_uefi_provider_init, NULL, 1 },
    { NULL, NULL, NULL, NULL, 0 }
};

/*
 * Forward declarations to ensure that interface functions are correctly
 * defined.
 */
static OSSL_FUNC_provider_gettable_params_fn deflt_gettable_params;
static OSSL_FUNC_provider_get_params_fn deflt_get_params;
static OSSL_FUNC_provider_query_operation_fn deflt_query;

#define ALGC(NAMES, FUNC, CHECK) { { NAMES, "provider=default", FUNC }, CHECK }
#define ALG(NAMES, FUNC) ALGC(NAMES, FUNC, NULL)

/* Functions provided by the core */
static OSSL_FUNC_core_gettable_params_fn *c_gettable_params = NULL;
static OSSL_FUNC_core_get_params_fn *c_get_params = NULL;

/* Parameters we provide to the core */
static const OSSL_PARAM deflt_param_types[] = {
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_NAME, OSSL_PARAM_UTF8_PTR, NULL, 0),
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_VERSION, OSSL_PARAM_UTF8_PTR, NULL, 0),
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_BUILDINFO, OSSL_PARAM_UTF8_PTR, NULL, 0),
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_STATUS, OSSL_PARAM_INTEGER, NULL, 0),
    OSSL_PARAM_END
};

static const OSSL_PARAM *deflt_gettable_params(void *provctx)
{
    return deflt_param_types;
}

static int deflt_get_params(void *provctx, OSSL_PARAM params[])
{
    OSSL_PARAM *p;

    p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_NAME);
    if (p != NULL && !OSSL_PARAM_set_utf8_ptr(p, "OpenSSL Default Provider"))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_VERSION);
    if (p != NULL && !OSSL_PARAM_set_utf8_ptr(p, OPENSSL_VERSION_STR))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_BUILDINFO);
    if (p != NULL && !OSSL_PARAM_set_utf8_ptr(p, OPENSSL_FULL_VERSION_STR))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_STATUS);
    if (p != NULL && !OSSL_PARAM_set_int(p, ossl_prov_is_running()))
        return 0;
    return 1;
}

/*
 * For the algorithm names, we use the following formula for our primary
 * names:
 *
 *     ALGNAME[VERSION?][-SUBNAME[VERSION?]?][-SIZE?][-MODE?]
 *
 *     VERSION is only present if there are multiple versions of
 *     an alg (MD2, MD4, MD5).  It may be omitted if there is only
 *     one version (if a subsequent version is released in the future,
 *     we can always change the canonical name, and add the old name
 *     as an alias).
 *
 *     SUBNAME may be present where we are combining multiple
 *     algorithms together, e.g. MD5-SHA1.
 *
 *     SIZE is only present if multiple versions of an algorithm exist
 *     with different sizes (e.g. AES-128-CBC, AES-256-CBC)
 *
 *     MODE is only present where applicable.
 *
 * We add diverse other names where applicable, such as the names that
 * NIST uses, or that are used for ASN.1 OBJECT IDENTIFIERs, or names
 * we have used historically.
 *
 * Algorithm names are case insensitive, but we use all caps in our "canonical"
 * names for consistency.
 */
static const OSSL_ALGORITHM deflt_digests[] = {
    /* Our primary name:NIST name[:our older names] */
    { PROV_NAMES_SHA1, "provider=default", ossl_sha1_functions },
    { PROV_NAMES_SHA2_224, "provider=default", ossl_sha224_functions },
    { PROV_NAMES_SHA2_256, "provider=default", ossl_sha256_functions },
    { PROV_NAMES_SHA2_384, "provider=default", ossl_sha384_functions },
    { PROV_NAMES_SHA2_512, "provider=default", ossl_sha512_functions },

#ifndef OPENSSL_NO_SM3
    { PROV_NAMES_SM3, "provider=default", ossl_sm3_functions },
#endif /* OPENSSL_NO_SM3 */

#ifndef OPENSSL_NO_MD5
    { PROV_NAMES_MD5, "provider=default", ossl_md5_functions },
#endif /* OPENSSL_NO_MD5 */

    { PROV_NAMES_NULL, "provider=default", ossl_nullmd_functions },
    { NULL, NULL, NULL }
};

static const OSSL_ALGORITHM_CAPABLE deflt_ciphers[] = {
    ALG(PROV_NAMES_NULL, ossl_null_functions),
    ALG(PROV_NAMES_AES_256_ECB, ossl_aes256ecb_functions),
    ALG(PROV_NAMES_AES_192_ECB, ossl_aes192ecb_functions),
    ALG(PROV_NAMES_AES_128_ECB, ossl_aes128ecb_functions),
    ALG(PROV_NAMES_AES_256_CBC, ossl_aes256cbc_functions),
    ALG(PROV_NAMES_AES_192_CBC, ossl_aes192cbc_functions),
    ALG(PROV_NAMES_AES_128_CBC, ossl_aes128cbc_functions),

    ALG(PROV_NAMES_AES_256_CTR, ossl_aes256ctr_functions),
    ALG(PROV_NAMES_AES_192_CTR, ossl_aes192ctr_functions),
    ALG(PROV_NAMES_AES_128_CTR, ossl_aes128ctr_functions),

    ALG(PROV_NAMES_AES_256_GCM, ossl_aes256gcm_functions),
    ALG(PROV_NAMES_AES_192_GCM, ossl_aes192gcm_functions),
    ALG(PROV_NAMES_AES_128_GCM, ossl_aes128gcm_functions),

    ALGC (
        PROV_NAMES_AES_128_CBC_HMAC_SHA256,
        ossl_aes128cbc_hmac_sha256_functions,
        ossl_cipher_capable_aes_cbc_hmac_sha256
        ),
    ALGC (
        PROV_NAMES_AES_256_CBC_HMAC_SHA256,
        ossl_aes256cbc_hmac_sha256_functions,
        ossl_cipher_capable_aes_cbc_hmac_sha256
        ),

    { { NULL, NULL, NULL }, NULL }
};
static OSSL_ALGORITHM exported_ciphers[OSSL_NELEM(deflt_ciphers)];

static const OSSL_ALGORITHM deflt_macs[] = {
    { PROV_NAMES_HMAC, "provider=default", ossl_hmac_functions },
    { NULL, NULL, NULL }
};

static const OSSL_ALGORITHM deflt_kdfs[] = {
    { PROV_NAMES_HKDF, "provider=default", ossl_kdf_hkdf_functions },
    { PROV_NAMES_SSKDF, "provider=default", ossl_kdf_sskdf_functions },
    { PROV_NAMES_PBKDF2, "provider=default", ossl_kdf_pbkdf2_functions },
    { PROV_NAMES_SSHKDF, "provider=default", ossl_kdf_sshkdf_functions },
    { PROV_NAMES_TLS1_PRF, "provider=default", ossl_kdf_tls1_prf_functions },
    { NULL, NULL, NULL }
};

static const OSSL_ALGORITHM deflt_keyexch[] = {
#ifndef OPENSSL_NO_DH
    { PROV_NAMES_DH, "provider=default", ossl_dh_keyexch_functions },
#endif
#ifndef OPENSSL_NO_EC
    { PROV_NAMES_ECDH, "provider=default", ossl_ecdh_keyexch_functions },
#endif
    { PROV_NAMES_TLS1_PRF, "provider=default", ossl_kdf_tls1_prf_keyexch_functions },
    { PROV_NAMES_HKDF, "provider=default", ossl_kdf_hkdf_keyexch_functions },
    { NULL, NULL, NULL }
};

static const OSSL_ALGORITHM deflt_rands[] = {
    { PROV_NAMES_CTR_DRBG, "provider=default", ossl_drbg_ctr_functions },
    { PROV_NAMES_HASH_DRBG, "provider=default", ossl_drbg_hash_functions },
    { NULL, NULL, NULL }
};

static const OSSL_ALGORITHM deflt_signature[] = {
    { PROV_NAMES_RSA, "provider=default", ossl_rsa_signature_functions },
#ifndef OPENSSL_NO_EC
    { PROV_NAMES_ECDSA, "provider=default", ossl_ecdsa_signature_functions },
#endif

    { NULL, NULL, NULL }
};

static const OSSL_ALGORITHM deflt_asym_cipher[] = {
    { PROV_NAMES_RSA, "provider=default", ossl_rsa_asym_cipher_functions },
    { NULL, NULL, NULL }
};

static const OSSL_ALGORITHM deflt_keymgmt[] = {
#ifndef OPENSSL_NO_DH
    { PROV_NAMES_DH, "provider=default", ossl_dh_keymgmt_functions,
      PROV_DESCS_DH },
    { PROV_NAMES_DHX, "provider=default", ossl_dhx_keymgmt_functions,
      PROV_DESCS_DHX },
#endif

    { PROV_NAMES_RSA, "provider=default", ossl_rsa_keymgmt_functions,
      PROV_DESCS_RSA },
    { PROV_NAMES_RSA_PSS, "provider=default", ossl_rsapss_keymgmt_functions,
      PROV_DESCS_RSA_PSS },
#ifndef OPENSSL_NO_EC
    { PROV_NAMES_EC, "provider=default", ossl_ec_keymgmt_functions,
      PROV_DESCS_EC },
#endif
    { PROV_NAMES_TLS1_PRF, "provider=default", ossl_kdf_keymgmt_functions,
      PROV_DESCS_TLS1_PRF_SIGN },
    { PROV_NAMES_HKDF, "provider=default", ossl_kdf_keymgmt_functions,
      PROV_DESCS_HKDF_SIGN },

    { NULL, NULL, NULL }
};

static const OSSL_ALGORITHM deflt_decoder[] = {
#define DECODER_PROVIDER "default"
#include "decoders.inc"
    { NULL, NULL, NULL }
#undef DECODER_PROVIDER
};

static const OSSL_ALGORITHM *deflt_query(void *provctx, int operation_id,
                                         int *no_cache)
{
    *no_cache = 0;
    switch (operation_id) {
    case OSSL_OP_DIGEST:
        return deflt_digests;
    case OSSL_OP_CIPHER:
        return exported_ciphers;
    case OSSL_OP_MAC:
        return deflt_macs;
    case OSSL_OP_KDF:
        return deflt_kdfs;
    case OSSL_OP_RAND:
        return deflt_rands;
    case OSSL_OP_KEYMGMT:
        return deflt_keymgmt;
    case OSSL_OP_KEYEXCH:
        return deflt_keyexch;
    case OSSL_OP_SIGNATURE:
        return deflt_signature;
    case OSSL_OP_ASYM_CIPHER:
        return deflt_asym_cipher;
    case OSSL_OP_DECODER:
        return deflt_decoder;
    }
    return NULL;
}


static void deflt_teardown(void *provctx)
{
    BIO_meth_free(ossl_prov_ctx_get0_core_bio_method(provctx));
    ossl_prov_ctx_free(provctx);
}

/* Functions we provide to the core */
static const OSSL_DISPATCH deflt_dispatch_table[] = {
    { OSSL_FUNC_PROVIDER_TEARDOWN, (void (*)(void))deflt_teardown },
    { OSSL_FUNC_PROVIDER_GETTABLE_PARAMS, (void (*)(void))deflt_gettable_params },
    { OSSL_FUNC_PROVIDER_GET_PARAMS, (void (*)(void))deflt_get_params },
    { OSSL_FUNC_PROVIDER_QUERY_OPERATION, (void (*)(void))deflt_query },
    { OSSL_FUNC_PROVIDER_GET_CAPABILITIES,
      (void (*)(void))ossl_prov_get_capabilities },
    { 0, NULL }
};

OSSL_provider_init_fn ossl_uefi_provider_init;

int ossl_uefi_provider_init(const OSSL_CORE_HANDLE *handle,
                               const OSSL_DISPATCH *in,
                               const OSSL_DISPATCH **out,
                               void **provctx)
{
    OSSL_FUNC_core_get_libctx_fn *c_get_libctx = NULL;
    BIO_METHOD *corebiometh;

    if (!ossl_prov_bio_from_dispatch(in)
            || !ossl_prov_seeding_from_dispatch(in))
        return 0;
    for (; in->function_id != 0; in++) {
        switch (in->function_id) {
        case OSSL_FUNC_CORE_GETTABLE_PARAMS:
            c_gettable_params = OSSL_FUNC_core_gettable_params(in);
            break;
        case OSSL_FUNC_CORE_GET_PARAMS:
            c_get_params = OSSL_FUNC_core_get_params(in);
            break;
        case OSSL_FUNC_CORE_GET_LIBCTX:
            c_get_libctx = OSSL_FUNC_core_get_libctx(in);
            break;
        default:
            /* Just ignore anything we don't understand */
            break;
        }
    }

    if (c_get_libctx == NULL)
        return 0;

    /*
     * We want to make sure that all calls from this provider that requires
     * a library context use the same context as the one used to call our
     * functions.  We do that by passing it along in the provider context.
     *
     * This only works for built-in providers.  Most providers should
     * create their own library context.
     */
    if ((*provctx = ossl_prov_ctx_new()) == NULL
            || (corebiometh = ossl_bio_prov_init_bio_method()) == NULL) {
        ossl_prov_ctx_free(*provctx);
        *provctx = NULL;
        return 0;
    }
    ossl_prov_ctx_set0_libctx(*provctx,
                                       (OSSL_LIB_CTX *)c_get_libctx(handle));
    ossl_prov_ctx_set0_handle(*provctx, handle);
    ossl_prov_ctx_set0_core_bio_method(*provctx, corebiometh);

    *out = deflt_dispatch_table;
    ossl_prov_cache_exported_algorithms(deflt_ciphers, exported_ciphers);

    return 1;
}
