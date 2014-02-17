/*! 
 * \file WRATHCanvas.hpp
 * \brief file WRATHCanvas.hpp
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




#ifndef WRATH_HEADER_CANVAS_HPP_
#define WRATH_HEADER_CANVAS_HPP_

#include "WRATHConfig.hpp"
#include <map>
#include "WRATHNew.hpp"
#include "WRATHRawDrawData.hpp"
#include "WRATHItemGroup.hpp"

/*! \addtogroup Group
 * @{
 */

/*!\class WRATHCanvas
  A WRATHCanvas provides an
  abstract interface to create/fetch
  a WRATHCanvas::DataHandle according to a 
  WRATHItemDrawState and a custom key.
  The typical case is where the custom
  key represents a transformation node,
  as such one does not retrieve a unique
  WRATHCanvas::DataHandle from a WRATHItemDrawState-
  custom key pair, rather a single 
  WRATHCanvas::DataHandle is expected to be able 
  to handle several distinct custom keys.

  The basic use pattern is to specify
  an attribute store and GL state to fetch/create
  a WRATHCanvas::DataHandle from
  a WRATHCanvas. 

  In order to help minimize buffer object 
  changes, the attribute store is also fetched
  from a WRATHCanvas through
  that it inherits from WRATHAttributeStoreAllocator.

  Attribute data is broken into two sets: implicit
  attribute data and explicit attribute data.
  Explicit attribute data is specified by code
  using a WRATHCanvas. Implicit
  attribute data is set by an implementation
  of a derived class of WRATHCanvas.
  Of critical importance is that for each shader
  expected to be handled by a WRATHCanvas,
  the default value for the implicit attribute value
  makes it so that the vertex is clipped. This
  default value is set in the ctor of a 
  WRATHCanvas (which in truth passes
  that value onto the ctor of a WRATHAttributeStoreAllocator).

  For a pseudo-code example of using a WRATHCanvas please check \ref Group
 */
class WRATHCanvas:public WRATHAttributeStoreAllocator
{
public:

  /*!\typedef signal_t
    Conveniance typedef for the signal fired when the
    WRATHCanvas is set to be phased deleted (see 
    \ref connect_phased_delete()).
   */
  typedef boost::signals2::signal<void ()> signal_t;

  /*!\typedef connect_t
    Conveniance typedef for the connection type
    of the signal for when the WRATHCanvas is 
    set to be phased deleted (see \ref 
    connect_phased_delete() ).
   */
  typedef boost::signals2::connection connect_t;

  /*!\class SubKeyBase
    SubKeyBase is the base class to
    any custom key type. A derived class
    of WRATHCanvas will likely
    possess it's own custom key type,
    that custom key type should be derived
    from SubKeyBase.
   */
  class SubKeyBase:boost::noncopyable
  {
  public:

    virtual
    ~SubKeyBase()
    {}

    /*!\fn SubKeyBase* create_copy
      To be reimplemented by each derived
      class to return a pointer to a copy
      of this object, needed for when
      UI widgets wish to save a copy
      of the SubKeyBase paramater.
     */
    virtual
    SubKeyBase*
    create_copy(void) const=0;
  };

  
  /*!\class CustomDataBase
    WRATHItemGroup 's requested are returned
    in a handle structure that has a pointer
    to the WRATHItemGroup and a pointer to
    a CustomDataBase. The expectation is that
    a derived class of WRATHCanvas
    will derive it's own custom data class from
    CustomDataBase, and the returned handles'
    data pointer will point to such an object.
   */
  class CustomDataBase
  {
  public:

    virtual
    ~CustomDataBase()
    {}

    /*!\fn const SubKeyBase* subkey
      To be implemented by each derived
      class to return a pointer to a const
      SubKeyBase object that is equivalent
      to the SubKeyBase object used to
      fetch the DataHandle that the
      CustomDataBase object is a part.
      I.e.
      \code
      DataHandle g, g2;
      WRATHItemDrawState k;
      const SubKeyBase *sk;

      g=create(k, sk);
      g2=create(k, g.custom_data()->subkey());
      WRATHassert(g2==g);
      //i.e. g.custom_data()->subkey() is _equivalent_
      //to sk (though it does not need to be the same object),
      \endcode
     */
    virtual
    const SubKeyBase*
    subkey(void) const=0;

