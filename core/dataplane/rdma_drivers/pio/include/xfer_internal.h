
#ifndef XFER_INTERNAL_H_
#define XFER_INTERNAL_H_

#include <xfer_if.h>
#include <DtTransferInterface.h>

#define PIO       1

struct pio_template_ : public XFTemplate
{
  void *src_va;        /* virtual address of source buffer */
  void *dst_va;        /* virtual address of destination buffer */
  void *rdst_va[100]; 
  int   rdst;
  pio_template_():rdst(0){};
};

typedef struct pio_template_ * PIO_template;

struct pio_transfer_ : public XFTransfer
{
  struct pio_transfer_ *next;  /* pointer to next transfer */
  void                 *src_va;                /* virtual address of src buffer */
  void                 *dst_va;                /* virtual address of dst buffer */
  CPI::OS::uint32_t     src_off;
  CPI::OS::uint32_t     dst_off;
  CPI::OS::uint32_t     src_stride;            /* number of bytes between each element */
  CPI::OS::uint32_t     dst_stride;            /* number of bytes between each element */
  CPI::OS::int32_t      nbytes;                 /* size of transfer */
};

typedef struct pio_transfer_ * PIO_transfer;

struct xf_template_ : public XFTemplate
{
  void *src_va;              /* virtual address of src buffer (for PIO) */
  void *dst_va;              /* virtual address of dst buffer (for PIO) */
  PIO_template pio_template; /* pio template */
  CPI::OS::int32_t type;                /* template type, PIO, PXB DMA, CE DMA */
};

struct xf_transfer_ : public XFTransfer
{
  struct xf_template_ *xf_template;  
  PIO_transfer first_pio_transfer;
  PIO_transfer pio_transfer; /* pio transfer (for HOST) */
  PIO_transfer last_pio_transfer;
};

extern CPI::OS::int32_t xfer_pio_create(DataTransfer::SmemServices*, DataTransfer::SmemServices*, PIO_template *);
extern CPI::OS::int32_t xfer_pio_copy(PIO_template, CPI::OS::uint32_t, CPI::OS::uint32_t, CPI::OS::int32_t, CPI::OS::int32_t, 
                          PIO_transfer *);
extern CPI::OS::int32_t xfer_pio_start(PIO_transfer, CPI::OS::int32_t);
extern CPI::OS::int32_t xfer_pio_group(PIO_transfer *, CPI::OS::int32_t, PIO_transfer *);
extern CPI::OS::int32_t xfer_pio_release(PIO_transfer);
extern CPI::OS::int32_t xfer_pio_destroy(PIO_template);
extern void xfer_pio_modify( PIO_transfer, int,  CPI::OS::uint32_t*,  CPI::OS::uint32_t* );


#endif /* !defined XFER_INTERNAL_H */
