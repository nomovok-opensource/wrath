/*! 
 * \file WRATHAttributePacker.hpp
 * \brief file WRATHAttributePacker.hpp
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




#ifndef __WRATH_ATTRIBUTE_PACKER_HPP__
#define __WRATH_ATTRIBUTE_PACKER_HPP__

#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include "WRATHGLProgram.hpp"
#include "WRATHResourceManager.hpp"

/*! \addtogroup Group
 * @{
 */

/*!\class WRATHAttributePacker
  A WRATHAttributePacker is a generic base
  class for objects that perform attribute
  packing. Such objects need to name their
  attribute identifiers to be used in GLSL. 
 */
class WRATHAttributePacker:boost::noncopyable
{
public:

  /*!\typedef ResourceKey
    Resource key type for WRATHAttributePacker 
    resource manager.
   */
  typedef std::string ResourceKey;

  /*!\class attribute_names
    Conveniance wrapper over std::vector<std::string>
    to specify the attribute names of a
    WRATHAttributePacker
   */ 
  class attribute_names:public std::vector<std::string>
  {
  public:
    /*!\fn name
      Sets the attribute name for an attribute index
      \param attribute_index attribute index to set, if this->size()
                             is smaller than or equal to attribute_index,
                             resizes to 1+attribute_index
      \param attribute_name name for the attribute as it appears in GLSL
     */
    attribute_names&
    name(unsigned int attribute_index, 
         const std::string &attribute_name)
    {
      resize(std::max(static_cast<unsigned int>(size()),
                      1+attribute_index));
      operator[](attribute_index)=attribute_name;
      return *this;
    }
  };

  /*!\class AttributePackerFactory
    The purpose of a AttributePackerFactory is to help
    automate the case where a WRATHAttributePacker derived
    class is essentially a singleton (i.e. there should
    only be one alive). In that case, the ctor is private.
    An AttributePackerFactory class only purpose is to
    allow for the creation of such a WRATHAttributePacker
    derived object by the method \ref WRATHAttributePacker::fetch_make().
   */
  class AttributePackerFactory
  {
  public:
    /*!\fn WRATHAttributePacker* create
      To be implemented by a derived class
      to create and return a WRATHAttributePacker
      object. The resource name of the
      returned WRATHAttributePacker, Obj, 
      _MUST_ be typeid(Obj).name().
     */
    virtual
    WRATHAttributePacker*
    create(void) const=0;
  };

  /// @cond
  WRATH_RESOURCE_MANAGER_DECLARE(WRATHAttributePacker, ResourceKey);
  /// @endcond

  /*!\fn WRATHAttributePacker(const ResourceKey &pname, const std::vector<std::string>&)
    Ctor. Specifies the resource name of the attribute packer
    and the names of each attribute as an STL range.
    The number of attributes is then std::distance(begin,end)
    and the attribute of index I has value *(begin+I).
    \tparam iterator to string type
    \param pname resource name to identify the WRATHAttributePacker
    \param pattribute_names names of the attributes, value at index 0 
                            will be for attribute #0 in GL
   */
  WRATHAttributePacker(const ResourceKey &pname,
                       const std::vector<std::string> &pattribute_names):
    m_resource_name(pname),
    m_attribute_names(pattribute_names)
  {
    register_resource();
  }

  /*!\fn WRATHAttributePacker(const ResourceKey &pname,
                              iterator, iterator)
    Ctor. Specifies the resource name of the attribute packer
    and the names of each attribute as an STL range.
    The number of attributes is then std::distance(begin,end)
    and the attribute of index I has value *(begin+I).
    \tparam iterator to string type
    \param pname resource name to identify the WRATHAttributePacker
    \param begin iterator to name of attribute #0 in GL
    \param end iterator to one past the name of the last attribute
   */
  template<typename iterator>
  WRATHAttributePacker(const ResourceKey &pname,
                       iterator begin, iterator end):
    m_resource_name(pname),
    m_attribute_names(begin, end)
  {
    register_resource();
  }

  virtual
  ~WRATHAttributePacker();

  /*!\fn const ResourceKey& resource_name(void)
    returns the resource name of this WRATHAttributePacker.
   */
  const ResourceKey&
  resource_name(void)
  {
    return m_resource_name;
  }

  /*!\fn int number_attributes(void) const
    To be implemented by a derived class
    to return the number of attributes
    the WRATHAttributePacker derived object
    has for it's attribute type.
   */
  int
  number_attributes(void) const
  {
    return m_attribute_names.size();
  }

  /*!\fn const std::string& attribute_name(int) const
    Returns the name that a GLSL vertex
    shader is to use for an attribute
    at an attribute index.
    \param attribute_index index of attribute, must
                           be in the range [0, N),
                           where N=number_attributes()
   */
  const std::string&
  attribute_name(int attribute_index) const
  {
    return m_attribute_names[attribute_index];
  }


  /*!\fn void bind_attributes(WRATHGLPreLinkActionArray &) const
    Provided as a convenience to call 
    \code
    WRATHGLPreLinkActionArray::add_binding(i, attribute_name(i))
    \endcode
    for each a i, 0<=i<N, N=number_attributes().
    \param binder WRATHGLPreLinkActionArray on which to act
   */
  void
  bind_attributes(WRATHGLPreLinkActionArray &binder) const;

  /*!\fn const std::vector<std::string>& all_attribute_names(void) const
    Returns all the attribute names as an 
    std::vector<std::string> with that
    the size is number_attributes()
    and the element at index I is
    attribute_name(I).
   */
  const std::vector<std::string>&
  all_attribute_names(void) const
  {
    return m_attribute_names;
  }

  /*!\fn T* fetch_make(const AttributePackerFactory &)
    Method to allow for implementing singleton
    WRATHAttributePacker derived classes easier.
    It checks if a WRATHAttributePacker object
    whose \ref resource_name() is _exactly_
    typeid(T).name() exists, and if so return that
    object dynamic_cast'ed to type T. If such
    an object does not exist, it then creates
    an object with the passed AttributePackerFactory.
    Method assert-checks if the returned object's
    \ref resource_name() is exactly typeid(T).name().

    \param factory AttributePackerFactory derived object
                   used to produce the return value if
                   the object of type T does not yet exist.
   */
  template<typename T>
  static
  T*
  fetch_make(const AttributePackerFactory &factory)
  {
    WRATHAutoLockMutex(fetch_make_mutex());
    WRATHAttributePacker *q;
    T *p;

    q=retrieve_resource(typeid(T).name());
    if(q==NULL)
      {
        q=factory.create();
      }

    p=dynamic_cast<T*>(q);
    WRATHassert(p!=NULL);
    WRATHassert(p->resource_name()==typeid(T).name());

    return p;
  }


private:
  static
  WRATHMutex&
  fetch_make_mutex(void);

  void
  register_resource(void);

  ResourceKey m_resource_name;
  std::vector<std::string> m_attribute_names;
};



/*!\class WRATHStateBasedPackingData
  WRATHStateBasedPackingData represents additional immutable
  state used by a WRATHAttributePacker derived object to
  generate attribute data to pack. This data is to be passed
  to a WRATHAttributePacker derived classes methods
*/
class WRATHStateBasedPackingData:public WRATHReferenceCountedObjectT<WRATHStateBasedPackingData>
{};

/*! @} */

#endif
