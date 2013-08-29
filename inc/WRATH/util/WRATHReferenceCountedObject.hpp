/*! 
 * \file WRATHReferenceCountedObject.hpp
 * \brief file WRATHReferenceCountedObject.hpp
 * 
 * Copyright 2013 by Nomovok Ltd.
 * 
 * Contact: info@nomovok.com
 * 
 * This Source Code Form is subject to the
 * terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with
 * this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 * 
 * \author Kevin Rogovin <kevin.rogovin@nomovok.com>
 * 
 */


#ifndef __WRATH_REFERENCE_COUNTED_OBJECT_HPP__
#define __WRATH_REFERENCE_COUNTED_OBJECT_HPP__

#include "WRATHConfig.hpp"
#include "WRATHassert.hpp" 
#include <boost/utility.hpp>
#include "WRATHMutex.hpp"
#include "WRATHNew.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\class WRATHReferenceCountedObject
  A  WRATHReferencecountedObject holds
  a private reference count which indicates 
  the number of references to the object active. 
  The handle types \ref WRATHReferenceCountedObject::handle_t
  and \ref WRATHReferenceCountedObject::const_handle_t
  automatically increment and decrement that
  reference count as they go in and out of scope.
  The object itselfis de-allocated when the reference
  count is decremenented to zero. WRATHReferencecountedObject 
  derived objects MUST be allocated dynamically with 
  \ref WRATHNew (because they are deleted with \ref WRATHDelete).
 */
class WRATHReferenceCountedObject:boost::noncopyable
{
public:

  template<typename T>
  class const_handle_t;

  template<typename T>
  class handle_t;

  /*!\fn WRATHReferenceCountedObject(void)
    Default constructor, initializes
    the reference count as _zero_.
    The usage pattern is:
    \code
    WRATHReferenceCountedObject::handle_t<Type> my_handle;
    my_handle=WRATHNew Type(arguments);
    \endcode
    where Type inherits publicly from
    WRATHReferenceCountedObject. 

    If WRATH_DISABLE_ATOMICS is defined, then
    incrementing and decrementing is mutex locked by
    WRATHMutex::default_mutex(). If WRATH_DISABLE_ATOMICS
    is not defined then incrementing and decrementing
    is performed by atomic operations.
   */
  WRATHReferenceCountedObject(void);

  /*!\fn WRATHReferenceCountedObject(WRATHMutex*)
    Initializes
    the reference count as _zero_.
    The usage patter is:
    \code
    ReferenceCountedObject::handle_t<Type> my_handle;
    my_handle=WRATHNew Type(arguments);
    \endcode
    where Type inherits publicly from
    ReferenceCountedObject.
    
    If WRATH_DISABLE_ATOMICS is defined, then
    incrementing and decrementing is thread safe via
    a provided WRATHMutex, otherwise it is performed
    by atomic operations on the reference count.
    \param m WRATHMutex to perform locking
             of the WRATHReferenceCountedObject
             counter. NULL indicates
             to NOT have mutex locking and to
             not use atomic operations to 
             increment/decrement the reference
             count. Use the value NULL only if 
             one is 100% sure the object is to be
             used in one thread only for it's entire
             lifetime.
   */
  WRATHReferenceCountedObject(WRATHMutex *m);

  /*!\fn WRATHReferenceCountedObject(WRATHMutex&)
    Initializes
    the reference count as _zero_.
    The usage patter is:
    \code
    ReferenceCountedObject::handle_t<Type> my_handle;
    my_handle=WRATHNew Type(arguments);
    \endcode
    where Type inherits publicly from
    ReferenceCountedObject. 

    If WRATH_DISABLE_ATOMICS is defined, then
    incrementing and decrementing is thread safe via
    a provided WRATHMutex, otherwise it is performed
    by atomic operations on the reference count.

    \param m WRATHMutex to perform locking
             of the ReferenceCountedObject
             counter.
   */
  WRATHReferenceCountedObject(WRATHMutex &m);

  virtual
  ~WRATHReferenceCountedObject();
  
