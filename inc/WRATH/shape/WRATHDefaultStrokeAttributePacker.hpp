/*! 
 * \file WRATHDefaultStrokeAttributePacker.hpp
 * \brief file WRATHDefaultStrokeAttributePacker.hpp
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

#ifndef __WRATH_DEFAULT_STROKE_ATTRIBUTE_PACKER_HPP__
#define __WRATH_DEFAULT_STROKE_ATTRIBUTE_PACKER_HPP__

#include "WRATHConfig.hpp"
#include "WRATHShapeAttributePacker.hpp"
#include "WRATHShapePreStroker.hpp"
#include "WRATHGenericStrokeAttributePacker.hpp"

/*!\namespace WRATHDefaultStrokeAttributePacker
  The actual implementation of creating the attributes
  from a WRATHShapePreStrokerPayload does NOT depend
  on the type T of WRATHShape<T> template parameter.
 */
namespace WRATHDefaultStrokeAttributePacker
{
  enum 
    {
      /*!
        location within the attribute
        type packed by \ref set_attribute_data()
        of draw position (x,y), a vec2 (in GLSL).
        Attribute name in GLSL is "pos".
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
    };

  /*!\namespace WRATHDefaultStrokeAttributePacker::StrokingTypes
    Namespace to encapsulate the parameters that
    define stroking.
   */
  namespace StrokingTypes
  {

    /*!\enum join_style_type
      Enumeration dictating how joins are drawn.
    */
    enum join_style_type
      {
        /*!
          Indicates to join edges via a Bevel,
          i.e. to give a flat notch on the exterior
          of the stroked line at edge points.
        */
        bevel_join,
        
        /*!
          Indicates that at join edges to be
          sharp on the exterior where edges
          meet. Note that using miter_join
          with almost parallel edges can 
          induce that the corner of that sharp
          point is very far away. Use \ref 
          StrokingParameters::m_miter_limit to adjust 
          to limit how far away that sharp 
          corner can be.
        */
        miter_join,
        
        /*!
          Indicates that a circular arc is used
          on the exterior of the stroked line
          path.
        */
        round_join,
        
        /*!
          Indicates to not draw any join
        */
        no_join
      };
    
    /*!\enum cap_style_type
      Enumeration dictating how caps are 
      stroked if outlines are not closed.
    */
    enum cap_style_type
      {
        /*!
          cap the end of an outline with
          a square
        */
        square_cap,
        
        /*!
          Essentially no cap
        */
        flat_cap,
        
        /*!
          Cap the end of an outline with
          a rounded edge.
        */
        rounded_cap,
      };