    /*!\fn void set_implicit_attribute_data
      To be implemented by a derived class to 
      set the implicit attribute data for the specified
      ranges of _elements_ of the memory of the specified
      WRATHBufferObject. It is guaranteed that the memory
      will be allocated for those elements. NOTE! The size
      of each element must be known by the CustomDataBase
      derived object's implementation and be the same size.
      \param R array of ranges, range values in _elements_
      \param implicit_attributesBO buffer object to which 
                                   to write implicit values
     */
    virtual
    void
    set_implicit_attribute_data(const_c_array<range_type<int> > R,
                                WRATHBufferObject *implicit_attributesBO) const=0;
  };

  /*!\class CustomDataBaseT
    Provides some type safety comfort. The assumption
    is that the implicit attribute data has type T
    (and thus its size is sizeof(T))
   */
  template<typename T>
  class CustomDataBaseT:public CustomDataBase
  {
  public:
    /*!\fn void set_implicit_attribute_data(const_c_array< c_array<T> >) const
      To be implemented by a derived class to set implicit
      attribute data. The WRATHBufferObject holding
      the implicit attribute data will be locked and
      the bytes associated to the passed ranges will
      be marked dirty.
      \param R array of array to which to write implicit data
     */
    virtual
    void
    set_implicit_attribute_data(const_c_array< c_array<T> > R) const=0;

    virtual
    void
    set_implicit_attribute_data(const_c_array<range_type<int> > R,
                                WRATHBufferObject *implicit_attributesBO) const
    {
      std::vector< c_array<T> > outR;

      WRATHLockMutex(implicit_attributesBO->mutex());

      for(const_c_array<range_type<int> >::iterator iter=R.begin(),
            end=R.end(); iter!=end; ++iter)
        {
          uint8_t *ptr;
          range_type<int> scaledR(iter->m_begin*sizeof(T),
                                  iter->m_end*sizeof(T));

          
          ptr=implicit_attributesBO->c_ptr(scaledR.m_begin);
          outR.push_back( c_array<T>(reinterpret_cast<T*>(ptr), iter->m_end - iter->m_begin));
          implicit_attributesBO->mark_bytes_dirty_no_lock(scaledR.m_begin, scaledR.m_end);
        }
      set_implicit_attribute_data(outR);
      WRATHUnlockMutex(implicit_attributesBO->mutex());
    }
  };

  /*!\class DataHandle
    A DataHandle is a wrapper over a 
    WRATHItemGroup-CustomDataBase pointer
    pair, this is the type returned on
    fetching/creating draw groups.
    DataHandle has methods to allocate, deallocate
    and manipulating attribute data, these
    methods map to calling the relevant method
    of WRATHAttributeStore. 
   */
  class DataHandle
  {
  public:
    /*!\fn DataHandle(void)
      Default ctor, initializes the DataHandle
      as an invalid draw group.
     */
    DataHandle(void):
      m_item_group(NULL),
      m_custom_data(NULL),
      m_parent(NULL),
      m_implicit_buffer_object(NULL)
    {}

    /*!\fn DataHandle(WRATHItemGroup *gp, const CustomDataBase *dp, WRATHCanvas *pp)
      Ctor used by derived class of 
      WRATHCanvas to create
      return values for fetching/creating
      draw groups.
      \param gp Pointer to actual WRATHItemGroup for which
                the created DataHandle acts as a proxy.
      \param dp pointer to custom data created by the
                WRATHCanvas derived class for the draw group
      \param pp WRATHCanvas that allocated the WRATHItemGroup gp
     */
    DataHandle(WRATHItemGroup *gp, const CustomDataBase *dp,
               WRATHCanvas *pp):
      m_item_group(gp),
      m_custom_data(dp),
      m_parent(pp)
    {
      m_implicit_buffer_object=attribute_store()->implicit_attribute_data(implicit_store());
    }

    /*!\fn bool valid
      Returns true if and only if the 
      DataHandle refers to a WRATHItemGroup
      (rather than NULL).
     */
    bool
    valid(void) const
    {
      return m_item_group!=NULL;
    }

    /*!\fn unsigned int implicit_store
      Returns the index to feed to 
      WRATHAttributeStore::implicit_attribute_data()
      to fetch the buffer object storing the implicit 
      attributes used by this DataHandle.
    */
    unsigned int
    implicit_store(void) const
    {
      WRATHassert(valid());
      return m_item_group->implicit_store();
    }
    
    /*!\fn const WRATHCompiledItemDrawStateCollection& item_draw_state
      Fetches the "draw key" for the items made via
      this DrawHandle, equivalent to
      \code
      item_group()->item_draw_state()
      \endcode
     */
    const WRATHCompiledItemDrawStateCollection&
    item_draw_state(void) const
    {
      return m_item_group->item_draw_state();
    }

