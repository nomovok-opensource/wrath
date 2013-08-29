/*! 
 * \file WRATHDefaultStrokeAttributePacker.cpp
 * \brief file WRATHDefaultStrokeAttributePacker.cpp
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
#include "WRATHDefaultStrokeAttributePacker.hpp"
#include "WRATHInterleavedAttributes.hpp"
#include "WRATHGenericStrokeAttributePacker.hpp"
#include "WRATHStaticInit.hpp"


namespace
{
  class attribute_type:
    public WRATHInterleavedAttributes<vec2, float>
  {
  public:

    void
    position(const vec2 &v)
    {
      get<WRATHDefaultStrokeAttributePacker::position_location>()=v;
    }

    void
    hint(float I)
    {
      get<WRATHDefaultStrokeAttributePacker::hint_distance_location>()=static_cast<float>(I);
    }
  };

  template<typename A>
  class AttributeMaker
    :public WRATHGenericStrokeAttributePacker::OutputAttributeProducer
  {
  public:
    WRATHDefaultStrokeAttributePacker::StrokingParameters m_stroke_params;

    explicit
    AttributeMaker(const WRATHDefaultStrokeAttributePacker::StrokingParameters &pp):
      m_stroke_params(pp)
    {}


    template<typename T>
    void
    generate_attribute(c_array<uint8_t> output_destination,
                       const T &pt) const
    {
      c_array<A> ptr(output_destination.reinterpret_pointer<A>());
      A &R(ptr[0]);
      vec2 v;

      v=pt.offset_vector(m_stroke_params.m_miter_limit);
      R.position(pt.pre_position() 
                 + m_stroke_params.m_radius*v
                 + m_stroke_params.m_translate);

      if(dot(v,v)>0.1f)
        {
          R.hint(1.0f);
        }
      else
        {
          R.hint(0.0f);
        }
    }

    virtual
    int
    attribute_size(void) const { return sizeof(A); }

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
                               const CurvePoint &input_pt, int) const
    {
      float wf(hf*m_stroke_params.m_radius);
      c_array<A> ptr;

      ptr=output_destination.reinterpret_pointer<A>();
      A &R(ptr[0]);

      R.position(input_pt.position()
                 + wf*input_pt.normal()
                 + m_stroke_params.m_translate);
      
      R.hint(hf);
    }
  };

  

  typedef const char *attribute_label_type;

}


/////////////////////////////////////
// WRATHDefaultStrokeAttributePacker methods
WRATHGenericStrokeAttributePacker::StrokingParameters
WRATHDefaultStrokeAttributePacker::StrokingParameters::
generate_generic_parameters(void) const
{
  WRATHGenericStrokeAttributePacker::StrokingParameters return_value;
  
  return_value
    .generate_flags(0)
    .close_outline(m_close_outline)
    .stroke_curves(m_stroke_curves);
  
  const uint32_t join_flags[]=
    {
      WRATHShapePreStrokerPayload::generate_bevel_joins, // bevel_join
      WRATHShapePreStrokerPayload::generate_miter_joins, // miter_join
      WRATHShapePreStrokerPayload::generate_rounded_joins, // round_join
      0, // no_join
    };
  
  const uint32_t cap_flags[]=
    {
      WRATHShapePreStrokerPayload::generate_square_caps, // square_cap
      0, // flat_cap
      WRATHShapePreStrokerPayload::generate_rounded_caps, // rounded_cap
    };
  
  if(m_join_style<4)
    {
      return_value.m_generate_flags|=join_flags[m_join_style];
    }
  
  if(!m_close_outline and m_cap_style<3)
    {
      return_value.m_generate_flags|=cap_flags[m_cap_style];
    }
  
  return return_value;
}


const_c_array<attribute_label_type>
WRATHDefaultStrokeAttributePacker::
attribute_names(void)
{
  /*
    NOTE! that the attribute name for the position
    is the same as for WRATHDefaultFillShapeAttributePacker,
    this way they can share the same shaders.
   */
  static const attribute_label_type attribute_labels[]=
    {
      "pos",
      "in_aa_hint",
    };
  return const_c_array<attribute_label_type>(attribute_labels, 2);
}


WRATHShapeAttributePackerBase::allocation_requirement_type
WRATHDefaultStrokeAttributePacker::
allocation_requirement(WRATHShapePreStrokerPayload::handle h,
                       const StrokingParameters &pp)
{
  WRATHShapeAttributePackerBase::allocation_requirement_type A;
  WRATHGenericStrokeAttributePacker::StrokingParameters gen(pp.generate_generic_parameters());

  return WRATHGenericStrokeAttributePacker::allocation_requirement(h, gen, false);
}


void
WRATHDefaultStrokeAttributePacker::
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
                                                        AttributeMaker<attribute_type>(pp),
                                                        gen, false);
    
}

GLenum
WRATHDefaultStrokeAttributePacker::
attribute_key(WRATHAttributeStoreKey &attrib_key)
{
  attrib_key.type_and_format(type_tag<attribute_type>());
  return GL_TRIANGLES;
}
