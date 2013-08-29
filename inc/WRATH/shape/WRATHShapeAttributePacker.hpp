/*! 
 * \file WRATHShapeAttributePacker.hpp
 * \brief file WRATHShapeAttributePacker.hpp
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


#ifndef __WRATH_SHAPE_ATTRIBUTE_PACKER_HPP__
#define __WRATH_SHAPE_ATTRIBUTE_PACKER_HPP__


#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include "c_array.hpp"
#include "WRATHShape.hpp"
#include "WRATHResourceManager.hpp"
#include "WRATHCanvas.hpp"
#include "WRATHAttributePacker.hpp"

/*! \addtogroup Shape
 * @{
 */

/*!\class WRATHShapeAttributePackerBase
  Generic base class used WRATHShapeAttributePacker
  to provide a common interface for certain interface
  bits.
 */
class WRATHShapeAttributePackerBase:public WRATHAttributePacker
{
public:
  /*!\class PackingParametersBase
    A PackingParametersBase is a base class for
    providing additional attribute packing parameters
    (for example say width of stroking). 
   */
  class PackingParametersBase
  {
  public:
    virtual
    ~PackingParametersBase()
    {}
  };


  /*!\class allocation_requirement_type
    An allocation_requirement holds the number
    of attributes and indices required to pack
    attributes and indices to draw a WRATHShape. 
    Since drawing  of a shape may have an opaque 
    pass and a translucent pass with _different_ 
    indices, an additional boolean is given indicating 
    if that is the case.
   */
  class allocation_requirement_type
  {
  public:
    
    /*!\typedef payload_type
      Conveniace typedef
     */
    typedef WRATHReferenceCountedObject::handle_t<WRATHReferenceCountedObject> payload_type; 

    /*!\fn allocation_requirement_type
      Ctor, inits as having no allocation requirements
     */
    allocation_requirement_type(void):
      m_number_attributes(0),
      m_primary_number_indices(0),
      m_secondary_number_indices(0)
    {}

    /*!\var m_number_attributes
      Number of attributes required, the attributes
      do NOT need to be allocated in one block.
     */
    int m_number_attributes;

    /*!\var m_primary_number_indices
      Number of indices required for the primary
      drawing comamnd, the indices need to be allocated 
      in a single continuous block.
     */
    int m_primary_number_indices;

    /*!\var m_secondary_number_indices
      Number of indices required for the secondary
      drawing comamnd, the indices need to be allocated 
      in a single continuous block. This value should
      be ignored for one pass rendering
     */
    int m_secondary_number_indices;

    /*!\fn bool empty
      Returns true if bot \ref m_primary_number_indices 
      and \ref m_secondary_number_indices are zero.
     */
    bool
    empty(void) const
    {
      return m_primary_number_indices==0 and m_secondary_number_indices==0;
    }
  };

 
  /*!\fn WRATHShapeAttributePackerBase
    Ctor. Specifies the resource name of the attribute packer
    and the names of each attribute as an STL range.
    The number of attributes is then std::distance(begin,end)
    and the attribute of index I has value *(begin+I).
    \param pname resource name to identify the WRATHShapeAttributePackerBase
    \param begin iterator to name of attribure #0
    \param end iterator to one past the name of the last attribute
   */
  template<typename iterator>
  WRATHShapeAttributePackerBase(const ResourceKey &pname,
                                iterator begin, iterator end):
    WRATHAttributePacker(pname, begin, end)
  {}
  
  virtual
  ~WRATHShapeAttributePackerBase()
  {}

  /*!\fn GLenum attribute_key
    To be implemented by a derived class to
    fetch the attribute key and to also 
    return the primitive type to be used.
    \param attrib_key WRATHAttributeStoreKey to which to set
   */
  virtual
  GLenum
  attribute_key(WRATHAttributeStoreKey &attrib_key) const=0;  

  /*!\fn bool has_secondary_pass
    If has_secondary_pass() returns true, then
    the shape attribute packer produces two different
    index sets: one for the primary pass and one
    for the secondary pass. If false, multi-pass drawing 
    is still possible, but both passes then use the 
    exact same index set. Default implementation is 
    to return false.
  */
  virtual
  bool 
  has_secondary_pass(void) const
  {
    return false;
  }
};

/*!\class WRATHShapeAttributePacker
  A WRATHShapeAttributePacker is to implement attribute
  packing for WRATHShape data. A WRATHShapeAttributePacker
  implementation by it's existance packs attributes for
  drawing shape data.
 */
template<typename T>
class WRATHShapeAttributePacker:public WRATHShapeAttributePackerBase
{
public:

