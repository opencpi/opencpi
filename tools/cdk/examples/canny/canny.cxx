/*
=====
Copyright (C) 2011 Massachusetts Institute of Technology


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
======
*/


#include <iostream>
#include "cv.h"
#include "OcpiContainerApi.h"
#include "OcpiPValueApi.h"
#include "highgui.h"

namespace OA = OCPI::API;

int main ( int argc, char* argv [ ] )
{
  if(argc != 2) {
    std::cout << std::endl
      << "Usage: ./canny <image_name>\n"
      << std::endl;
    return 0;
  }

  try
  {
    /* ---- Load the image from file (grayscale) -------------- */
    IplImage* inImg = cvLoadImage( argv[1], 0 );
    // Create image with 8U pixel depth, single color channel
    IplImage* img = cvCreateImage(
        cvSize(inImg->width, inImg->height),
        IPL_DEPTH_8U, 1
        );
    // Convert
    cvConvertScale(inImg, img);
    cvNamedWindow( "Input", CV_WINDOW_AUTOSIZE );
    cvShowImage( "Input", img );

    // Create an image for the output (grayscale)
    IplImage* outImg = cvCreateImage(
        cvSize(img->width, img->height),
        img->depth, img->nChannels
        );
    cvNamedWindow( "Output", CV_WINDOW_AUTOSIZE );

    /* ---- Create the RCC container and application -------------- */
    OA::Container *rcc_container = OA::ContainerManager::find("rcc");
    OA::PValue params[] = {OA::PVString("package", "ocpi"), OA::PVEnd};
    OA::ContainerApplication *rcc_application = rcc_container->createApplication("canny", params );

    /* ---- Create the workers --------------------------------- */
    // canny_partial

    OCPI::API::PValue canny_pvlist[] = {
      OCPI::API::PVULong("height", img->height),
      OCPI::API::PVULong("width", img->width),
      OCPI::API::PVDouble("low_thresh", 10), // Canny
      OCPI::API::PVDouble("high_thresh", 100), // Canny
      OCPI::API::PVEnd
    };
    OA::Worker &canny_worker =
      rcc_application->createWorker("canny", "canny_partial", canny_pvlist );

    OCPI::API::PValue canny_in_dx_pvlist[] = {
      OCPI::API::PVString("protocol", "ocpi-socket-rdma"),
      OCPI::API::PVEnd
    };
    OA::Port
      &cOut = canny_worker.getPort("out"),
      &cIn_dx = canny_worker.getPort("in_dx", canny_in_dx_pvlist),
      &cIn_dy = canny_worker.getPort("in_dy");

    // sobel_x

    OCPI::API::PValue sobel_x_pvlist[] = {
      OCPI::API::PVULong("height", img->height),
      OCPI::API::PVULong("width", img->width),
      OCPI::API::PVBool("xderiv", 1),
      OCPI::API::PVEnd
    };
    OA::Worker &sobel_x_worker =
      rcc_application->createWorker("sobel_x", "sobel",  sobel_x_pvlist );
    OA::Port
      &sxOut = sobel_x_worker.getPort("out"),
      &sxIn = sobel_x_worker.getPort("in");

    // sobel_y
    OCPI::API::PValue sobel_y_pvlist[] = {
      OCPI::API::PVULong("height", img->height),
      OCPI::API::PVULong("width", img->width),
      OCPI::API::PVBool("xderiv", 0),
      OCPI::API::PVEnd
    };
    OA::Worker &sobel_y_worker =
      rcc_application->createWorker("sobel_y", "sobel",  sobel_y_pvlist );


    OA::Port
      &syOut = sobel_y_worker.getPort("out"),
      &syIn = sobel_y_worker.getPort("in");

    /* ---- Create connections --------------------------------- */

    cIn_dx.connect( sxOut );
    cIn_dy.connect( syOut );

    // Set external ports
    OCPI::API::ExternalPort
      &myOutX = sxIn.connectExternal("aci_out_dx"),
      &myOutY = syIn.connectExternal("aci_out_dy"),
      &myIn = cOut.connectExternal("aci_in");

    /* ---- Start all of the workers ------------------------------------- */
    rcc_application->start();

    // Output info
    uint8_t *odata;
    uint32_t olength;

    // Input info
    uint8_t *idata;
    uint32_t ilength;
    uint8_t opcode = 0;
    bool isEndOfData = false;

    // Current line
    uint8_t *imgLine = (uint8_t *) img->imageData;

    static int pc = 0;
    for(int i = 0; i < img->height; i++) {
      // Set output data
      OCPI::API::ExternalBuffer* myOutput;

      do {
	myOutput = myOutX.getBuffer(odata, olength);
	if ( ! myOutput ) {
	  if ( (pc++%10000) == 0 ) {
	    //	    printf("Waiting for an X output buffer\n");
	  }
	}
      }
      while( ! myOutput );
      memcpy(odata, imgLine, img->widthStep);
      myOutput->put( img->widthStep,0,false);

      do {
	myOutput = myOutY.getBuffer(odata, olength);
	if ( ! myOutput ) {
	  printf("Waiting for a Y output buffer\n");
	}
      }
      while( ! myOutput );
      memcpy(odata, imgLine, img->widthStep);
      myOutput->put( img->widthStep,0, false);
      imgLine += img->widthStep;
      std::cout << "My output buffer is size " << olength << std::endl;
    }

    // Get input data
    OCPI::API::ExternalBuffer* myInput = NULL;
    while((myInput = myIn.getBuffer( idata, ilength, opcode, isEndOfData)) == NULL);

    std::cerr << "My input buffer is size " << ilength << std::endl;

    memcpy((uint8_t *) outImg->imageData, idata, outImg->height * outImg->widthStep);

    myInput->release();

    // Show image
    cvShowImage( "Output", outImg );

    std::cerr << "\nOpenOCPI application is done\n" << std::endl;

    // Save image
    cvSaveImage("output_image.jpg", outImg);

    // Cleanup
    cvWaitKey(0);
    cvReleaseImage( &img );
    cvReleaseImage( &outImg );
    cvDestroyWindow( "Input" );
    cvDestroyWindow( "Output" );
  }
  catch ( std::string s )
  {
    std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
    return 1;
  }
  catch ( std::exception& g )
  {
    std::cerr << "\nException(g): " << typeid( g ).name( ) << " : "
                                       << g.what ( ) << "\n" << std::endl;
    return 1;
  }
  catch ( ... )
  {
    std::cerr << "\n\nException(u): unknown\n" << std::endl;
    return 1;
  }

  return 0;
}


