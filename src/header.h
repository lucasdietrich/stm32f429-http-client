// VS code C++ wrong error

#include "errno.h"

#define _LOG_INF(single) LOG_INF(single " [%d]", errno)
#define _LOG_WRN(single) LOG_WRN(single " [%d]", errno)
// #define _LOG_DBG(single) LOG_DBG(single "[%d]", errno)
#define _LOG_ERR(single) LOG_ERR(single " [%d]", errno)