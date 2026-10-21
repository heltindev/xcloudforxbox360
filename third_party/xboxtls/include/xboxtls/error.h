#ifndef XBOXTLS_ERROR_H
#define XBOXTLS_ERROR_H

namespace xboxtls {

enum Error {
    XT_OK = 0,
    XT_ERR_INVALID_ARGUMENT = -1,
    XT_ERR_BUFFER_TOO_SMALL = -2,
    XT_ERR_BAD_RECORD = -3,
    XT_ERR_BAD_HANDSHAKE = -4,
    XT_ERR_UNSUPPORTED = -5,
    XT_ERR_CRYPTO = -6,
    XT_ERR_VERIFY = -7,
    XT_ERR_IO = -8,
    XT_ERR_CLOSED = -9
};

}

#endif
