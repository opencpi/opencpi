/*
 * ----------------------------------------------------------------------
 * A "kill" command.
 * ----------------------------------------------------------------------
 */

#include <tcl.h>

#if defined (__WIN32)

#include <windows.h>

static
int
tclkill (ClientData clientData,
	 Tcl_Interp * interp,
	 int objc,
	 Tcl_Obj * CONST objv[])
{
  int pid, res;
  HANDLE h;

  if (objc != 2) {
    Tcl_WrongNumArgs (interp, 1, objv, "pid");
    return TCL_ERROR;
  }

  if ((res = Tcl_GetIntFromObj (interp, objv[1], &pid)) != TCL_OK) {
    return res;
  }

  if (!(h = OpenProcess (PROCESS_TERMINATE, 0, pid))) {
    Tcl_AppendResult (interp, "invalid pid or access denied", NULL);
    return TCL_ERROR;
  }

  if (TerminateProcess (h, 1) == 0) {
    Tcl_AppendResult (interp, "TerminateProcess failed", NULL);
    return TCL_ERROR;
  }

  CloseHandle (h);

  Tcl_SetObjResult (interp, Tcl_NewObj());
  return TCL_OK;
}

#else

#include <sys/types.h>
#include <signal.h>

static
int
tclkill (ClientData clientData,
	 Tcl_Interp * interp,
	 int objc,
	 Tcl_Obj * CONST objv[])
{
  int pid, res;

  if (objc != 2) {
    Tcl_WrongNumArgs (interp, 1, objv, "pid");
    return TCL_ERROR;
  }

  if ((res = Tcl_GetIntFromObj (interp, objv[1], &pid)) != TCL_OK) {
    return res;
  }

  if (kill ((pid_t) pid, SIGTERM) != 0) {
    Tcl_AppendResult (interp, "invalid pid or access denied", NULL);
    return TCL_ERROR;
  }

  Tcl_SetObjResult (interp, Tcl_NewObj());
  return TCL_OK;
}

#endif

int
Kill_Init (Tcl_Interp * interp)
{
#ifdef USE_TCL_STUBS
  if (Tcl_InitStubs (interp, TCL_VERSION, 0) == NULL) {
    return TCL_ERROR;
  }
#else
  if (Tcl_PkgRequire (interp, "Tcl", TCL_VERSION, 0) == NULL) {
    return TCL_ERROR;
  }
#endif

  Tcl_CreateObjCommand (interp, "kill", tclkill, NULL, NULL);
  Tcl_PkgProvide (interp, "kill", "1.0");
  return TCL_OK;
}
