// VS code C++ wrong error
#define _LOG_INF(single) LOG_INF(single " [%d]", errno)
#define _LOG_WRN(single) LOG_WRN(single " [%d]", errno)
// #define _LOG_DBG(single) LOG_DBG(single "[%d]", errno)
#define _LOG_ERR(single) LOG_ERR(single " [%d]", errno)

#define JSON_PARAM_FILLED(param_bitmask, n) READ_BIT(param_bitmask, 1 << (n))