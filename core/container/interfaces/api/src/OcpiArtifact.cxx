
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <climits>
#include "OcpiContainerMisc.h"
#include "OcpiContainerApplication.h"
#include "OcpiArtifact.h"

namespace OCPI {
  namespace Container {
    // Get the metadata from the end of the file.
    // The length of the appended file is appended on a line starting with X
    // i.e. (cat meta; sh -c 'echo X$4' `ls -l meta`) >> bitstream
    // This scheme allows for binary metadata, but we are doing XML now.
    static char *getMetadata(const char *url, unsigned &length) {
      int fd = open(url, O_RDONLY);
      if (fd < 0)
	throw ApiError("Cannot open file: \"", url, "\"", NULL);
#define STR(x) #x
      // +3 for X, \r (if needed), \n, and assuming the definition is decimal
      char buf[sizeof(STR(UINT64_MAX))+3];
      off_t fileLength, second, third;
      if (fd >= 0 &&
          (fileLength = lseek(fd, 0, SEEK_END)) != -1 &&
          // I have no idea why the off_t caste below is required,
          // but without it, the small negative number is not sign extended...
          // on MACOS gcc v4.0.1 with 64 bit off_t
          (second = lseek(fd, -(off_t)sizeof(buf), SEEK_CUR)) != -1 &&
          (third = read(fd, buf, sizeof(buf))) == sizeof(buf)) {
        for (char *cp = &buf[sizeof(buf)-2]; cp >= buf; cp--)
          if (*cp == 'X' && isdigit(cp[1])) {
            char *end;
            long long n = strtoll(cp + 1, &end, 10);
            // strtoll error reporting is truly bizarre
            if (n != LLONG_MAX && n >= 0 && cp[1] && isspace(*end)) {
              off_t metaStart = fileLength - sizeof(buf) + (cp - buf) - n;
              if (lseek(fd, metaStart, SEEK_SET) != -1) {
                char *data = new char[n + 1];
                if (read(fd, data, n) == n) {
                  data[n] = '\0';
                  length = n + 1;
                  close(fd);
                  return data;
                }
                delete [] data;
              } else
                perror("LSEEK");
            }
            break;
          }
      }
      if (fd >= 0)
        (void)close(fd);
      throw ApiError("Invalid Metadata in bitstream/artifact file", NULL);
    }
    Artifact::Artifact(Interface &i, const char *url) 
      : OCPI::Util::Child<Interface,Artifact>(i), myUrl(strdup(url)), myMetadata(0), myXml(0) {
      unsigned length;
      myMetadata = getMetadata(url, length);
      myXml = ezxml_parse_str(myMetadata, length);
      const char *name = ezxml_name(myXml);
      if (!name || strcmp(name, "artifact"))
        throw ApiError("invalid metadata in binary/artifact file \"", url,
                           "\": no <artifact/>", NULL);
    }
    Artifact::~Artifact(){
      ezxml_free(myXml);
      delete [] myMetadata;
    }
    bool Artifact::hasUrl(const char *url) {
      return strcmp(url, myUrl) == 0;
    }
      // Return the value of a boolean attribute that defaults to false
    static bool boolAttrValue(ezxml_t x, const char *attr) {
      const char *val = ezxml_attr(x, attr);
      if (val) {
        if (!strcmp("true", val) || !strcmp("TRUE", val) || !strcmp("1", val))
          return true;
        if (strcmp("false", val) && strcmp("FALSE", val) && strcmp("0", val) &&
            val[0] != 0)
          throw ApiError("Boolean attribute \"", attr, "\" has bad value \"",
                         val, "\"", NULL);
      }
      return false;
    }
    static bool hasAttrEq(ezxml_t x, const char *attrName, const char *val) {
      const char *attr = ezxml_attr(x, attrName);
      return attr && !strcmp(attr, val);
    }
    static ezxml_t findChildWithAttr(ezxml_t x, const char *cName, const char *aName,
                                     const char *value)
    {
      for (ezxml_t c = ezxml_child(x, cName); c; c = ezxml_next(c))
        if (hasAttrEq(c, aName, value))
          return c;
      return 0;
    }
    // We want an instance from an implementation, and optionally,
    // a specifically identified instance of that implementation,
    // when the artifact would contain such a thing.  Here are the cases:
    // Instance tag supplied, no instances in the artifact: error
    // Instance tag supplied, instance not in artifact: error
    // Instance tag supplied, instance already used: error
    // No instance tag supplied, no instances in artifact: ok
    // No instance tag supplied, instances in artifact, all used: error
    // No instance tag supplied, instances in artifact, one is available: ok
    Worker & Artifact::
    createWorker(Application &app, const char *implTag, const char *instTag, OCPI::Util::PValue* execProps) {
      ezxml_t impl, inst;

      if (!implTag ||
          !(impl = findChildWithAttr(myXml, "worker", "name", implTag)))
        throw ApiError("No implementation found for \"", implTag, "\"", NULL);
      if (instTag) {
        inst = findChildWithAttr(myXml, "instance", "name", instTag);
        if (!inst)
          throw ApiError("no worker instance named \"", instTag,
                             "\" found in this artifact", NULL);
        if (!implTag || !hasAttrEq(inst, "worker", implTag))
          throw ApiError("worker instance named \"", instTag,
                             "\" is not a \"", implTag ? implTag : "<null>",
                             "\" worker", NULL);
        if (findChild(&Worker::hasInstTag, instTag))
          throw ApiError("worker instance named \"", instTag,
                             "\" already used", NULL);
      } else {
        inst = 0;
        // I didn't specify an instance, so I must want a floating
        // implementation.  Find one.
        if (boolAttrValue(impl, "connected"))
          throw ApiError("specified implementatio, \"", implTag,
                             "\", is already connected", NULL);
        if (!boolAttrValue(impl, "reusable") &&
            findChild(&Worker::hasImplTag, implTag))
          throw ApiError("non-reusable worker named \"", implTag,
                             "\" already used", NULL);
      }
      Worker &w = createWorkerX(app, impl, inst, execProps);
      w.initialize();
      return w;
    }
  }
}
