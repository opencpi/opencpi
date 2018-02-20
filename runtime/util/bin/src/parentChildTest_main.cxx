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
