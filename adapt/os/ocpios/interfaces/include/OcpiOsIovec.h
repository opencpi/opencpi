#ifndef OCPI_OS_IOVEC_H
#define OCPI_OS_IOVEC_H
namespace OCPI {
  namespace OS {
    // This is defined in POSIX 1003..1g for those datagram systems that can
    // take advantage of that.
    struct IOVec {
      void *iov_base;
      size_t iov_len;
    };
  }
}
#endif
