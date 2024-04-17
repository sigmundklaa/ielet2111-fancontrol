
/* Errno guard, which defines errno values only if they are not already defined
 */
#ifndef ERRNO_GUARD_H__
#define ERRNO_GUARD_H__

#include_next <errno.h>

#ifndef ENODEV
#define ENODEV (19)
#endif

#ifndef EBUSY
#define EBUSY (16)
#endif

#ifndef EIO
#define EIO (5)
#endif

#ifndef ENODATA
#define ENODATA (61)
#endif

#ifndef EINVAL
#define EINVAL (22)
#endif

#endif /* ERRNO_GUARD_H__ */