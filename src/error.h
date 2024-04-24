/* The Microchip errno.h file has defined all errno values to
 * ENOERR
 */
#ifndef ERROR_H__
#define ERROR_H__

#include <errno.h>

#define E_NODEV (19)
#define E_BUSY (16)
#define E_IO (5)
#define E_NODATA (61)
#define E_INVAL (22)
#define E_NOMEM (12)
#define E_NOENT (2)

const char* e_str(unsigned int err);

#endif /* ERROR_H__ */