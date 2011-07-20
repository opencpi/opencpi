# Common definition for altera tools
ifndef OCPI_ALTERA_TOOLS_DIR
$(error The variable OCPI_ALTERA_TOOLS_DIR must be defined.)
endif
# The trick here is to filter the output and capture the exit code.
# Note THIS REQUIRES BASH not just POSIX SH due to pipefail option
DoAltera=set -o pipefail; $(OCPI_ALTERA_TOOLS_DIR)/quartus/bin/$1 2>&1 | sed 's/^.\[0m.\[0;32m//'; set +o pipefail
#; echo HELLO X=\$$?Y)
