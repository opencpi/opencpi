
#ifndef XFER_IF_H
#define XFER_IF_H


#include <DtTransferInterface.h>

/* Constants */
#define XFER_FLAG  1
#define XFER_FIRST 2
#define XFER_LAST  4
#define XFER_MASK  6

/* Types */
typedef unsigned long ulong;

struct  XFTemplate {
  DataTransfer::SmemServices* s_smem;
  DataTransfer::SmemServices* t_smem;
  int                         s_off;
  int                         t_off;
};

class Mapit {
 public:
  virtual void map( unsigned int, unsigned int){};
  virtual void unmap(){};
  virtual ~Mapit(){};
};

struct XFTransfer  {
XFTransfer():m(NULL){};
  Mapit *m;
};

typedef XFTemplate* XF_template;
typedef XFTransfer* XF_transfer;

/* External functions */
extern long xfer_create(DataTransfer::SmemServices*, DataTransfer::SmemServices*, CPI::OS::int32_t , XF_template *);
extern long xfer_copy(XF_template, CPI::OS::uint32_t, CPI::OS::uint32_t, CPI::OS::uint32_t, CPI::OS::int32_t, XF_transfer *);
extern long xfer_group(XF_transfer *, CPI::OS::int32_t, XF_transfer *);
extern long xfer_release(XF_transfer, CPI::OS::int32_t);
extern long xfer_destroy(XF_template, CPI::OS::int32_t);
extern long xfer_start(XF_transfer, CPI::OS::int32_t);
extern long xfer_get_status(XF_transfer);
extern long xfer_modify( XF_transfer, CPI::OS::uint32_t* noff, CPI::OS::uint32_t* ooff );

#endif /* !defined XFER_IF_H */
