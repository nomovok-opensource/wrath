/*! 
 * \file WRATHAntiAliasFillShapeAttributePacker.hpp
 * \brief file WRATHAntiAliasFillShapeAttributePacker.hpp
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


/*! \addtogroup Shape
 * @{
 */

#ifndef __WRATH_ANTI_ALIAS_FILL_SHAPE_ATTRIBUTE_PACKER_HPP__
#define __WRATH_ANTI_ALIAS_FILL_SHAPE_ATTRIBUTE_PACKER_HPP__

#include "WRATHConfig.hpp"
#include "WRATHShapeAttributePacker.hpp"
#include "WRATHShapeTriangulator.hpp"

/*!\namespace WRATHAntiAliasFillShapeAttributePacker

  The actual implementation of creating the attributes
  from a WRATHAntiAliasFillShapeAttributePacker does NOT depend
  on the type T of WRATHShape<T> template parameter.

  TODO: not yet implemented correctly.
 */
namespace WRATHAntiAliasFillShapeAttributePacker
{
  enum
    {
      /*!
        location of draw position (x,y), 
        a vec2 (in GLSL). Attribute name in 
        GLSL is "pos".
      */
      position_location=0,

      /*!
        location within the attribute
        type packed by \ref set_attribute_data()
        of the distance from the original
        WRATHShape, this value can be used to fake
        anti-aliasing. Value is a float. On the boundary
        of stroking it has _absolute_ value 1.0
        and in the middle of stroking it is 0.
        Thus the value 1 - abs(A) where A is 
        the attribute interpolated across a primitive
        is 0 near the boundary, and positive within
        the primitive and can be used to compute an
        alpha coverage value

        Attribute name in GLSL is "in_aa_hint".
       */
      hint_distance_location=1,
      

      /*!
        If the attribute packer includes a y-texture
        coordinate for a gradient, this is it's
        attribute location and the GLSL name
        of the attribute is "gradient_y_coordinate"
       */
      gradient_y_coordinate_location=2


    };
  
  /*
    Namespace to specify how to fill
   */
  using namespace WRATHDefaultFillShapeAttributePacker::FillingTypes;
  

  /*!\fn allocation_requirement(WRATHShapeTriangulatorPayload::handle,
                                const FillingParameters&)
  
    Returns the allocation requirements (attribute
    and index room needed) for a given WRATHShapeTriangulatorPayload.
    \param payload handle to a WRATHShapeTriangulatorPayload, method
                   WRATHassert 's for handle to be valid
    \param fill_params provides a translation and fill rule used                   
                       filling the WRATHShape<T> that was used
                       to create payload
   */
  WRATHShapeAttributePackerBase::allocation_requirement_type
  allocation_requirement(WRATHShapeTriangulatorPayload::handle payload,
                         const FillingParameters &fill_params);

  /*!\fn set_attribute_data(WRATHShapeTriangulatorPayload::handle,
                            WRATHAttributeStore::handle,
                            const std::vector<range_type<int> > &,
                            WRATHShapeAttributePackerBase::IndexGroupBase<GLushort>*,
                            WRATHShapeAttributePackerBase::IndexGroupBase<GLushort>*,
                            const FillingParameters&,
                            std::pair<bool, float>)
  
    Assigns attribute data to a given draw group and index
    data to a given index group given a WRATHShapeTriangulatorPayload.
    \param handle to a WRATHShapeTriangulatorPayload, method
                   WRATHassert 's for handle to be valid.
    \param attribute_store attribute store to which to write the attribte data
    \param attr_location attribute locations to use for attribute data,
                         WRATHassert 's that there is sufficient
                         room to put the attribute data
    \param primary_index_group index group to write the index data for the 1st 
                               drawing pass, WRATHassert 's that there is sufficient
                               room to put the index data
    \param secondary_index_group index group to write the index data for the 2nd 
                                       drawing pass, WRATHassert 's that there is sufficient
                                       room to put the index data
    \param fill_params provides a translation and fill rule used                   
                       filling the WRATHShape<T> that was used
                       to create payload
    \param texture_coordinate_y_gradient if .first is true, then adds
                                         an attribute that stores the
                                         y-texture coordinate of a
                                         gradient, that value is taken
                                         from .second
   */
  void
  set_attribute_data(WRATHShapeTriangulatorPayload::handle payload,
                     WRATHAttributeStore::handle attribute_store,
                     const std::vector<range_type<int> > &attr_location,
                     WRATHShapeAttributePackerBase::IndexGroupBase<GLushort> *primary_index_group,
                     WRATHShapeAttributePackerBase::IndexGroupBase<GLushort> *secondary_index_group,
                     const FillingParameters &fill_params,
                     std::pair<bool, float> texture_coordinate_y_gradient);

  /*!\fn attribute_key(WRATHAttributeStoreKey&)
  
    Attribute key for the packing of a filled shape.
    \param attrib_key key to which to write
    \param include_y_gradient If true, includes an attribute to store the
                              y-texture coordinate of the gradient.
   */
  GLenum
  attribute_key(WRATHAttributeStoreKey &attrib_key, 
                bool include_y_gradient_attribute);

  /*!\fn attribute_names
  
    Returns the attribute names as a const_c_array<>.
    \param include_y_gradient If true, includes an attribute to store the
                              y-texture coordinate of the gradient.
   */
  const_c_array<const char*>
  attribute_names(bool include_y_gradient_attribute);
}

