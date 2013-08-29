/*! 
 * \file WRATHGenericStrokeAttributePacker.hpp
 * \brief file WRATHGenericStrokeAttributePacker.hpp
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

#ifndef __WRATH_GENERIC_STROKE_ATTRIBUTE_PACKER_HPP__
#define __WRATH_GENERIC_STROKE_ATTRIBUTE_PACKER_HPP__

#include "WRATHConfig.hpp"
#include "WRATHShapeAttributePacker.hpp"
#include "WRATHShapePreStroker.hpp"

/*!\namespace WRATHGenericStrokeAttributePacker 
  Provides an interface which "walks" the data
  of a WRATHShapePreStrokerPayload object calling
  user provided functions to set attribute data
  for points of a WRATHShapePreStrokerPayload.
 */
namespace WRATHGenericStrokeAttributePacker
{
  /*!\namespace WRATHGenericStrokeAttributePacker::CurveStrokingTypes
    namespace to encapsulate stroking types.
   */
  namespace CurveStrokingTypes
  {
    /*!\enum pen_style_type
      Enumeration to specify how
      the curves connecting points
      of a WRATHShape are stroked
     */
    enum pen_style_type 
      {
        /*!
          Indicates to NOT stroke the curves
         */
        no_stroke,

        /*!
          Indicate to stroke the curves as 
          solid
         */
        solid_stroke,

        /*!
          Indicate to stroke the curves with
          spaced dots
         */
        dotted_stroke,

        /*!
          Indicate to stroke the curves with
          spaced dashed
         */
        dashed_stroke,
      };

    /*!\enum outline_close_type
      An enumeration specifying how
      the WRATHOutline objects of a WRATHShape 
      should be stroked closed or open.
     */
    enum outline_close_type
      {
        /*!
          When stroking, stroke each outline
          as left open
         */
        each_outline_open,

        /*!
          When stroking, stroke each outline
          as closed
         */
        each_outline_closed,
        
        /*!
          When stroking, an outline is closed
          if it is both present in a table AND
          that value is true
         */
        outline_entry_default_open,
        
        /*!
          When stroking, an outline is open
          if it is both present in a table AND
          that value is false
         */
        outline_entry_default_closed,

      };
  };

  
  /*
    import name space specifying curve stroking
  */
  using namespace WRATHGenericStrokeAttributePacker::CurveStrokingTypes;

    
  /*!class StrokingParameters
    A PackingParameters objects holds stroking 
    parameters used by set_attribute_data():
    - which join and cap styles to generate attribute data
    - weather or not to close the outlines
    - weather or not to stroke the curves
   */
  class StrokingParameters
  {
  public:
   
    /*!\fn StrokingParameters
      Ctor.
     */
    StrokingParameters(void):
      m_generate_flags(WRATHShapePreStrokerPayload::generate_bevel_joins),
      m_stroke_curves(solid_stroke),
      m_close_outline(each_outline_open)
    {}
    
    /*!\fn StrokingParameters& close_outline(bool)
      Sets \ref m_close_outline.
      \param v value to use
    */
    StrokingParameters&
    close_outline(bool v)
    {
      m_close_outline=v?each_outline_closed:each_outline_open;
      return *this;
    }

    /*!\fn StrokingParameters& close_outline(enum outline_close_type)
      Sets \ref m_close_outline.
      \param v value to use
    */
    StrokingParameters&
    close_outline(enum outline_close_type v)
    {
      m_close_outline=v;
      return *this;
    }

    /*!\fn StrokingParameters& close_outline(int, bool)
      Adds an entry to \ref m_per_outline_close
      indicating to stroke the specified
      outline closed or open.
      \param outlineID ID of outline (see WRATHOutline::ID())
      \param v true indicates to stroke closed, false indicates
               to stroke open
     */
    StrokingParameters&
    close_outline(int outlineID, bool v)
    {
      m_per_outline_close[outlineID]=v;
      return *this;
    }

