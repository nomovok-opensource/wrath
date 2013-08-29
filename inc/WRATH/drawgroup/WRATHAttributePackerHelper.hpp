/*! 
 * \file WRATHAttributePackerHelper.hpp
 * \brief file WRATHAttributePackerHelper.hpp
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




#ifndef __WRATH_ATTRIBUTE_PACKER_HELPER_HPP__
#define __WRATH_ATTRIBUTE_PACKER_HELPER_HPP__

#include "WRATHConfig.hpp"
#include <vector>
#include <algorithm>
#include "WRATHItemGroup.hpp"
#include "c_array.hpp"
#include "WRATHAbstractDataSink.hpp"


/*! \addtogroup Group
 * @{
 */

/*!\class WRATHDefaultAttributeWriter
  A WRATHDefaultAttributeWriter implement the
  attribute_writer requirements of the template
  class \ref WRATHGenericAttributePackerHelper.
  It is for when attribute type and size are
  known at compile time. It implements \ref set()
  directly with \ref WRATHAbstractDataSink::pointer()
  and writing to the attribute store via the 
  attribute_type::operator=.
  \tparam attribute_type attribute type
*/
template<typename attribute_type>
class WRATHDefaultAttributeWriter
{
public:
  /*!\class initialize_args
    Ctor parameter type is empty.
   */
  class initialize_args
  {};
  
  /*!\fn WRATHDefaultAttributeWriter(initialize_args)
    Default (empty) ctor.
   */
  WRATHDefaultAttributeWriter(initialize_args)
  {}
  
  /*!\fn void set(WRATHAbstractDataSink &, const range_type<int> &)
    Set the location to which to write attribute data
    \param attribute_store WRATHAbstractDataSink to which to write data
    \param R range within attribute_store to which to write data
   */
  void
  set(WRATHAbstractDataSink &attribute_store,
      const range_type<int> &R)
  {
    m_ptr=attribute_store.pointer<attribute_type>(R);
  }
  
  /*!\fn void write_value(int, const attribute_type&)
    Write an attribute value to a location
    \param v value to write
    \param I index relative to the range passed in set() to which to write the value
   */
  void
  write_value(int I, const attribute_type &v)
  {
    m_ptr[I]=v;
  }
  
private:
  c_array<attribute_type> m_ptr;
};


/*!\class WRATHDefaultIndexWriter
  A WRATHDefaultIndexWriter implements the
  index_write interface for 
  \ref WRATHGenericAttributePackerHelper
  to write indices to a \ref c_array
  \tparam index_type index type
 */
template<typename index_type>
class WRATHDefaultIndexWriter
{
public:

  /*!\typedef initialize_args
    Typedef for argument type to ctor.
   */
  typedef c_array<index_type> initialize_args;

  /*!\fn WRATHDefaultIndexWriter(initialize_args)
    Ctor.
    \param dest array on indices to which to write.
   */
  WRATHDefaultIndexWriter(initialize_args dest):
    m_index_ptr(dest),
    m_current_index_loc(0)
  {}

  /*!\fn enum return_code add_index(index_type)
    Add an index (implemented as writing the value
    to an internal location of the array passed in ctor 
    and then incrementing the internal location value).
   */
  enum return_code
  add_index(index_type I)
  {
    if(m_current_index_loc<m_index_ptr.size())
      {
        m_index_ptr[m_current_index_loc]=I;
        ++m_current_index_loc;
        return routine_success;
      }
    else
      {
        return routine_fail;
      }
  }

private:
  c_array<index_type> m_index_ptr;
  unsigned int m_current_index_loc;
};


/*!\class WRATHGenericAttributePackerHelper
  A WRATHGenericAttributePackerHelper purpose is to 
  facilitate in an easier fashion packing attribute 
  data across multiple blocks (i.e. as returned by 
  \ref WRATHAttributeStore::fragmented_allocate_attribute_data()).
  It's state consists of
  - a \ref WRATHAbstractDataSink to which to write attribute data,
  - an std::vector which stores the translation from input indices
    to indices to refer to data written to the WRATHAbstractDataSink
  
  
  The first class type is attribute_writer which
  specifies where and how to write attribute data.
  That template argument class must implement:
  - set(WRATHAttributeDataSink &h, range_type<int> R) 
    which sets the attribute_walker to write to the attributes
    in the passed range of the passed WRATHAttributeDataSink.
  - write_value(int I, const attribute_type &v)  which is
    to write the value v in the WRATHAbstractDataSink h at 
    location I+R.m_begin, where R and h were set last time in set()
  - has the member class initialize_args which is passed
    to the ctor of attribute_writer to initialize it

  The second class type is a index_writer which
  specifies where and how to write index data.
  That class must satsify:
  - implements the method add_index(index_type) which adds
    the index to list of indices to which to write
    (for example WRATHDefaultIndexWriter writes to
    the c_array and increments a counter in it's implementation).
  - has the member class initialize_args which is passed
    to the ctor of index writer to initialize it
 */
