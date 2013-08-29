/*! 
 * \file WRATHImperative.hpp
 * \brief file WRATHImperative.hpp
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


#ifndef __WRATH_IMPERATIVE_HPP__
#define __WRATH_IMPERATIVE_HPP__

#include "WRATHConfig.hpp"
#include "WRATHDefaultStrokeAttributePacker.hpp"
#include "WRATHFillRule.hpp"
#include "WRATHTextData.hpp"
#include "WRATHFormattedTextStream.hpp"
#include "WRATHTextDataStream.hpp"
#include "matrixGL.hpp"
#include <boost/utility.hpp>

/*!\namespace WRATHImperativeTypes
 */
namespace WRATHImperativeTypes
{
  /*!\class DrawnData
    DrawData is a base class data to draw.
    The actual internals of derived classes
    of it it are private to WRATHImperative
   */
  class DrawnData:public boost::noncopyable
  {
  public:
    virtual
    ~DrawnData()
    {}
  };

  /*!\class Rect
   */
  class Rect
  {
  public:
    vec2 m_position;
    vec2 m_width_height;
  };

  /*!\enum PorterDuffMode

    See
    http://ssp.impulsetrain.com/2013-03-17_Porter_Duff_Compositing_and_Blend_Modes.html
   */
  enum PorterDuffMode 
    {
      /*!
       */
      porter_duff_src,
      /*!
       */
      porter_duff_src_atop,
      /*!
       */
      porter_duff_src_over,
      /*!
       */
      porter_duff_src_in,  
      /*!
       */ 
      porter_duff_src_out,
      /*!
       */
      porter_duff_dest,
      /*!
       */
      porter_duff_dest_atop,
      /*!
       */
      porter_duff_dest_over,
      /*!
       */
      porter_duff_dest_in,
      /*!
       */
      porter_duff_dest_out,   
      /*!
       */
      porter_duff_clear,   
      /*!
       */
      porter_duff_xor,   
    };

  /*!\class Brush
   */
  class Brush:public WRATHBrushBits::BrushBits<Brush>
  {
  public:
    WRATHImage *m_image;
    WRATHGradient *m_gradient;

    /*
      TODO: how image is sampled:
      - linear/nearest (cubic maybe too?)
      - repeat mode

      TODO: how gradient color stops are used
      - linear/radial
      - repeat?

      TODO: transformation to apply to brush:
      - stretch, phase, etc.      
     */
  };

  /*
    Import:
     - WRATHDefaultStrokeAttributePacker::join_style_type
     - WRATHDefaultStrokeAttributePacker::cap_style_type
     - WRATHGenericStrokeAttributePacker::pen_style_type
     - WRATHGenericStrokeAttributePacker::outline_close_type
     - WRATHDefaultStrokeAttributePacker::StrokingParameters
   */
  using namespace WRATHDefaultStrokeAttributePacker::StrokingTypes;

  /*
     Import:
     - fill_rule_function
     - fill_rule
   */
  using namespace WRATHFillRule;
};

/*!\class WRATHImperative
  A WRATHImperative is WRATH's imperative drawing interface.
  Please have a brain and do not use this.
  The WRATHImperative drawing interface attempts
  to guide a user of it to avoid CPU recomputation
  as much as possible. The interface essentially
  operates as follows:
  - 1) one creates a data to draw a UI element
  - 2) one passes that data back to WRATHImperative to
       draw the data.

 */
class WRATHImperative:public boost::noncopyable
{
public:

  enum transform_combine_type
    {
      /*!
        Just set, do not combine
       */
      set_transform,

      /*!
       */
      concat_transform
    };

  WRATHImperative(void);

  virtual
  ~WRATHImperative();
  
  /////////////////////////////////////////////
  // transformtion jazz, using the matrix variations
  // will causes draw flushes
  void
  transform(const WRATHScaleXYTranslate &matrix, 
            enum transform_combine_type tp);

  void
  push_transform(const WRATHScaleXYTranslate &matrix, 
                 enum transform_combine_type tp);
  void
  transform(const float3x3 &matrix, 
            enum transform_combine_type tp);

  void
  push_transform(const float3x3 &matrix, 
                 enum transform_combine_type tp);

  void
  pop_transform(void);

  //////////////////////////////////////////
  // clipping, can only be pushed and popped.
  // if only clipping added is clip_in with rectangles,
  // draw call breaks are avoided across clipping changes
  void
  push_clipping(const_c_array<const WRATHImperativeTypes::DrawnData*> clip_in,
                const_c_array<const WRATHImperativeTypes::DrawnData*> clip_out);

  void
  push_clipping(const Rect &prect);

  void
  pop_clipping(void);

  ///////////////////////////////
  // default value is porter_duff_src which allows
  // for reordering and is the most common one to use.
  void
  set_composition_mode(enum WRATHImperativeTypes::PorterDuffMode);

  ///////////////////////////////////
  // draw to framebuffer with the specified brush.
  void
  draw(const WRATHImperativeTypes::DrawnData *data,
       const WRATHImperativeTypes::Brush &brush,
       const WRATHScaleXYTranslate &position=WRATHScaleXYTranslate());

  void
  flush(void);


  ////////////////////////////////////////////
  // interface to create things to draw
  static
  WRATHImperativeTypes::DrawnData*
  make_rectangle(const Rect &prect);

  static  
  WRATHImperativeTypes::DrawnData*
  make_stroked_shape(const WRATHShape<float> &pshape,
                     const WRATHImperativeTypes::StrokingParameters &params);

  static
  WRATHImperativeTypes::DrawnData*
  make_filled_shape(const WRATHShape<float> &pshape,
                    const WRATHImperativeTypes::fill_rule &prule);

  static
  WRATHImperativeTypes::DrawnData*
  make_filled_convex_shape(const WRATHShape<float> &pshape);

  static
  WRATHImperativeTypes::DrawnData*
  make_text(range_type<int> R,
            const WRATHFormattedTextStream &ptext,
            const WRATHStateStream &state_stream);

  static
  WRATHImperativeTypes::DrawnData*
  make_text(const WRATHFormattedTextStream &ptext,
            const WRATHStateStream &state_stream)
  {
    return make_text(range_type<int>(0, ptext.data_stream().size()),
                     ptext, state_stream);
  }

  static
  WRATHImperativeTypes::DrawnData*
  make_text(const WRATHTextDataStream &ptext)
  {
    return make_text(ptext.formatted_text(), ptext.state_stream());
  }            

private:
};

#endif