    /*
      import name space specifying curve stroking
     */
    using namespace WRATHGenericStrokeAttributePacker::CurveStrokingTypes;
    
    
    /*!class StrokingParameters
      A PackingParameters objects holds stroking 
      parameters used by set_attribute_data():
      - width of stroking
      - join style
      - cap style
      - weather or not to close the outlines
      - miter limit
      - weather or not to stroke the curves
    */
    class StrokingParameters:
      public WRATHShapeAttributePackerBase::PackingParametersBase
    {
    public:
      
      /*!\fn StrokingParameters(const vec2&)
        Ctor.
        \param tr pre-translation to apply
      */
      StrokingParameters(const vec2 &tr=vec2(0.0f)):
        m_translate(tr),
        m_join_style(bevel_join),
        m_cap_style(square_cap),
        m_radius(5.0f),
        m_miter_limit(2.0f),
        m_stroke_curves(solid_stroke),
        m_close_outline(false)
      {}
      
      /*!\fn StrokingParameters(float, float)
        Ctor.
        \param x x pre-translation to apply
        \param y y pre-translation to apply
      */
      StrokingParameters(float x, float y):
        m_translate(x,y),
        m_join_style(bevel_join),
        m_cap_style(square_cap),
        m_radius(5.0f),
        m_miter_limit(2.0f),
        m_stroke_curves(solid_stroke),
        m_close_outline(false)
      {}
      
      /*!\fn StrokingParameters& close_outline(bool)
        Sets \ref m_close_outline.
        \param v value to use
      */
      StrokingParameters&
      close_outline(bool v)
      {
        m_close_outline=v;
        return *this;
      }
      
      /*!\fn StrokingParameters& join_style
        Sets \ref m_join_style.
        \param v value to use
      */
      StrokingParameters&
      join_style(enum join_style_type v)
      {
        m_join_style=v;
        return *this;
      }
    
      /*!\fn StrokingParameters& cap_style
        Sets \ref m_cap_style.
        \param v value to use
      */
      StrokingParameters&
      cap_style(enum cap_style_type v)
      {
        m_cap_style=v;
        return *this;
      }
      
      /*!\fn StrokingParameters& radius
        Sets \ref m_radius.
        \param v value to use
      */
      StrokingParameters&
      radius(float v)
      {
        m_radius=v;
        return *this;
      }
      
      /*!\fn StrokingParameters& width
        Provided as a conveniance,
        equivalent to 
        \code
        radius(w*0.5f);
        \endcode
        \param v value to use
      */
      StrokingParameters&
      width(float v)
      {
        return radius(v*0.5f);
      }
      
      /*!\fn StrokingParameters& miter_limit
        Sets \ref m_miter_limit.
        \param v value to use
      */
      StrokingParameters&
      miter_limit(float v)
      {
        m_miter_limit=v;
        return *this;
      }
      
      /*!\fn StrokingParameters& stroke_curves(enum pen_style_type)
        Sets \ref m_stroke_curves.
        \param v value to use
      */
      StrokingParameters&
      stroke_curves(enum pen_style_type v)
      {
        m_stroke_curves=v;
        return *this;
      }
      
      /*!\fn WRATHGenericStrokeAttributePacker::StrokingParameters generate_generic_parameters
        Generate corresponding \ref WRATHGenericStrokeAttributePacker::StrokingParameters
        from this StrokingParameters object: 
        - \ref WRATHGenericStrokeAttributePacker::StrokingParameters::m_generate_flags
          will be set to only generate those caps or joins as indicated
          by this StrokingParameters object
        - \ref WRATHGenericStrokeAttributePacker::StrokingParameters::m_close_outline
          and \ref WRATHGenericStrokeAttributePacker::StrokingParameters::m_stroke_curves
          come directly from \ref m_close_outline and \ref m_stroke_curves repsectively
      */
      WRATHGenericStrokeAttributePacker::StrokingParameters
      generate_generic_parameters(void) const;
      
      /*!\var m_translate
        Amount by which to translate the shape.
      */
      vec2 m_translate;
      
      /*!\var m_join_style
        Dictates how the joins between edges of an
        outline are draw. Default value is \ref 
        bevel_join.
      */
      enum join_style_type m_join_style;
      
      /*!\var m_cap_style 
        Dictates how the end caps are drawn.
        Note that this has effect if and only
        if \ref m_close_outline is false.
        Default value is \ref square_cap.
      */
      enum cap_style_type m_cap_style;
      
      /*!\var m_radius
        Dictates the thickness of the stroking.
        The value is a _radius_ from the path
        of stroking, the width of stroking is
        twice the radius of stroking.

        Default value is 5.
      */
      float m_radius;
      
      /*!\var m_miter_limit
        Only has affect when \ref m_join_style
        is \ref miter_join. This value dictates
        the maximum distance from the edge point 
        of a miter join to the tip  of the triangle  
        in miter joins. The units of the miter limit 
        are in units of \ref m_radius, i.e. 
        the actual distance is then m_miter_limit*m_radius.
        Default value is 2.0f.
      */
      float m_miter_limit;
      
      /*!\var m_stroke_curves
        Specifies the stroking of the curves,
        Default value is \ref 
        WRATHGenericStrokeAttributePacker::CurveStrokingTypes::solid_stroke.
      */
      enum pen_style_type m_stroke_curves;
    
      /*!\var m_close_outline
        If true draws the stroke of the implicit
        curve connecting from the last point
        of the outline to the first point of the
        outline. Default value is false.
      */
      bool m_close_outline;
    };
  }

  using namespace StrokingTypes;
  
  /*!\fn WRATHShapeAttributePackerBase::allocation_requirement_type allocation_requirement(WRATHShapePreStrokerPayload::handle, const StrokingParameters&)
    Returns the allocation requirements (attribute
    and index room needed) for a given WRATHShapePreStrokerPayload.
    \param payload handle to a WRATHShapePreStrokerPayload, method
                   WRATHassert 's for handle to be valid.
    \param stroking_params Stroking parameters for the payload.
   */
  WRATHShapeAttributePackerBase::allocation_requirement_type
  allocation_requirement(WRATHShapePreStrokerPayload::handle payload,
                         const StrokingParameters &stroking_params);

  /*!\fn void set_attribute_data(WRATHShapePreStrokerPayload::handle,
                                 WRATHAbstractDataSink&,
                                 const std::vector< range_type<int> >&,
                                 WRATHAbstractDataSink*,
                                 const StrokingParameters&)
  
    Writes attribute and index data to given WRATHAbstractDataSink
    object from a given WRATHShapePreStrokerPayload.
    \param payload handle to a WRATHShapePreStrokerPayload, method
                   WRATHassert 's for handle to be valid.
    \param attribute_store WRATHAbstractDataSink to which to write attribute data
    \param attr_location attribute locations to use for attribute data
    \param index_group WRATHAbstractDataSink to which to write index data.
                       Will write index as GLushort's.
    \param stroking_params stroking parameters
   */
  void
  set_attribute_data(WRATHShapePreStrokerPayload::handle payload,
                     WRATHAbstractDataSink &attribute_store,
                     const std::vector<range_type<int> > &attr_location,
                     WRATHAbstractDataSink *index_group,
                     const StrokingParameters &stroking_params);

  /*!\fn GLenum attribute_key(WRATHAttributeStoreKey&)
    Attribute key for the packing of a stroked shape.
    \param attrib_key key to which to write
   */
  GLenum
  attribute_key(WRATHAttributeStoreKey &attrib_key);

