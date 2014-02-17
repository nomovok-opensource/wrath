/*! 
 * \file WRATHDynamicStrokeAttributePacker.hpp
 * \brief file WRATHDynamicStrokeAttributePacker.hpp
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



#ifndef WRATH_HEADER_DYNAMIC_STROKE_ATTRIBUTE_PACKER_HPP_
#define WRATH_HEADER_DYNAMIC_STROKE_ATTRIBUTE_PACKER_HPP_

#include "WRATHConfig.hpp"
#include "WRATHDefaultStrokeAttributePacker.hpp"



/*! \addtogroup Shape
 * @{
 */

/*!\namespace WRATHDynamicStrokeAttributePacker
  The actual implementation of creating the attributes
  from a WRATHShapeStrokerPayload does NOT depend
  on the type T of WRATHShape<T> template parameter.
 */
namespace WRATHDynamicStrokeAttributePacker
{
  enum 
    {
      /*!
        location within the attribute
        type packed by \ref set_attribute_data()
        of draw pre-position (x,y), a vec2 (in GLSL).
        Attribute name in GLSL is "pos".
      */
      pre_position_location=0, 

      /*!
        location within the attribute
        type packed by \ref set_attribute_data()
        of the "normal" vector. The position
        to place the vertex is given by
        the value of the pre_position plus
        the width of stroking times the value
        of the normal [normal is a vec2 in GLSL].
        Attribute name in GLSL is "normal".
       */
      normal_location=1,

      /*!
        location within the attribute type
        packed by \ref set_attribute_data()
        of the "aa-hint". This value is always
        one of {-1, 0, 1}. On a point of the
        edge of stroking it is -1 or 1, on
        a point in the interior it is 0.     
        Attribute name in GLSL is "in_aa_hint".   
       */
      aa_hint_location=2,

    };

  /*
    Reuse WRATHDefaultStrokeAttributePacker::StrokingTypes::StrokingParameters 
   */
  using namespace WRATHDefaultStrokeAttributePacker::StrokingTypes;
  
  /*!\fn WRATHShapeAttributePackerBase::allocation_requirement_type allocation_requirement(WRATHShapePreStrokerPayload::handle, const StrokingParameters&)
  
    Returns the allocation requirements (attribute
    and index room needed) for a given WRATHShapeStrokerPayload.
    \param payload handle to a WRATHShapeStrokerPayload, method
                   WRATHassert 's for handle to be valid. 
    \param stroking_params additional stroking parameters
   */
  WRATHShapeAttributePackerBase::allocation_requirement_type
  allocation_requirement(WRATHShapePreStrokerPayload::handle payload,
                         const StrokingParameters &stroking_params);


  /*!\fn void set_attribute_data(WRATHShapePreStrokerPayload::handle,
                                 WRATHAbstractDataSink&,
                                 const std::vector<range_type<int> > &,
                                 WRATHAbstractDataSink*,
                                 const StrokingParameters&)
    Assigns attribute data to a given draw group and index
    data to a given inde group given a WRATHShapeStrokerPayload.
    \param payload handle to a WRATHShapePreStrokerPayload, method
                   WRATHassert 's for handle to be valid.
    \param attribute_store WRATHAbstractDataSink to which to write attribute data
    \param attr_location attribute locations to use for attribute data,
                         WRATHassert 's that there is sufficient
                         room to put the attribute data
    \param index_group WRATHAbstractDataSink to which to write index data.
                       Will write index as GLushort's. 
    \param stroking_params additional stroking parameters
   */
  void
  set_attribute_data(WRATHShapePreStrokerPayload::handle payload,
                     WRATHAbstractDataSink &attribute_store,
                     const std::vector<range_type<int> > &attr_location,
                     WRATHAbstractDataSink *index_group,
                     const StrokingParameters &stroking_params);

  /*!\fn GLenum attribute_key(WRATHAttributeStoreKey&)
    Attribute key for the packing of a dynamic stroked shape.
    \param attrib_key location to which to write the key
   */
  GLenum
  attribute_key(WRATHAttributeStoreKey &attrib_key);

  /*!\fn const_c_array<const char*> attribute_names(void)
    Returns the attribute names as a const_c_array<>.
   */
  const_c_array<const char*>
  attribute_names(void);
}

/*!\class WRATHDynamicStrokeAttributePackerT
  A WRATHDynamicStrokeAttributePacker is an attribute
  packer for stroking of paths. It requires that the payload
  passed to it can be dynamic casted to a \ref WRATHShapePreStrokerPayload.
 */
