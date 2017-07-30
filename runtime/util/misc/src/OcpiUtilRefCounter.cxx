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

#include <OcpiUtilRefCounter.h>
#include <OcpiOsAssert.h>

OCPI::Util::Misc::RefCounter::RefCounter()
:refCount(1)
{

}
OCPI::Util::Misc::RefCounter::~RefCounter()
{
        ocpiAssert( refCount == 0 );
}
int OCPI::Util::Misc::RefCounter::incRef()
{
        return refCount++;
}
int OCPI::Util::Misc::RefCounter::decRef()
{
        refCount--;
        if ( refCount == 0 ) {
                delete this;
                return 0;
        }
        return refCount;
}


