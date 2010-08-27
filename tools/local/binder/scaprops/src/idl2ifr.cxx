// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

// preprocess and parse IDL files into an interface repository

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "IFR_Client/IFR_ComponentsC.h"
#include "orbsvcs/IFRService/IFR_Service_Utils.h"
#include "idl_defines.h"
#include "be_extern.h"
#include "be_util.h"
#include "utl_string.h"
#include "global_extern.h"
#include "fe_extern.h"
#include "ast_extern.h"
#include "ast_generator.h"
#include "idl2ifr.h"

#define CPP "cpp"
// UGH: tao ifr MUST have a file to write to for the IFR IOR
#ifdef WIN32
#define DEVNULL "NUL:"
#else
#define DEVNULL "/dev/null"
#endif

#if 0
 static ACE_THR_FUNC_RETURN
run_repo(void *orb_arg)
{
  CORBA::ORB *orb = (CORBA::ORB*)orb_arg;
  orb->run();
  return 0;
}
#endif
 static bool
make_ifr(CORBA::Repository_var &repo)
{
  static CORBA::ORB_var orb;
  static TAO_IFR_Server ifr; //  ifr.fini() not done...
  //  pthread_t repo_thread;
  // init the orb
  char *orb_args[2] = {"me", NULL};
  int orb_argc = 1;
  orb = CORBA::ORB_init (orb_argc, orb_args);
  // init the repository and run it in a thread
  // the strdup is to avoid a bug in TAO - it always frees the output file string...
  char *ifr_argv[] = {"ifr", "-o", strdup(DEVNULL), NULL};
  // Launch it, which registers itself as an initial reference
  assert(!ifr.init_with_orb(sizeof(ifr_argv) / sizeof(char*) - 1,
			    ifr_argv, orb.in()));
  // Get the reference, both for the idl-to-ifr phase, and to use for reading later.
  CORBA::Object_var obj = orb->resolve_initial_references ("InterfaceRepository");
  assert(!CORBA::is_nil(obj.in()));
  repo = CORBA::Repository::_narrow (obj.in());
  assert(!CORBA::is_nil(repo.in()));
  //  ACE_Thread::spawn(run_repo, orb);

  //  assert(!pthread_create(&repo_thread, NULL, run_repo, orb));
  return false;
}

 const char *
idl2ifr(char **argv, CORBA::Repository_var &repo)
{
  int argc = 0;
  for (char **ap = argv; *ap; ap++)
    argc++;
  if (!argc)
    return 0;

  // pass 1: compute command buffer size, record files.
  unsigned flag_max = 0, file_max = 0;
  for (char **ap = argv; *ap; ap++) {
    unsigned len = strlen(*ap) + 3;
    if (ap[0][0] == '-') {
      flag_max += len;
      if (strchr("DUI", ap[0][1])) {
	// if no attached string, next arg is it
	if (ap[0][2] == '\0') {
	  ap++;
	  if (!*ap)
	    return "expected one more argumnent after -D, -I, or -U";
	  flag_max += strlen(*ap) + 3;
	}
      } else
	return "invalid flag argument - not D, U, or I";
    } else if (len > file_max)
      file_max = len;
  }    
  // No files, no work
  if (!file_max)
    return 0;
  // Create command buffer big enough for all preprocessor runs
  char *cp = (char *)malloc(strlen(CPP) + 1 + flag_max + file_max);
  if (!cp)
    return "malloc failed";
  strcpy(cp, CPP);
  // Pass 2, create the command buffer filled with options.
  for (char **ap = argv; *ap; ap++)
    if (ap[0][0] == '-') {
      strcat(cp, " \"");
      strcat(cp, *ap);
      strcat(cp, "\"");
      if (ap[0][2] == '\0') {
	strcat(cp, " \"");
	strcat(cp, *++ap);
	strcat(cp, "\"");
      }
    }
  strcat(cp, " ");
  char *end = cp + strlen(cp);

  // These are scoped here.
  if (make_ifr(repo))
    return "couldn't create the ifr";
  // Now really do it.
  FE_init();
  // where to run cpp
  idl_global->set_prog_name("CPI_fix_impl");
  // no args to back end
  argc = 0;
  BE_init(argc, NULL);
  AST_Generator *gen = new AST_Generator();
  //be_util::generator_init ();
  idl_global->set_gen (gen);
  FE_populate ();
  // pass 3, process the files
  for (char **ap = argv; *ap; ap++)
    if (ap[0][0] == '-') {
      if (ap[0][2] == '\0')
	ap++;
    } else {
      const char *s = *ap;
      idl_global->idl_src_file (new UTL_String(s));
      idl_global->set_main_filename (new UTL_String(s));
      idl_global->set_real_filename (new UTL_String(s));
      idl_global->set_filename(new UTL_String(s));
      strcpy(end, s);
      // run the preprocessor here
      FILE * const preproc = popen (cp, "r");
      FE_set_yyin(preproc);
      
      (void) FE_yyparse ();
      // file is idl_global->filename ()->get_string ();
      idl_global->check_primary_keys ();
      if (idl_global->err_count () > 0)
	return "IDL errors encountered";
      AST_check_fwd_decls ();
      if (idl_global->err_count () > 0)
	return "IDL errors encountered";
      BE_produce();
    }
  if (idl_global->err_count ())
    return "Errors encountered";
  BE_cleanup ();
  be_global->destroy ();
  idl_global->fini ();
  return 0;
}
