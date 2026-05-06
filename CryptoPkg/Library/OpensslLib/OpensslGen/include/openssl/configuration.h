#if defined(EDK2_OPENSSL_NOEC)
# include "configuration-noec.h"
#elif defined(EDK2_OPENSSL_LITE)
# include "configuration-ec-lite.h"
#else
# include "configuration-ec.h"
#endif