    /*!\fn StrokingParameters& generate_flags
      Sets \ref m_generate_flags 
      \param v value to use
    */
    StrokingParameters&
    generate_flags(uint32_t v)
    {
      m_generate_flags=v;
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
    
    /*!\fn bool stroke_closed
      Provided as a conveniance to return 
      true if and only if the outline of the
      passed outline ID is to be stroked.
      \param outlineID ID of outline to query
     */
    bool
    stroke_closed(int outlineID) const
    {
      std::map<int, bool>::const_iterator iter;
      bool use_map(m_close_outline==outline_entry_default_open
                   or m_close_outline==outline_entry_default_closed);
      bool vO, vC;

      iter=use_map?
        m_per_outline_close.find(outlineID):
        m_per_outline_close.end();

      vO=(iter!=m_per_outline_close.end())?
        iter->second:false;

      vC=(iter!=m_per_outline_close.end())?
        iter->second:true;

      return (m_close_outline==each_outline_closed)
        or (m_close_outline==outline_entry_default_closed and vC)
        or (m_close_outline==outline_entry_default_open and vO);
    }

    /*!\var m_generate_flags
      Dictates if and which joins and cap styles generate 
      attributes, uses the same bitflags as \ref 
      WRATHShapePreStrokerPayload::PayloadParams::m_flags,
      Default value is \ref WRATHShapePreStrokerPayload::generate_bevel_joins.
      See also
      \ref WRATHShapePreStrokerPayload::generate_square_caps, 
      \ref WRATHShapePreStrokerPayload::generate_rounded_caps, 
      \ref WRATHShapePreStrokerPayload::generate_caps,
      \ref WRATHShapePreStrokerPayload::generate_miter_joins, 
      \ref WRATHShapePreStrokerPayload::generate_bevel_joins, 
      \ref WRATHShapePreStrokerPayload::generate_rounded_joins,
      and \ref WRATHShapePreStrokerPayload::generate_all
    */
    uint32_t m_generate_flags;
    
    /*!\var m_stroke_curves
      If is WRATHGenericStrokeAttributePacker::CurveStrokingTypes::no_stroke, 
      attributes are not generated for the stroking of curves, only the stroking
      of the joins and caps (see \ref m_generate_flags)
      are generated. Default value is WRATHGenericStrokeAttributePacker::CurveStrokingTypes::solid_stroke.
    */
    enum pen_style_type m_stroke_curves;
    
    /*!\var m_close_outline
      Controls the closing of stroking the WRATHOutline
      objects of a WRATHShape. If the value is \ref
      WRATHGenericStrokeAttributePacker::CurveStrokingTypes::outline_entry_default_open
      then only those outlines present in \ref m_per_outline_close
      for which the value is true are closed. If the
      value is \ref 
      WRATHGenericStrokeAttributePacker::CurveStrokingTypes::outline_entry_default_closed only
      those outlines presents in \ref m_per_outline_close
      for which the value is false are open. A value
      of 
      \ref WRATHGenericStrokeAttributePacker::CurveStrokingTypes::each_outline_open 
      dictates that each outline is stroked open and \ref m_per_outline_close
      is ignored.  A value of \ref 
      WRATHGenericStrokeAttributePacker::CurveStrokingTypes::each_outline_closed
      dictates that each outline is stroked closed and 
      \ref m_per_outline_close is ignored. 

      Default value is WRATHGenericStrokeAttributePacker::CurveStrokingTypes::each_outline_open.
    */
    enum outline_close_type m_close_outline;

    /*!\var m_per_outline_close
     If \ref m_close_outline is \ref 
     WRATHGenericStrokeAttributePacker::CurveStrokingTypes::outline_entry_default_open
     or \ref 
     WRATHGenericStrokeAttributePacker::CurveStrokingTypes::outline_entry_default_closed, 
     specifies for each WRATHOutline of a WRATHShape it to be
     stroked open or clsed. The map is keyed by the outline
     ID (WRATHOutline::ID() ).
     */
    std::map<int, bool> m_per_outline_close;
  };

