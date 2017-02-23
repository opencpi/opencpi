#include "ContainerWorker.h"
#include "HdlWciControl.h"
#include "HdlDummyWorker.h"
#include "HdlDevice.h"

namespace OCPI {
  namespace HDL {
    namespace OC = OCPI::Container;
    namespace OU = OCPI::Util;

DummyWorker::
DummyWorker(Device &device, ezxml_t impl, ezxml_t inst, const char *idx) 
  : OC::Worker(NULL, impl, inst, NULL, false),
    WciControl(device, impl, inst, properties(), false),
    m_name(ezxml_cattr(inst, "name")),
    m_wName(ezxml_cattr(impl, "name"))
{
  // We need to initialize the status of the worker since the OC::Worker class
  // object is being created without knowledge of previous state.
  // The worker's status register tells us the last control operation
  // that was performed.  It also has a sticky indication of
  // errors from the worker itself, but it doesn't remember whether the
  // previous control operation failed for other reasons (FIXME: the OCCP should
  // capture this information).  We do our best here by first bypassing the software.
  unsigned worker = atoi(idx);

  device.cAccess().offsetRegisters(m_wAccess, (intptr_t)(&((OccpSpace*)0)->worker[worker]));
  uint32_t
    control = m_wAccess.get32Register(control, OccpWorkerRegisters),
    l_status =  m_wAccess.get32Register(status, OccpWorkerRegisters);
  OU::Worker::ControlState cs;
  OU::Worker::ControlOperation lastOp =
    (OU::Worker::ControlOperation)OCCP_STATUS_LAST_OP(l_status);
  if (!(control & OCCP_WORKER_CONTROL_ENABLE))
    cs = OU::Worker::EXISTS; // there is no specific reset state since it isn't hetero
  else if (!(l_status & OCCP_STATUS_CONFIG_OP_VALID) || lastOp == 4)
    cs = OU::Worker::EXISTS; // no control op since reset
  else if (l_status & OCCP_STATUS_CONTROL_ERRORS)
    cs = OU::Worker::UNUSABLE;
  else if (lastOp == OU::Worker::OpRelease)
    cs = OU::Worker::UNUSABLE;
  else if (l_status & OCCP_STATUS_FINISHED)
    cs = OU::Worker::FINISHED;
  else
    switch(lastOp) {
    case OU::Worker::OpInitialize: cs = OU::Worker::INITIALIZED; break;
    case OU::Worker::OpStart: cs = OU::Worker::OPERATING; break;
    case OU::Worker::OpStop: cs = OU::Worker::SUSPENDED; break;
    default:
      cs = OU::Worker::OPERATING;
      // FIXME:  the beforeQuery, and AfterConfig and test ops screw us up here.
    }
  setControlState(cs);
}
const char *DummyWorker::
status() {
  return m_wAccess.get32Register(control, OccpWorkerRegisters) & OCCP_WORKER_CONTROL_ENABLE ?
    OU::Worker::s_controlStateNames[getState()] : "RESET";
}
}
}
