/*! 
 * \file WRATHDefaultFillShapeAttributePacker.hpp
 * \brief file WRATHDefaultFillShapeAttributePacker.hpp
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




#ifndef __WRATH_DEFAULT_FILL_SHAPE_ATTRIBUTE_PACKER_HPP__
#define __WRATH_DEFAULT_FILL_SHAPE_ATTRIBUTE_PACKER_HPP__

#include "WRATHConfig.hpp"
#include "WRATHShapeAttributePacker.hpp"
#include "WRATHShapeTriangulator.hpp"
#include "WRATHFillRule.hpp"


/*! \addtogroup Shape
 * @{
 */

/*!\namespace WRATHDefaultFillShapeAttributePacker
  The actual implementation of creating the attributes
  from a WRATHDefaultFillShapeAttributePacker does NOT depend
  on the type T of WRATHShape<T> template parameter.
 */
namespace WRATHDefaultFillShapeAttributePacker
{
  enum
    {
      /*!
        location of draw position (x,y), 
        a vec2 (in GLSL). Attribute name in 
        GLSL is "pos".
      */
      position_location=0,
    };

  
  
  
  /*!\namespace WRATHDefaultFillShapeAttributePacker::FillingTypes
    Namespace to encapsulate the parameters that
    define filling.
   */
  namespace FillingTypes
  {
    /*!class FillingParameters
      A FillingParameters object specifies the fill rule
      and a translation that is applied to the WRATHShape<T>
      to be filled.
    */
    class FillingParameters:
      public WRATHShapeAttributePackerBase::PackingParametersBase
    {
    public:
      /*!\fn FillingParameters(const vec2&, WRATHFillRule::fill_rule)    
        Ctor.
        \param fn fill rule
        \param tr translation to apply
      */
      FillingParameters(const vec2 &tr=vec2(0.0f),
                        WRATHFillRule::fill_rule fn=WRATHFillRule::non_zero_rule):
        m_translate(tr),
        m_fill_rule(fn)
      {}
      
      /*!\fn FillingParameters(float, float, WRATHFillRule::fill_rule)       
        Ctor.
        \param x x-translation to apply
        \param y y-translation to apply
        \param fn fill rule
      */
      FillingParameters(float x, float y,
                        WRATHFillRule::fill_rule fn=WRATHFillRule::non_zero_rule):
        m_translate(x,y),
        m_fill_rule(fn)
      {}

      /*!\fn bool fill
        Provided as a readability conveniance, equivalent to
        \code
        m_fill_rule(winding_number)
        \endcode
        \param winding_number winding number to evaluate
       */
      bool
      fill(int winding_number) const
      {
        return m_fill_rule(winding_number);
      }
      
      /*!\var m_translate  
        Amount by which to translate the shape.
      */
      vec2 m_translate;

      /*!\var m_fill_rule
        Specifies the fill rule.
       */
      WRATHFillRule::fill_rule m_fill_rule;
    };

    /*!\typedef NonZeroWindingFill
      Conveniance typedef where fill rule defaults to
      the non-zero winding fill rule.
     */
    typedef FillingParameters NonZeroWindingFill;

    /*!\class OddEvenFill
      Conveniance class where fill rule defaults to
      the odd-even winding fill rule.
     */
    class OddEvenFill:public FillingParameters
    {
    public:
      /*!\fn OddEvenFill(const vec2&)        
        Ctor.
        \param tr translation to apply
      */
      OddEvenFill(const vec2 &tr=vec2(0.0f)):
        FillingParameters(tr, WRATHFillRule::odd_even_rule)
      {}
     
      /*!\fn OddEvenFill(float, float)    
        Ctor.
        \param x x-translation to apply
        \param y y-translation to apply
      */
     OddEvenFill(float x, float y):
       FillingParameters(x, y, WRATHFillRule::odd_even_rule)
      {}
    };
  }

  using namespace FillingTypes;


