
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



#include <cstdio>
#include <OcpiParentChild.h>

class Child1;
class Child2;

using namespace OCPI::Util;

class Parent1 : public Parent<Child1> , public Parent<Child2>
{
public:
  Parent1(){}

};

class Child1 : public Child<Parent1,Child1>
{
public:
  Child1( Parent1& p )
    : Child<Parent1,Child1>(p, *this, "Child1"){}
};


class Child2 : public Child<Parent1,Child2>
{
public:
  Child2( Parent1& p )
    : Child<Parent1,Child2>(p, *this, "Child2"){}
};

int main()
{
  
  Parent1 p;
  Parent1 *pp;
  Child1 c1(p);
  Child2 c2(p);
  pp = &p;

  Child1* c = p.Parent<Child1>::firstChild();
  if ( !c )
  {
    printf ( "Child1* c = p.Parent<Child1>::firstChild() is null\n" );
  }

  Child2* cc = pp->Parent<Child2>::firstChild();
  if ( !cc )
  {
    printf ( "Child2* .c = pp->Parent<Child2>::firstChild() is null\n" );
  }

  return 0;
}
