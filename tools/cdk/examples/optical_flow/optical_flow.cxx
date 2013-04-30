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
#include "highgui.h"
#include "OcpiContainerApi.h"
#include "OcpiPValueApi.h"

namespace OA = OCPI::API;

static const double pi = 3.14159265358979323846;

int main ( int argc, char* argv [ ] )
{
  if(argc != 3) {
    printf("Usage: ./motion <imgA> <imgB>\n");
    return 0;
  }

  // load images
  IplImage* imgA_color = cvLoadImage( argv[1] );
  IplImage* imgB_color = cvLoadImage( argv[2] );
  IplImage* imgC = cvLoadImage( argv[1] ); // for marking flow

  CvSize img_sz = cvGetSize( imgA_color );

  IplImage* imgA = cvCreateImage( img_sz, IPL_DEPTH_8U, 1 );
  IplImage* imgB = cvCreateImage( img_sz, IPL_DEPTH_8U, 1 );

  cvConvertImage( imgA_color, imgA );
  cvConvertImage( imgB_color, imgB );

  try
    {
      /* ---- Create the shared RCC container and application -------------- */
      OA::Container *rcc_container = OA::ContainerManager::find("rcc");
      OA::ContainerApplication *rcc_application = rcc_container->createApplication( );

      /* ---- Create the workers --------------------------------- */
      // good_features_to_track
      OCPI::API::PValue features_pvlist[] = {
	OCPI::API::PVULong("height", imgA->height),
	OCPI::API::PVULong("width", imgA->width),
	OCPI::API::PVULong("max_corners", 50),
	OCPI::API::PVDouble("quality_level", 0.03),
	OCPI::API::PVDouble("min_distance", 5.0),
	OCPI::API::PVEnd
      };
      OA::Worker &features_worker =
	rcc_application->createWorker("good_features_to_track", "ocpi.good_features_to_track",
				      features_pvlist);
      OA::Port
	&featuresOut = features_worker.getPort("out"),
	&featuresIn = features_worker.getPort("in");

      // min_eigen_val
      OCPI::API::PValue min_pvlist[] = {
	OCPI::API::PVULong("height", imgA->height),
	OCPI::API::PVULong("width", imgA->width),
	OCPI::API::PVEnd
      };
      OA::Worker &min_worker =
	rcc_application->createWorker("min_eigen_val", "ocpi.min_eigen_val",
				      min_pvlist);
      OA::Port
	&minOut = min_worker.getPort("out"),
	&minIn = min_worker.getPort("in");

      // corner_eigen_vals_vecs
      OCPI::API::PValue corner_pvlist[] = {
	OCPI::API::PVULong("height", imgA->height),
	OCPI::API::PVULong("width", imgA->width),
	OCPI::API::PVEnd
      };
      OA::Worker &corner_worker =
	rcc_application->createWorker("corner_eigen_vals_vecs", "ocpi.corner_eigen_vals_vecs",
				      corner_pvlist);
      OA::Port
	&cornerOut = corner_worker.getPort("out"),
	&cornerIn = corner_worker.getPort("in");

      // good_features_to_track (copy)
      OA::Worker &features_workerA =
	rcc_application->createWorker("good_features_to_track", "ocpi.good_features_to_track",
				      features_pvlist);

      OA::Port
	&featuresAOut = features_workerA.getPort("out"),
	&featuresAIn = features_workerA.getPort("in");

      // min_eigen_val (copy)
      OA::Worker &min_workerA =
	rcc_application->createWorker("min_eigen_val", "ocpi.min_eigen_val",
				      min_pvlist);
      OA::Port
	&minAOut = min_workerA.getPort("out"),
	&minAIn = min_workerA.getPort("in");

      // corner_eigen_vals_vecs (copy)
      OA::Worker &corner_workerA =
	rcc_application->createWorker("corner_eigen_vals_vecs", "ocpi.corner_eigen_vals_vecs",
				      corner_pvlist);
      OA::Port
	&cornerAOut = corner_workerA.getPort("out"),
	&cornerAIn = corner_workerA.getPort("in");

      // sobel_32f (A_dx)
      OCPI::API::PValue sobel_dx_pvlist[] = {
	OCPI::API::PVULong("height", imgA->height),
	OCPI::API::PVULong("width", imgA->width),
	OCPI::API::PVBool("xderiv", 1),
	OCPI::API::PVEnd
      };
      OA::Worker &sobel_adx_worker =
	rcc_application->createWorker("sobel_32f", "ocpi.sobel_32f",  sobel_dx_pvlist);

      OA::Port
	&sobelAdxOut = sobel_adx_worker.getPort("out_32f"),
	&sobelAdx8UOut = sobel_adx_worker.getPort("out"),
	&sobelAdxIn = sobel_adx_worker.getPort("in");

      // sobel_32f (A_dy)
      OCPI::API::PValue sobel_dy_pvlist[] = {
	OCPI::API::PVULong("height", imgA->height),
	OCPI::API::PVULong("width", imgA->width),
	OCPI::API::PVBool("xderiv", 0),
	OCPI::API::PVEnd
      };
      OA::Worker &sobel_ady_worker =
	rcc_application->createWorker("sobel_32f1", "ocpi.sobel_32f",  sobel_dy_pvlist);
      OA::Port
	&sobelAdyOut = sobel_ady_worker.getPort("out_32f"),
	&sobelAdy8UOut = sobel_ady_worker.getPort("out"),
	&sobelAdyIn = sobel_ady_worker.getPort("in");

      // sobel_32f (A_d2x)
      OA::Worker &sobel_ad2x_worker =
	rcc_application->createWorker("sobel_32f2", "ocpi.sobel_32f",  sobel_dx_pvlist);

      OA::Port
	&sobelAd2xOut = sobel_ad2x_worker.getPort("out_32f"),
	&sobelAd2x8UOut = sobel_ad2x_worker.getPort("out"),
	&sobelAd2xIn = sobel_ad2x_worker.getPort("in");

      // sobel_32f (A_d2y)
      OA::Worker &sobel_ad2y_worker =
	rcc_application->createWorker("sobel_32f3", "ocpi.sobel_32f",  sobel_dy_pvlist);

      OA::Port
	&sobelAd2yOut = sobel_ad2y_worker.getPort("out_32f"),
	&sobelAd2y8UOut = sobel_ad2y_worker.getPort("out"),
	&sobelAd2yIn = sobel_ad2y_worker.getPort("in");

      // sobel_32f (A_dxdy_x)
      OA::Worker &sobel_adxdy_x_worker =
	rcc_application->createWorker("sobel_32f4", "ocpi.sobel_32f",  sobel_dx_pvlist);

      OA::Port
	&sobelAdxdyXOut = sobel_adxdy_x_worker.getPort("out_32f"),
	&sobelAdxdyX8UOut = sobel_adxdy_x_worker.getPort("out"),
	&sobelAdxdyXIn = sobel_adxdy_x_worker.getPort("in");

      // sobel_32f (A_dxdy_y)
      OA::Worker &sobel_adxdy_y_worker =
	rcc_application->createWorker("sobel_32f5", "ocpi.sobel_32f",  sobel_dy_pvlist);

      OA::Port
	&sobelAdxdyYOut = sobel_adxdy_y_worker.getPort("out_32f"),
	&sobelAdxdyY8UOut = sobel_adxdy_y_worker.getPort("out"),
	&sobelAdxdyYIn = sobel_adxdy_y_worker.getPort("in");

      // sobel_32f (B_dx)
      OA::Worker &sobel_bdx_worker =
	rcc_application->createWorker("sobel_32f6", "ocpi.sobel_32f",  sobel_dx_pvlist);

      OA::Port
	&sobelBdxOut = sobel_bdx_worker.getPort("out_32f"),
	&sobelBdx8UOut = sobel_bdx_worker.getPort("out"),
	&sobelBdxIn = sobel_bdx_worker.getPort("in");

      // sobel_32f (B_dy)
      OA::Worker &sobel_bdy_worker =
	rcc_application->createWorker("sobel_32f7", "ocpi.sobel_32f",  sobel_dy_pvlist);

      OA::Port
	&sobelBdyOut = sobel_bdy_worker.getPort("out_32f"),
	&sobelBdy8UOut = sobel_bdy_worker.getPort("out"),
	&sobelBdyIn = sobel_bdy_worker.getPort("in");

      // optical_flow_pyr_lk
      OCPI::API::PValue optical_flow_pvlist[] = {
	OCPI::API::PVULong("height", imgA->height),
	OCPI::API::PVULong("width", imgA->width),
	OCPI::API::PVULong("win_height", 10),
	OCPI::API::PVULong("win_width", 10),
	OCPI::API::PVULong("level", 0),
	OCPI::API::PVULong("term_max_count", 30),
	OCPI::API::PVDouble("term_epsilon", 0.01),
	OCPI::API::PVDouble("deriv_lambda", 0.5),
	OCPI::API::PVEnd
      };
      OA::Worker &optical_flow_worker =
	rcc_application->createWorker("optical_flow_pyr_lk", "ocpi.optical_flow_pyr_lk", optical_flow_pvlist);

      OA::Port
	&opticalFlowInA = optical_flow_worker.getPort("in_A"),
	&opticalFlowInAdx = optical_flow_worker.getPort("in_Adx"),
	&opticalFlowInAdy = optical_flow_worker.getPort("in_Ady"),
	&opticalFlowInAd2x = optical_flow_worker.getPort("in_Ad2x"),
	&opticalFlowInAd2y = optical_flow_worker.getPort("in_Ad2y"),
	&opticalFlowInAdxdy = optical_flow_worker.getPort("in_Adxdy"),
	&opticalFlowInB = optical_flow_worker.getPort("in_B"),
	&opticalFlowInBdx = optical_flow_worker.getPort("in_Bdx"),
	&opticalFlowInBdy = optical_flow_worker.getPort("in_Bdy"),
	&opticalFlowInFeature = optical_flow_worker.getPort("in_feature"),
	&opticalFlowOut = optical_flow_worker.getPort("out"),
	&opticalFlowStatusOut = optical_flow_worker.getPort("out_status"),
	&opticalFlowErrOut = optical_flow_worker.getPort("out_err");

      printf(">>> DONE INIT!\n");

      /* ---- Create connections --------------------------------- */

      minIn.connect( cornerOut );
      featuresIn.connect( minOut );
      opticalFlowInFeature.connect( featuresOut );

      minAIn.connect( cornerAOut );
      featuresAIn.connect( minAOut );

      printf(">>> DONE CONNECTING (feature)!\n");

      opticalFlowInAdx.connect( sobelAdxOut );
      opticalFlowInAdy.connect( sobelAdyOut );
      opticalFlowInAd2x.connect( sobelAd2xOut );
      opticalFlowInAd2y.connect( sobelAd2yOut );
      opticalFlowInAdxdy.connect( sobelAdxdyYOut );

      printf(">>> DONE CONNECTING (A)!\n");

      opticalFlowInBdx.connect( sobelBdxOut );
      opticalFlowInBdy.connect( sobelBdyOut );

      printf(">>> DONE CONNECTING (B)!\n");

      sobelAd2xIn.connect( sobelAdx8UOut );
      sobelAd2yIn.connect( sobelAdy8UOut );
      sobelAdxdyYIn.connect( sobelAdxdyX8UOut );

      printf(">>> DONE CONNECTING (Sobel)!\n");

      // Set external ports
      OA::ExternalPort
	&myOutA = opticalFlowInA.connectExternal("aci_out_A"),
	&myOutB = opticalFlowInB.connectExternal("aci_out_B"),
	&myIn = opticalFlowOut.connectExternal("aci_in"),
	&myInStatus = opticalFlowStatusOut.connectExternal("aci_in_status"),
	&myInErr = opticalFlowErrOut.connectExternal("aci_in_err");

      OA::ExternalPort
	&myOutAdx = sobelAdxIn.connectExternal("aci_out_Adx"),
	&myOutAdy = sobelAdyIn.connectExternal("aci_out_Ady");

      OA::ExternalPort
	&myInAd2x = sobelAd2x8UOut.connectExternal("aci_in_Ad2x"),
	&myInAd2y = sobelAd2y8UOut.connectExternal("aci_in_Ad2y");

      OA::ExternalPort
	&myInAdxdy = sobelAdxdyY8UOut.connectExternal("aci_in_Adxdy"),
	&myIn32fAdxdy_X = sobelAdxdyXOut.connectExternal("aci_in_32f_Adxdy_X"),
	&myOutAdxdy = sobelAdxdyXIn.connectExternal("aci_out_Adxdy");

      OA::ExternalPort
	&myOutFeature = cornerIn.connectExternal("aci_out"),
	&myOutFeatureA = cornerAIn.connectExternal("aci_outA"),
	&myInFeatureA = featuresAOut.connectExternal("aci_in_featureA");

      OA::ExternalPort
	&myInBdx = sobelBdx8UOut.connectExternal("aci_in_Bdx"),
	&myInBdy = sobelBdy8UOut.connectExternal("aci_in_Bdy"),
	&myOutBdx = sobelBdxIn.connectExternal("aci_out_Bdx"),
	&myOutBdy = sobelBdyIn.connectExternal("aci_out_Bdy");

      printf(">>> DONE CONNECTING (all)!\n");

      /* ---- Start all of the workers ------------------------------------- */
      rcc_application->start();

      printf(">>> DONE STARTING!\n");

      // Output info
      uint8_t *odata;
      uint32_t olength;

      // Input info
      uint8_t *idata;
      uint32_t ilength;
      uint8_t opcode = 0;
      bool isEndOfData = false;

      // Set output data
      OA::ExternalBuffer* myOutput;
    
      while (!(myOutput = myOutFeature.getBuffer(odata, olength)));
      memcpy(odata, imgA->imageData, imgA->height * imgA->width);
      myOutput->put(0, imgA->height * imgA->width, false);

      while (!(myOutput = myOutFeatureA.getBuffer(odata, olength)));
      memcpy(odata, imgA->imageData, imgA->height * imgA->width);
      myOutput->put(0, imgA->height * imgA->width, false);

      while (!(myOutput = myOutA.getBuffer(odata, olength)));
      memcpy(odata, imgA->imageData, imgA->height * imgA->width);
      myOutput->put(0, imgA->height * imgA->width, false);

      while (!(myOutput = myOutAdx.getBuffer(odata, olength)));
      memcpy(odata, imgA->imageData, imgA->height * imgA->width);
      myOutput->put(0, imgA->height * imgA->width, false);

      while (!(myOutput = myOutAdy.getBuffer(odata, olength)));
      memcpy(odata, imgA->imageData, imgA->height * imgA->width);
      myOutput->put(0, imgA->height * imgA->width, false);

      while (!(myOutput = myOutAdxdy.getBuffer(odata, olength)));
      memcpy(odata, imgA->imageData, imgA->height * imgA->width);
      myOutput->put(0, imgA->height * imgA->width, false);

      while (!(myOutput = myOutB.getBuffer(odata, olength)));
      memcpy(odata, imgB->imageData, imgB->height * imgB->width);
      myOutput->put(0, imgB->height * imgB->width, false);

      while (!(myOutput = myOutBdx.getBuffer(odata, olength)));
      memcpy(odata, imgB->imageData, imgB->height * imgB->width);
      myOutput->put(0, imgB->height * imgB->width, false);

      while (!(myOutput = myOutBdy.getBuffer(odata, olength)));
      memcpy(odata, imgB->imageData, imgB->height * imgB->width);
      myOutput->put(0, imgB->height * imgB->width, false);

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


