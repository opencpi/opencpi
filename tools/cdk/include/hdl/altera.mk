# Common definition for altera tools
ifndef OCPI_ALTERA_TOOLS_DIR
  ifneq (clean,$(MAKECMDGOALS))
    $(error The variable OCPI_ALTERA_TOOLS_DIR must be defined.)
  endif
endif
# The trick here is to filter the output and capture the exit code.
# Note THIS REQUIRES BASH not just POSIX SH due to pipefail option
DoAltera=(set -o pipefail; set +e; \
         export LM_LICENSE_FILE=$(OCPI_ALTERA_LICENSE_FILE) ; \
         $(TIME) $(OCPI_ALTERA_TOOLS_DIR)/quartus/bin/$1 --64bit $2 > $3-$4.tmp 2>&1; \
         ST=\$$?; sed 's/^.\[0m.\[0;32m//' < $3-$4.tmp > $3-$4.out ; \
         rm $3-$4.tmp ; \
         if test \$$ST = 0; then \
           $(ECHO) -n $1:' succeeded ' ; \
           tail -1 $3-$4.out | tee x1 | tr -d \\\n | tee x2; \
           $(ECHO) -n ' at ' && date +%T ; \
         else \
           $(ECHO)  $1: failed with status \$$ST \(see results in $(TargetDir)/$3-$4.out\); \
         fi; \
         exit \$$ST)

DoAltera1=(set -o pipefail; set +e; \
         export LM_LICENSE_FILE=$(OCPI_ALTERA_LICENSE_FILE) ; \
         $(TIME) $(OCPI_ALTERA_TOOLS_DIR)/quartus/bin/$1 --64bit $2 > $3-$4.tmp 2>&1; \
         ST=$$$$?; sed 's/^.\[0m.\[0;32m//' < $3-$4.tmp > $3-$4.out ; \
         rm $3-$4.tmp ; \
         if test $$$$ST = 0; then \
           $(ECHO) -n $1:' succeeded ' ; \
           tail -1 $3-$4.out | tee x1 | tr -d \\\n | tee x2; \
           $(ECHO) -n ' at ' && date +%T ; \
         else \
           $(ECHO)  $1: failed with status $$$ST \(see results in $(TargetDir)/$3-$4.out\); \
         fi; \
         exit $$$$ST)