template<typename attribute_writer, typename index_type>
class WRATHGenericAttributePackerHelper
{
public:
  /*!\fn WRATHGenericAttributePackerHelper
    Ctor.
    \param attribute_store \ref WRATHAbstractDataSink to which to write attribute data
    \param pranges_begin iterator to 1st block range (begin and end) 
                         to which to write attribute data
    \param pranges_end iterator to 1 past the last block range 
                       to which to write attribute data
    \param args initialization arguments for the attribute walker
   */
  WRATHGenericAttributePackerHelper(WRATHAbstractDataSink &attribute_store,
                                    std::vector<range_type<int> >::const_iterator pranges_begin,
                                    std::vector<range_type<int> >::const_iterator pranges_end,
                                    typename attribute_writer::initialize_args args):
    m_attribute_store(attribute_store),
    m_ranges_iter(pranges_begin),
    m_ranges_end(pranges_end),
    m_current_element_in_attr_ptr(0),
    m_attr_ptr(args),
    m_current_attribute_count(0)
  {
    if(m_ranges_iter!=m_ranges_end)
      {
        m_attr_ptr.set(m_attribute_store, *m_ranges_iter);
      }
  }

  /*!\fn enum return_code add_data(unsigned int,
                                   attribute_iterator,
                                   attribute_iterator,
                                   index_iterator,
                                   index_iterator,
                                   index_writer&)
 
    Add a set of attributes and indices. Indices are remapped
    to the location to which the attributes in the blocks.
    It is assumed that the index for the attribute named
    by *(begin_attribute+I) is I.

    Routine returns \ref routine_success if all the attributes
    and indices were able to fit with in the blocks and
    index array that this WRATHAttributePackerHelper is 
    set to use. If not all the attributes were able
    to fit, will return \ref routine_fail and the index value
    used for indices referring to attributes that failed
    to fit is set as 0.

    The type attribute_iterator must provide:
    - operator*(void) that returns a type which can be passed as the 2nd argument to attribute_walker::set
    - operator++(void) i.e. pre-increment operator
    - equality operator to another attribute_iterator
    - inequality operator to another attribute_iterator

    The type index_iterator must provide:
    - operator*(void) that returns a type to which index_type may be assigned
    - operator++(void) i.e. pre-increment operator
    - equality operator to another index_iterator
    - inequality operator to another index_iterator

    \param num_attributes number of attributes. This is passed
                          directly for the cases where the iterator  
                          type attribute_iterator does not conform
                          to STL iterators and as such std::distance
                          may not work with attribute_iterator
    \param begin_attribute iterator to 1st attribute to add
    \param end_attribute iterator to 1 past the last attribute to add
    \param begin_indices iterator to 1st index to add
    \param end_indices iterator to 1 past the last index to add
    \param index_destination index destination to which to append indices
   */
  template<typename attribute_iterator, typename index_iterator, typename index_writer>
  enum return_code
  add_data(unsigned int num_attributes,
           attribute_iterator begin_attribute,
           attribute_iterator end_attribute,
           index_iterator begin_indices,
           index_iterator end_indices,
           index_writer &index_destination)
  {
    enum return_code RA, RI;

    RA=set_attribute_src(num_attributes, begin_attribute, end_attribute);
    RI=add_indices(begin_indices, end_indices, index_destination);

    return ( RA==routine_success and RI==routine_success)?
      routine_success:
      routine_fail;
  }

  /*!\fn enum return_code add_data(attribute_iterator,
                                   attribute_iterator,
                                   index_iterator,
                                   index_iterator,
                                   index_writer&)
  
    Add a set of attributes and indices. Indices are remapped
    to the location to which the attributes in the blocks.
    It is assumed that the index for the attribute named
    by *(begin_attribute+I) is I. Equivalent to
    \code
    add_data(std::distance(begin_attribute, end_attribute),
             begin_attribute, end_attribute,
             begin_indices, end_indices, index_destination);
    \endcode
    As std::distance is used, some implementations of std::distance
    will require that attribute_iterator is an STL conformant
    iterator type (for example GNU C++ STL under debug builds
    with _GLIBCXX_DEBUG defined).

    Routine returns \ref routine_success if all the attributes
    and indices were able to fit with in the blocks and
    index array that this WRATHAttributePackerHelper is 
    set to use. If not all the attributes were able
    to fit, will return \ref routine_fail and the index value
    used for indices referring to attributes that failed
    to fit is set as 0.

    \param begin_attribute iterator to 1st attribute to add
    \param end_attribute iterator to 1 past the last attribute to add
    \param begin_indices iterator to 1st index to add
    \param end_indices iterator to 1 past the last index to add
    \param index_destination index destination to which to append indices
   */
  template<typename attribute_iterator, typename index_iterator, typename index_writer>
  enum return_code
  add_data(attribute_iterator begin_attribute,
           attribute_iterator end_attribute,
           index_iterator begin_indices,
           index_iterator end_indices,
           index_writer &index_destination)
  {
    return add_data(std::distance(begin_attribute, end_attribute),
                    begin_attribute, end_attribute,
                    begin_indices, end_indices, index_destination);
  }