  /*!\class OutputAttributeProducer
    Pure virtual function of whose methods 
    \ref set_attribute_data() will call
    to fill attribute data.
   */
  class OutputAttributeProducer
  {
  public:
    /*!\typedef MiterJoinPoint
      Conviance typedef to WRATHShapePreStrokerPayload::MiterJoinPoint
     */
    typedef WRATHShapePreStrokerPayload::MiterJoinPoint MiterJoinPoint;

    /*!\typedef JoinPoint
      Conviance typedef to WRATHShapePreStrokerPayload::JoinPoint
     */
    typedef WRATHShapePreStrokerPayload::JoinPoint JoinPoint;

    /*!\typedef CapPoint
      Conviance typedef to WRATHShapePreStrokerPayload::CapPoint
     */
    typedef WRATHShapePreStrokerPayload::CapPoint CapPoint;

    /*!\typedef CurvePoint
      Conviance typedef to WRATHShapeSimpleTessellatorPayload::CurvePoint
     */
    typedef WRATHShapeSimpleTessellatorPayload::CurvePoint CurvePoint;

    virtual
    ~OutputAttributeProducer()
    {}

    /*!\fn int attribute_size
      To be implemented by a derived class
      to return the number of bytes per
      attribute
     */
    virtual
    int
    attribute_size(void) const=0;

    /*!\fn void generate_attribute_miter
      To be implemented by a derived class
      to fill the bytes with a single attribute
      from a data point
      \param output_destination location to place attribute data,
                                size of the array is exactly \ref attribute_size().
      \param input_pt input point for attribute data
      \param attribute_index index of the attribute written to output_destination,
                             save this value (if need be) for later post processing
     */
    virtual
    void
    generate_attribute_miter(c_array<uint8_t> output_destination,
                             const MiterJoinPoint &input_pt,
                             int attribute_index) const=0;

    /*!\fn void generate_attribute_bevel
      To be implemented by a derived class
      to fill the bytes with a single attribute
      from a data point
      \param output_destination location to place attribute data,
                                size of the array is exactly \ref attribute_size().
      \param input_pt input point for attribute data
      \param attribute_index index of the attribute written to output_destination,
                             save this value (if need be) for later post processing
     */
    virtual
    void
    generate_attribute_bevel(c_array<uint8_t> output_destination,
                             const JoinPoint &input_pt,
                             int attribute_index) const=0;

    /*!\fn void generate_attribute_round
      To be implemented by a derived class
      to fill the bytes with a single attribute
      from a data point
      \param output_destination location to place attribute data,
                                size of the array is exactly \ref attribute_size().
      \param input_pt input point for attribute data
      \param attribute_index index of the attribute written to output_destination,
                             save this value (if need be) for later post processing
     */
    virtual
    void
    generate_attribute_round(c_array<uint8_t> output_destination,
                             const JoinPoint &input_pt,
                             int attribute_index) const=0;

    /*!\fn void generate_attribute_cap
      To be implemented by a derived class
      to fill the bytes with a single attribute
      from a data point
      \param output_destination location to place attribute data,
                                size of the array is exactly \ref attribute_size().
      \param input_pt input point for attribute data
      \param attribute_index index of the attribute written to output_destination,
                             save this value (if need be) for later post processing
     */
    virtual
    void
    generate_attribute_cap(c_array<uint8_t> output_destination,
                           const CapPoint &input_pt,
                           int attribute_index) const=0;

    /*!\fn void generate_attribute_edge_pt
      To be implemented by a derived class
      to fill the bytes with a single attribute
      from a data point
      \param output_destination location to place attribute data,
                                size of the array is exactly \ref attribute_size().
      \param input_pt input point for attribute data
      \param normal_direction_multiplier points generated from connecting
                                         between points of each WRATHOutline
                                         generate edges. Each such edge is
                                         drawn as a quad. The value of
                                         normal_direction_multiplier is always
                                         one of +1, -1 or 0. +1 indicates that  
                                         point should offsetted by width of 
                                         stroking times \ref CurvePoint::normal(), 
                                         -1 indicates it should be offsetted in the 
                                         opposite direction and 0 indicates it should 
                                         not be offsetted at all.
      \param attribute_index index of the attribute written to output_destination,
                             save this value (if need be) for later post processing
     */
    virtual
    void
    generate_attribute_edge_pt(c_array<uint8_t> output_destination,
                               float normal_direction_multiplier,
                               const CurvePoint &input_pt,
                               int attribute_index) const=0;

  };