  /*!\class handle_t
    Handle class for WRATHReferenceCountedObject,
    templated to allow for access to any
    WRATHReferenceCountedObject derived type.
    Upon assignment, and copy construction,
    increments the reference count, upon
    deconstruction decrements the reference
    count. If reference count is decremented
    to zero then the underlying object is 
    deleted.
   */
  template<typename T>
  class handle_t
  {
  public:
    /*!\typedef object_type
      Typedef to the object type
     */
    typedef T object_type;

    /*!\fn handle_t(T*)
      initialize as pointing to a specified
      WRATHReferenceCountedObject.
      \param ptr pointer to WRATHReferenceCountedObject,
                 can be NULL, if so valid() will
                 return false, raw_pointer() will
                 return NULL, and both operator*
                 and operator-> will WRATHassert.
     */
    handle_t(T *ptr=NULL):
      m_ptr(ptr)
    {
      increment(m_ptr);
    }

    /*!\fn handle_t(const handle_t&)
      Copy constructor, correctly increments
      the refernce count.
      \param obj const reference to a handle_t
     */
    handle_t(const handle_t &obj):
      m_ptr(obj.m_ptr)
    {
      increment(m_ptr);
    }

    /*!\fn handle_t(const handle_t<S>&)
      Constructor for _down_ casting.
      i.e. S* must be a downcastable to T*
      \tparam S object type from which to cast. 
                S* must be a downcastable to T*
      \param obj const reference to a hanlde_t
     */
    template<typename S>
    handle_t(const handle_t<S> &obj):
      m_ptr(obj.raw_pointer())
    {
      increment(m_ptr);
    }

    ~handle_t()
    {
      decrement(m_ptr);
    }

    /*!\fn const handle_t& operator=(const handle_t&)
      Assignment operator correctly handles
      reference counting.
      \param obj right hand side of assignment operation
     */
    const handle_t&
    operator=(const handle_t &obj) 
    {
      if(&obj!=this)
        {
          decrement(m_ptr);
          m_ptr=obj.m_ptr;
          increment(m_ptr);
        }

      return *this;
    }

    /*!\fn valid valid(void) const
      Returns true if the handle_t
      refers to a WRATHReferenceCountedObject,
      i.e. raw_pointer() is not NULL.
     */
    bool
    valid(void) const
    {
      return m_ptr!=NULL;
    }

    /*!\fn T* operator->(void) const
      Returns a pointer to the underlying 
      WRATHReferenceCountedObject, if this
      handle_t is not valid, WRATHasserts.
     */
    T*
    operator->(void) const
    {
      WRATHassert(valid());
      return m_ptr;
    }

    /*!\fn T& operator*(void) const
      Returns a reference to the underlying 
      WRATHReferenceCountedObject, if this
      handle_t is not valid, WRATHasserts.
     */
    T&
    operator*(void) const
    {
      WRATHassert(valid());
      return *m_ptr;
    }

    /*!\fn T* raw_pointer
      Returns a pointer to the underlying 
      WRATHReferenceCountedObject, does NOT 
      WRATHassert even if the handle_t is not 
      valid.
     */
    T*
    raw_pointer(void) const
    {
      return m_ptr;
    }

    /*!\fn void swap
      Implemented via swapping underlying pointers,
      and thus avoids incrementing and decrementing
      the reference counts.
      \param obj handle_t to swap with
     */
    void
    swap(handle_t &obj)
    {
      std::swap(m_ptr, obj.m_ptr);
    }

    /*!\fn bool operator<(const handle_t&) const
      Comparison operator for sorting,
      equivalent to comparing the return
      value of raw_pointer().
      \param h handle_t to compare to.
     */
    bool
    operator<(const handle_t &h) const
    {
      return m_ptr<h.m_ptr;
    }

    /*!\fn bool operator==(const handle_t&) const
      Equality operator, equivalent to 
      comparing the return
      value of raw_pointer().
      \param h handle_t to compare to.
     */
    bool
    operator==(const handle_t &h) const
    {
      return m_ptr==h.m_ptr;
    }

