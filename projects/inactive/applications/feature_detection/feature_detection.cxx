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
#include <string>
#include "cv.h"
#include "OcpiApi.hh"
#include "OcpiPValueApi.h"
#include "highgui.h"

namespace OA = OCPI::API;

int main ( int argc, char* argv [ ] )
{
  if(argc != 2) {
    std::cout << std::endl
	      << "Usage: ./feature_detection <image_name>\n"
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

      // Create an image for the output
      IplImage* outImg = cvLoadImage( argv[1] );
      cvNamedWindow( "Output", CV_WINDOW_AUTOSIZE );

#if 1
    char *appString;
    asprintf(&appString,
	     "<application package='ocpi.inactive'>\n"
	     "  <instance component='corner_eigen_vals_vecs' connect='min_eigen_val' externals='1'>\n"
	     "    <property name='height' value='%u'/>\n"
	     "    <property name='width' value='%u'/>\n"
	     "  </instance>\n"
	     "  <instance component='min_eigen_val' connect='good_features_to_track'>\n"
	     "    <property name='height' value='%u'/>\n"
	     "    <property name='width' value='%u'/>\n"
	     "  </instance>\n"
	     "  <instance component='good_features_to_track' externals='1'>\n"
	     "    <property name='height' value='%u'/>\n"
	     "    <property name='width' value='%u'/>\n"
	     "    <property name='max_corners' value='50'/>\n"
	     "    <property name='quality_level' value='0.03'/>\n"
	     "    <property name='min_distance' value='5.0'/>\n"
	     "  </instance>\n"
	     "</application>\n",
	     img->height, img->width, img->height, img->width, img->height, img->width);
    OA::Application app(appString);
    app.initialize();
    OA::ExternalPort
      &myOut = app.getPort("in"),
      &myIn = app.getPort("out");
    app.start();
#else
      /* ---- Create the RCC container and application -------------- */
      OA::Container *rcc_container = OA::ContainerManager::find("rcc");
      OA::ContainerApplication *rcc_application = rcc_container->createApplication( );

      /* ---- Create the workers --------------------------------- */
      // good_features_to_track

      OA::PValue features_pvlist[] = {
	OA::PVULong("height", img->height),
	OA::PVULong("width", img->width),
	OA::PVULong("max_corners", 50),
	OA::PVDouble("quality_level", 0.03),
	OA::PVDouble("min_distance", 5.0),
	OA::PVEnd
      };
      OA::Worker &features_worker =
	rcc_application->createWorker("features", "ocpi.inactive.good_features_to_track", features_pvlist);
      // features_worker.setProperties( features_worker_pvlist );
      std::string name, value;
      for (unsigned n = 0; features_worker.getProperty(n, name, value); n++)
	fprintf(stderr, "Property %u: name: %s, value: %s\n", n, name.c_str(), value.c_str());

      OA::Port
	&featuresOut = features_worker.getPort("out"),
	&featuresIn = features_worker.getPort("in");

      // min_eigen_val worker
      OA::PValue min_pvlist[] = {
	OA::PVULong("height", img->height),
	OA::PVULong("width", img->width),
	OA::PVEnd
      };
      OA::Worker &min_worker =
	rcc_application->createWorker("min", "ocpi.inactive.min_eigen_val", min_pvlist);
      OA::Port
	&minOut = min_worker.getPort("out"),
	&minIn = min_worker.getPort("in");

      // corner_eigen_vals_vecs
      OA::PValue corner_pvlist[] = {
	OA::PVULong("height", img->height),
	OA::PVULong("width", img->width),
	OA::PVEnd
      };
      OA::Worker &corner_worker =
	rcc_application->createWorker("corner", "ocpi.inactive.corner_eigen_vals_vecs", corner_pvlist);

      OA::Port
	&cornerOut = corner_worker.getPort("out"),
	&cornerIn = corner_worker.getPort("in");

      /* ---- Create connections --------------------------------- */

      featuresIn.connect( minOut );
      minIn.connect( cornerOut );

      // Set external ports
      OA::ExternalPort
	&myOut = cornerIn.connectExternal("aci_out"),
	&myIn = featuresOut.connectExternal("aci_in");

      printf(">>> DONE CONNECTING!\n");

      /* ---- Start all of the workers ------------------------------------- */
      rcc_application->start();

      printf(">>> DONE STARTING!\n");
#endif
      // Output info
      uint8_t *odata;
      size_t olength;

      // Input info
      uint8_t *idata;
      size_t ilength;
      uint8_t opcode = 0;
      bool isEndOfData = false;

      // Set output data
      OA::ExternalBuffer* myOutput = NULL;
      while ( (myOutput = myOut.getBuffer(odata, olength)) == NULL );
      memcpy(odata, img->imageData, img->height * img->width);
      myOutput->put(0, img->height * img->width, false);

      std::cout << "My output buffer is size " << olength << std::endl;


      // Get input data
      OA::ExternalBuffer* myInput = NULL;
      while((myInput = myIn.getBuffer( idata, ilength, opcode, isEndOfData)) == NULL);
      std::cout << "My input buffer is size " << ilength << std::endl;
      // Mark features
      size_t ncorners = ilength / (2 * sizeof(float));
      float *corners = (float *) idata;
      std::cout << "My corners " << ncorners << std::endl;
      for( unsigned int i = 0; i < ncorners; i++ ) {
	float x = corners[2*i];
	float y = corners[2*i+1];
	CvPoint p = cvPoint( cvRound(x), cvRound(y) );
	CvScalar color = CV_RGB(255, 0, 0);
	cvCircle( outImg, p, 5, color, 2 );
      }
      myInput->release();

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
      return 1;
    }
  catch ( std::exception& g )
    {
      std::cerr << "\nException(g): "
		<< typeid( g ).name( )
		<< " : "
		<< g.what ( )
		<< "\n"
		<< std::endl;
      return 1;
    }
  catch ( ... )
    {
      std::cerr << "\n\nException(u): unknown\n" << std::endl;
      return 1;
    }

  return 0;
}
