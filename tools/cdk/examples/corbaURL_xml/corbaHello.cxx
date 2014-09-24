#include <stdio.h>
#include <cassert>
#include <string>
#include "OcpiApi.h"

namespace OA = OCPI::API;

int main(int argc, char **argv) {
  try {
    fprintf(stderr, "Starting the CORBA server process\n");
    char url[1024];
    FILE *server = NULL;
    if (argv[1])
      strcpy(url, argv[1]);
    else {
      server = popen("./server", "r");
      if (!server)
	throw "Couldn't start server process";
      fprintf(stderr, "Retrieving the object URL from the server's standard output\n");
      if (fgets(url, 1024, server) == NULL)
	throw "Couldn't get URL from server process";
      char *nl = strchr(url, '\n');
      if (!nl)
	throw "URL from server process has no newline";
      *nl = 0;
      fprintf(stderr, "Server process provided URL for object: %s\n", url);
    }
    std::string xml("<application>"
		    "  <instance component='hello' selection='model==\"rcc\"'/>"
		    "  <connection>"
		    "    <external url='");
    xml += url;
    xml +=                            "'/>"
                    "    <port instance='hello' name='out'/>"
		    "  </connection>"
		    "</application>";
    OA::Application app(xml);
    fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
    app.initialize();
    fprintf(stderr, "Application established: containers, workers, connections all created\n");
    sleep(10);
    app.start();
    fprintf(stderr, "Application started/running\n");

    if (server)
      while (fgetc(server) != EOF)
	;
    else
      app.wait(10000000);
    fprintf(stderr, "CORBA server exited, shutting down\n");
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  } catch (const char *&e) {
    fprintf(stderr, "Exception thrown: %s\n", e);
  }
  return 1;
}
