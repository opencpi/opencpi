# #### Location of the Altera tools ####################################### #

export OCPI_ALTERA_TOOLS_DIR=$OCPI_ALTERA_DIR/$OCPI_ALTERA_VERSION
if test "$OCPI_ALTERA_KITS_DIR" = ""; then
  export OCPI_ALTERA_KITS_DIR=$OCPI_ALTERA_DIR/$OCPI_ALTERA_VERSION/kits
fi

