
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
#ifndef OCPI_PARENTCHILD_H
#define OCPI_PARENTCHILD_H
#include <assert.h>
#include <string>

#define PCOLD 1
namespace OCPI {
  namespace Util {

    extern std::string childName(const char *name, const char *prefix);
    template <class TChild> class Parent;
    // This is the behavior of being a sibling that doesn't know
    // anything about its parent, only its name and its next
    // sibling.
    // TChild is the class inheriting this.
    // prefix is the string prefix to use when no name is provided
    template <class TChild> class Sibling {
      friend class Parent<TChild>;
      TChild *m_next;
    protected:
      Sibling<TChild>() : m_next(NULL) {}
      virtual ~Sibling<TChild>(){};
    public:
      // virtual TChild *child() = 0;
      inline TChild *nextChild() { return *nextChildP(); }
      inline TChild **nextChildP() { return &m_next; }
      virtual const std::string &name() const = 0;
    };
    template <class TParent, class TChild, const char *&prefix> class ChildOnly;
    // This is the class inherited by the parent, given the child's type
    template <class TChild>  class Parent {
      TChild * m_children;
      bool m_done;
    public:
      Parent<TChild>()
      : m_children(NULL), m_done(false) {}
      // FIXME: add in order with "last" ptr etc.
      void addChild(TChild *child) {
#ifndef NDEBUG
	for (TChild *cp = m_children; cp; cp = cp->nextChild())
	  if (cp == child)
	    assert(!"duplicate child in parent");
#endif
	child->m_next = m_children;
	m_children = child;
      }
      // called by child destructor
      void releaseChild(TChild *child) {
	if (!m_done) {
	  for (TChild **c = &m_children; *c; c = (*c)->nextChildP())
	    if (*c == child) {
	      *c = *child->nextChildP();
	      return;
	    }
          assert(!"child missing from parent");
	}
      }
#if 0
      // call a function on all children, stopping if it returns true
      // it should not add or remove anything.
      TChild * doChildren(bool (*func)(TChild &)) {
	for (TChild *cp = m_children; cp; cp = *cp->nextChildP())
	  if ((*func)(*cp))
	    return cp;
	return NULL;
      }
#endif
      TChild *firstChild() {
	return static_cast<TChild *>(m_children);
      }

      // call a function on all children, stopping if it returns true
      // it should not add or remove anything.
      TChild *findChild(bool (TChild::*doChild)(const void*), const void* arg) {
	for (TChild *cp = m_children; cp; cp = *cp->nextChildP())
	  if ((cp->*doChild)(arg))
            return cp;
	return 0;
      }
      TChild *findChildByName(const char *argName) {
	for (TChild *cp = m_children; cp; cp = *cp->nextChildP())
	  if (!strcmp(cp->name().c_str(), argName))
	    return cp;
	return 0;
      }
#if 0
      inline TChild *findChildByName(const std::string &name) {
	return findChildByName(name.c_str());
	for (TChild *cp = m_children; cp; cp = *cp->nextChildP())
	  if (cp->m_childName == name)
	    return cp;
	return 0;
      }
#endif
      void deleteChildren() {
	if (!m_done) {
	  m_done = true; // suppress release
	  for (TChild *child = m_children; child; child = m_children) {
	    m_children = *child->nextChildP();
	    delete child; // this should call most derived class
	  }
	}
      }
      virtual ~Parent<TChild> () {
	deleteChildren();
      }

    };
#if 0
    // This template class is inherited by base classes of children when such a 
    // base class is the child class of the parent.  This arrangement,
    // where the parent's children are base classes of actual child classes,
    // is appropriate when the relationship between parent and child is
    // that the parent is dealing generically with its children
    template <class TChild>
    class BaseChild : public virtual AbstractChild<TChild> {
      // This works because this template class is inherited by the
      // TChild parameter class
    public:
      virtual TChild *child() { return static_cast<TChild *>(this);}
    };
#endif
    template <class TParent,class TChild, const char *&prefix> class Child;
    // This class is the behavior of knowing your parent
    template <class TParent, class TChild, const char *&prefix>
    class ChildOnly {
      TParent &m_parent;
      std::string m_childName;
      bool m_released; // testing this 
      friend class Parent<TChild>;
    protected:
      ChildOnly<TParent,TChild, prefix>(TParent &p, const char *myName)
      : m_parent(p), m_childName(childName(myName, prefix)), m_released(false) {
	m_parent.
	  Parent<TChild>::addChild(static_cast<TChild*>(this));
      }
      void releaseFromParent() {
	if (!m_released) {
	  m_parent.
	    Parent<TChild>::releaseChild(static_cast<TChild*>(this));
	  m_released = true;
	}
      }
      // virtual just to quiet the compiler warning
      virtual ~ChildOnly<TParent, TChild, prefix>() {
	releaseFromParent();
      }
    public:
      virtual const std::string &name() const { return m_childName; }
      inline TParent &parent() { return m_parent; }
    };
      
    // This is the child where the derived TChild is inherited from here
    extern const char *child;
    template <class TParent,class TChild,const char *&prefix = child>
    class ChildWithBase : public ChildOnly<TParent,TChild,prefix>, public TChild {
      friend class Parent<TChild>;
      //      virtual TChild *child() = 0;
    public:
      // These are public because you can't use "friend class TChild"
      ChildWithBase<TParent,TChild,prefix> (TParent & p, const char *name)
      : ChildOnly<TParent, TChild,prefix>(p, name) {
      }
      //      virtual ~ChildWithBase<TParent,TChild,prefix> () {
      //	releaseDerived(this->child());
      //      }
      virtual const std::string &name() const {
	return ChildOnly<TParent,TChild,prefix>::name();
      }
    };

    // This is the child where the TChild inherits from this template
    // Note Sibling is inherited before ChildOnly so it is initialized first.
    // which it must be (m_next must be NULL before child it added to parent)
    template <class TParent,class TChild, const char *&prefix = child>
    class Child : public Sibling<TChild>, public ChildOnly<TParent,TChild,prefix> {
      friend class Parent<TChild>;
    public:
      // These are public because you can't use "friend class TChild"
      // Let's not put in a null string name so that any usage is more obvious
      Child<TParent,TChild,prefix> (TParent & p, const char *name = "unknown")
      : ChildOnly<TParent, TChild,prefix>(p, name)
      {}
      virtual const std::string &name() const {
	return ChildOnly<TParent,TChild,prefix>::name();
      }
    };

  }
}



#endif
