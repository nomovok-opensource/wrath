/*! 
 * \file WRATHResourceManager.hpp
 * \brief file WRATHResourceManager.hpp
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




#ifndef __WRATH_RESOURCE_MANAGER_HPP__
#define __WRATH_RESOURCE_MANAGER_HPP__

#include "WRATHConfig.hpp"
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <boost/utility.hpp>
#include "WRATHassert.hpp" 
#include "type_tag.hpp"
#include "WRATHNew.hpp"
#include "WRATHMutex.hpp"
#include "WRATHStaticInit.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\class WRATHResourceManagerBase
  A WRATHResourceManagerBase is the base
  interface for a generic resource
  manager. WRATHResourceManagerBase provides
  an -interface- to delete (see clear())
  all elements tracked by a WRATHResourceManagerBase
  derived object. In addition, each 
  WRATHResourceManagerBase is tracked and one
  can issue a call to clear all 
  WRATHResourceManagerBase derived objects,
  see clear_all_resource_managers()
 */
class WRATHResourceManagerBase:boost::noncopyable
{
public:
  /*!\enum add_resource_return_type
    Enumeration to describe how
    a resource was added to a 
    resource manager.
   */
  enum add_resource_return_type
    {
      /*!
        Resource was added with
        name as passed.
       */
      element_added_as_named,

      /*!
        Resource was added, but unnamed.
        This happens if another resource
        is already using the name.
       */
      element_added_as_unnamed,

      /*!
        Resource was already added.
       */
      element_already_in_manager
    };

  WRATHResourceManagerBase(void);

  virtual
  ~WRATHResourceManagerBase(void);

  /*!\fn void clear
    To be implemented by a derived
    class to clear itself: delete
    all objects tracked and to clear
    its table of tracked objects.
   */
  virtual
  void
  clear(void)=0;

  /*!\fn clear_all_resource_managers
    Calls clear() on all WRATHResourceManagerBase
    derived objects alive.
   */
  static
  void
  clear_all_resource_managers(void);
};


/*!\class WRATHResourceManager
  A simple resource manager class, meant to manage
  "big" objects, when the WRATHResourceManager is deleted, 
  elements on the manager are deleted. Elements
  are deleted with \ref WRATHDelete. 

  Each function of a WRATHResourceManager is thread-safe,
  i.e. may be called from multiple threads.

  When associating a resource manager with a class
  type use the macros \ref WRATH_RESOURCE_MANAGER_DECLARE
  and \ref WRATH_RESOURCE_MANAGER_IMPLEMENT to associate
  a resource manager with the class type.

  \tparam T type of resource
  \tparam K key for resource look up, must be sortable by operator<
 */
template<typename T, typename K>
class WRATHResourceManager:public WRATHResourceManagerBase
{
public:
  
  ~WRATHResourceManager(void)
  {
    clear();
  }

  virtual
  void
  clear(void)
  {
    std::set<T*> vs;

    WRATHLockMutex(m_mutex);
    std::swap(vs, m_resources);
    m_named_resources.clear();
    m_reverse_map.clear();
    WRATHUnlockMutex(m_mutex);

    for(typename std::set<T*>::iterator 
          iter=vs.begin(), end=vs.end();
        iter!=end; ++iter)
      {
        WRATHDelete(*iter);
      }    
  }

  /*!\fn enum add_resource_return_type add_resource
    Register a resource to this WRATHResourceManager.
    If the resource is still registered to the
    WRATHResourceManager when it is deleted, then
    the object is deleted by the deconstructor
    of WRATHResourceManager via \ref WRATHDelete.
    \param pname name to identify the resource by
    \param element pointer to resource.
   */
  enum add_resource_return_type
  add_resource(const K &pname, T *element)
  {
    WRATHAutoLockMutex(m_mutex);
    if(m_resources.find(element)==m_resources.end())
      {
        m_resources.insert(element);     
        if(m_named_resources.find(pname)==m_named_resources.end())
          {
            m_named_resources[pname]=element;
            m_reverse_map[element]=pname;
            return element_added_as_named;
          }
        else
          {
            return element_added_as_unnamed;
          }
      }
    else
      {
        return element_already_in_manager;
      }
  }