    /*!\fn bool operator!=(const handle_t&) const
      Inequality operator, equivalent to 
      comparing the return
      value of raw_pointer().
      \param h handle_t to compare to.
     */
    bool
    operator!=(const handle_t &h) const
    {
      return m_ptr!=h.m_ptr;
    }

    /*!\fn handle_t<S> dynamic_cast_handle(void) const
      Analogue of C++'s dynamic_cast<> for handles.
      \tparam type to attempt to upcast to. Requires
              that dynamic_cast<S*>(T*) is a valid
              C++ expression
     */
    template<typename S>
    handle_t<S>
    dynamic_cast_handle(void) const
    {
      S *ptr;
      ptr=dynamic_cast<S*>(m_ptr);
      return handle_t<S>(ptr);
    }

    /*!\fn handle_t<S> static_cast_handle(void) const
      Analogue of C++'s static_cast<> for handles.
      \tparam type to attempt to upcast to. Requires
              that static_cast<S*>(T*) is a valid
              C++ expression
     */
    template<typename S>
    handle_t<S>
    static_cast_handle(void) const
    {
      S *ptr;
      ptr=static_cast<S*>(m_ptr);
      return handle_t<S>(ptr);
    }

    /*!\fn std::ostream& operator<<(std::ostream&, const handle_t&)
      Overload operator<<, prints the raw poiner value
      to the stream of the handle.
      \param ostr std::ostream to which to print
      \param obj handle to print
     */
    friend
    std::ostream&
    operator<<(std::ostream &ostr, const handle_t &obj)
    {
      ostr << obj.m_ptr;
      return ostr;
    }

  private:
    T *m_ptr;
    friend class const_handle_t<T>;
  };
  
  /*!\class const_handle_t
    Handle class for WRATHReferenceCountedObject,
    templated to allow for access to any
    WRATHReferenceCountedObject derived type.
    Upon assignment, and copy construction,
    increments the reference count, upon
    deconstruction decrements the reference
    count. If reference count is decremented
    to zero then the underlying object is 
    deleted.
   */
  template<typename T>
  class const_handle_t
  {
  public:
    /*!\typedef object_type
      Typedef to the object type
     */
    typedef T object_type;

    /*!\fn const_handle_t(const T*)
      initialize as pointing to a specified
      WRATHReferenceCountedObject.
      \param ptr pointer to WRATHReferenceCountedObject,
                 maybe NULL, if so valid() will
                 return false, raw_pointer() will
                 return NULL, and both operator*
                 and operator-> will WRATHassert.
     */
    const_handle_t(const T *ptr=NULL):
      m_ptr(const_cast<T*>(ptr))
    {
      increment(m_ptr);
    }

    /*!\fn const_handle_t(const const_handle_t&)
      Copy constructor, correctly increments
      the refernce count.
      \param obj const reference to a hanlde_t
     */
    const_handle_t(const const_handle_t &obj):
      m_ptr(obj.m_ptr)
    {
      increment(m_ptr);
    }

    /*!\fn const_handle_t(const handle_t<T>&)
      Constructor, correctly increments
      the reference count.
      \param obj const reference to a hanlde_t
     */
    const_handle_t(const handle_t<T> &obj):
      m_ptr(obj.m_ptr)
    {
      increment(m_ptr);
    }

    /*!\fn const_handle_t(const const_handle_t<S>&)
      Constructor for _down_ casting,
      i.e. S* must be a downcastable to T*
      \tparam S object type from which to cast. 
              S* must be a downcastable to T*
      \param obj const reference to a const_handle_t
     */
    template<typename S>
    const_handle_t(const const_handle_t<S> &obj):
      m_ptr(const_cast<S*>(obj.raw_pointer()))
    {
      increment(m_ptr);
    }

    /*!\fn const_handle_t(const handle_t<S>&)
      Constructor for _down_ casting,
      i.e. S* must be a downcastable to T*
      \tparam S object type from which to cast. 
              S* must be a downcastable to T*
      \param obj const reference to a handle_t
     */
    template<typename S>
    const_handle_t(const handle_t<S> &obj):
      m_ptr(obj.raw_pointer())
    {
      increment(m_ptr);
    }