  /*!\fn WRATHShapeAttributePackerBase::allocation_requirement_type allocation_requirement(WRATHShapeTriangulatorPayload::handle, const FillingParameters &)
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

  /*!\fn void set_attribute_data(WRATHShapeTriangulatorPayload::handle,
                                 WRATHAbstractDataSink &attribute_store,
                                 const std::vector<range_type<int> >&,
                                 WRATHAbstractDataSink*,
                                 const FillingParameters&)
    Writes attribute and index data to given WRATHAbstractDataSink
    object from a given a given WRATHShapeTriangulatorPayload.
    \param payload handle to a WRATHShapeTriangulatorPayload, method
                   WRATHassert 's for handle to be valid.
    \param attribute_store WRATHAbstractDataSink to which to write attribute data
    \param attr_location attribute locations to use for attribute data
    \param index_group WRATHAbstractDataSink to which to write index data.
                       Will write index as GLushort's.
    \param fill_params provides a translation and fill rule used                   
                       filling the WRATHShape<T> that was used
                       to create payload
   */
  void
  set_attribute_data(WRATHShapeTriangulatorPayload::handle payload,
                     WRATHAbstractDataSink &attribute_store,
                     const std::vector<range_type<int> > &attr_location,
                     WRATHAbstractDataSink *index_group,
                     const FillingParameters &fill_params);

  /*!\fn GLenum attribute_key(WRATHAttributeStoreKey&)  
    Attribute key for the packing of a filled shape.
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

/*!\class WRATHDefaultFillShapeAttributePackerT
  A WRATHDefaultFillShapeAttributePackerT is an attribute
  packer for the filling of paths. It requires that the payload
  passed to it can be dynamic casted to a \ref WRATHShapeTriangulatorPayload.
  \tparam T the template parameter for the WRATHShape type
 */
template<typename T>
class WRATHDefaultFillShapeAttributePackerT:public WRATHShapeAttributePacker<T>
{
public:

  /*!\fn WRATHShapeAttributePacker<T>* fetch
    For each type T, onle one WRATHDefaultFillShapeAttributePackerT<T> object
    exists, use fetch() to get that object.
   */
  static
  WRATHShapeAttributePacker<T>*
  fetch(void)
  {
    return WRATHAttributePacker::fetch_make<WRATHDefaultFillShapeAttributePackerT>(Factory());
  }

  virtual
  WRATHShapeAttributePackerBase::allocation_requirement_type
  allocation_requirement(const WRATHShape<T> *pshape,
                         WRATHShapeProcessorPayload payload,
                         const WRATHShapeAttributePackerBase::PackingParametersBase &pp,
                         const WRATHStateBasedPackingData::handle&) const
  {
    WRATHShapeTriangulatorPayload::handle h;
    const WRATHDefaultFillShapeAttributePacker::FillingParameters *pptr;
    const WRATHDefaultFillShapeAttributePacker::FillingParameters v;

    pptr=dynamic_cast<const WRATHDefaultFillShapeAttributePacker::FillingParameters*>(&pp);
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

    return WRATHDefaultFillShapeAttributePacker::allocation_requirement(h, *pptr);
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
    return WRATHDefaultFillShapeAttributePacker::attribute_key(attrib_key);
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
    WRATHShapeTriangulatorPayload::handle h;
    const WRATHDefaultFillShapeAttributePacker::FillingParameters *pptr;
    const WRATHDefaultFillShapeAttributePacker::FillingParameters v;

    h=payload.dynamic_cast_handle<WRATHShapeTriangulatorPayload>();
    pptr=dynamic_cast<const WRATHDefaultFillShapeAttributePacker::FillingParameters*>(&pp);
    
    if(pptr==NULL)
      {
        pptr=&v;
      }

    if(!h.valid())
      {
        payload=default_payload(pshape);
        h=payload.dynamic_cast_handle<WRATHShapeTriangulatorPayload>();
      }

   
    WRATHDefaultFillShapeAttributePacker::set_attribute_data(h, attribute_store, 
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
      return WRATHNew WRATHDefaultFillShapeAttributePackerT();
    }

  };

  explicit
  WRATHDefaultFillShapeAttributePackerT(void):
    WRATHShapeAttributePacker<T>(typeid(WRATHDefaultFillShapeAttributePackerT).name(), 
                                 WRATHDefaultFillShapeAttributePacker::attribute_names().begin(), 
                                 WRATHDefaultFillShapeAttributePacker::attribute_names().end())
  {}

};

/*!\typedef WRATHDefaultFillShapeAttributePackerF
  Conveniance typedef to WRATHDefaultFillShapeAttributePackerT\<float\>
 */
typedef WRATHDefaultFillShapeAttributePackerT<float> WRATHDefaultFillShapeAttributePackerF;

/*!\typedef WRATHDefaultFillShapeAttributePackerI
  Conveniance typedef to WRATHDefaultFillShapeAttributePackerT\<int\>
 */
typedef WRATHDefaultFillShapeAttributePackerT<int> WRATHDefaultFillShapeAttributePackerI;



/*! @} */


#endif