/*!\class WRATHAntiAliasFillShapeAttributePackerT

  A WRATHAntiAliasFillShapeAttributePackerT is an attribute
  packer for the filling of paths. It requires that the payload
  passed to it can be dynamic casted to a \ref WRATHShapeTriangulatorPayload.
  \tparam T the template parameter for the WRATHShape type
  \tparam B if true, then packer packs gradient-y-texture coordinate, if false does not.
 */
template<typename T, bool B=false>
class WRATHAntiAliasFillShapeAttributePackerT:public WRATHShapeAttributePacker<T, GLushort>
{
public:

  static
  WRATHShapeAttributePacker<T, GLushort>*
  fetch(void)
  {
    return WRATHAttributePacker::fetch_make<WRATHAntiAliasFillShapeAttributePackerT>(Factory());
  }

  virtual
  WRATHShapeAttributePackerBase::allocation_requirement_type
  allocation_requirement(const WRATHShape<T> *pshape,
                         WRATHShapeProcessorPayload payload,
                         const WRATHShapeAttributePackerBase::PackingParametersBase &pp,
                         const WRATHStateBasedPackingData::handle&) const
  {
    WRATHShapeTriangulatorPayload::handle h;
    const WRATHAntiAliasFillShapeAttributePacker::FillingParameters *pptr;
    const WRATHAntiAliasFillShapeAttributePacker::FillingParameters v;

    pptr=dynamic_cast<const WRATHAntiAliasFillShapeAttributePacker::FillingParameters*>(&pp);
    h=payload.dynamic_cast_handle<WRATHShapeTriangulatorPayload>();
    
    if(pptr==NULL)
      {
        pptr=&v;
      }

    if(!h.valid())
      {
        payload=default_payload(pshape);
        h=payload.dynamic_cast_handle<WRATHShapeTriangulatorPayload>();
      }

    return WRATHAntiAliasFillShapeAttributePacker::allocation_requirement(h, *pptr);
  }

  virtual
  void
  set_attribute_data(const WRATHShape<T> *pshape, 
                     WRATHShapeProcessorPayload payload,
                     WRATHAttributeStore::handle attribute_store,
                     const std::vector<range_type<int> > &attr_location,
                     WRATHShapeAttributePackerBase::IndexGroupBase<GLushort>* primary_index_group,
                     WRATHShapeAttributePackerBase::IndexGroupBase<GLushort>* secondary_index_group,
                     const WRATHShapeAttributePackerBase::PackingParametersBase &pp,
                     const WRATHStateBasedPackingData::handle &additional_datum) const
  {
    WRATHShapeTriangulatorPayload::handle h;
    const WRATHAntiAliasFillShapeAttributePacker::FillingParameters *pptr;
    const WRATHAntiAliasFillShapeAttributePacker::FillingParameters v;

    h=payload.dynamic_cast_handle<WRATHShapeTriangulatorPayload>();
    pptr=dynamic_cast<const WRATHAntiAliasFillShapeAttributePacker::FillingParameters*>(&pp);
    
    if(pptr==NULL)
      {
        pptr=&v;
      }

    if(!h.valid())
      {
        payload=default_payload(pshape);
        h=payload.dynamic_cast_handle<WRATHShapeTriangulatorPayload>();
      }

    std::pair<bool, float> gradient_y_texture_coordinate(B, 0.0f);
    if(B)
      {
        WRATHassert(additional_datum.dynamic_cast_handle<WRATHGradient::GradientYCoordinate>().valid());
        gradient_y_texture_coordinate.second=
          additional_datum.static_cast_handle<WRATHGradient::GradientYCoordinate>()->texture_coordinate_y();
        
      }

    WRATHAntiAliasFillShapeAttributePacker::set_attribute_data(h, attribute_store, 
                                                               attr_location, 
                                                               primary_index_group, secondary_index_group,
                                                               *pptr, gradient_y_texture_coordinate);
  }

  virtual
  WRATHShapeProcessorPayload
  default_payload(const WRATHShape<T> *pshape) const
  {
    return pshape->template fetch_payload<WRATHShapeTriangulatorPayload>();
  }

  virtual
  GLenum
  attribute_key(WRATHAttributeStoreKey &attrib_key) const
  {
    return WRATHAntiAliasFillShapeAttributePacker::attribute_key(attrib_key, B);
  }

private:

  class Factory:public WRATHAttributePacker::AttributePackerFactory
  {
  public:
    virtual
    WRATHAttributePacker*
    create(void) const 
    {
      return WRATHNew WRATHAntiAliasFillShapeAttributePackerT();
    }

  };

  explicit
  WRATHAntiAliasFillShapeAttributePackerT(void):
    WRATHShapeAttributePacker<T, GLushort>(typeid(WRATHAntiAliasFillShapeAttributePackerT).name(), 
                                           WRATHAntiAliasFillShapeAttributePacker::attribute_names(B).begin(), 
                                           WRATHAntiAliasFillShapeAttributePacker::attribute_names(B).end())
  {}

};

typedef WRATHAntiAliasFillShapeAttributePackerT<float, false> WRATHAntiAliasFillShapeAttributePackerF;
typedef WRATHAntiAliasFillShapeAttributePackerT<int, false> WRATHAntiAliasFillShapeAttributePackerI;

typedef WRATHAntiAliasFillShapeAttributePackerT<float, true> WRATHAntiAliasFillShapeAttributePackerWithGradientF;
typedef WRATHAntiAliasFillShapeAttributePackerT<int, true> WRATHAntiAliasFillShapeAttributePackerWithGradientI;



/*! @} */


#endif