    ~const_handle_t()
    {
      decrement(m_ptr);
    }

    /*!\fn const const_handle_t& operator=(const const_handle_t&)
      Assignment operator correctly handles
      reference counting.
      \param obj right hand siade of assignment operation
     */
    const const_handle_t&
    operator=(const const_handle_t &obj)
    {
      if(&obj!=this)
        {
          decrement(m_ptr);
          m_ptr=obj.m_ptr;
          increment(m_ptr);
        }

      return *this;
    }

    /*!\fn bool valid
      Returns true if this const_handle_t
      refers to a WRATHReferenceCountedObject,
      i.e. raw_pointer() is not NULL.
     */
    bool
    valid(void) const
    {
      return m_ptr!=NULL;
    }

    /*!\fn const T* operator->(void) const
      Returns a pointer to the underlying 
      WRATHReferenceCountedObject, if this
      const_handle_t is not valid, WRATHasserts.
     */
    const T*
    operator->(void) const
    {
      WRATHassert(valid());
      return m_ptr;
    }

    /*!\fn const T& operator*(void) const
      Returns a reference to the underlying 
      WRATHReferenceCountedObject, if this
      const_handle_t is not valid, WRATHasserts.
     */
    const T&
    operator*(void) const
    {
      WRATHassert(valid());
      return *m_ptr;
    }

    /*!\fn const T* raw_pointer
      Returns a pointer to the underlying 
      WRATHReferenceCountedObject, does NOT 
      WRATHassert even if this const_handle_t 
      is not valid.
     */
    const T*
    raw_pointer(void) const
    {
      return m_ptr;
    }

    /*!\fn void swap
      Implemented via swapping underlying pointers,
      avoiding incrementing and decrementing
      the reference counters
      \param obj const_handle_t to swap with
     */
    void
    swap(const_handle_t &obj)
    {
      std::swap(m_ptr, obj.m_ptr);
    }

    /*!\fn bool operator<(const const_handle_t&) const
      Comparison operator for sorting,
      equivalent to comparing the return
      value of raw_pointer().
      \param h const_handle_t to compare to.
     */
    bool
    operator<(const const_handle_t &h) const
    {
      return m_ptr<h.m_ptr;
    }

    /*!\fn bool operator==(const const_handle_t&) const    
      Equality operator, equivalent to 
      comparing the return
      value of raw_pointer().
      \param h handle_t to compare to.
     */
    bool
    operator==(const const_handle_t &h) const
    {
      return m_ptr==h.m_ptr;
    }

    /*!\fn bool operator!=(const const_handle_t&) const    
      Inequality operator, equivalent to 
      comparing the return
      value of raw_pointer().
      \param h handle_t to compare to.
     */
    bool
    operator!=(const const_handle_t &h) const
    {
      return m_ptr!=h.m_ptr;
    }

    /*!\fn const_handle_t<S> dynamic_cast_handle(void) const
      Analogue of C++'s dynamic_cast<> for const-handles.
      \tparam type to attempt to upcast to. Requires
              that dynamic_cast<const S*>(const T*) is a valid
              C++ expression
     */
    template<typename S>
    const_handle_t<S>
    dynamic_cast_handle(void) const
    {
      S *ptr;

      ptr=dynamic_cast<S*>(m_ptr);
      return const_handle_t<S>(ptr);
    }

    /*!\fn const_handle_t<S> static_cast_handle(void) const
      Analogue of C++'s static_cast<> for handles.
      \tparam type to attempt to upcast to. Requires
              that static_cast<const S*>(const T*) is a valid
              C++ expression
     */
    template<typename S>
    const_handle_t<S>
    static_cast_handle(void) const
    {
      S *ptr;

      ptr=static_cast<S*>(m_ptr);
      return const_handle_t<S>(ptr);
    }

    /*!\fn handle_t<T> handle_t<T> const_cast_handle(void) const
      Analogue of C++'s const_cast<> for handles.
     */
    handle_t<T>
    const_cast_handle(void) const
    {
      return handle_t<T>(m_ptr);
    }

