/*! 
 * \file WRATHDynamicStrokeAttributePacker.cpp
 * \brief file WRATHDynamicStrokeAttributePacker.cpp
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



#include "WRATHConfig.hpp"
#include "WRATHDynamicStrokeAttributePacker.hpp"
#include "WRATHInterleavedAttributes.hpp"
#include "WRATHGenericStrokeAttributePacker.hpp"


namespace
{
  class attribute_type:
    public WRATHInterleavedAttributes<vec2, vec2, float>
  {
  public:

    const vec2&
    position(void) const
    {
      return get<WRATHDynamicStrokeAttributePacker::pre_position_location>();
    }

    vec2&
    position(void)
    {
      return get<WRATHDynamicStrokeAttributePacker::pre_position_location>();
    }

    const vec2&
    normal(void) const
    {
      return get<WRATHDynamicStrokeAttributePacker::normal_location>();
    }

    vec2&
    normal(void)
    {
      return get<WRATHDynamicStrokeAttributePacker::normal_location>();
    }

    const float&
    aa_hint(void) const
    {
      return get<WRATHDynamicStrokeAttributePacker::aa_hint_location>();
    }

    float&
    aa_hint(void) 
    {
      return get<WRATHDynamicStrokeAttributePacker::aa_hint_location>();
    }
    
  };

  class AttributeMaker
    :public WRATHGenericStrokeAttributePacker::OutputAttributeProducer
  {
  public:
    WRATHDynamicStrokeAttributePacker::StrokingParameters m_stroke_params;

    AttributeMaker(const WRATHDynamicStrokeAttributePacker::StrokingParameters &pp):
      m_stroke_params(pp)
    {}

    template<typename T>
    void
    generate_attribute(c_array<uint8_t> output_destination,
                       const T &pt) const
    {
      c_array<attribute_type> ptr(output_destination.reinterpret_pointer<attribute_type>());
      attribute_type &R(ptr[0]);
      float v;

      R.position()=pt.pre_position() + m_stroke_params.m_translate;
      R.normal()=pt.offset_vector(m_stroke_params.m_miter_limit);

      v=dot(R.normal(), R.normal());
      R.aa_hint()=(v>0.0001f)?1.0f:0.0f;
    }

    virtual
    int
    attribute_size(void) const { return sizeof(attribute_type); }

    virtual
    void
    generate_attribute_miter(c_array<uint8_t> output_destination,
                             const MiterJoinPoint &input_pt, int) const
    {
      generate_attribute(output_destination, input_pt);
    }
    
    virtual
    void
    generate_attribute_bevel(c_array<uint8_t> output_destination,
                             const JoinPoint &input_pt, int) const
    {
      generate_attribute(output_destination, input_pt);
    }

    virtual
    void
    generate_attribute_round(c_array<uint8_t> output_destination,
                             const JoinPoint &input_pt, int) const
    {
      generate_attribute(output_destination, input_pt);
    }

    virtual
    void
    generate_attribute_cap(c_array<uint8_t> output_destination,
                           const CapPoint &input_pt, int) const
    {
      generate_attribute(output_destination, input_pt);
    }

    void
    generate_attribute_edge_pt(c_array<uint8_t> output_destination,
                               float hf,
                               const CurvePoint &pt, int) const
    {
      c_array<attribute_type> ptr(output_destination.reinterpret_pointer<attribute_type>());
      attribute_type &R(ptr[0]);

      R.position()=pt.position() + m_stroke_params.m_translate;
      R.normal()=hf*pt.normal();
      R.aa_hint()=hf;
    }
  };

  typedef const char *attribute_label_type;

}

const_c_array<attribute_label_type>
WRATHDynamicStrokeAttributePacker::
attribute_names(void)
{
  static const attribute_label_type attribute_labels[]=
    {
      "pos",
      "normal",
      "in_aa_hint"
    };
  static const_c_array<attribute_label_type> R(attribute_labels, 3);
  return R;
}

WRATHShapeAttributePackerBase::allocation_requirement_type
WRATHDynamicStrokeAttributePacker::
allocation_requirement(WRATHShapePreStrokerPayload::handle h,
                       const StrokingParameters &pp)
{
  WRATHShapeAttributePackerBase::allocation_requirement_type A;
  WRATHGenericStrokeAttributePacker::StrokingParameters gen(pp.generate_generic_parameters());

  return WRATHGenericStrokeAttributePacker::allocation_requirement(h, gen, false);
}



void
WRATHDynamicStrokeAttributePacker::
set_attribute_data(WRATHShapePreStrokerPayload::handle h,
                   WRATHAbstractDataSink &attribute_store,
                   const std::vector<range_type<int> > &attr_location,
                   WRATHAbstractDataSink *index_group,
                   const StrokingParameters &pp) 
{
  WRATHGenericStrokeAttributePacker::StrokingParameters gen(pp.generate_generic_parameters());
  WRATHGenericStrokeAttributePacker::set_attribute_data(h,
                                                        attribute_store,
                                                        attr_location,
                                                        index_group,
                                                        AttributeMaker(pp),
                                                        gen, false);
}

GLenum
WRATHDynamicStrokeAttributePacker::
attribute_key(WRATHAttributeStoreKey &attrib_key)
{
  attrib_key.type_and_format(type_tag<attribute_type>());
  return GL_TRIANGLES;
}
