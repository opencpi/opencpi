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
  if(argc != 3) {
    std::cout << std::endl
	      << "Usage: ./image_filtering <image_name> <worker_name>\n"
	      << std::endl;
    return 0;
  }
  char* worker_name = argv[2];

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

      /* ---- Create the worker --------------------------------- */
      Demo::WorkerFacade worker ( worker_name,//_full,
				  rcc_application,
				  worker_name ,
				  worker_name );

      // Set properties
      
      OCPI::API::PValue worker_pvlist[] = {
	OCPI::API::PVULong("height", img->height),
	OCPI::API::PVULong("width", img->width),
	OCPI::API::PVEnd
      };
      worker.set_properties(worker_pvlist);


      // filter specific
      if ( worker_name == "gaussian_blur" ) {
	OCPI::API::PValue wpvlist[] = {
	  OCPI::API::PVDouble("sigmaX", 0.8), // Gaussian blur
	  OCPI::API::PVDouble("sigmaY", 0.8), // Gaussian blur
	  OCPI::API::PVEnd
	};      
	worker.set_properties(wpvlist);
      }
      else if ( worker_name == "blur" ) {
	OCPI::API::PValue wpvlist[] = {
	  OCPI::API::PVBool("normalize", 1), // Blur only
	  OCPI::API::PVEnd
	};      
	worker.set_properties(wpvlist);
      }
      else if ( worker_name == "sobel" || worker_name == "scharr" ) {
	OCPI::API::PValue wpvlist[] = {
	  OCPI::API::PVBool("xderiv", 1), // Sobel/Scharr only
	  OCPI::API::PVEnd
	};      
	worker.set_properties(wpvlist);
      }

      facades.push_back ( &worker );

      // Set ports
      OA::Port
	&wOut = worker.port("out"),
	&wIn = worker.port("in");

      // Set external ports (need 3 buffers for out)
      OA::ExternalPort
	&myOut = wIn.connectExternal("aci_out"),
	&myIn = wOut.connectExternal("aci_in");

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
      uint8_t *outImgLine = (uint8_t *) outImg->imageData;

      for(int i = 0; i < img->height; i++) {
	// std::cout << "Starting line..." << std::endl;

	// Set output data
	OA::ExternalBuffer* myOutput;
	while( (myOutput = myOut.getBuffer(odata, olength)) == NULL );
	memcpy(odata, imgLine, img->widthStep);
	imgLine += img->widthStep;
	myOutput->put( img->widthStep, 0, false);

	std::cout << "My output buffer is size " << olength << std::endl;

	// std::cout << "Done working..." << std::endl;

	// Don't get result on first line
	if(i == 0)
	  continue;

	// Get input data
	OA::ExternalBuffer* myInput;
	while ( (myInput = myIn.getBuffer( idata, ilength, opcode, isEndOfData)) == NULL );
	

	std::cout << "My input buffer is size " << ilength << std::endl;

	myInput->release();
	memcpy(outImgLine, idata, img->widthStep);
	outImgLine += img->widthStep;
      }

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
  catch ( std::exception& g )
    {
      std::cerr << "\nException(g): "
		<< typeid( g ).name( )
		<< " : "
		<< g.what ( )
		<< "\n"
		<< std::endl;
    }
  catch ( ... )
    {
      std::cerr << "\n\nException(u): unknown\n" << std::endl;
    }

  return 0;
}