  /*!\fn enum return_code set_attribute_src
    In the event that one's index data for
    a fixed iterator range of attribute data is 
    spread across multiple different iterator ranges,
    one can set the attribute source and then add the
    indices one range at a time as follows:
    \code
     set_attribute_src(number_attributes, begin_attribute, end_attribute);
     for(each index_range I)
       {
         add_indices(I.begin(), I.end(), index_writer)
       }
    \endcode

    The function set_attribute_src() sets the attribute "array"
    as indicated by the iterator range [begin_attribute, end_attribute).
    Until set_attributes_src() is called again, all indices added
    with add_indices() are viewed as indices into the iterator
    range [begin_attribute, end_attribute), i.e. index I
    means te attibute at *(begin_attribute+I).

    Routine returns \ref routine_success if all the attributes
    were able to fit with in the blocks that this WRATHAttributePackerHelper 
    is set to use. If not all the attributes were able
    to fit, will return \ref routine_fail, but will fit all
    those attributes that could be fit into the blocks.

    The type attribute_iterator must provide:
    - operator*(void) that returns a type to which can be passed as the 2nd argument to attribute_walker::set
    - operator++(void) i.e. pre-increment operator
    - equality operator to another attribute_iterator
    - inequality operator to another attribute_iterator

    \param num_attributes number of attributes. This is passed
                          directly for the cases where the iterator  
                          type attribute_iterator does not conform
                          to STL iterators and as such std::distance
                          may not work with attribute_iterator
    \param begin_attribute iterator to 1st attribute to add
    \param end_attribute iterator to 1 past the last attribute to add
   */
  template<typename attribute_iterator>
  enum return_code
  set_attribute_src(unsigned int num_attributes,
                    attribute_iterator begin_attribute,
                    attribute_iterator end_attribute)
  {
    clear_attribute_src();
    return add_attribute_data(num_attributes, begin_attribute, end_attribute);
  }

  /*!\fn void clear_attribute_src
    Provided as a conveniance, equivalent to
    \code
    set_attribute_src(0, end, end)    
    \endcode
    where end is an attribute_iterator. 
   */
  void
  clear_attribute_src(void)
  {
    m_current_attribute_count=0;
    m_index_remapper.clear();
  }

  /*!\fn enum return_code add_attribute_data
    In the event that ones attribute data is spread across
    multiple blocks, one can use this function to add
    attribute data. 

    Let S=index_remapper().size(), then add_attribute_data()
    does as follows:

     -Let I be an index with S<= I < S+number_attributes,
      then that index I refers to the attribute found at
      *(begin_attribute + I - S).
     - The size of index_remapper() is increased by number_attributes.

    Note that a user needs to carefully remap ones indices
    using S=index_remapper().size().
   */
  template<typename attribute_iterator>
  enum return_code
  add_attribute_data(unsigned int number_attributes,
                     attribute_iterator begin_attribute,
                     attribute_iterator end_attribute)
  {
    unsigned int old_size(m_index_remapper.size());
    
    m_index_remapper.resize(number_attributes + old_size);
    for(;begin_attribute!=end_attribute and m_ranges_iter!=m_ranges_end; ++begin_attribute, 
          ++m_current_element_in_attr_ptr, ++m_current_attribute_count)
      {
        if(m_current_element_in_attr_ptr+m_ranges_iter->m_begin==m_ranges_iter->m_end)
          {
            ++m_ranges_iter;
            m_current_element_in_attr_ptr=0;
            if(m_ranges_iter!=m_ranges_end)
              {
                m_attr_ptr.set(m_attribute_store, *m_ranges_iter);
              }
          }

        if(m_ranges_iter!=m_ranges_end)
          {
            m_attr_ptr.write_value(m_current_element_in_attr_ptr, *begin_attribute);
            m_index_remapper[m_current_attribute_count]=m_current_element_in_attr_ptr+m_ranges_iter->m_begin;
          }
      }

    return (begin_attribute==end_attribute)?
      routine_success:
      routine_fail;
  }


