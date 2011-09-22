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


#include "OcpiContainerApi.h"
#include "OcpiPValueApi.h"
#include <iostream>
#include "DemoWorkerFacade.h"
#include "highgui.h"
#include "cv.h"

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
    ( void ) argc;
    ( void ) argv;

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

    /* ---- Create the shared RCC container and application -------------- */
    OCPI::API::Container* rcc_container ( Demo::get_rcc_interface ( ) );

    // Holds RCC worker interfaces passed to the RCC dispatch thread
    std::vector<OCPI::API::Container*> interfaces;
    interfaces.push_back ( rcc_container );

    OCPI::API::ContainerApplication*
      rcc_application ( rcc_container->createApplication ( ) );

    // Holds facades for group operations
    std::vector<Demo::WorkerFacade*> facades;

    /* ---- Create the workers --------------------------------- */
    // canny_partial
    Demo::WorkerFacade canny_worker (
        "Canny (RCC)",
        rcc_application,
        "canny_partial",
        "canny_partial" );

    OCPI::API::PValue canny_worker_pvlist[] = {
      OCPI::API::PVULong("height", img->height),
      OCPI::API::PVULong("width", img->width),
      OCPI::API::PVDouble("low_thresh", 10), // Canny
      OCPI::API::PVDouble("high_thresh", 100), // Canny
      OCPI::API::PVEnd
    };
    canny_worker.set_properties( canny_worker_pvlist );
    facades.push_back ( &canny_worker );

    OA::Port
      &cOut = canny_worker.port("out"),
      &cIn_dx = canny_worker.port("in_dx"),
      &cIn_dy = canny_worker.port("in_dy");

    // sobel_x
    Demo::WorkerFacade sobel_x_worker (
        "Sobel X (RCC)",
        rcc_application,
        "sobel",
	"sobel_x");

    OCPI::API::PValue sobel_x_worker_pvlist[] = {
      OCPI::API::PVULong("height", img->height),
      OCPI::API::PVULong("width", img->width),
      OCPI::API::PVBool("xderiv", 1),
      OCPI::API::PVEnd
    };
    sobel_x_worker.set_properties( sobel_x_worker_pvlist );
    facades.push_back ( &sobel_x_worker );

    OA::Port
      &sxOut = sobel_x_worker.port("out"),
      &sxIn = sobel_x_worker.port("in");

    // sobel_y
    Demo::WorkerFacade sobel_y_worker (
        "Sobel Y (RCC)",
        rcc_application,
	"sobel", "sobel_y" );

    OCPI::API::PValue sobel_y_worker_pvlist[] = {
      OCPI::API::PVULong("height", img->height),
      OCPI::API::PVULong("width", img->width),
      OCPI::API::PVBool("xderiv", 0),
      OCPI::API::PVEnd
    };
    sobel_y_worker.set_properties( sobel_y_worker_pvlist );
    facades.push_back ( &sobel_y_worker );

    OA::Port
      &syOut = sobel_y_worker.port("out"),
      &syIn = sobel_y_worker.port("in");

    /* ---- Create connections --------------------------------- */

    cIn_dx.connect( sxOut );
    cIn_dy.connect( syOut );

    // Set external ports
    OCPI::API::ExternalPort
      &myOutX = sxIn.connectExternal("aci_out_dx"),
      &myOutY = syIn.connectExternal("aci_out_dy"),
      &myIn = cOut.connectExternal("aci_in");

    /* ---- Start all of the workers ------------------------------------- */
    std::for_each ( facades.rbegin ( ),
        facades.rend ( ),
        Demo::start );

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

    std::cout << "My input buffer is size " << ilength << std::endl;

    myInput->release();
    memcpy((uint8_t *) outImg->imageData, idata, outImg->height * outImg->widthStep);

    // Show image
    cvShowImage( "Output", outImg );

    std::cout << "\nOpenOCPI application is done\n" << std::endl;

    // Save image
    cvSaveImage("output_image.jpg", outImg);

    // Cleanup
    cvWaitKey(0);
    cvReleaseImage( &img );
    cvReleaseImage( &outImg );
    cvDestroyWindow( "Input" );
    cvDestroyWindow( "Output" );
  }
  catch ( const std::string& s )
  {
    std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
  }
  catch ( const OCPI::API::Error& e )
  {
    std::cerr << "\nException(e): rc=" << e.error( ) << std::endl;
  }
  catch ( std::exception& g )
  {
    std::cerr << "\nException(g): " << typeid( g ).name( ) << " : "
                                       << g.what ( ) << "\n" << std::endl;
  }
  catch ( ... )
  {
    std::cerr << "\n\nException(u): unknown\n" << std::endl;
  }

  return 0;
}


