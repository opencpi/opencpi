This is where the HDL/FPGA apps go.
All apps result in a synthesized core called ocpi_app, with a precompiled library with the stub for ocpi_app, and an ngc file.  To use this app in the xst/ngc build process,
for xst add "app" to the end of the .lso file, add "app=$OCPI_DIR/hdl/apps/<theapp>/lib/hdl/virtex[5|6]" to the .ini file, and add
-sd $OCPI_DIR/hdl/apps/<theapp>/lib/hdl/xc5vsx95t-2-ff1136 
to both the xst script, and as an argument to ngdbuild.
(of course you need to rename the ngc file to ocpi_app).
