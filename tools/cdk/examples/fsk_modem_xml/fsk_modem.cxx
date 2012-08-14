#include <iostream>
#include "OcpiApi.h"
#include "OcpiContainerApi.h"
#include "OcpiPValueApi.h"


namespace OA = OCPI::API;
static const double pi = 3.14159265358979323846;


int main ( int argc, char* argv [ ] )
{
  std::string app_xml("<application>"
		      " <policy mapping='MaxProcessors' processors='0'/>"

		      "  <instance worker='file_read_real' name='file_reader' selection='model==\"rcc\"'>"
		      "    <property name='fileName' value='dataIn.dat'/> "		      
		      "    <property name='genTestFile' value='true'/> "		      
		      "    <property name='stepThruMsg' value='true'/> "
		      "    <property name='stepNow' value='true'/> "
		      "  </instance> "

		      "  <instance worker='sym_fir_real' name='tx_fir_r' selection='model==\"rcc\"'>"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='nTaps' value='256'/> "
		      "  </instance> "

		      "  <instance worker='fsk_mod_complex' name='fsk_mod' selection='model==\"rcc\"'>"
		      "  </instance> "

		      "  <instance worker='sym_fir_complex' name='tx_fir_c' selection='model==\"rcc\"'>"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='nTaps' value='256'/> "
		      "  </instance> "

		      "  <instance worker='cic_hpfilter_complex' name='tx_cic' selection='model==\"rcc\"'>"
		      "    <property name='M' value='2'/> "
		      "  </instance> "
			 

		      "  <instance worker='loopback_complex' name='loopback' selection='model==\"rcc\"'>"
		      "  </instance> "


		      "  <instance worker='dds_complex' name='ddc_dds' selection='model==\"rcc\"'>"
		      "    <property name='phaseIncrement' value='12345678'/> "
		      "    <property name='syncPhase' value='0'/> "
		      "  </instance> "

		      "  <instance worker='mixer_complex' name='ddc_mixer' selection='model==\"rcc\"'>"
		      "  </instance> "


		      "  <instance worker='cic_lpfilter_complex' name='rx_cic' selection='model==\"rcc\"'>"
		      "    <property name='M' value='2'/> "
		      "  </instance> "

		      "  <instance worker='sym_fir_complex' name='rx_fir_c' selection='model==\"rcc\"'>"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='nTaps' value='256'/> "
		      "  </instance> "

		      "  <instance worker='fm_demod_complex' name='fm_demod' selection='model==\"rcc\"'>"
		      "  </instance> "


		      "  <instance worker='sym_fir_real' name='rx_fir_r' selection='model==\"rcc\"'>"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='nTaps' value='256'/> "
		      "  </instance> "


"<!--"
"-->"

		      "  <instance worker='file_write' name='file_writer' selection='model==\"rcc\"'>"
		      "    <property name='fileName' value='dataOut.dat'/> "		      
		      "  </instance> "



		      "  <connection>"
		      "    <port instance='file_reader' name='out'/>"
		      "    <port instance='tx_fir_r' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='tx_fir_r' name='out'/>"
		      "    <port instance='fsk_mod' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='tx_fir_c' name='in'/>"
		      "    <port instance='fsk_mod' name='out'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='tx_fir_c' name='out'/>"
		      "    <port instance='tx_cic' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='loopback' name='in'/>"
		      "    <port instance='tx_cic' name='out'/>"
		      "  </connection>"
			 

		      "  <connection>"
		      "    <port instance='loopback' name='out'/>"
		      "    <port instance='ddc_mixer' name='in_if'/>"
		      "  </connection>"


		      "  <connection>"
		      "    <port instance='rx_cic' name='in'/>"
		      "    <port instance='ddc_mixer' name='out'/>"
		      "  </connection>"


		      "<!--   Connections internal to the DDC -->"
		      "  <connection>"
		      "    <port instance='ddc_mixer' name='out_sync_only'/>"
		      "    <port instance='ddc_dds' name='in'/>"
		      "  </connection>"


		      "  <connection>"
		      "    <port instance='ddc_mixer' name='in_dds'/>"
		      "    <port instance='ddc_dds' name='out'/>"
		      "  </connection>"



		      "  <connection>"
		      "    <port instance='rx_cic' name='out'/>"
		      "    <port instance='rx_fir_c' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='fm_demod' name='in'/>"
		      "    <port instance='rx_fir_c' name='out'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='fm_demod' name='out'/>"
		      "    <port instance='rx_fir_r' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='rx_fir_r' name='out'/>"
		      "    <port instance='file_writer' name='in'/>"
		      "  </connection>"			 

		      "</application>");
  

  OA::Application * app = NULL;
  try
    {      
      int nContainers = 1;
      // Create several containers to distribute the workers on
      for ( int n=0; n<nContainers; n++ ) {
	char buf[1024];
	sprintf(buf, "Rcc Container %d\n", n );
	(void)OA::ContainerManager::find("rcc",buf);
      }

      OCPI::API::PValue minp_policy[] = {
	OCPI::API::PVULong("MinProcessors",0),
	OCPI::API::PVEnd
      };
      
      app = new OA::Application( app_xml, minp_policy);	
      fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
      app->initialize();
      fprintf(stderr, "Application established: containers, workers, connections all created\n");
      printf(">>> DONE Initializing!\n");
      app->start();

      while ( 1 ) {
	std::string value;
	app->getProperty( "file_reader", "stepThruMsg", value);
        if ( value == "true" ) {
	  app->getProperty( "file_reader", "stepNow", value);
	  if ( value == "false" ) {
	    // wait for user
	    char c;
	    std::cout << "Hit any key to continue" << std::endl;
	    std::cin >> c;
	    app->setProperty("file_reader","stepNow","true");
	  }
	}

	sleep( 1 );
      }




#ifdef DONE

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
#endif


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