    /*!\fn const CustomDataBase* custom_data
      Returns a const-pointer to the custom
      data associated to this DataHandle.
     */
    const CustomDataBase*
    custom_data(void) const
    {
      return m_custom_data;
    }

    /*!\fn WRATHItemGroup* item_group
      Returns the underlying WRATHItemGroup
     */
    WRATHItemGroup*
    item_group(void) const
    {
      return m_item_group;
    }

    /*!\fn const WRATHAttributeStore::handle& attribute_store
      Returns a handle to the \ref WRATHAttributeStore used 
      by the DrawHandle, equivalent to
      \code
      item_group()->attribute_store()
      \endcode
     */
    const WRATHAttributeStore::handle&
    attribute_store(void) const
    {
      WRATHassert(valid());
      return m_item_group->attribute_store();
    }

    /*!\fn const WRATHIndexGroupAllocator::handle& index_store
      Returns a handle to the WRATHIndexGroupAllocator
      used by the DataHandle, equivalent to
      \code
      item_group()->index_store()
      \endcode
     */
    const WRATHIndexGroupAllocator::handle&
    index_store(void) const
    {
      WRATHassert(valid());
      return m_item_group->index_store();
    }

    /*!\fn WRATHCanvas* parent
      Returns the WRATHCanvas
      that returned this DataHandle.
     */
    WRATHCanvas*
    parent(void) const
    {
      return m_parent;
    }

    /*!\fn void release_group
      Releases this DataHandle, after being
      called this DataHandle is not valid.
      Of importance to a user of DataHandle 's
      is that release_group() does NOT deallocate
      attribute or index data allocated through the
      DataHandle, as such a client needs to free
      those attributes and indices themselves.
     */
    void
    release_group(void) 
    {
      if(valid())
        {
          m_parent->release_group(*this);
        }
    }

    /*!\fn bool operator==(const DataHandle &) const
      Comparison equality operator.
      \param obj DataHandle to which to compare.
     */
    bool
    operator==(const DataHandle &obj) const
    {
      return m_item_group==obj.m_item_group
        and m_custom_data==obj.m_custom_data
        and m_implicit_buffer_object==obj.m_implicit_buffer_object;
    }

    /*!\fn bool operator!=(const DataHandle &) const
      Comparison inequality operator.
      \param obj DataHandle to which to compare.
     */
    bool
    operator!=(const DataHandle &obj) const
    {
      return m_item_group!=obj.m_item_group
        or m_custom_data!=obj.m_custom_data
        or m_implicit_buffer_object!=obj.m_implicit_buffer_object;
    }
    
    /*!\fn WRATHMutex& attribute_mutex
      Returns the WRATHMutex used for the attribute data.
      Equivalent to
      \code
      attribute_store()->mutex();
      \endcode
     */
    WRATHMutex&
    attribute_mutex(void) const
    {
      return attribute_store()->mutex();
    }

    /*!\fn void set_implicit_attribute_data(const_c_array<range_type<int> >) const
      Sets the implicit attribute data specified by a range
      of attribute to correspond to this handle's
      \ref custom_data() object.
      \param R array of ranges to set the implicit attributes.
     */
    void
    set_implicit_attribute_data(const_c_array<range_type<int> > R) const;

    /*!\fn void set_implicit_attribute_data(const range_type<int> &) const
      Sets the implicit attribute data specified by a range
      of attribute to correspond to this handle's
      \ref custom_data() object.
      \param R range to set the implicit attributes.
     */
    void
    set_implicit_attribute_data(const range_type<int> &R) const
    {
      const_c_array<range_type<int> > Rs(&R, 1);
      set_implicit_attribute_data(Rs);
    }
   
    /*!\fn int allocate_attribute_data(int) const
      Allocate attribute data in a single block. Also sets 
      the implicit attribute data for the data allocated, see
      \ref WRATHAttributeStore::allocate_attribute_data(int)
      and \ref set_implicit_attribute_data(). Returns -1
      if unable to allocate, otherwise returns the location
      of the first attribute allocated.
      \param number_elements number of _attributes_ to allocate.
     */
    int
    allocate_attribute_data(int number_elements) const;

    /*!\fn enum return_code allocate_attribute_data(int, range_type<int>&) const
      Allocate attribute dataa single block.Also sets 
      the implicit attribute data for the data allocated.
      see \ref WRATHAttributeStore::allocate_attribute_data(int, range_type<int>&)
      and \ref set_implicit_attribute_data().
      \param number_elements number of _attributes_ to allocate.
      \param R (output) upon success writes to R the range of where
                        the attribute data resides
    */
    enum return_code
    allocate_attribute_data(int number_elements, range_type<int> &R) const;

