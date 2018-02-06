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

#include <unistd.h>
#include <iostream>
#include "cv.h"
#include "highgui.h"
#include "OcpiApi.hh"
#include "OcpiContainerApi.h"
#include "OcpiPValueApi.h"


namespace OA = OCPI::API;

static const double pi = 3.14159265358979323846;

int main ( int argc, char* argv [ ] )
{
  std::string of_app_xml("<application package='ocpi.inactive'>"
			 " <policy mapping='MaxProcessors' processors='0'/>"
			 "  <instance component='min_eigen_val' name='min_worker'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "  </instance> "
			 "  <instance component='good_features_to_track' name='feature_worker'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "    <property name='max_corners' value='50'/> "
			 "    <property name='quality_level' value='0.03'/> "
			 "    <property name='min_distance' value='5.0'/> "
			 "  </instance> "
			 "  <instance component='corner_eigen_vals_vecs' name='corner_worker'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "  </instance> "
			 "  <instance component='good_features_to_track' name='feature_workerA'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "    <property name='max_corners' value='50'/> "
			 "    <property name='quality_level' value='0.03'/> "
			 "    <property name='min_distance' value='5.0'/> "
			 "  </instance> "
			 "  <instance component='min_eigen_val' name='min_workerA'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "  </instance> "
			 "  <instance component='corner_eigen_vals_vecs' name='corner_workerA'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "  </instance> "
			 "  <instance component='sobel_32f' name='sobel_adx_worker'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "    <property name='xderiv' value='1'/> "
			 "  </instance> "
			 "  <instance component='sobel_32f' name='sobel_ady_worker'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "    <property name='xderiv' value='0'/> "
			 "  </instance> "
			 "  <instance component='sobel_32f' name='sobel_ad2x_worker'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "    <property name='xderiv' value='1'/> "
			 "  </instance> "
			 "  <instance component='sobel_32f' name='sobel_ad2y_worker'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "    <property name='xderiv' value='0'/> "
			 "  </instance> "
			 "  <instance component='sobel_32f' name='sobel_adxdy_x_worker'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "    <property name='xderiv' value='1'/> "
			 "  </instance> "
			 "  <instance component='sobel_32f' name='sobel_adxdy_y_worker'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "    <property name='xderiv' value='0'/> "
			 "  </instance> "
			 "  <instance component='sobel_32f' name='sobel_bdx_worker'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "    <property name='xderiv' value='1'/> "
			 "  </instance> "
			 "  <instance component='sobel_32f' name='sobel_bdy_worker'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "    <property name='xderiv' value='0'/> "
			 "  </instance> "
			 "  <instance component='optical_flow_pyr_lk' name='optical_flow_worker'>"
			 "    <property name='height' value='360'/> "
			 "    <property name='width' value='480'/> "
			 "    <property name='win_height' value='10'/> "
			 "    <property name='win_width' value='10'/> "
			 "    <property name='level' value='0'/> "
			 "    <property name='term_max_count' value='30'/> "
			 "    <property name='term_epsilon' value='0.01'/> "
			 "    <property name='deriv_lambda' value='0.5'/> "
			 "  </instance> "


			 "  <connection>"
			 "    <port instance='min_worker' name='in'/>"
			 "    <port instance='corner_worker' name='out'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <port instance='feature_worker' name='in'/>"
			 "    <port instance='min_worker' name='out'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <port instance='optical_flow_worker' name='in_feature'/>"
			 "    <port instance='feature_worker' name='out'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <port instance='min_workerA' name='in'/>"
			 "    <port instance='corner_workerA' name='out'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <port instance='feature_workerA' name='in'/>"
			 "    <port instance='min_workerA' name='out'/>"
			 "  </connection>"



			 "  <connection>"
			 "    <port instance='optical_flow_worker' name='in_Adx'/>"
			 "    <port instance='sobel_adx_worker' name='out_32f'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <port instance='optical_flow_worker' name='in_Ady'/>"
			 "    <port instance='sobel_ady_worker' name='out_32f'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <port instance='optical_flow_worker' name='in_Ad2x'/>"
			 "    <port instance='sobel_ad2x_worker' name='out_32f'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <port instance='optical_flow_worker' name='in_Ad2y'/>"
			 "    <port instance='sobel_ad2y_worker' name='out_32f'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <port instance='optical_flow_worker' name='in_Adxdy'/>"
			 "    <port instance='sobel_adxdy_y_worker' name='out_32f'/>"
			 "  </connection>"


			 "  <connection>"
			 "    <port instance='optical_flow_worker' name='in_Bdx'/>"
			 "    <port instance='sobel_bdx_worker' name='out_32f'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <port instance='optical_flow_worker' name='in_Bdy'/>"
			 "    <port instance='sobel_bdy_worker' name='out_32f'/>"
			 "  </connection>"

			 "  <connection>"
			 "    <port instance='sobel_ad2x_worker' name='in'/>"
			 "    <port instance='sobel_adx_worker' name='out'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <port instance='sobel_ad2y_worker' name='in'/>"
			 "    <port instance='sobel_ady_worker' name='out'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <port instance='sobel_adxdy_y_worker' name='in'/>"
			 "    <port instance='sobel_adxdy_x_worker' name='out'/>"
			 "  </connection>"

			 "  <connection>"
			 "    <external name='myOutA'/>"
			 "    <port instance='optical_flow_worker' name='in_A'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <external name='myOutB'/>"
			 "    <port instance='optical_flow_worker' name='in_B'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <external name='myIn'/>"
			 "    <port instance='optical_flow_worker' name='out'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <external name='myInStatus'/>"
			 "    <port instance='optical_flow_worker' name='out_status'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <external name='myInErr'/>"
			 "    <port instance='optical_flow_worker' name='out_err'/>"
			 "  </connection>"

			 "  <connection>"
			 "    <external name='myOutAdx'/>"
			 "    <port instance='sobel_adx_worker' name='in'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <external name='myOutAdy'/>"
			 "    <port instance='sobel_ady_worker' name='in'/>"
			 "  </connection>"



			 "  <connection>"
			 "    <external name='myInAd2x'/>"
			 "    <port instance='sobel_ad2x_worker' name='out'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <external name='myInAd2y'/>"
			 "    <port instance='sobel_ad2y_worker' name='out'/>"
			 "  </connection>"

			 "  <connection>"
			 "    <external name='myOutAdxdy'/>"
			 "    <port instance='sobel_adxdy_x_worker' name='in'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <external name='myInAdxdy'/>"
			 "    <port instance='sobel_adxdy_y_worker' name='out'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <external name='myIn32fAdxdy_X'/>"
			 "    <port instance='sobel_adxdy_x_worker' name='out_32f'/>"
			 "  </connection>"



			 "  <connection>"
			 "    <external name='myOutFeature'/>"
			 "    <port instance='corner_worker' name='in'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <external name='myOutFeatureA'/>"
			 "    <port instance='corner_workerA' name='in'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <external name='myInFeatureA'/>"
			 "    <port instance='feature_workerA' name='out'/>"
			 "  </connection>"



			 "  <connection>"
			 "    <external name='myOutBdx'/>"
			 "    <port instance='sobel_bdx_worker' name='in'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <external name='myOutBdy'/>"
			 "    <port instance='sobel_bdy_worker' name='in'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <external name='myInBdx'/>"
			 "    <port instance='sobel_bdx_worker' name='out'/>"
			 "  </connection>"
			 "  <connection>"
			 "    <external name='myInBdy'/>"
			 "    <port instance='sobel_bdy_worker' name='out'/>"
			 "  </connection>"

			 "</application>");

  if(argc != 3) {
    printf("Usage: ./motion <imgA> <imgB>\n");
    return 0;
  }

  OA::Application * app = NULL;
  try
    {
      int nContainers = 4;
      // Create several containers to distribute the workers on
      for ( int n=0; n<nContainers; n++ ) {
	char buf[1024];
	sprintf(buf, "Rcc Container %d", n );
	(void)OA::ContainerManager::find("rcc",buf);
      }

      int policy = 1;
      while ( policy < 4 ) {

	// load images
	IplImage* imgA_color = cvLoadImage( argv[1] );
	IplImage* imgB_color = cvLoadImage( argv[2] );
	IplImage* imgC = cvLoadImage( argv[1] ); // for marking flow

	CvSize img_sz = cvGetSize( imgA_color );

	IplImage* imgA = cvCreateImage( img_sz, IPL_DEPTH_8U, 1 );
	IplImage* imgB = cvCreateImage( img_sz, IPL_DEPTH_8U, 1 );

	cvConvertImage( imgA_color, imgA );
	cvConvertImage( imgB_color, imgB );

	OCPI::API::PValue minp_policy[] = {
	  OCPI::API::PVULong("MinProcessors",0),
	  OCPI::API::PVEnd
	};

	OCPI::API::PValue maxp_policy[] = {
	  OCPI::API::PVULong("MaxProcessors",0),
	  OCPI::API::PVEnd
	};

	OCPI::API::PValue fixedp_policy[] = {
	  OCPI::API::PVULong("MaxProcessors",3),
	  OCPI::API::PVEnd
	};

	if ( app != NULL ) {
	  OA::Application * tapp = app;
	  if ( policy == 1 ) {
	    app = new OA::Application(*tapp, maxp_policy);
	    policy++;
	  }
	  else if ( policy == 2 ) {
	    app = new OA::Application(*tapp, minp_policy);
	    policy++;
	  }
	  else if ( policy == 3 ) {
	    app = new OA::Application(*tapp, fixedp_policy);
	    policy++;
	  }
	  try {
	    delete tapp;
	  }
	  catch ( ... ) {
	    printf("Got an error while trying to delete app \n");
	  }
	}
	else {
	  app = new OA::Application( of_app_xml );
	}

	fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
	app->initialize();
	fprintf(stderr, "Application established: containers, workers, connections all created\n");

	printf(">>> DONE STARTING!\n");

	// Output info
	uint8_t *odata;
	size_t olength;

	// Input info
	uint8_t *idata;
	size_t ilength;
	uint8_t opcode = 0;
	bool isEndOfData = false;

	// Set output data
	OA::ExternalBuffer* myOutput;
	OA::ExternalPort
	  &myInBdy = app->getPort("myInBdy"),
	  &myInBdx = app->getPort("myInBdx"),
	  &myOutFeature = app->getPort("myOutFeature"),
	  &myOutFeatureA = app->getPort("myOutFeatureA"),
	  &myOutA= app->getPort("myOutA"),
	  &myOutAdx= app->getPort("myOutAdx"),
	  &myOutAdy = app->getPort("myOutAdy"),
	  &myOutAdxdy= app->getPort("myOutAdxdy"),
	  &myOutB= app->getPort("myOutB"),
	  &myOutBdx = app->getPort("myOutBdx"),
	  &myOutBdy = app->getPort("myOutBdy"),
	  &myInFeatureA = app->getPort("myInFeatureA"),
	  &myIn= app->getPort("myIn"),
	  &myInStatus= app->getPort("myInStatus"),
	  &myInErr = app->getPort("myInErr"),
	  &myInAd2x = app->getPort("myInAd2x"),
	  &myInAd2y = app->getPort("myInAd2y"),
	  &myInAdxdy = app->getPort("myInAdxdy"),
	  &myIn32fAdxdy_X = app->getPort("myIn32fAdxdy_X");


	app->start();

	if ( policy == 3 ) {
	  app->wait( 1 );
	  std::string value;
	  app->getProperty( "min_workerA", "height", value);
	  printf("worker property = %s\n", value.c_str() );
	}

	while (!(myOutput = myOutFeature.getBuffer(odata, olength))) sleep(0);
	memcpy(odata, imgA->imageData, imgA->height * imgA->width);
	myOutput->put(imgA->height * imgA->width, 0, false);

	while (!(myOutput = myOutFeatureA.getBuffer(odata, olength)))sleep(0);
	memcpy(odata, imgA->imageData, imgA->height * imgA->width);
	myOutput->put(imgA->height * imgA->width, 0, false);

	while (!(myOutput = myOutA.getBuffer(odata, olength))) sleep(0);
	memcpy(odata, imgA->imageData, imgA->height * imgA->width);
	myOutput->put(imgA->height * imgA->width, 0, false);

	while (!(myOutput = myOutAdx.getBuffer(odata, olength))) sleep(0);
	memcpy(odata, imgA->imageData, imgA->height * imgA->width);
	myOutput->put(imgA->height * imgA->width, 0, false);

	while (!(myOutput = myOutAdy.getBuffer(odata, olength))) sleep(0);
	memcpy(odata, imgA->imageData, imgA->height * imgA->width);
	myOutput->put(imgA->height * imgA->width, 0, false);

	while (!(myOutput = myOutAdxdy.getBuffer(odata, olength))) sleep(0);
	memcpy(odata, imgA->imageData, imgA->height * imgA->width);
	myOutput->put(imgA->height * imgA->width, 0, false);

	while (!(myOutput = myOutB.getBuffer(odata, olength))) sleep(0);
	memcpy(odata, imgB->imageData, imgB->height * imgB->width);
	myOutput->put(imgB->height * imgB->width, 0, false);

	while (!(myOutput = myOutBdx.getBuffer(odata, olength))) sleep(0);
	memcpy(odata, imgB->imageData, imgB->height * imgB->width);
	myOutput->put( imgB->height * imgB->width, 0, false);

	while (!(myOutput = myOutBdy.getBuffer(odata, olength))) sleep(0);
	memcpy(odata, imgB->imageData, imgB->height * imgB->width);
	myOutput->put(imgB->height * imgB->width, 0, false);

	std::cout << "My output buffer is size " << olength << std::endl;

	// Get input data
	OA::ExternalBuffer* myInput;

	myInput = NULL;
	while((myInput = myInFeatureA.getBuffer( idata, ilength, opcode, isEndOfData)) == NULL);
	size_t ncorners = ilength / (2 * sizeof(float));
	float *cornersA = (float *) malloc(ncorners * 2 * sizeof(float));
	memcpy(cornersA, idata, ilength);
	// myInput->release();
	std::cout << "My old corners " << ncorners << std::endl;

	// Get corners, statuses, errors
	myInput = NULL;
	while((myInput = myIn.getBuffer( idata, ilength, opcode, isEndOfData)) == NULL);
	// size_t ncorners = ilength / (2 * sizeof(float));
	float *cornersB = (float *) malloc(ncorners * 2 * sizeof(float));
	memcpy(cornersB, idata, ilength);
	// myInput->release();
	std::cout << "My corners " << ilength / (2 * sizeof(float)) << std::endl;

	myInput = NULL;
	while((myInput = myInStatus.getBuffer( idata, ilength, opcode, isEndOfData)) == NULL);
	char *status = (char *) malloc(ncorners * sizeof(char));
	memcpy(status, idata, ilength);
	// myInput->release();
	std::cout << "My status" << ilength << std::endl;

	myInput = NULL;
	while((myInput = myInErr.getBuffer( idata, ilength, opcode, isEndOfData)) == NULL);
	float *err = (float *) malloc(ncorners * sizeof(float));
	memcpy(err, idata, ilength);
	// myInput->release();
	std::cout << "My err" << ilength / sizeof(float) << std::endl;

	// Draw flow
	for( size_t i = 0; i < ncorners; i++ ) {

	  if( status[i] == 0 || err[i] > 500 )
	    continue;

	  double x0 = cornersA[2*i];
	  double y0 = cornersA[2*i+1];
	  CvPoint p = cvPoint( cvRound(x0), cvRound(y0) );
	  double x1 = cornersB[2*i];
	  double y1 = cornersB[2*i+1];
	  CvPoint q = cvPoint( cvRound(x1), cvRound(y1) );
	  printf("%.4lf %.4lf -> %.4lf %.4lf\n", x0, y0, x1, y1);

	  CvScalar line_color = CV_RGB(255, 0, 0);
	  int line_thickness = 1;

	  // Main line (p -> q) lengthened
	  double angle = atan2( (double) y1 - y0, (double) x1 - x0 );
	  double hypotenuse = 1.5;
	  q.x = cvRound(x0 + 6 * hypotenuse * cos(angle));
	  q.y = cvRound(y0 + 6 * hypotenuse * sin(angle));
	  cvLine( imgC, p, q, line_color, line_thickness, CV_AA, 0 );

	  // Arrows
	  p.x = (int) (x0 + 5 * hypotenuse * cos(angle + pi / 4));
	  p.y = (int) (y0 + 5 * hypotenuse * sin(angle + pi / 4));
	  cvLine( imgC, p, q, line_color, line_thickness, CV_AA, 0 );

	  p.x = (int) (x0 + 5 * hypotenuse * cos(angle - pi / 4));
	  p.y = (int) (y0 + 5 * hypotenuse * sin(angle - pi / 4));
	  cvLine( imgC, p, q, line_color, line_thickness, CV_AA, 0 );
	}

	// Save image
	cvSaveImage("output_image.jpg", imgC);

	std::cout << "\nOpenOCPI application is done\n" << std::endl;

	// display
	cvNamedWindow( "Image A", CV_WINDOW_AUTOSIZE );
	cvNamedWindow( "Image B", CV_WINDOW_AUTOSIZE );
	cvNamedWindow( "Image Flow", CV_WINDOW_AUTOSIZE );

	cvShowImage( "Image A", imgA );
	cvShowImage( "Image B", imgB );
	cvShowImage( "Image Flow", imgC );

	// cleanup
	cvWaitKey(0);

	cvReleaseImage( &imgA );
	cvReleaseImage( &imgB );
	cvReleaseImage( &imgC );

	cvDestroyWindow( "Image A" );
	cvDestroyWindow( "Image B" );
	cvDestroyWindow( "Image Flow" );

	free( cornersA );
	free( cornersB );
	free( status );
	free( err );

      }
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
