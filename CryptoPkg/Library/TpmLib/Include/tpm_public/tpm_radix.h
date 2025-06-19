//** Introduction
// Common defines for supporting large numbers and cryptographic buffer sizing.
//*********************
#ifndef RADIX_BITS
#  if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__)                 \
      || defined(__amd64) || defined(_WIN64) || defined(_M_X64) || defined(_M_ARM64) \
      || defined(__aarch64__) || defined(__PPC64__) || defined(__s390x__)            \
      || defined(__powerpc64__) || defined(__ppc64__)
#    define RADIX_BITS 64
#  elif defined(__i386__) || defined(__i386) || defined(i386) || defined(_WIN32) \
      || defined(_M_IX86)
#    define RADIX_BITS 32
#  elif defined(_M_ARM) || defined(__arm__) || defined(__thumb__)
#    define RADIX_BITS 32
#  elif defined(__riscv)
// __riscv and __riscv_xlen are standardized by the RISC-V community and should be available
// on any compliant compiler.
//
// https://github.com/riscv-non-isa/riscv-toolchain-conventions
#    define RADIX_BITS __riscv_xlen
#  else
#    error Unable to determine RADIX_BITS from compiler environment
#  endif
#endif  // RADIX_BITS

#if RADIX_BITS == 64
#  define RADIX_BYTES 8
#  define RADIX_LOG2  6
#elif RADIX_BITS == 32
#  define RADIX_BYTES 4
#  define RADIX_LOG2  5
#else
#  error "RADIX_BITS must either be 32 or 64"
#endif

#define HASH_ALIGNMENT      RADIX_BYTES
#define SYMMETRIC_ALIGNMENT RADIX_BYTES

#define RADIX_MOD(x) ((x) & ((1 << RADIX_LOG2) - 1))
#define RADIX_DIV(x) ((x) >> RADIX_LOG2)
#define RADIX_MASK   ((((crypt_uword_t)1) << RADIX_LOG2) - 1)

#define BITS_TO_CRYPT_WORDS(bits)   RADIX_DIV((bits) + (RADIX_BITS - 1))
#define BYTES_TO_CRYPT_WORDS(bytes) BITS_TO_CRYPT_WORDS(bytes * 8)
#define SIZE_IN_CRYPT_WORDS(thing)  BYTES_TO_CRYPT_WORDS(sizeof(thing))

#if RADIX_BITS == 64
#  define SWAP_CRYPT_WORD(x) REVERSE_ENDIAN_64(x)
typedef uint64_t crypt_uword_t;
typedef int64_t  crypt_word_t;
#  define TO_CRYPT_WORD_64             BIG_ENDIAN_BYTES_TO_UINT64
#  define TO_CRYPT_WORD_32(a, b, c, d) TO_CRYPT_WORD_64(0, 0, 0, 0, a, b, c, d)
#elif RADIX_BITS == 32
#  define SWAP_CRYPT_WORD(x) REVERSE_ENDIAN_32((x))
typedef uint32_t crypt_uword_t;
typedef int32_t  crypt_word_t;
#  define TO_CRYPT_WORD_64(a, b, c, d, e, f, g, h) \
      BIG_ENDIAN_BYTES_TO_UINT32(e, f, g, h), BIG_ENDIAN_BYTES_TO_UINT32(a, b, c, d)
#endif

#define MAX_CRYPT_UWORD (~((crypt_uword_t)0))
#define MAX_CRYPT_WORD  ((crypt_word_t)(MAX_CRYPT_UWORD >> 1))
#define MIN_CRYPT_WORD  (~MAX_CRYPT_WORD)

// Avoid expanding LARGEST_NUMBER into a long expression that inlines 3 other long expressions.
// TODO: Decrease the size of each of the MAX_* expressions with improvements to the code generator.
#if ALG_RSA == ALG_YES
// The smallest supported RSA key (1024 bits) is larger than
// the largest supported ECC curve (628 bits)
// or the largest supported digest (512 bits)
#  define LARGEST_NUMBER MAX_RSA_KEY_BYTES
#elif ALG_ECC == ALG_YES
#  define LARGEST_NUMBER MAX(MAX_ECC_KEY_BYTES, MAX_DIGEST_SIZE)
#else
#  define LARGEST_NUMBER MAX_DIGEST_SIZE
#endif  // ALG_RSA == YES

#define LARGEST_NUMBER_BITS (LARGEST_NUMBER * 8)

#define MAX_ECC_PARAMETER_BYTES (MAX_ECC_KEY_BYTES * ALG_ECC)

// round up to a multiple of stride; by 0 if already a multiple
#define ALIGN_UP(size, stride) ((((size) + (stride) - 1) / (stride)) * (stride))
// rounds down to multiple of stride
#define ALIGN_DOWN(size, stride) ((size) - ((size) % (stride)))
#ifndef IS_ALIGNED
#define IS_ALIGNED(ptr, stride)  ((((size_t)ptr) % (stride)) == 0)
#endif
#define IS_ALIGNED4(ptr)         IS_ALIGNED(ptr, 4)

// These macros use the selected libraries to get the proper include files.
// clang-format off
#define LIB_QUOTE(_STRING_)                    #_STRING_
#define LIB_INCLUDE2(_PREFIX_, _LIB_, _TYPE_)  LIB_QUOTE(_LIB_/_PREFIX_##_LIB_##_TYPE_.h)
#define LIB_INCLUDE(_PREFIX_, _LIB_, _TYPE_)   LIB_INCLUDE2(_PREFIX_,_LIB_, _TYPE_)
// clang-format on