    /*!\fn enum return_code fragmented_allocate_attribute_data
      Allocate attribute data, also sets the implicit
      attribute data for the data allocated.
      see \ref WRATHAttributeStore::fragmented_allocate_attribute_data()
      and \ref set_implicit_attribute_data().
      \param number_elements number of _attributes_ to allocate
      \param out_allocations on allocation sucess, _appends_,
                             the locations of the fragments 
                             of the allocating as a range_type
                             (i.e. marking the beginning and
                             ending of the fragmented allocation).
    */
    enum return_code
    fragmented_allocate_attribute_data(int number_elements,
                                       std::vector< range_type<int> > &out_allocations) const;

    /*!\fn enum return_code proxy_attribute_allocate
      Eqivalent to
      \code
      attribute_store()->proxy_attribute_allocate(number_elements)
      \endcode
      see \ref WRATHAttributeStore::proxy_attribute_allocate()
      \param number_elements allocation query
     */
    enum return_code
    proxy_attribute_allocate(int number_elements) const
    {
      return attribute_store()->proxy_attribute_allocate(number_elements);
    }

    /*!\fn enum return_code proxy_fragmented_allocate_attribute
      Eqivalent to
      \code
      attribute_store()->proxy_fragmented_allocate_attribute(number_elements)
      \endcode
      see \ref WRATHAttributeStore::proxy_fragmented_allocate_attribute()
      \param number_elements allocation query
     */
    enum return_code
    proxy_fragmented_allocate_attribute(int number_elements) const
    {
      return attribute_store()->proxy_fragmented_allocate_attribute(number_elements);
    }

    /*!\fn void deallocate_attribute_data(int begin, int end) const
      Eqivalent to
      \code
      attribute_store()->deallocate_attribute_data(int begin, int end)
      \endcode
      see \ref WRATHAttributeStore::deallocate_attribute_data(int, int),
      i.e. deallocoates a range of attributes.
      \param begin first attribute to free
      \param end one past the last attribute to free
     */
    void
    deallocate_attribute_data(int begin, int end) const
    {
      attribute_store()->deallocate_attribute_data(begin, end);
    }

    /*!\fn void deallocate_attribute_data(range_type<int> R) const
      Equivalent to
      \code
      attribute_store()->deallocate_attribute_data(R)
      \endcode
      see \ref WRATHAttributeStore::deallocate_attribute_data(range_type<int>)
      \param R range of attributes to free
     */
    void
    deallocate_attribute_data(range_type<int> R) const
    {
      attribute_store()->deallocate_attribute_data(R);
    }

    /*!\fn void deallocate_attribute_datas
      Equivalent to
      \code
      attribute_store()->deallocate_attribute_datas(begin, end)
      \endcode
      see \ref WRATHAttributeStore::deallocate_attribute_datas()
      \tparam iterator to range_type<int>
      \param begin iterator to 1st range 
      \param end iterator to one past last range
     */
    template<typename iterator>
    void
    deallocate_attribute_datas(iterator begin, iterator end) const
    {
      attribute_store()->deallocate_attribute_datas(begin, end);
    }

    /*!\fn int max_fragmented_allocate_possible
      Eqivalent to
      \code
      attribute_store()->(max_fragmented_allocate_possible)
      \endcode
      see WRATHAttributeStore::max_fragmented_allocate_possible()
     */
    int
    max_fragmented_allocate_possible(void) const
    {
      return attribute_store()->max_fragmented_allocate_possible();
    }
    
    /*!\fn int max_cts_allocate_possible
      Eqivalent to
      \code
      attribute_store()->max_cts_allocate_possible()
      \endcode
      see WRATHAttributeStore::max_cts_allocate_possible()
     */
    int
    max_cts_allocate_possible(void) const
    {
      return attribute_store()->max_cts_allocate_possible();
    }

    /*!\fn int attributes_allocated
      Eqivalent to
      \code
      attribute_store()->attributes_allocated()
      \endcode
      see WRATHAttributeStore::attributes_allocated()
     */
    int
    attributes_allocated(void) const
    {
      return attribute_store()->attributes_allocated();
    }

    /*!\fn c_array<T> pointer(int, int) const
      Equivalent to
      \code
      attribute_store()->pointer<T>(first_element, number_elements)
      \endcode
      see WRATHAttributeStore::pointer<T>(int, int)
      \param first_element attribute index that is mapped to return values [0]
      \param number_elements number of elements that returned value maps to
     */
    template<typename T>
    c_array<T>
    pointer(int first_element, int number_elements) const
    {
      return attribute_store()->pointer<T>(first_element, number_elements);
    }

