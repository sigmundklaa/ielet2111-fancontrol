
#include "error.h"

const char* e_str(unsigned int err)
{
        switch (err) {
        case E_NODEV:
                return "No such device";
        case E_BUSY:
                return "Device or resource busy";
        case E_IO:
                return "I/O error";
        case E_NODATA:
                return "No data available";
        case E_INVAL:
                return "Invalid argument";
        case E_NOMEM:
                return "Out of memory";
        case E_NOENT:
                return "No such file or directory";
        default:
                break;
        }

        return "Unknown error";
}