// A template that implements a parent/child relationship such that:
// 1. the constructor of the child will record its parent
// 2. the constructor of the child will register itself with its parent
// 3. the destructor of the child will inform the parent.
// 4. the destructor of the parent will destroy all children.
//
// To use it:

// In the parent class:
// 1. With child class "Child", inherit: public Parent<Child>
// 2. Have a named constructor (factory method) of the child class, which calls
//    a private constructor of the child class that takes the parent (reference)
//    as an argument.

// In the child class:
// 1. With parent class "XParent" and child class "XChild", inherit: public Child<XParent,XChild>
// 2. Declare the parent as a friend class
// 3. Have a private constructor that takes the parent reference as one of its argument, and initialize
//    this template class with: Child<XParent,XChild>(XParent &parent)

// I wish you didn't need to feed the child's class to the template that it inherits.
// I wish that 
#ifndef CPI_PARENTCHILD_H
#define CPI_PARENTCHILD_H
#include <stdint.h>
#include <assert.h>
#include <set>
#include <string>

namespace CPI {
  namespace Util {

  
    template <class TChild> class Parent;

    // This is the "internal" template that simply provide a linked list among children.
    template <class TChild> class ChildI  {
      friend class Parent<TChild>; // Allow the parent to use this link
      ChildI<TChild> *next;
    public:
      virtual ~ChildI<TChild>(){};
    };

    // This is the class inherited by the child, given the parent's type and the child's type
    template <class TParent,class TChild> class Child : public ChildI<TChild> {
    public:
      // These are public because you can't use "friend class TChild"
      TParent *myParent;
      std::string m_cname;
      Child<TParent,TChild> (TParent & p, const char* childname=NULL) :
        myParent(&p) {
        myParent->Parent<TChild>::addChild(*this);
      };
        Child<TParent,TChild> (const char* childname=NULL) : 
          myParent(NULL)  
          {
            if ( childname ) {
              m_cname = childname;
            }
          }
          void setParent( TParent & p ) {
            myParent = &p;
            myParent->addChild(*this);
          }
          ~Child<TParent,TChild> () {
            myParent->Parent<TChild>::releaseChild(*this);
          }
    };



    // This is the class inherited by the parent, given the child's type
    template <class TChild>  class Parent {
      ChildI<TChild> * myChildren;
      bool done;
    public:
      Parent<TChild>(const char* instancename=NULL) :
        myChildren(0), done(false) {}
        void releaseChild(ChildI<TChild>& child) {
          if (done)
            return;
          for (ChildI<TChild> **cp = &myChildren; *cp; cp = &(*cp)->next)
            if (*cp == &child) {
              *cp = child.next;
              return;
            }
          assert(!"child missing from parent");
        }
        // call a function on all children, stopping if it returns true
        // it should not add or remove anything.
        bool doChildren(bool (*func)(TChild &)) {
          for (ChildI<TChild> *cp = myChildren; cp; cp = cp->next)
            if ((*func)(*static_cast<TChild *>(cp)))
              return true;
          return false;
        }


        TChild *firstChild() {
          return static_cast<TChild *>(myChildren);
        }
        TChild *nextChild( ChildI<TChild>* c) {
          return static_cast<TChild *>(c->next);
        }


        // call a function on all children, stopping if it returns true
        // it should not add or remove anything.
        TChild *findChild(bool (TChild::*doChild)(const char*), const char* arg) {
          for (ChildI<TChild> *cp = myChildren; cp; cp = cp->next)
            if ((static_cast<TChild *>(cp)->*doChild)(arg))
              return static_cast<TChild *>(cp);
          return 0;
        }
        void addChild(ChildI<TChild> &child) {
#ifndef NDEBUG
          for (ChildI<TChild> **cp = &myChildren; *cp; cp = &(*cp)->next)
            if (*cp == static_cast<ChildI<TChild> *>(&child))
              assert(!"duplicate child in parent");
#endif
          child.next = myChildren;
          myChildren = &child;
        }
        virtual ~Parent<TChild> () {
          done = true; // suppress release
          while (myChildren) {
            ChildI<TChild> *child = myChildren;
            myChildren = child->next;
            delete child; // this should call most derived class
          }
        }

    };
  }
}



#endif