  /*!\fn WRATHShapeAttributePacker
    Ctor. Specifies the resource name of the attribute packer
    and the names of each attribute as an STL range.
    The number of attributes is then std::distance(begin,end)
    and the attribute of index I has value begin+I.
    \param pname resource name to identify the WRATHShapeAttributePacker
    \param begin iterator to name of attribure #0
    \param end iterator to one past the name of the last attribute
   */
  template<typename iterator>
  WRATHShapeAttributePacker(const ResourceKey &pname,
                            iterator begin, iterator end):
    WRATHShapeAttributePackerBase(pname, begin, end)
  {}

  /*!\fn allocation_requirement_type allocation_requirement(const WRATHShape<T>*,
                                WRATHShapeProcessorPayload,
                                const PackingParametersBase&,
                                const WRATHStateBasedPackingData::handle&) const
    To be implemented by a derived class to indicate
    how many attributes and indices are required to
    pack attributes and indices to draw a WRATHShape<T>.
    \param pshape pointer to WRATHShape.
    \param payload handle to payload of data orignating from pshape
    \param additional_parameters additional parameters that may affect attributes
                                 packed
    \param h handle to hold additional immutable state that affects packing
             that is not within additional_parameters

   */
  virtual
  allocation_requirement_type
  allocation_requirement(const WRATHShape<T> *pshape,
                         WRATHShapeProcessorPayload payload,
                         const PackingParametersBase &additional_parameters,
                         const WRATHStateBasedPackingData::handle &h) const=0;

  /*!\fn void set_attribute_data(const WRATHShape<T>*,
                                 WRATHShapeProcessorPayload,
                                 WRATHAbstractDataSink&,
                                 const std::vector< range_type<int> >&,
                                 WRATHAbstractDataSink*,
                                 WRATHAbstractDataSink*,
                                 const PackingParametersBase&,
                                 const WRATHStateBasedPackingData::handle&) const
    To be implemented by a derived class to
    pack attribute data. The assumption is that 
    the total number of attributes allocated is 
    alteast that which is returned by 
    allocation_requirement() and the total number of 
    indices needed is also. Results are undefined if 
    the \ref WRATHShape or \ref WRATHShapeProcessorPayload 
    passed to set_attribute_data() is not the same 
    or has been changed as used in allocation_requirement().

    \param pshape WRATHShape to draw
    \param payload handle to payload of data orignating from pshape
    \param attribute_store refernce to sink to which to write attribute data
    \param attr_location location within attribute_store to which to write attribute data
    \param primary_index_group pointer to sink to which to write index data for primary draw
                               Acceptable for this to be NULL, indicating that the primary
                               indices will not be packed
    \param secondary_index_group pointer to sink to which to write index data for secondary draw.
                                 Acceptable for this to be NULL, indicating that the secondary
                                 indices will not be packed
    \param additional_parameters additional parameters that may affect attributes
                                 packed
    \param h handle to hold additional immutable state that affects packing
             that is not within additional_parameters
   */
  void
  set_attribute_data(const WRATHShape<T> *pshape, 
                     WRATHShapeProcessorPayload payload,
                     WRATHAbstractDataSink &attribute_store,
                     const std::vector<range_type<int> > &attr_location,
                     WRATHAbstractDataSink *primary_index_group,
                     WRATHAbstractDataSink *secondary_index_group,
                     const PackingParametersBase &additional_parameters,
                     const WRATHStateBasedPackingData::handle &h) const
  {
    set_attribute_data_implement(pshape, payload, attribute_store, attr_location,
                                 primary_index_group, secondary_index_group,
                                 additional_parameters, h);
  }