    /*!\fn c_array<T> pointer(range_type<int>) const
      Equivalent to
      \code
      attribute_store()->pointer<T>(range_type<int>)
      \endcode
      see WRATHAttributeStore::pointer<T>(range_type<int>)
      \param R range of elements that return type maps to
     */
    template<typename T>
    c_array<T>
    pointer(range_type<int> R) const
    {
      return attribute_store()->pointer<T>(R);
    }

    /*!\fn const_c_array<T> read_pointer(int, int) const
      Equivalent to
      \code
      attribute_store()->read_pointer<T>(first_element, number_elements)
      \endcode
      see WRATHAttributeStore::read_pointer<T>(int, int) const
      \param first_element attribute index that is mapped to return values [0]
      \param number_elements number of elements that returned value maps to
     */
    template<typename T>
    const_c_array<T>
    read_pointer(int first_element, int number_elements) const
    {
      return attribute_store()->read_pointer<T>(first_element, number_elements);
    }

    /*!\fn const_c_array<T> read_pointer(range_type<int>) const
      Eqivalent to
      \code
      attribute_store()->read_pointer<T>(R)
      \endcode
      see WRATHAttributeStore::read_pointer<T>(range_type<int>) const
      \param R range of elements that return type maps to
     */
    template<typename T>
    const_c_array<T>
    read_pointer(range_type<int> R) const
    {
      return attribute_store()->read_pointer<T>(R);
    }

    /*!\fn WRATHIndexGroupAllocator::index_group<I> allocate_index_group
      Allocate an index group from the WRATHIndexGroupAllocator
      \ref index_store()
      Equivalent to
      \code
      index_store()->allocate_index_group<I>(number_elements);
      \endcode
      \tparam I index type
      \param number_elements number of indices to allocate.
     */
    template<typename I>
    WRATHIndexGroupAllocator::index_group<I>
    allocate_index_group(int number_elements)
    {
      WRATHassert(valid());
      return index_store()->allocate_index_group<I>(number_elements);
    }

    /*!\fn WRATHIndexGroupAllocator::index_group<I> allocate_copy_index_group
      Creates a new index group whose parameters and index
      data are copied from a source index_group, see
      WRATHIndexGroupAllocator::allocate_copy_index_group<I>(index_group<I>).
      Equivalent to
      \code
      index_store()->allocate_copy_index_group<I>(h);
      \endcode
      \tparam I index type
      \param h handle to an index group from which to copy
     */
    template<typename I>
    WRATHIndexGroupAllocator::index_group<I>
    allocate_copy_index_group(WRATHIndexGroupAllocator::index_group<I> h)
    {
      WRATHassert(valid());
      return index_store()->allocate_copy_index_group<I>(h);
    }

  private:
    WRATHItemGroup *m_item_group;
    const CustomDataBase *m_custom_data;
    WRATHCanvas *m_parent;
    WRATHBufferObject *m_implicit_buffer_object;
  };

  /*!\fn WRATHCanvas(const WRATHTripleBufferEnabler::handle &r,
                       const std::vector<opengl_trait_value> &pimplicit_attribute_format,
                       const std::vector<uint8_t> &pvalue_at_index0)
  
    Ctor.
     \param r handle to a WRATHTripleBufferEnabler to
              which the the users of the created object will
              sync. In particular it will update
              it's drawing list data for the rendering
              thread according to the signaling
              of ptriple_buffer_enabler. It is an
              error if the handle is not valid.
     \param pimplicit_attribute_format each WRATHAttributeStore allocated by this
                                       WRATHAttributeStoreAllocator will also hold
                                       "implicit" attribute data that is used by
                                       a WRATHCanvas to specify those attribute values 
                                       that are determined by what node (i.e transformation, 
                                       visbility, etc) a drawn element is on.
     \param pvalue_at_index0 raw bytes to use for the implicit attribute value that
                             guarantees that the vertex will be clipped.
   */
  WRATHCanvas(const WRATHTripleBufferEnabler::handle &r,
                           const std::vector<opengl_trait_value> &pimplicit_attribute_format,
                           const std::vector<uint8_t> &pvalue_at_index0):
    WRATHAttributeStoreAllocator(r, pimplicit_attribute_format, pvalue_at_index0)
  {}