  /*!\fn enum return_code add_indices
    Add index data, the index data is assumed to index
    into the attribute range last set from the call
    add_attribute_src(). If indices are encountered
    that are beyond the attribute src range last set,
    will return routine_fail and remap those indices
    to 0. Those indices that are within the last attribute
    src range set are remapped into the the blocks set
    at this WRATHAttributePackerHelper ctor.

    The type index_iterator must provide:
    - operator*(void) that returns a type to which index_type may be assigned
    - operator++(void) i.e. pre-increment operator
    - equality operator to another index_iterator
    - inequality operator to another index_iterator

    \param begin_indices iterator to 1st index to add
    \param end_indices iterator to 1 past the last index to add
    \param index_destination index destination to which to append indices    
   */
  template<typename index_iterator, typename index_writer>
  enum return_code
  add_indices(index_iterator begin_indices,
              index_iterator end_indices,
              index_writer &index_destination)
  {
    enum return_code return_value(routine_success);

    for(;begin_indices!=end_indices and return_value==routine_success; ++begin_indices)
      {
        index_type I;
        
        I=*begin_indices;

        if(I<static_cast<unsigned int>(m_index_remapper.size()))
          {
            return_value=index_destination.add_index(m_index_remapper[I]);
          }
        else
          {
            return_value=index_destination.add_index(index_type(0));
          }
      }

    return (begin_indices==end_indices)?
      routine_success:
      routine_fail;
  }
  
  /*!\fn const std::vector<index_type>& index_remapper
    Returns the array that coverts from input indices
    to indices where attributes are actually located.
   */
  const std::vector<index_type>&
  index_remapper(void) const
  {
    return m_index_remapper;
  }

  /*!\fn WRATHAbstractDataSink& attribute_store
    Returns the sink to which attributes are written.
   */
  WRATHAbstractDataSink&
  attribute_store(void)
  {
    return m_attribute_store;
  }

private:
  WRATHAbstractDataSink &m_attribute_store;
  std::vector<range_type<int> >::const_iterator m_ranges_iter, m_ranges_end;

  int m_current_element_in_attr_ptr;
  attribute_writer m_attr_ptr;

  int m_current_attribute_count;
  std::vector<index_type> m_index_remapper;
};
 




/*!\class WRATHAttributePackerHelper
  A WRATHAttributePackerHelper covers the main
  case of a \ref WRATHGenericAttributePackerHelper,
  where the attribute type and size are known at
  compile time.
  The most common uses case is to pack attribute
  data stored in containters (or container proxies/
  facades) into a fragmented region of a
  WRATHAbstractDataSink.
  \code
  
  // attributes and indices hold attribute and
  // index data, with indices holding indices into
  // the array attributes
  std::vector<attribute_type> attributes;
  std::vector<index_type> indices;

  // we wish to pack the attribute and index
  // data into multiple blocks an attribute store 
  std::vector<range_type<int> > blocks;
  WRATHAttributeStore::handle attribute_store;
  WRATHIndexGroupAllocator::index_group<index_type> index_group;

  attribute_store->fragmented_allocate_attribute_data(attributes.size(), blocks);
  index_group=some_item_group.allocate_index_group<index_type>(indices.size());
  
  WRATHLockMutex(attribute_store->mutex());
  WRATHLockMutex(index_group.mutex());

  WRATHAttributePackerHelper<attribute_type, index_type> H(attribute_store.data_sink(), 
                                                           blocks.begin(), blocks.end());
                                 
  H.add_data(attributes.size(),
             attributes.begin(), attributes.end(),
             indices.begin(), indices.end());

  WRATHUnlockMutex(attribute_store->mutex());
  WRATHUnlockMutex(index_group.mutex());

  //now the data of attributes and indices is repacked into
  //attribute_store at blocks and the indices are remapped
  //to their location

  \endcode
 */
template<typename attribute_type, typename index_type>
class WRATHAttributePackerHelper:
  public WRATHGenericAttributePackerHelper< WRATHDefaultAttributeWriter<attribute_type>, index_type>
{
public:
  /*!\typedef BaseClass
    Conveniance local typedef.
   */
  typedef WRATHGenericAttributePackerHelper< WRATHDefaultAttributeWriter<attribute_type>, index_type> BaseClass;

  /*!\fn WRATHAttributePackerHelper
    Ctor.
    \param attribute_store WRATHAbstractDataSink to which to write attribute data
    \param pranges_begin iterator to 1st block range (begin and end) 
                         to which to write attribute data
    \param pranges_end iterator to 1 past the last block range 
                       to which to write attribute data
   */
  WRATHAttributePackerHelper(WRATHAbstractDataSink &attribute_store,
                             std::vector<range_type<int> >::const_iterator pranges_begin,
                             std::vector<range_type<int> >::const_iterator pranges_end):
    BaseClass(attribute_store, 
              pranges_begin, pranges_end, 
              typename WRATHDefaultAttributeWriter<attribute_type>::initialize_args())
  {}
};


/*! @} */

#endif