    /*!\fn std::ostream& operator<<(std::ostream&, const const_handle_t&)
      Overload operator<<, prints the raw poiner value
      to the stream of the handle.
      \param ostr std::ostream to which to print
      \param obj handle to print
     */
    friend
    std::ostream&
    operator<<(std::ostream &ostr, const const_handle_t &obj)
    {
      ostr << obj.m_ptr;
      return ostr;
    }

  private:
    T *m_ptr;    
  };

  /*!\typedef handle
    Conveniance typedef
   */
  typedef handle_t<WRATHReferenceCountedObject> handle;

  /*!\typedef const_handle
    Conveniance typedef
   */
  typedef const_handle_t<WRATHReferenceCountedObject> const_handle;

private:
  int m_reference_count;
  WRATHMutex *m_mutex;
  /*
    if WRATH_DISABLE_ATOMICS
    is not defined, that m_mutex
    being non-NULL indicates to use
    atomic ops. If we made it a bool it would
    likely soak up 4 more bytes anyways
    to keep 4 byte alignment.
  */

  static 
  void
  increment(WRATHReferenceCountedObject *ptr);

  static 
  void
  decrement(WRATHReferenceCountedObject *ptr);
  
};

/*!\class WRATHReferenceCountedObjectT
  A WRATHReferenceCountedObjectT provided the conveniance
  of not needing to define the typedef's handle
  and const_handle within a derived class.
  The usage patter is:
  \code                                         
  class MyClass:public WRATHReferenceCountedObjectT<MyClass>
  {
   .
   .
  };

  //the type MyClass::handle and MyClass::const_handle are 
  //defined so that the -> operator returns a MyClass pointer.

  \endcode
 */
template<typename T>
class WRATHReferenceCountedObjectT:public WRATHReferenceCountedObject
{
public:
  /*!\typedef handle
    Conveniance typedef and the purpose of the
    class WRATHReferenceCountedObjectT over 
    WRATHReferenceCountedObject. 
   */
  typedef WRATHReferenceCountedObject::handle_t<T> handle;

  /*!\typedef const_handle
    Conveniance typedef and the purpose of the
    class WRATHReferenceCountedObjectT over 
    WRATHReferenceCountedObject. 
   */
  typedef WRATHReferenceCountedObject::const_handle_t<T> const_handle;

  /*!\fn WRATHReferenceCountedObjectT(void)
    Ctor. Echoes to WRATHReferenceCountedObject::WRATHReferenceCountedObject(void)
   */
  WRATHReferenceCountedObjectT(void)
  {}

  /*!\fn WRATHReferenceCountedObjectT(WRATHMutex*)
    Ctor, echoes to WRATHReferenceCountedObject::WRATHReferenceCountedObject(WRATHMutex*)
    \param m WRATHMutex to pass to WRATHReferenceCountedObject(WRATHMutex*)
   */
  WRATHReferenceCountedObjectT(WRATHMutex *m):
    WRATHReferenceCountedObject(m)
  {}

  /*!\fn WRATHReferenceCountedObjectT(WRATHMutex&)
    Ctor, echoes to WRATHReferenceCountedObject::WRATHReferenceCountedObject(WRATHMutex&)
    \param m WRATHMutex to pass to WRATHReferenceCountedObject(WRATHMutex&)
   */
  WRATHReferenceCountedObjectT(WRATHMutex &m):
    WRATHReferenceCountedObject(m)
  {}

  virtual
  ~WRATHReferenceCountedObjectT()
  {}

};


namespace std
{
  template<typename T>
  void
  swap(WRATHReferenceCountedObject::handle_t<T> &a,
       WRATHReferenceCountedObject::handle_t<T> &b)
  {
    a.swap(b);
  }

  template<typename T>
  void
  swap(WRATHReferenceCountedObject::const_handle_t<T> &a,
       WRATHReferenceCountedObject::const_handle_t<T> &b)
  {
    a.swap(b);
  }
}

/*! @} */

#endif