template<typename T>
class WRATHDynamicStrokeAttributePackerT:public WRATHShapeAttributePacker<T>
{
public:

  /*!\fn WRATHShapeAttributePacker<T>* fetch
    For each type T, only one WRATHDynamicStrokeAttributePackerT<T> object
    exists, use fetch() to get that object.
   */
  static
  WRATHShapeAttributePacker<T>*
  fetch(void)
  {
    return WRATHAttributePacker::fetch_make<WRATHDynamicStrokeAttributePackerT>(Factory());
  }

  virtual
  WRATHShapeAttributePackerBase::allocation_requirement_type
  allocation_requirement(const WRATHShape<T> *pshape,
                         WRATHShapeProcessorPayload payload,
                         const WRATHShapeAttributePackerBase::PackingParametersBase &pp,
                         const WRATHStateBasedPackingData::handle &) const
  {
    WRATHShapePreStrokerPayload::handle h;
    const WRATHDynamicStrokeAttributePacker::StrokingParameters *pptr;
    const WRATHDynamicStrokeAttributePacker::StrokingParameters v;

    h=payload.dynamic_cast_handle<WRATHShapePreStrokerPayload>();
    pptr=dynamic_cast<const WRATHDynamicStrokeAttributePacker::StrokingParameters*>(&pp);
    
    if(pptr==NULL)
      {
        pptr=&v;
      }

    if(!h.valid())
      {
        payload=default_payload(pshape);
        h=payload.dynamic_cast_handle<WRATHShapePreStrokerPayload>();
      }

    return WRATHDynamicStrokeAttributePacker::allocation_requirement(h, *pptr);
  }

  virtual
  GLenum
  attribute_key(WRATHAttributeStoreKey &attrib_key) const
  {
    return WRATHDynamicStrokeAttributePacker::attribute_key(attrib_key);
  }

  virtual
  WRATHShapeProcessorPayload
  default_payload(const WRATHShape<T> *pshape) const
  {
    return pshape->template fetch_payload<WRATHShapePreStrokerPayload>();
  }

protected:

  virtual
  void
  set_attribute_data_implement(const WRATHShape<T> *pshape, 
                               WRATHShapeProcessorPayload payload,
                               WRATHAbstractDataSink &attribute_store,
                               const std::vector<range_type<int> > &attr_location,
                               WRATHAbstractDataSink *primary_index_group,
                               WRATHAbstractDataSink* /*secondary_index_group*/,
                               const WRATHShapeAttributePackerBase::PackingParametersBase &pp,
                               const WRATHStateBasedPackingData::handle&) const
  {
    WRATHShapePreStrokerPayload::handle h;
    const WRATHDynamicStrokeAttributePacker::StrokingParameters *pptr;
    const WRATHDynamicStrokeAttributePacker::StrokingParameters v;

    h=payload.dynamic_cast_handle<WRATHShapePreStrokerPayload>();
    pptr=dynamic_cast<const WRATHDynamicStrokeAttributePacker::StrokingParameters*>(&pp);
    
    if(pptr==NULL)
      {
        pptr=&v;
      }

    if(!h.valid())
      {
        payload=default_payload(pshape);
        h=payload.dynamic_cast_handle<WRATHShapePreStrokerPayload>();
      }

    WRATHDynamicStrokeAttributePacker::set_attribute_data(h, attribute_store, 
                                                          attr_location, primary_index_group,
                                                          *pptr);
  }

private:
  
  class Factory:public WRATHAttributePacker::AttributePackerFactory
  {
  public:
    virtual
    WRATHAttributePacker*
    create(void) const 
    {
      return WRATHNew WRATHDynamicStrokeAttributePackerT();
    }

  };
  
  WRATHDynamicStrokeAttributePackerT(void):
    WRATHShapeAttributePacker<T>(typeid(WRATHDynamicStrokeAttributePackerT).name(),
                                 WRATHDynamicStrokeAttributePacker::attribute_names().begin(),
                                 WRATHDynamicStrokeAttributePacker::attribute_names().end())
  {}

};

/*!\typedef WRATHDynamicStrokeAttributePackerF
  Conveniance typedef to WRATHDynamicStrokeAttributePackerT\<float\>
 */
typedef WRATHDynamicStrokeAttributePackerT<float> WRATHDynamicStrokeAttributePackerF;

/*!\typedef WRATHDynamicStrokeAttributePackerI
  Conveniance typedef to WRATHDynamicStrokeAttributePackerT\<int\>
 */
typedef WRATHDynamicStrokeAttributePackerT<int> WRATHDynamicStrokeAttributePackerI;

/*! @} */


#endif