  /*!\fn WRATHCanvas(const WRATHTripleBufferEnabler::handle &r, type_tag<T>, const T &pvalue_at_index0=T())
    Ctor. Template friendly version for ctor. The type T is used as the 
    type for the implicit attributes type. The type T must
    provide:
    - an enumeration T::number_attributes indicating how many attributes the type T uses
    - a function T::attribute_key(vecN<opengl_trait_value, T::number_attributes>&) which for each attribute of T, "computes" the \ref opengl_trait_value correctly.
    Note that \ref WRATHInterleavedAttributes provides these features.
    \param r handle to a WRATHTripleBufferEnabler to
             which the users of the created object will
             sync. In particular it will update
             it's drawing list data for the rendering
             thread according to the signaling
             of ptriple_buffer_enabler. It is an
             error if the handle is not valid.
    \param pvalue_at_index0 the value to use for implicit attribute at index 0  that
                            guarantees that the vertex will be clipped.
   */
  template<typename T>
  WRATHCanvas(const WRATHTripleBufferEnabler::handle &r,
              type_tag<T>, const T &pvalue_at_index0=T()):
    WRATHAttributeStoreAllocator(r, type_tag<T>(), pvalue_at_index0)
  {}

  virtual
  ~WRATHCanvas()
  {}

  /*!\fn connect_t connect_phased_delete
    Connect to the signal fired when WRATHCanvas
    is set to be phased deleted (i.e. WRATHPhasedDelete is
    called on the WRATHCanvas).
    \param subscriber slot called on signal fire
    \param gp_order order of slot call. Lower values of gp_order
                    are guarnteed to be call those of higher values
                    of gp_order. Slots connected with the same
                    value of gp_order are called in a non-deterministic
                    order (i.e. order of calling connect_dtor does
                    not imply any order about the order of being called).
   */
  connect_t
  connect_phased_delete(const signal_t::slot_type &subscriber, int gp_order=0)
  {
    return m_phased_delete_signal.connect(gp_order, subscriber);
  }

  /*!\fn bool accepts_subkey
    To be implemented by a derived class
    to return true if the derived class can
    use the passed SubKeyBase derived object
    in it's imlementation of create().
    \param psubkey SubKeyBase derived object to query.
   */
  virtual
  bool
  accepts_subkey(const SubKeyBase &psubkey)=0;


  /*!\fn DataHandle create(const WRATHAttributeStore::handle&,
                           const WRATHCompiledItemDrawStateCollection&,
                           const SubKeyBase&, unsigned int)

    Interface to fetch or create a DataHandle according
    to a WRATHCompiledItemDrawStateCollection and user
    defined data in a SubKeyBase.
    Any returned DataHandle _must_ be released
    by release_group() BEFORE the creating
    WRATHCanvas object is deleted
    with WRATHPhasedDelete.

    \param attrib_store handle to attribute store to use
                        to allocate attributes
    \param item_draw_state item draw state specifying how to 
                           process indices and attribute allocated
                           through the returned DrawHandle             
    \param psubkey Custom criteria made by an implementation of
                   WRATHCanvas for creation/selection 
    \param implicit_store Specifies what value to pass to
                          WRATHAttributeStore::implicit_attribute_data()
                          where the implicit attribute data is to
                          be stored.
   */
  DataHandle
  create(const WRATHAttributeStore::handle &attrib_store,
         const WRATHCompiledItemDrawStateCollection &item_draw_state,
         const SubKeyBase &psubkey, unsigned int implicit_store=0)
  {
    return create_implement(attrib_store, item_draw_state, psubkey, implicit_store);
  }

 

