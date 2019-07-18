/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// This module will be a submodule under the opencpi package
%module "aci"
%include <exception.i>
%include <std_string.i>
%include <cstring.i>
%include <cpointer.i>
%include <stdint.i>
%include <typemaps.i>

%apply std::string& OUTPUT {std::string& value};

// ExternalBuffer *getBuffer(uint8_t *&data, size_t &length, uint8_t &opCode, bool &endOfData)
// Typemaps to adapt the c++ return by reference calls into return values. Allows for data to be read FROM
// an external port
%typemap(in, numinputs=0) (uint8_t *&data, size_t &length, uint8_t &opCode, bool &endOfData) (uint8_t *tdata, size_t tlength, uint8_t topCode, bool tendOfData) {
    tdata = NULL;
    tlength = 0;
    topCode = 0;
    tendOfData = false;
    $1 = &tdata;
    $2 = &tlength;
    $3 = &topCode;
    $4 = &tendOfData;
}

%typemap(argout) (uint8_t *&data, size_t &length, uint8_t &opCode, bool &endOfData) {
    if (result) {
        PyObject *list = PyList_New(tlength$argnum);
        for (size_t i = 0; i < tlength$argnum; i++) {
            PyList_SetItem(list, i, PyInt_FromLong(tdata$argnum[i]));
        }

        %append_output(list);
        %append_output(PyInt_FromSize_t(*$2));
        %append_output(PyInt_FromLong(*$3));
        %append_output(PyBool_FromLong(*$4));
    }
}

%rename(writeBuffer) getBuffer(uint8_t *&data, size_t &length);

// ExternalBuffer *getBuffer(uint8_t *&data, size_t &length)
// Typemaps to adapt the c++ return by reference calls into return values. Allows for data to be written TO
// an external port
%typemap(in) (uint8_t *&data, size_t &length) (uint8_t *tdata, size_t tlength) {
    if (!PyList_Check($input)) {
        PyErr_SetString(PyExc_ValueError, "Expecting a python list");
        return NULL;
    }

    tdata = NULL;
    tlength = 0;

    $1 = &tdata;
    $2 = &tlength;
}

%typemap(argout) (uint8_t *&data, size_t &length) {
    if (result) {
        size_t num_bytes = (size_t)PyList_Size($input);
        size_t xfer_bytes = std::min(num_bytes, tlength$argnum);
        for (size_t i = 0; i < xfer_bytes; i++) {
            tdata$argnum[i] = (uint8_t)PyInt_AsLong(PyList_GetItem($input, i));
        }

        %append_output(PyInt_FromLong(xfer_bytes));
    }
}

// OCPI::API::BaseType getOperationInfo(uint8_t opCode, size_t &nbytes)
// Typemap to adapt the c++ return by reference calls into return values.
// This is a trivial case so we can just use %apply
%apply size_t &OUTPUT { size_t &nbytes }

%{
#include "OcpiApi.h"
%}

%exception {
    try {
        $action
    } catch (std::string &e) {
        SWIG_exception(SWIG_RuntimeError, e.c_str());
    } catch (...) {
        SWIG_exception(SWIG_RuntimeError, "Unknown Exception");
    }
}

// m_info is a predeclared pointer in the ContainerAPI. Make it 'immutable' so
// swig doesn't try to create a setter for it
%immutable OCPI::API::Property::m_info;

%include "OcpiUtilDataTypesApi.h"
%include "OcpiPValueApi.h"
%include "OcpiUtilPropertyApi.h"
%include "OcpiUtilExceptionApi.h"
%include "OcpiLibraryApi.h"
%include "OcpiContainerApi.h"
%include "OcpiApplicationApi.h"
%include "OcpiApi.h"
