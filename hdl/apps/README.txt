This is where the HDL/FPGA apps go.
All apps result in a synthesized core called ocpi_app, with a precompiled library with the stub for ocpi_app, and an ngc file.  To use this app in the xst/ngc build process,

For xst in a shep drop:

1. setenv OCPIDIR to top of CP tree
2. to make "board" until it fails (fix to do the other stuff)
3. go to build/tmp-board
4. add "app" to the end of the .lso file
5. add "app=$OCPI_DIR/hdl/apps/<theapp>/lib/hdl/virtex[5|6]" to the .ini file
e.g.:
app=$OCPI_DIR/hdl/apps/testpsd/lib/hdl/virtex5

5a. make sure "bsv" and "ocpi" are also there as in:
e.g.:
bsv=$OCPI_DIR/ocpi/lib/hdl/bsv/virtex5
ocpi=$OCPI_DIR/ocpi/lib/hdl/ocpi/virtex5

6. add -sd $OCPI_DIR/hdl/apps/<theapp>/lib/hdl/xc5vsx95t-2-ff1136 to .xst file
e.g.:
-sd $OCPI_DIR/hdl/apps/testpsd/lib/hdl/xc5vlx50t-1-ff1136

7. add -sd $OCPI_DIR/hdl/apps/<theapp>/lib/hdl/xc5vsx95t-2-ff1136 as an argument to ngdbuild in the build fpgaTop script.
e.g.:
  -sd $OCPI_DIR/hdl/apps/testpsd/lib/hdl/$PART

8. edit the project file to remove stuff and reference the correct mkOCApp.v file in apps
e.g.:
  remove any existing OCApp, or app workers
  remote any existing library primitives
9. run build_fpgaTop