  /*!\fn DataHandle create_and_allocate(const WRATHAttributeStoreKey &,
                                        int ,
                                        range_type<int> &,
                                        const WRATHCompiledItemDrawStateCollection &,
                                        const SubKeyBase &, 
                                        unsigned int )

    Convenience function to a allocate attributes
    in one block and fetch DataHandle via create()
    specifying a WRATHAttributeStoreKey and an 
    allocation requirement. Attributes allocated
    via create_and_allocate() will also have
    the implicit attribute values set via
    DataHandle::set_implicit_attributes().
    Function is equivalent to:
    \code
    WRATHAttributeStore::handle A;
    DataHandle G;
    A=attribute_store(k, req_number_elements, R);
    G=create(A, item_draw_state, psubkey);
    G.set_implicit_attribute_data(R);
    return G;
    \endcode

    \param k attribute key specifying attribute format
    \param req_number_elements number of attribute to allocate in once continuous block
    \param R (output) where the attributes are located in the attribute store are written to in R
    \param item_draw_state item draw state specifying how to 
                           process indices and attribute allocated
                           through the returned DrawHandle              
    \param psubkey Custom criteria made by an implementation of
                   WRATHCanvas for creation/selection 
    \param implicit_store Specifies what value to pass to
                          WRATHAttributeStore::implicit_attribute_data()
                          where the implicit attribute data is to
                          be stored.
   */
  DataHandle
  create_and_allocate(const WRATHAttributeStoreKey &k,
                      int req_number_elements,
                      range_type<int> &R,
                      const WRATHCompiledItemDrawStateCollection &item_draw_state,
                      const SubKeyBase &psubkey, 
                      unsigned int implicit_store=0);

  
  /*!\fn DataHandle create_and_allocate(const WRATHAttributeStoreKey&,
                                        int,
                                        std::vector<range_type<int> >&,
                                        const WRATHCompiledItemDrawStateCollection&,
                                        const SubKeyBase &,
                                        unsigned int)  
    Convenience function to allocate attributes
    in multiple blocks and fetch DataHandle via create()
    specifying a WRATHAttributeStoreKey and an 
    allocation requirement. Attributes allocated
    via create_and_allocate() will also have
    the implicit attribute values set via
    DataHandle::set_implicit_attributes().
    Function is equivalent to:
    \code
    WRATHAttributeStore::handle A;
    DataHandle G;
    A=attribute_store(k, req_number_elements, R);
    G=create(A, item_draw_state, psubkey);
    G.set_implicit_attribute_data(R);
    return G;
    \endcode

    \param k attribute key specifying attribute format
    \param req_number_elements number of attribute to allocate in multiple blocks
    \param R (output) where the attributes are located in the attribute store 
                      are written to in R, the array is cleared before it is written to.
    \param item_draw_state item draw state specifying how to 
                           process indices and attribute allocated
                           through the returned DrawHandle              
    \param psubkey Custom criteria made by an implementation of
                   WRATHCanvas for creation/selection 
    \param implicit_store Specifies what value to pass to
                          WRATHAttributeStore::implicit_attribute_data()
                          where the implicit attribute data is to
                          be stored.
   */
  DataHandle
  create_and_allocate(const WRATHAttributeStoreKey &k,
                      int req_number_elements,
                      std::vector<range_type<int> > &R,
                      const WRATHCompiledItemDrawStateCollection &item_draw_state,
                      const SubKeyBase &psubkey, unsigned int implicit_store=0);
 
 
  
  /*!\fn void add_raw_draw_command
    To be implemented by a derived class to add a 
    WRATHRawDrawDataElement to be drawn at the
    indicated pass. To remove the WRATHRawDrawDataElement,
    one can use WRATHRawDrawData::remove_element() using
    WRATHRawDrawDataElement::raw_draw_data().
    \param pass WRATHDrawType indicating what pass
                to draw the raw draw data element. The
                interpretation of the pass is determined
                by the implementation of WRATHCanvas
    \param b the WRATHRawDrawDataElement to add, the
             object pointed to by b must stay in scope
             until it is removed via WRATHRawDrawData::remove_element().
   */
  virtual
  void
  add_raw_draw_command(WRATHDrawType pass,
                       WRATHRawDrawDataElement *b)=0;

  /*!\fn enum return_code transfer(DataHandle&)
    Transfers a DataHandle from a given DataHandle
    which resides on a different WRATHCanvas
    to this WRATHCanvas. A user still
    needs to remember to move the index
    buffers and set the implicit attribute
    data. Additionally, the user needs to guarantee
    that the WRATHCanvas types of the source
    and destination WRATHCanvas objects are 
    sufficiently compatible. Returns \ref routine_fail
    on failure and \ref routine_success on success.
    \param in_group DataHandle object to change
   */
  enum return_code
  transfer(DataHandle &in_group);

  /*!\fn enum return_code transfer(DataHandle&, const_c_array< range_type<int> > allocations)
    Transfers a DataHandle from a given DataHandle
    which resides on a different WRATHCanvas
    to this WRATHCanvas. In addition, sets the
    implicit attribute data of specified
    blocks of the attribute store of the
    DataHandle.
    \param in_group DataHandle object to change
    \param allocations array of attribute ranges to 
                       which to set the implicit attribute
                       data
   */
  enum return_code
  transfer(DataHandle &in_group,
           const_c_array< range_type<int> > allocations);

