Steps to build linux kernel from the Digilent repo in a subdirectory called linux.
Note this ALSO provides a kernel build area to point to when we build our linux driver
Information derived from (at least) three places:
1. Digilent document: ZedBoard_GSwEL_Guide.pdf
2. The README from the ZedBoard_OOB_Design/doc/README.txt
3. The Xilinx wiki at: http://www.wiki.xilinx.com/Build+kernel

First, grab a copy of the linux source tree from the Digilent repo
   
  % git clone https://github.com/Digilent/linux-digilent.git

This will create a directory linux-digilent with a linux source tree in it.

Next, checkout the tagged version in this repo clone to the version approved by digilent

  % cd linux-digilent
  % git checkout -b zedboard_oob v3.3.0-digilent-12.07-zed-beta

Now the tree coresponds to that label.
At this point you must have the OpenCPI environment set up to cross-build for zync.
An example is in the "jkzed.sh" env setup file in the OpenCPI root dir, which you must
modify to point to the Xilinx tools properly.  We currently get the cross-tools from
the xilinx tools tree, and not any code sourcery download.

  % pushd <your-ocpi-root>; . jkzed.sh; popd


Next, configure the linux tree according to Digilent's default configuration, which
corresponds to the linux kernel on the default Digilent/Zedboard SD card.

  % make ARCH=arm digilent_zed_defconfig

This basically results in a file ".config" at the top level that defines the
configuration.

Next we modify the linux configuration to include NFS client support.  To do this we
run the linux text-based configuration tool, "menuconfig", to do this:

  % make menuconfig to add nfs client:

This puts up a text window (can't run this in an emacs shell window)

 1. make menuconfig ARCH=arm
 2. navigate down to "file systems", hit return
 3. navigate down to "network file systems", hit Y to enable and turn on asterisk
 4. hit return to go INSIDE Network File Systems
 5. navigate down to NFS Client Support, and hit Y to enable and expose more options
 6. navigate down to NFS Client SUpport for NFS3, hit Y
 7. navigate down to NFS Client Support for NFS4, hit Y (skip the "ACL protocol extension")
 8. hit double escapes until prompted to save new configuration, say yes/return

This now results in an updated ".config" file.

Now we can build linux (again assuming you have OpenCPI env cross tools set up):

  % make ARCH=arm CROSS_COMPILE=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-

Yes that hyphen is at the end of the previous line...
If there are problems and you need to see all the commands that get executed
during the kernel build you can add "KBUILD_VERBOSE=1" to the end of the line.

The result of this process is:
1. You have a kernel image for your boot SD card in: arch/arm/boot/zImage
2. You have a tree that is ready to build the OpenCPI linux kernel driver against.

To build the OpenCPI linux driver you to "make cleandriver; make driver",
(again, with the cross tools environment set up as in jkzed.sh), but
you must set the OCPI_KERNEL_DIR variable to point to the linux tree you
are building against. As in:

  % export OCPI_KERNEL_DIR=somewhere/linux-digilent
  % make cleandriver; make driver





