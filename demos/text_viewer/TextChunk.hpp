/*! 
 * \file TextChunk.hpp
 * \brief file TextChunk.hpp
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


#ifndef TEXT_CHUNK_HPP
#define TEXT_CHUNK_HPP

#include "WRATHConfig.hpp"
#include <fstream>
#include <iomanip>
#include <limits>
#include <dirent.h>

#include "vecN.hpp"
#include "WRATHNew.hpp"
#include "WRATHLayer.hpp"
#include "WRATHLayerItemNodeRotateTranslate.hpp"
#include "WRATHDefaultTextAttributePacker.hpp"
#include "WRATHTextItem.hpp"

const int underline_stream_id=2;
const int strikethrough_stream_id=3;
typedef bool line_stream_type;

const int underline_pos_location=0;
const int underline_color_location=1;
const int underline_index_location=2;

class FilePacket;

//!\class TextChunk
/*!
  A TextChunk represents a portion of a document,
  the portion is cullable as a whole as it is
  a part of its own WRATHLayer
 */
class TextChunk
{
public:

  TextChunk(range_type<int> R,
            const WRATHFormattedTextStream &ptext,
            const WRATHStateStream &state_stream,
            WRATHLayer *pparent, 
            FilePacket *fpacket,
            WRATHLayerItemNodeRotateTranslate *text_transformation_node);

  virtual
  ~TextChunk();

  void
  visible(bool v)
  {
    m_sub->visible(v);
  }

  const WRATHTextAttributePacker::BBox&
  bbox(void)
  {
    return m_bbox;
  }

private:

  class LinePacketData:boost::noncopyable
  {
  public:
    LinePacketData(void);
    ~LinePacketData();

    bool m_has_underlines;
    WRATHCanvas::DataHandle m_item_group;
    WRATHIndexGroupAllocator::index_group<GLushort> m_index_data_location;
    range_type<int> m_attribute_data_location;
    unsigned int m_number_attributes;
  };

  class per_line_data
  {
  public:
    per_line_data(void):
      m_x(0.0f,0.0f),
      m_y(0.0f), 
      m_color(255,255,255,255),
      m_character_range(0,0),
      m_max_ascend(0.0f)
    {}

    range_type<float> m_x;
    float m_y;
    vecN<GLubyte,4> m_color;
    range_type<int> m_character_range;
    float m_max_ascend;

    friend
    std::ostream&
    operator<<(std::ostream& ostr, const per_line_data &obj)
    {
      ostr << "x=" << obj.m_x
           << " y=" << obj.m_y
           << " R=" << obj.m_character_range
           << " MA=" << obj.m_max_ascend;
      return ostr;
    }
  };

  void
  add_underlines(range_type<int> R,
                 const WRATHFormattedTextStream &ptext,
                 const WRATHStateStream &state_stream,
                 WRATHLayer *pparent,
                 const WRATHSubItemDrawState &pextra_state,
                 WRATHItemDrawer *pline_drawer,
                 WRATHLayerItemNodeRotateTranslate *text_node);

  void
  find_line_ranges(int stream_id,
                   range_type<int> R,
                   const WRATHStateStream &state_stream,
                   std::list< range_type<int> > &outline_ranges);

  void
  compute_lines(const WRATHFormattedTextStream &ptext,
                const WRATHStateStream &state_stream,
                const std::list< range_type<int> > &underline_ranges,
                std::list<per_line_data> &out_lines);

  void
  create_underlines(const std::list<per_line_data> &in_lines,
                    WRATHLayer *pparent,
                    WRATHItemDrawer *pline_drawer,
                    const WRATHSubItemDrawState &pextra_state,
                    WRATHLayerItemNodeRotateTranslate *text_node);

  WRATHTextItem *m_text_item;
  WRATHLayer *m_sub;
  WRATHLayerItemNodeRotateTranslate *m_vis;
  WRATHTextAttributePacker::BBox m_bbox;

  LinePacketData m_lines;
};


#endif