  /*!\fn enum return_code transfer(DataHandle&, range_type<int>)
    Transfers a DataHandle from a given DataHandle
    which resides on a different WRATHCanvas
    to this WRATHCanvas. In addition, sets the
    implicit attribute data of a specified
    block of the attribute store of the
    DataHandle.
    \param in_group DataHandle object to change
    \param R attribute range to which to set 
             the implicit attribute data
   */
  enum return_code
  transfer(DataHandle &in_group,
           range_type<int> R)
  {
    return transfer(in_group,
                    const_c_array< range_type<int> >(&R, 1));
  }

  /*!\fn enum return_code transfer(DataHandle& in_group, 
                  const_c_array< range_type<int> > allocations, 
                  WRATHIndexGroupAllocator::index_group<I> &h)
    Transfers a DataHandle from a given DataHandle
    which resides on a different WRATHCanvas
    to this WRATHCanvas. In addition, sets the
    implicit attribute data of specified
    blocks of the attribute store of the
    DataHandle and moves the index data of
    an index group.
    \param in_group DataHandle object to change
    \param allocations array of attribute ranges to 
                       which to set the implicit attribute
                       data
    \param h handle to an index group, values are copied
             to a new index group (of this WRATHCanvas),
             the old handle is released and h is set to
             the new index group.
   */
  template<typename I>
  enum return_code
  transfer(DataHandle &in_group,
           const_c_array< range_type<int> > allocations,
           WRATHIndexGroupAllocator::index_group<I> &h)
  {
    enum return_code R;

    if(in_group.parent()==this)
      {
        return routine_success;
      }
    R=transfer(in_group, allocations);

    if(R==routine_success and h.valid())
      {
        WRATHIndexGroupAllocator::index_group<I> newH;

        newH=in_group.allocate_copy_index_group<I>(h);
        h.delete_group();
        h=newH;
      }
    return R;
  }

  /*!\fn enum return_code transfer(DataHandle&, range_type<int>,
                                   WRATHIndexGroupAllocator::index_group<I>&)
    Transfers a DataHandle from a given DataHandle
    which resides on a different WRATHCanvas
    to this WRATHCanvas. In addition, sets the
    implicit attribute data of a specified
    block of the attribute store of the
    DataHandle and moves the index data of
    an index group.
    \param in_group DataHandle object to change
    \param R attribute range to which to set 
             the implicit attribute data
    \param h handle to an index group, values are copied
             to a new index group (of this WRATHCanvas),
             the old handle is released and h is set to
             the new index group.
   */
  template<typename I>
  enum return_code
  transfer(DataHandle &in_group,
           range_type<int> R,
           WRATHIndexGroupAllocator::index_group<I> &h)
  {
    return transfer(in_group,
                    const_c_array< range_type<int> >(&R, 1),
                    h);
  }

  /*!\fn void release_group(DataHandle&)
    To be implemented by a derived class
    to release a DataHandle. The underlying
    WRATHItemGroup may or may not be deallocated
    by this call, but the expectation is that
    for each call to create(), then there 
    should be call to release_group().
    Of importance to a user of DataHandle 's
    is that release_group() does NOT deallocate
    attribute or index data allocated through the
    DataHandle, as such a client needs to free
    those attributes and indices themselves.
    \param g DataHandle returned by create() to release.
   */
  virtual
  void
  release_group(DataHandle &g)=0; 

protected:

  /*!\fn  create_implement(const WRATHAttributeStore::handle&,
                          const WRATHCompiledItemDrawStateCollection&,
                          const SubKeyBase&, unsigned int)
    To be implemented by a derived class
    to fetch or create a DataHandle according
    to a WRATHCompiledItemDrawStateCollection 
    and user defined data in a SubKeyBase.

    \param attrib_store handle to attribute store to use
                        to allocate attributes
    \param item_draw_state item draw state specifying how to 
                           process indices and attribute allocated
                           through the returned DrawHandle             
    \param psubkey Custom criteria made by an implementation of
                   WRATHCanvas for creation/selection 
    \param implicit_store Specifies what value to pass to
                          WRATHAttributeStore::implicit_attribute_data()
                          where the implicit attribute data is to
                          be stored.
   */
  virtual
  DataHandle
  create_implement(const WRATHAttributeStore::handle &attrib_store,
                   const WRATHCompiledItemDrawStateCollection &item_draw_state,
                   const SubKeyBase &psubkey, unsigned int implicit_store)=0;


 
  virtual
  void
  on_place_on_deletion_list(void)
  {
    m_phased_delete_signal();
    WRATHAttributeStoreAllocator::on_place_on_deletion_list();
  }


private:
  signal_t m_phased_delete_signal;

};

/*! @} */

#endif
