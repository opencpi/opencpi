
This demo can be run by typing the following command

>image_filtering <image> <filter>

       Where 
       image is the name of a .jpg file
       filter is the name of the component filter to apply

This demo will read in the jpeg image and apply the specified filter.  Note that the name of the filter must be the name of an appropriate compont within OpenCPI. 

A description of the demo can be found at 
http://web.mit.edu/tonyliu/www/opencpi/thesis-final-5-18-11.pdf

Both the original and filtered images will be displayed in separate windows using OpenCV.  

There are several filters that are part of the standard OpenCPI distribution that will work with this demo.  
      gaussian_blur
      sobel
      erode

Make sure that your XWindows DISPLAY environment variable is set correctly.

This test requires significant memory so make sure that the SMB size is set to at least the following value.

export OCPI_SMB_SIZE=100000000

You can run an example by typing
./runit
