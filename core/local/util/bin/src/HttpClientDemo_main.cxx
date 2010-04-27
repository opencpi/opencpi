#include <CpiUtilUri.h>
#include <CpiUtilHttpClient.h>
#include <CpiUtilTcpConnector.h>
#include <iostream>
#include <string>
#include <fstream>

int
main (int argc, char *argv[])
{
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <URI>" << std::endl;
    return 1;
  }

  CPI::Util::Uri uri (argv[1]);
  CPI::Util::Http::Client<CPI::Util::Tcp::Connector> stream;

 again:
  try {
    stream.get (uri);
  }
  catch (const CPI::Util::Http::Redirection & ex) {
    std::cout << "request redirected to "
	      << ex.newLocation
	      << std::endl;
    uri = ex.newLocation;
    goto again;
  }
  catch (const CPI::Util::Http::HttpError & ex) {
    std::cout << "server status "
	      << ex.statusCode << " "
	      << ex.reasonPhrase
	      << std::endl;
    return 1;
  }
  catch (const std::string & ex) {
    std::cout << "error: " << ex << std::endl;
    return 1;
  }

  if (!stream.good()) {
    std::cout << "oops, stream is no good." << std::endl;
    return 1;
  }

  CPI::Util::Http::Headers::const_iterator it;
  for (it = stream.responseHeaders().begin();
       it != stream.responseHeaders().end(); it++) {
    std::cout << "    " << (*it).first
	      << ": " << (*it).second
	      << std::endl;
  }

  std::string filename = uri.getFileName ();

  if (!filename.length()) {
    filename = "index.html";
  }

  std::ofstream out (filename.c_str(), std::ios_base::binary);

  if (!out.good()) {
    std::cout << "error: cannot open "
	      << filename
	      << " for output"
	      << std::endl;
    return 1;
  }

  std::cout << "downloading "
	    << uri.get()
	    << " as "
	    << filename
	    << " "
	    << std::flush;

  char buffer[10240];
  while (stream.good() && !stream.eof() && out.good()) {
    stream.read (buffer, 10240);
    out.write (buffer, stream.gcount());
    std::cout << "." << std::flush;
  }

  if (!stream.eof() && !stream.good()) {
    std::cout << " error." << std::endl;
  }
  else if (!out.good()) {
    std::cout << " error." << std::endl;
  }
  else {
    std::cout << " done." << std::endl;
  }

  stream.close ();
  out.close ();
  return 0;
}
