#include  <x86/limits.h>

#define __POINTER_BIT   64

#if defined(__GNUC__)
#if __GNUC_PREREQ__(4,4)
  #define __LONG_BIT      64

  /** minimum value for an object of type long int **/
  #define __LONG_MIN    (-9223372036854775807LL - 1LL) // -(2^63 - 2)

  /** maximum value for an object of type long int **/
  #define __LONG_MAX     (9223372036854775807LL) // 2^63 - 1

  /** maximum value for an object of type unsigned long int **/
  #define __ULONG_MAX   0xFFFFFFFFFFFFFFFFULL // 2^64 - 1
#else
  #define __LONG_BIT      32
  /** minimum value for an object of type long int **/
  #define __LONG_MIN    (-2147483647L - 1L) // -(2^31 - 1)

  /** maximum value for an object of type long int **/
  #define __LONG_MAX     2147483647L // 2^31 - 1

  /** maximum value for an object of type unsigned long int **/
  #define __ULONG_MAX   0xffffffff // 2^32 - 1
#endif


#else /* NOT defined(__GNUC__)  */
#define __LONG_BIT      32
/** minimum value for an object of type long int **/
#define __LONG_MIN    (-2147483647L - 1L) // -(2^31 - 1)

/** maximum value for an object of type long int **/
#define __LONG_MAX     2147483647L // 2^31 - 1

/** maximum value for an object of type unsigned long int **/
#define __ULONG_MAX   0xffffffff // 2^32 - 1
#endif
