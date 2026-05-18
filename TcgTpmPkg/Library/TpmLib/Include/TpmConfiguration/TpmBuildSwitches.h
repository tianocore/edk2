/** @file
  This file is copied from
    - https://github.com/TrustedComputingGroup/TPM/blob/main/TPMCmd/TpmConfiguration/TpmConfiguration/TpmBuildSwitches.h
  to set build option for TPM reference library.

  All option is the same to original file except SIMULATION option.
  This is turned off to disable building of the simulation layers.

  If additional compile options are required which not specified in
  this file, add TpmLibCompileOptions.h (i.e) crypto library and etc.

**/

#pragma once

#if defined (YES) || defined (NO)
  #error YES and NO should be defined in TpmBuildSwitches.h
#endif
#if defined (SET) || defined (CLEAR)
  #error SET and CLEAR should be defined in TpmBuildSwitches.h
#endif

#define YES    1
#define SET    1
#define NO     0
#define CLEAR  0

// TRUE/FALSE may be coming from system headers, but if not, provide them.
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE  0
#endif

// Need an unambiguous definition for DEBUG. Do not change this
#undef DEBUG
#ifdef NDEBUG
#define DEBUG  NO
#else
#define DEBUG  YES
#endif

////////////////////////////////////////////////////////////////
// DEBUG OPTIONS
////////////////////////////////////////////////////////////////

// The SIMULATION switch allows certain other macros to be enabled. The things that
// can be enabled in a simulation include key caching, reproducible "random"
// sequences, instrumentation of the RSA key generation process, and certain other
// debug code. SIMULATION Needs to be defined as either YES or NO. This grouping of
// macros will make sure that it is set correctly. A simulated TPM would include a
// Virtual TPM. The interfaces for a Virtual TPM should be modified from the standard
// ones in the Simulator project.
#define SIMULATION  NO

// ENABLE_TPM_DEBUG_PRINT enables arbitrary string printing.
// enables the TPM_DEBUG_PRINT macro to route debugging strings
// to the _plat_debug_out function
#define ENABLE_TPM_DEBUG_PRINT      (YES * SIMULATION)

//  ENABLE_TPM_DEBUG_TRACE enables code tracing macros - depends on TPM_DEBUG_PRINT
#define ENABLE_TPM_DEBUG_TRACE      (NO  * ENABLE_TPM_DEBUG_PRINT)

//  ENABLE_CRYPTO_DEBUG enables printing of actual crypto values. This is entirely insecure.
#define ENABLE_CRYPTO_DEBUG         (YES * ENABLE_TPM_DEBUG_PRINT)

// The CRYPTO_LIB_REPORTING switch allows the TPM to report its
// crypto library implementation, e.g., at simulation startup.
#define CRYPTO_LIB_REPORTING  NO

// If doing debug, can set the DRBG to print out the intermediate test values.
// Before enabling this, make sure that the dbgDumpMemBlock() function
// has been added someplace (preferably, somewhere in CryptRand.c)
#define DRBG_DEBUG_PRINT  (NO  * DEBUG)

// This define is used to control the debug for the CertifyX509 command.
#define CERTIFYX509_DEBUG  (YES * DEBUG)

// This provides fixed seeding of the RNG when doing debug on a simulator. This
// should allow consistent results on test runs as long as the input parameters
// to the functions remains the same.
#define USE_DEBUG_RNG  (NO  * DEBUG)

////////////////////////////////////////////////////////////////
// RSA DEBUG OPTIONS
////////////////////////////////////////////////////////////////

// Enable the instrumentation of the sieve process. This is used to tune the sieve
// variables.
#define RSA_INSTRUMENT  (NO  * DEBUG)

// Enables use of the key cache. Default is YES
#define USE_RSA_KEY_CACHE  (NO  * DEBUG)

// Enables use of a file to store the key cache values so that the TPM will start
// faster during debug. Default for this is YES
#define USE_KEY_CACHE_FILE  (NO  * DEBUG)