  /*!\class OutputAttributeProducerT
    Template version where user provides methods writing
    to a reference to an attribute_type.
    \tparam attribute_type Attribute type.
   */
  template<typename attribute_type>
  class OutputAttributeProducerT:public OutputAttributeProducer
  {
  public:

    /*!\fn void attribute_miter
      To be implemented by a derived class
      to set an attribute_type value  from a data point
      \param out_value location to place attribute_type value
      \param input_pt input point for attribute data
      \param attribute_index index of the attribute written to output_destination,
                             save this value (if need be) for later post processing
     */
    virtual
    void
    attribute_miter(attribute_type &out_value, 
                    const MiterJoinPoint &input_pt,
                    int attribute_index) const=0;

    /*!\fn void attribute_bevel
      To be implemented by a derived class
      to set an attribute_type value  from a data point
      \param out_value location to place attribute_type value
      \param input_pt input point for attribute data
      \param attribute_index index of the attribute written to output_destination,
                             save this value (if need be) for later post processing
     */
    virtual
    void
    attribute_bevel(attribute_type &out_value, 
                    const JoinPoint &input_pt,
                    int attribute_index) const=0;

    /*!\fn void attribute_round
      To be implemented by a derived class
      to set an attribute_type value  from a data point
      \param out_value location to place attribute_type value
      \param input_pt input point for attribute data
      \param attribute_index index of the attribute written to output_destination,
                             save this value (if need be) for later post processing
     */
    virtual
    void
    attribute_round(attribute_type &out_value, 
                    const JoinPoint &input_pt,
                    int attribute_index) const=0;

    /*!\fn void attribute_cap
      To be implemented by a derived class
      to set an attribute_type value  from a data point
      \param out_value location to place attribute_type value
      \param input_pt input point for attribute data
      \param attribute_index index of the attribute written to output_destination,
                             save this value (if need be) for later post processing
     */
    virtual
    void
    attribute_cap(attribute_type &out_value, 
                  const CapPoint &input_pt,
                  int attribute_index) const=0;

    /*!\fn void attribute_pt
      To be implemented by a derived class
      to set an attribute_type value  from a data point
      \param out_value location to place attribute_type value
      \param input_pt input point for attribute data
      \param normal_direction_multiplier points generated from connecting
                                         between points of each WRATHOutline
                                         generate edges. Each such edge is
                                         drawn as a quad. The value of
                                         normal_direction_multiplier is always
                                         +1 or -1. +1 indicates that the point
                                         should offsetted by width of stroking times
                                         \ref CurvePoint::normal() where as
                                         -1 indicates it should be offsetted in
                                         the opposite direction.
      \param attribute_index index of the attribute written to output_destination,
                             save this value (if need be) for later post processing
     */
    virtual
    void
    attribute_pt(attribute_type &out_value, 
                 float normal_direction_multiplier,
                 const CurvePoint &input_pt,
                 int attribute_index) const=0;

    /*!\fn attribute_size
      Implements OutputAttributeProducer::attribute_size()
     */
    virtual
    int
    attribute_size(void) const
    {
      return sizeof(attribute_type);
    }

    /*!\fn generate_attribute_miter
      Implements OutputAttributeProducer::generate_attribute_miter()
     */
    virtual
    void
    generate_attribute_miter(c_array<uint8_t> output_destination,
                             const MiterJoinPoint &input_pt,
                             int attribute_index)
    {
      c_array<attribute_type> ptr(output_destination.reinterpret_pointer<attribute_type>());
      generate_miter(ptr[0], input_pt, attribute_index);
    }

