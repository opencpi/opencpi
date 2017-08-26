#ifndef HOST_SMEM_SERVICES_H
#define HOST_SMEM_SERVICES_H

#include "XferEndPoint.h"
namespace DataTransfer {
  SmemServices& createHostSmemServices(EndPoint& loc);
}
#endif
