#include  <x86/limits.h>

#define __POINTER_BIT   32
#define __LONG_BIT      32

/** minimum value for an object of type long int **/
#define __LONG_MIN    (-2147483647L - 1L) // -(2^31 - 1)

/** maximum value for an object of type long int **/
#define __LONG_MAX    +2147483647L // 2^31 - 1

/** maximum value for an object of type unsigned long int **/
#define __ULONG_MAX   0xffffffff // 2^32 - 1