  /*!\fn bool resource_exists
    Returns true if a resource with the
    specified name is a part of this
    WRATHResourceManager.
    \param pname name to query
   */
  bool
  resource_exists(const K &pname)
  {
    WRATHAutoLockMutex(m_mutex);
    return m_named_resources.find(pname)!=m_named_resources.end();
  }

  /*!\fn enum return_code remove_resource
    Unregister a resource by resource (not by name),
    does NOT delete the resource. This should be
    used in object's deconstructor's to unregister
    themselves from a WRATHResourceManager. Returns
    \ref routine_fail if the resource was not registered
    to the WRATHResourceManager prior to the call.
    \param element pointer to resource.
   */
  enum return_code
  remove_resource(T *element)
  {    
    
    WRATHAutoLockMutex(m_mutex);
    if(m_resources.find(element)!=m_resources.end())
      {
        typename std::map<T*, K>::iterator iter;

        m_resources.erase(element);

        iter=m_reverse_map.find(element);
        if(iter!=m_reverse_map.end())
          {
            typename std::map<K, T*>::iterator niter;

            niter=m_named_resources.find(iter->second);
            WRATHassert(niter!=m_named_resources.end());

            m_named_resources.erase(niter);
            m_reverse_map.erase(iter);
          }
        return routine_success;
      }
    else
      {
        return routine_fail;
      }
  }

  /*!\fn T* retrieve_resource
    Retrieve a resource by name.
    If no resource has that name,
    returns NULL. This performs
    an std::map<K,T*> look up to find the 
    resource.
    \param pname resource to retrieve.
   */
  T*
  retrieve_resource(const K &pname)
  {
    typename std::map<K, T*>::iterator iter;
    WRATHAutoLockMutex(m_mutex);
    iter=m_named_resources.find(pname);
    return (iter!=m_named_resources.end())?
      iter->second:
      NULL;
  }

  
private:
  std::map<K, T*> m_named_resources;
  std::map<T*, K> m_reverse_map;
  std::set<T*> m_resources;
  WRATHMutex m_mutex;
};

/*!\def WRATH_RESOURCE_MANAGER_DECLARE
  Place the macro WRATH_RESOURCE_MANAGER_DECLARE 
  within a class declaration so that a resource
  manager is associated to the class type. 
  The macro creates the following static methods:
  \code
   WRATHResourceManager<T,K> resource_manager(void) returns the resource manager
   T* retrieve_resource(const K&) equivalent to resource manager().retrieve_resource(const K&)
   bool resource_exists(const K&) equivalent to resource manager().resource_exists(const K&)
   enum return_code remove_resource(T*) equivalent to resource manager().remove_resource(T*)
  \endcode
  \param T name of the class, defines the return type and the scope.
  \param K resource key, i.e. "name type" of resource.
 */
#define WRATH_RESOURCE_MANAGER_DECLARE(T,K)               \
  static WRATHResourceManager<T,K>& resource_manager(void);       \
  static T* retrieve_resource(const K &pname);\
  static bool resource_exists(const K &pname);\
  static enum WRATHResourceManagerBase::add_resource_return_type add_resource(const K &pname, T *element);\
  static enum return_code remove_resource(T *element);

/*!\def WRATH_RESOURCE_MANAGER_IMPLEMENT
  If the macro WRATH_RESOURCE_MANAGER_DECLARE was
  placed within a class declaration, then one must
  have the macro WRATH_RESOURCE_MANAGER_IMPLEMENT with
  the same exact arguments as used in WRATH_RESOURCE_MANAGER_DECLARE
  in exactly one source file to instaniate the
  resource manager.
  \param T name of the class, defines the return type and the scope.
  \param K resource key, i.e. "name type" of resource.
 */
#define WRATH_RESOURCE_MANAGER_IMPLEMENT(T,K)     \
  WRATHResourceManager<T,K>&                      \
  T::resource_manager(void) \
  {\
    WRATHStaticInit();\
    static WRATHResourceManager<T,K> return_value;\
    return return_value;\
  } \
  T* T::retrieve_resource(const K &pname) { return resource_manager().retrieve_resource(pname); } \
  bool T::resource_exists(const K &pname) { return resource_manager().resource_exists(pname); } \
  enum WRATHResourceManagerBase::add_resource_return_type T::add_resource(const K &pname, T *element) \
  { return resource_manager().add_resource(pname, element); }\
  enum return_code T::remove_resource(T *element) { return resource_manager().remove_resource(element); }
/*! @} */


#endif
