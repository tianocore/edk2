#if defined(EDK2_OPENSSL_NOEC)
# include "configuration-noec.h"
#elif defined(EDK2_OPENSSL_NOCAMELLIA)
# include "configuration-ec-nocamellia.h"
#else
# include "configuration-ec.h"
#endif
