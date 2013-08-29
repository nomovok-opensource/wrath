/*! 
 * \file WRATHSVGFont.hpp
 * \brief file WRATHSVGFont.hpp
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


#ifndef __WRATH_SVG_FONT_HPP__
#define __WRATH_SVG_FONT_HPP__

#include "WRATHConfig.hpp"
#include <stdint.h>
#include <boost/utility.hpp>
#include "WRATHgl.hpp"
#include "vecN.hpp"
#include "vectorGL.hpp"
#include "WRATHNew.hpp"
#include "WRATHResourceManager.hpp"
#include "WRATHInterleavedAttributes.hpp"

/*! \addtogroup Text
 * @{
 */

//!\class WRATHSVGFont
/*!
  A WRATHSVGFont is a base class interface
  for fonts who store glyphs as a set of triangles
  and splines to be rasterized. These fonts require
  more memory per letter instance (since a point has
  many points) but behave much better under 
  magnification than WRATHTextureFont 's.  
 */
class WRATHSVGFont:public boost::noncopyable
{
public:

  WRATH_RESOURCE_MANAGER_DECLARE(WRATHSVGFont);

  
  //!\class point
  /*!
    Questions:
      1) doing GLshort gives points a range of [-2^15, 2^15) is such range necessary?
      2) the value for the texture co-ordinate are, in decimal, 0, 1/2 or 1, thus
         GLushort is overkill. If we use GLshort for the position, we gain nothing
         in terms of memory consumption by using GLubyte for the texes since
         the type needs to be 4 byte alinged anyways.

     Maybe GLbyte for the points and GLubyte for the texes? that gives a range
     of [-128,127] for the point position, which should be fine up to a zoom factor
     of atleast 8.

     On the other hand, this data type will NOT be used directly by a renderer,
     since a rendere will also have a color, but such a color is 4bytes wide
     anyways (vecN<GLubyte,4>) so...
   */
  class point:public WRATHInterleavedAttributes< vecN<GLshort, 2>, vecN<GLushort,2> >
  {
  public:
    enum
      {
        point_location=0,
        tex_location=1
      };

    point(void)
    {}

    vecN<GLshort, 2>&
    point(void)
    {
      return get<point_location>();
    }

    const vecN<GLshort, 2>&
    point(void) const
    {
      return get<point_location>();
    }

    vecN<GLushort, 2>&
    tex(void)
    {
      return get<tex_location>();
    }

    const vecN<GLushort, 2>&
    tex(void) const
    {
      return get<tex_location>();
    }
  };


  //!\class character_data_type
  /*!
   */
  class character_data_type
  {
  public:
    
     
    character_data_type(void):
      m_size(-1,-1),
      m_advance(-1)
    {}
    
       
    /*!\var m_points
    
      Points that make the glyph.
     */ 
    std::vector<point> m_points;
    
    /*!\var m_indices
    
      Indices giving the triangle "commands"
     */
    std::vector<GLushort> m_indices;

    /*!\var m_size
    
      Size of glyph, in same units as m_points.
     */
    ivec2 m_size;

    /*!\var m_advance
    
      Amount to advance pen position horizontally
      after drawing the glyph, in same units as m_points.
     */
    int m_advance;

    /*!\fn height
    
      Returns the height of the glyph,
      equivalent to m_size.y()
     */
    int
    height(void) const
    {
      return m_size.y();
    }

    /*!\fn width
    
      Returns the width of the glyph,
      equivalent to m_size.x()
     */
    int
    width(void) const
    {
      return m_size.x();
    }    
  };

  /*!\fn WRATHSVGFont
  
    Ctor, registers the WRATHSVGFont to
    the resource manager of WRATHSVGFont.
   */
  explicit
  WRATHSVGFont(const std::string &pname);

  virtual
  ~WRATHSVGFont();

  /*!\fn new_line_height
  
    To be implemented a derived class to
    return the height of a line, i.e. the 
    distance to advance the pen's y-position
    to advance on a new-line.
   */
  virtual
  int
  new_line_height(void)=0;

  /*!\fn character_data
  
    To be implemented a derived class to
    give the character data for the named glyph.

    \param glyph ascii code of glyph to provide information about,
                 for example character_data('A') is to return
                 character data for the glyph A.
   */
  virtual
  character_data_type
  character_data(uint32_t glyph)=0;

  /*!\fn space_width
  
    Returns the width of the 
    space character, equivalent to
    character_data(' ').width().
   */  
  int
  space_width(void)
  {
    return character_data(' ').width();
  }

  /*!\fn tab_width
  
    Returns the width of the 
    "tab" character, equivalent to
    4*space_width().
   */  
  int
  tab_width(void)
  {
    return space_width()*4;
  }

  /*!\fn resource_name
  
   Returns the resoruce name of the font.
   */  
  const std::string& 
  resource_name(void) const
  {
    return m_name;
  }

private:
  std::string m_name;
};

/*! @} */




#endif
