
This demo can be run by typing the following command

>optical_flow  <image1> <image2>

       Where image1 and image2 are the names of .jpg files

This demo will read in the jpeg images and apply an optical flow algorithm.  

A description of the demo can be found at 
http://web.mit.edu/tonyliu/www/opencpi/thesis-final-5-18-11.pdf

Both the original and filtered images will be displayed in separate windows using OpenCV.  

Make sure that your XWindows DISPLAY environment variable is set correctly.

This test requires significant memory so make sure that the SMB size is set to at least the following value.

export OCPI_SMB_SIZE=400000000

You can run an example by typing
./runit