  /*!\fn void set_attribute_data(const WRATHShape<T>*,
                                 WRATHShapeProcessorPayload,
                                 const WRATHAttributeStore::handle&,
                                 const std::vector< range_type<int> >&,
                                 WRATHIndexGroupAllocator::index_group<I>,
                                 WRATHIndexGroupAllocator::index_group<I>,
                                 const PackingParametersBase&,
                                 const WRATHStateBasedPackingData::handle&) const

     Provided as a conveniance to "get" the WRATHAbstractDataSink
     objects from \ref WRATHAttributeStore and \ref 
     WRATHIndexGroupAllocator::index_group and then feed them to
     \ref set_attribute_data(const WRATHShape<T>*,
                        WRATHShapeProcessorPayload,
                        WRATHAbstractDataSink&,
                        const std::vector< range_type<int> >&,
                        WRATHAbstractDataSink*,
                        WRATHAbstractDataSink*,
                        const PackingParametersBase&,
                        const WRATHStateBasedPackingData::handle&) const

    \param pshape WRATHShape to draw
    \param payload handle to payload of data orignating from pshape
    \param attribute_store handle to \ref WRATHAttributeStore to which to write attribute data
    \param attr_location location within attribute_store to which to write attribute data
    \param primary_index_group WRATHIndexGroupAllocator::index_group<I> to which to write 
                               index data for primary draw. Acceptable for this to not be valid, 
                               indicating that the primary indices will not be packed
    \param secondary_index_group WRATHIndexGroupAllocator::index_group<I> to which to write 
                                 index data for secondary draw. Acceptable for this to not be valid, 
                                 indicating that the secondary indices will not be packed
    \param additional_parameters additional parameters that may affect attributes
                                 packed
    \param h handle to hold additional immutable state that affects packing
             that is not within additional_parameters
   */
  template<typename I>
  void
  set_attribute_data(const WRATHShape<T> *pshape, 
                     WRATHShapeProcessorPayload payload,
                     const WRATHAttributeStore::handle &attribute_store,
                     const std::vector<range_type<int> > &attr_location,
                     WRATHIndexGroupAllocator::index_group<I> primary_index_group,
                     WRATHIndexGroupAllocator::index_group<I> secondary_index_group,
                     const PackingParametersBase &additional_parameters,
                     const WRATHStateBasedPackingData::handle &h) const
  {
    WRATHAttributeStore::DataSink attribute_sink(attribute_store->data_sink());
    WRATHIndexGroupAllocator::DataSink primary(primary_index_group.data_sink());
    WRATHIndexGroupAllocator::DataSink secondary(secondary_index_group.data_sink());
    WRATHIndexGroupAllocator::DataSink *primary_ptr(&primary);
    WRATHIndexGroupAllocator::DataSink *secondary_ptr(&secondary);
    if(!secondary_index_group.valid())
      {
        secondary_ptr=NULL;
      }
    set_attribute_data(pshape, payload, 
                       attribute_sink, attr_location, 
                       primary_ptr, secondary_ptr,
                       additional_parameters, h);
  }

  /*!\fn WRATHShapeProcessorPayload default_payload
    To be implemented by a derived class to
    use WRATHShape<T>::fetch_payload() to
    fetch a payload of the correct type
    to feed to \ref set_attribute_data().
   */
  virtual
  WRATHShapeProcessorPayload
  default_payload(const WRATHShape<T> *pshape) const=0;

protected:

  /*!\fn set_attribute_data_implement(const WRATHShape<T>*,
                                      WRATHShapeProcessorPayload,
                                      WRATHAbstractDataSink&,
                                      const std::vector< range_type<int> >&,
                                      WRATHAbstractDataSink*,
                                      WRATHAbstractDataSink*,
                                      const PackingParametersBase&,
                                      const WRATHStateBasedPackingData::handle&) const
    To be implemented by a derived class to
    pack attribute data. The assumption is that 
    the total number of attributes allocated is 
    alteast that which is returned by 
    allocation_requirement() and the total number of 
    indices needed is also. Results are undefined if 
    the WRATHShape or WRATHShapeProcessorPayload 
    passed to set_attribute_data() is not the same 
    or has been changed as used in allocation_requirement().

    \param pshape WRATHShape to draw
    \param payload handle to payload of data orignating from pshape
    \param attribute_store refernce to sink to which to write attribute data
    \param attr_location location within attribute_store to which to write attribute data
    \param primary_index_group pointer to sink to which to write index data for primary draw
                               Acceptable for this to be NULL, indicating that the primary
                               indices will not be packed
    \param secondary_index_group pointer to sink to which to write index data for secondary draw.
                                 Acceptable for this to be NULL, indicating that the secondary
                                 indices will not be packed
    \param additional_parameters additional parameters that may affect attributes
                                 packed
    \param h handle to hold additional immutable state that affects packing
             that is not within additional_parameters
   */
  virtual
  void
  set_attribute_data_implement(const WRATHShape<T> *pshape, 
                               WRATHShapeProcessorPayload payload,
                               WRATHAbstractDataSink &attribute_store,
                               const std::vector<range_type<int> > &attr_location,
                               WRATHAbstractDataSink *primary_index_group,
                               WRATHAbstractDataSink *secondary_index_group,
                               const PackingParametersBase &additional_parameters,
                               const WRATHStateBasedPackingData::handle &h) const=0;   


};

/*!\typedef WRATHShapeAttributePackerF
  Conveniance typedef to WRATHShapeAttributePacker\<float\>
 */
typedef WRATHShapeAttributePacker<float> WRATHShapeAttributePackerF;

/*!\typedef WRATHShapeAttributePackerI
  Conveniance typedef to WRATHShapeAttributePacker\<int32_t\>
 */
typedef WRATHShapeAttributePacker<int32_t> WRATHShapeAttributePackerI;

/*! @} */

#endif
