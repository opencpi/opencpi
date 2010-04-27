

#include <CpiParentChild.h>

class Child1;
class Child2;

using namespace CPI::Util;

class Parent1 : public Parent<Child1> , public Parent<Child2>
{
public:
  Parent1(){}

};

class Child1 : public Child<Parent1,Child1>
{
public:
  Child1( Parent1& p )
    : Child<Parent1,Child1>(p){}
};


class Child2 : public Child<Parent1,Child2>
{
public:
  Child2( Parent1& p )
    : Child<Parent1,Child2>(p){}
};

int main()
{
  
  Parent1 p;
  Parent1 *pp;
  Child1 c1(p);
  Child2 c2(p);
  pp = &p;

  Child1* c = p.Parent<Child1>::firstChild();
  Child2* cc = pp->Parent<Child2>::firstChild();


  return 0;
}