////////////////////////////////////////////////////////////////
// TEST OPTIONS
////////////////////////////////////////////////////////////////
// The SIMULATION flag can enable test crypto behaviors and caching that
// significantly change the behavior of the code.  This flag controls only the
// g_forceFailureMode flag in the TPM library while leaving the rest of the TPM
// behavior alone.  Useful for testing when the full set of options controlled by
// SIMULATION may not be desired.
#define ALLOW_FORCE_FAILURE_MODE  NO

////////////////////////////////////////////////////////////////
// Internal checks
////////////////////////////////////////////////////////////////

// Define this to run the function that checks the compatibility between the
// chosen big number math library and the TPM code. Not all ports use this.
#define LIBRARY_COMPATIBILITY_CHECK  YES

// In some cases, the relationship between two values may be dependent on things that
// change based on various selections like the chosen cryptographic libraries. It is
// possible that these selections will result in incompatible settings. These are often
// detectable by the compiler but it is not always possible to do the check in the
// preprocessor code. For example, when the check requires use of 'sizeof'() then the
// preprocessor can't do the comparison. For these cases, we include a special macro
// that, depending on the compiler will generate a warning to indicate if the check
// always passes or always fails because it involves fixed constants.
//
// In modern compilers this is now commonly known as a static_assert, but the precise
// implementation varies by compiler. CompilerDependencies.h defines MUST_BE as a macro
// that abstracts out the differences, and COMPILER_CHECKS can remove the checks where
// the current compiler doesn't support it.  COMPILER_CHECKS should be enabled if the
// compiler supports some form of static_assert.
// See the CompilerDependencies_*.h files for specific implementations per compiler.
#define COMPILER_CHECKS  YES

// Some of the values (such as sizes) are the result of different options set in
// TpmProfile.h. The combination might not be consistent. A function is defined
// (TpmSizeChecks()) that is used to verify the sizes at run time. To enable the
// function, define this parameter.
#define RUNTIME_SIZE_CHECKS  YES

////////////////////////////////////////////////////////////////
// Compliance options
////////////////////////////////////////////////////////////////

// Enable extra behaviors to meet FIPS compliance requirements
#define FIPS_COMPLIANT  YES

// Indicates if the implementation is to compute the sizes of the proof and primary
// seed size values based on the implemented algorithms.
#define USE_SPEC_COMPLIANT_PROOFS  YES

// Set this to allow compile to continue even though the chosen proof values
// do not match the compliant values. This is written so that someone would
// have to proactively ignore errors.
#define SKIP_PROOF_ERRORS  NO

////////////////////////////////////////////////////////////////
// Implementation alternatives - don't  change external behavior
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// Implementation alternatives - don't  change external behavior
////////////////////////////////////////////////////////////////
// does the target system have longjmp support, AND we want to use it?
#define LONGJMP_SUPPORTED           NO

// This define is used to enable the new table-driven marshaling code.
#define TABLE_DRIVEN_MARSHAL  NO

// Enable the generation of RSA primes using a sieve.
#define RSA_KEY_SIEVE  YES

////////////////////////////////////////////////////////////////
// Implementation alternatives - changes external behavior
////////////////////////////////////////////////////////////////

// This switch enables the RNG state save and restore
#define _DRBG_STATE_SAVE  YES

// Definition to allow alternate behavior for non-orderly startup. If there is a
// chance that the TPM could not update 'failedTries'
#define USE_DA_USED  YES

// This switch is used to enable the self-test capability in AlgorithmTests.c
#define ENABLE_SELF_TESTS  YES

// This switch indicates where clock epoch value should be stored. If this value
// defined, then it is assumed that the timer will change at any time so the
// nonce should be a random number kept in RAM. When it is not defined, then the
// timer only stops during power outages.
#define CLOCK_STOPS  NO

// Indicate if the implementation is going to give lockout time credit for time up to
// the last orderly shutdown.
#define ACCUMULATE_SELF_HEAL_TIMER  YES

// If an assertion event is not going to produce any trace information (function and
// line number) then make FAIL_TRACE == NO
#define FAIL_TRACE  YES

// TODO_RENAME_INC_FOLDER: public refers to the TPM_CoreLib public headers
#include <tpm_public/CompilerDependencies.h>

#ifndef UINT32_MAX
#define UINT32_MAX  0xFFFFFFFF
#endif