    /*!\fn generate_attribute_bevel
      Implements OutputAttributeProducer::generate_attribute_bevel()
     */
    virtual
    void
    generate_attribute_bevel(c_array<uint8_t> output_destination,
                             const JoinPoint &input_pt,
                             int attribute_index)
    {
      c_array<attribute_type> ptr(output_destination.reinterpret_pointer<attribute_type>());
      generate_bevel(ptr[0], input_pt, attribute_index);
    }

    /*!\fn generate_attribute_round
      Implements OutputAttributeProducer::generate_attribute_round()
     */
    virtual
    void
    generate_attribute_round(c_array<uint8_t> output_destination,
                             const JoinPoint &input_pt,
                             int attribute_index)
    {
      c_array<attribute_type> ptr(output_destination.reinterpret_pointer<attribute_type>());
      generate_round(ptr[0], input_pt, attribute_index);
    }

    /*!\fn generate_attribute_cap
      Implements OutputAttributeProducer::generate_attribute_cap()
     */
    virtual
    void
    generate_attribute_cap(c_array<uint8_t> output_destination,
                           const CapPoint &input_pt,
                           int attribute_index)
    {
      c_array<attribute_type> ptr(output_destination.reinterpret_pointer<attribute_type>());
      generate_cap(ptr[0], input_pt, attribute_index);
    }

    /*!\fn generate_attribute_edge_pt
      Implements OutputAttributeProducer::generate_attribute_edge_pt()
     */
    virtual
    void
    generate_attribute_edge_pt(c_array<uint8_t> output_destination,
                               float normal_direction_multiplier,
                               const CurvePoint &input_pt,
                               int attribute_index)
    {
      c_array<attribute_type> ptr(output_destination.reinterpret_pointer<attribute_type>());
      generate_pt(ptr[0], input_pt, attribute_index);
    }

  };

  /*!\fn WRATHShapeAttributePackerBase::allocation_requirement_type allocation_requirement(WRATHShapePreStrokerPayload::handle, const StrokingParameters&, bool)
  
    Returns the allocation requirements (attribute
    and index room needed) for a given WRATHShapeStrokerPayload.
    \param payload handle to a WRATHShapeStrokerPayload, method
                   WRATHassert 's for handle to be valid.
    \param stroking_params Stroking parameters for the allocation
    \param draw_edges_as_double_quads if true, each edge when stroked
                                      is realized as _two_ quads, one
                                      for each side of the edge.
   */
  WRATHShapeAttributePackerBase::allocation_requirement_type
  allocation_requirement(WRATHShapePreStrokerPayload::handle payload,
                         const StrokingParameters &stroking_params,
                         bool draw_edges_as_double_quads);


  /*!\fn void set_attribute_data(WRATHShapePreStrokerPayload::handle ,
                                 WRATHAbstractDataSink &,
                                 const std::vector<range_type<int> > &,
                                 WRATHAbstractDataSink *,
                                 const OutputAttributeProducer &,
                                 const StrokingParameters &,
                                 bool)
    Writes attribute and index data to given WRATHAbstractDataSink
    object from a given WRATHShapeStrokerPayload.
    \param payload handle to a WRATHShapePreStrokerPayload, method
                   WRATHassert 's for handle to be valid.
    \param attribute_store WRATHAbstractDataSink to which to write attribute data
    \param attr_location attribute locations to use for attribute data
    \param index_group WRATHAbstractDataSink to which to write index data.
                       Will write index as GLushort's.
    \param P OutputAttributeProducer user-provided object that does
             that actual writing of attribute data
    \param stroking_params stroking parameters
    \param draw_edges_as_double_quads if true, each edge when stroked
                                      is realized as _two_ quads, one
                                      for each side of the edge.
   */
  void
  set_attribute_data(WRATHShapePreStrokerPayload::handle payload,
                     WRATHAbstractDataSink &attribute_store,
                     const std::vector<range_type<int> > &attr_location,
                     WRATHAbstractDataSink *index_group,
                     const OutputAttributeProducer &P,
                     const StrokingParameters &stroking_params,
                     bool draw_edges_as_double_quads);
};

/*! @} */
#endif
