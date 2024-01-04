/* Generated from windows-errors.txt */
#include <nbdkit-plugin.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <errno.h>
int
translate_winsock_error (const char *fn, int err) {
    nbdkit_debug ("%s: winsock error %d", fn, err);
    switch (err) {
#if defined(WSA_INVALID_HANDLE) && defined(EBADF)

    case WSA_INVALID_HANDLE: return EBADF;

#endif
#if defined(WSA_NOT_ENOUGH_MEMORY) && defined(ENOMEM)

    case WSA_NOT_ENOUGH_MEMORY: return ENOMEM;

#endif
#if defined(WSA_INVALID_PARAMETER) && defined(EINVAL)

    case WSA_INVALID_PARAMETER: return EINVAL;

#endif
#if defined(WSA_OPERATION_ABORTED) && defined(ECONNABORTED)

    case WSA_OPERATION_ABORTED: return ECONNABORTED;

#endif
#if defined(WSA_IO_INCOMPLETE) && defined(EWOULDBLOCK)

    case WSA_IO_INCOMPLETE: return EWOULDBLOCK;

#endif
#if defined(WSA_IO_PENDING	) && defined(EWOULDBLOCK)

    case WSA_IO_PENDING	: return EWOULDBLOCK;

#endif
#if defined(WSAEINTR	) && defined(EINTR)

    case WSAEINTR	: return EINTR;

#endif
#if defined(WSAEBADF	) && defined(EBADF)

    case WSAEBADF	: return EBADF;

#endif
#if defined(WSAEACCES	) && defined(EACCES)

    case WSAEACCES	: return EACCES;

#endif
#if defined(WSAEFAULT	) && defined(EFAULT)

    case WSAEFAULT	: return EFAULT;

#endif
#if defined(WSAEINVAL	) && defined(EINVAL)

    case WSAEINVAL	: return EINVAL;

#endif
#if defined(WSAEMFILE	) && defined(EMFILE)

    case WSAEMFILE	: return EMFILE;

#endif
#if defined(WSAEWOULDBLOCK	) && defined(EWOULDBLOCK)

    case WSAEWOULDBLOCK	: return EWOULDBLOCK;

#endif
#if defined(WSAEINPROGRESS	) && defined(EINPROGRESS)

    case WSAEINPROGRESS	: return EINPROGRESS;

#endif
#if defined(WSAEALREADY	) && defined(EALREADY)

    case WSAEALREADY	: return EALREADY;

#endif
#if defined(WSAENOTSOCK	) && defined(ENOTSOCK)

    case WSAENOTSOCK	: return ENOTSOCK;

#endif
#if defined(WSAEDESTADDRREQ	) && defined(EDESTADDRREQ)

    case WSAEDESTADDRREQ	: return EDESTADDRREQ;

#endif
#if defined(WSAEMSGSIZE	) && defined(EMSGSIZE)

    case WSAEMSGSIZE	: return EMSGSIZE;

#endif
#if defined(WSAEPROTOTYPE	) && defined(EPROTOTYPE)

    case WSAEPROTOTYPE	: return EPROTOTYPE;

#endif
#if defined(WSAENOPROTOOPT	) && defined(ENOPROTOOPT)

    case WSAENOPROTOOPT	: return ENOPROTOOPT;

#endif
#if defined(WSAEPROTONOSUPPORT) && defined(EPROTONOSUPPORT)

    case WSAEPROTONOSUPPORT: return EPROTONOSUPPORT;

#endif
#if defined(WSAESOCKTNOSUPPORT) && defined(ESOCKTNOSUPPORT)

    case WSAESOCKTNOSUPPORT: return ESOCKTNOSUPPORT;

#endif
#if defined(WSAEOPNOTSUPP	) && defined(EOPNOTSUPP)

    case WSAEOPNOTSUPP	: return EOPNOTSUPP;

#endif
#if defined(WSAEPFNOSUPPORT	) && defined(EPFNOSUPPORT)

    case WSAEPFNOSUPPORT	: return EPFNOSUPPORT;

#endif
#if defined(WSAEAFNOSUPPORT	) && defined(EAFNOSUPPORT)

    case WSAEAFNOSUPPORT	: return EAFNOSUPPORT;

#endif
#if defined(WSAEADDRINUSE	) && defined(EADDRINUSE)

    case WSAEADDRINUSE	: return EADDRINUSE;

#endif
#if defined(WSAEADDRNOTAVAIL) && defined(EADDRNOTAVAIL)

    case WSAEADDRNOTAVAIL: return EADDRNOTAVAIL;

#endif
#if defined(WSAENETDOWN	) && defined(ENETDOWN)

    case WSAENETDOWN	: return ENETDOWN;

#endif
#if defined(WSAENETUNREACH	) && defined(ENETUNREACH)

    case WSAENETUNREACH	: return ENETUNREACH;

#endif
#if defined(WSAENETRESET	) && defined(ENETRESET)

    case WSAENETRESET	: return ENETRESET;

#endif
#if defined(WSAECONNABORTED	) && defined(ECONNABORTED)

    case WSAECONNABORTED	: return ECONNABORTED;

#endif
#if defined(WSAECONNRESET	) && defined(ECONNRESET)

    case WSAECONNRESET	: return ECONNRESET;

#endif
#if defined(WSAENOBUFS	) && defined(ENOBUFS)

    case WSAENOBUFS	: return ENOBUFS;

#endif
#if defined(WSAEISCONN	) && defined(EISCONN)

    case WSAEISCONN	: return EISCONN;

#endif
#if defined(WSAENOTCONN	) && defined(ENOTCONN)

    case WSAENOTCONN	: return ENOTCONN;

#endif
#if defined(WSAESHUTDOWN	) && defined(ESHUTDOWN)

    case WSAESHUTDOWN	: return ESHUTDOWN;

#endif
#if defined(WSAETOOMANYREFS	) && defined(ETOOMANYREFS)

    case WSAETOOMANYREFS	: return ETOOMANYREFS;

#endif
#if defined(WSAETIMEDOUT	) && defined(ETIMEDOUT)

    case WSAETIMEDOUT	: return ETIMEDOUT;

#endif
#if defined(WSAECONNREFUSED	) && defined(ECONNREFUSED)

    case WSAECONNREFUSED	: return ECONNREFUSED;

#endif
#if defined(WSAELOOP	) && defined(ELOOP)

    case WSAELOOP	: return ELOOP;

#endif
#if defined(WSAENAMETOOLONG	) && defined(ENAMETOOLONG)

    case WSAENAMETOOLONG	: return ENAMETOOLONG;

#endif
#if defined(WSAEHOSTDOWN	) && defined(EHOSTDOWN)

    case WSAEHOSTDOWN	: return EHOSTDOWN;

#endif
#if defined(WSAEHOSTUNREACH	) && defined(EHOSTUNREACH)

    case WSAEHOSTUNREACH	: return EHOSTUNREACH;

#endif
#if defined(WSAENOTEMPTY	) && defined(ENOTEMPTY)

    case WSAENOTEMPTY	: return ENOTEMPTY;

#endif
#if defined(WSAEPROCLIM	) && defined(EMFILE)

    case WSAEPROCLIM	: return EMFILE;

#endif
#if defined(WSAEUSERS	) && defined(EUSERS)

    case WSAEUSERS	: return EUSERS;

#endif
#if defined(WSAEDQUOT	) && defined(EDQUOT)

    case WSAEDQUOT	: return EDQUOT;

#endif
#if defined(WSAESTALE	) && defined(ESTALE)

    case WSAESTALE	: return ESTALE;

#endif
#if defined(WSAEREMOTE	) && defined(EREMOTE)

    case WSAEREMOTE	: return EREMOTE;

#endif
#if defined(WSASYSNOTREADY	) && defined(EINVAL)

    case WSASYSNOTREADY	: return EINVAL;

#endif
#if defined(WSAVERNOTSUPPORTED) && defined(EINVAL)

    case WSAVERNOTSUPPORTED: return EINVAL;

#endif
#if defined(WSANOTINITIALISED) && defined(EINVAL)

    case WSANOTINITIALISED: return EINVAL;

#endif
#if defined(WSAEDISCON	) && defined(ESHUTDOWN)

    case WSAEDISCON	: return ESHUTDOWN;

#endif
#if defined(WSAENOMORE	) && defined(ESHUTDOWN)

    case WSAENOMORE	: return ESHUTDOWN;

#endif
#if defined(WSAECANCELLED	) && defined(ECANCELED)

    case WSAECANCELLED	: return ECANCELED;

#endif
    default:
    return err > 10000 && err < 10025 ? err - 10000 : EINVAL;
    }
}
#endif /* WIN32 */