  /*!\fn const_c_array<const char*> attribute_names
    Returns the attribute names as a const_c_array<>.
   */
  const_c_array<const char*>
  attribute_names(void);
}

/*!\class WRATHDefaultStrokeAttributePackerT
  A WRATHDefaultStrokeAttributePackerT is an attribute
  packer for stroking of paths. It requires that the payload
  passed to it can be dynamic casted to a \ref WRATHShapePreStrokerPayload.
  \tparam T the template parameter for the WRATHShape type
 */
template<typename T>
class WRATHDefaultStrokeAttributePackerT:public WRATHShapeAttributePacker<T>
{
public:
  /*!\fn WRATHShapeAttributePacker<T>* fetch
    For each type T, onle one WRATHDefaultStrokeAttributePackerT<T> object
    exists, use fetch() to get that object.    
   */
  static
  WRATHShapeAttributePacker<T>*
  fetch(void)
  {
    return WRATHAttributePacker::fetch_make<WRATHDefaultStrokeAttributePackerT>(Factory());
  }

  virtual
  WRATHShapeAttributePackerBase::allocation_requirement_type
  allocation_requirement(const WRATHShape<T> *pshape,
                         WRATHShapeProcessorPayload payload,
                         const WRATHShapeAttributePackerBase::PackingParametersBase &pp,
                         const WRATHStateBasedPackingData::handle&) const
  {
    WRATHShapePreStrokerPayload::handle h;
    const WRATHDefaultStrokeAttributePacker::StrokingParameters *pptr;
    const WRATHDefaultStrokeAttributePacker::StrokingParameters v;

    h=payload.dynamic_cast_handle<WRATHShapePreStrokerPayload>();
    pptr=dynamic_cast<const WRATHDefaultStrokeAttributePacker::StrokingParameters*>(&pp);
    
    if(pptr==NULL)
      {
        pptr=&v;
      }

    if(!h.valid())
      {
        payload=default_payload(pshape);
        h=payload.dynamic_cast_handle<WRATHShapePreStrokerPayload>();
      }

    return WRATHDefaultStrokeAttributePacker::allocation_requirement(h, *pptr);
  }

  virtual
  WRATHShapeProcessorPayload
  default_payload(const WRATHShape<T> *pshape) const
  {
    return pshape->template fetch_payload<WRATHShapePreStrokerPayload>();
  }

  virtual
  GLenum
  attribute_key(WRATHAttributeStoreKey &attrib_key) const
  {
    return WRATHDefaultStrokeAttributePacker::attribute_key(attrib_key);
  }

protected:
  virtual
  void
  set_attribute_data_implement(const WRATHShape<T> *pshape, 
                               WRATHShapeProcessorPayload payload,
                               WRATHAbstractDataSink &attribute_store,
                               const std::vector<range_type<int> > &attr_location,
                               WRATHAbstractDataSink* primary_index_group,
                               WRATHAbstractDataSink* /*secondary_index_group*/,
                               const WRATHShapeAttributePackerBase::PackingParametersBase &pp,
                               const WRATHStateBasedPackingData::handle &) const
  {
    WRATHShapePreStrokerPayload::handle h;
    const WRATHDefaultStrokeAttributePacker::StrokingParameters *pptr;
    const WRATHDefaultStrokeAttributePacker::StrokingParameters v;

    h=payload.dynamic_cast_handle<WRATHShapePreStrokerPayload>();
    pptr=dynamic_cast<const WRATHDefaultStrokeAttributePacker::StrokingParameters*>(&pp);
    
    if(pptr==NULL)
      {
        pptr=&v;
      }

    if(!h.valid())
      {
        payload=default_payload(pshape);
        h=payload.dynamic_cast_handle<WRATHShapePreStrokerPayload>();
      }

    WRATHDefaultStrokeAttributePacker::set_attribute_data(h, attribute_store, 
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
      return WRATHNew WRATHDefaultStrokeAttributePackerT();
    }
  };

  explicit
  WRATHDefaultStrokeAttributePackerT(void):
    WRATHShapeAttributePacker<T>(typeid(WRATHDefaultStrokeAttributePackerT).name(),
                                 WRATHDefaultStrokeAttributePacker::attribute_names().begin(),
                                 WRATHDefaultStrokeAttributePacker::attribute_names().end())
  {}

};

/*!\typedef WRATHDefaultStrokeAttributePackerF
  Conveniance typedef to WRATHDefaultStrokeAttributePackerT\<float\>
 */
typedef WRATHDefaultStrokeAttributePackerT<float> WRATHDefaultStrokeAttributePackerF;

/*!\typedef WRATHDefaultStrokeAttributePackerI
  Conveniance typedef to WRATHDefaultStrokeAttributePackerT\<int\>
 */
typedef WRATHDefaultStrokeAttributePackerT<int> WRATHDefaultStrokeAttributePackerI;

/*! @} */


#endif

