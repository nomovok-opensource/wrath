/*! 
 * \file KANTextureFontTTF.hpp
 * \brief file KANTextureFontTTF.hpp
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


#ifndef __WRATH_TEXTURE_FONT_TTF_HPP__
#define __WRATH_TEXTURE_FONT_TTF_HPP__

#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "WRATHTextureFont.hpp"
#include "WRATHNew.hpp"
#include "matrixGL.hpp"
#include "opengl_trait.hpp"
#include "WRATHInterleavedAttributes.hpp"

//!\class WRATHTextureFontTTF
/*!
  A WRATHTextureFontTTF uses lib Freetype2 to create
  a texture for font rendering. The texture of a
  WRATHTextureFontTTF has two or four channels.

  \n\t .w = blend/opacity value used for blending rendering of fonts 
            (usual way), value comes lib FreeType2
  \n\t .x = distance value, used for simple discard rendering of fonts,
            texel to be discarded if value is less than 0.5. The value
            is normalized signed distance, i.e. [-M,M] normalized to
            [0,1]. Because it is a distance value, the texture font
            behaves much better under magnification as the distance
            function behaves nearly linearly. The values themselves
            are computed analytically from the vector representation
            of the font from lib FreeType2

  \n\t .y = indicates if the anayltic computation to determine if a pixel
            is inside of or outside of the glyph has failed, 0.0 means pass,
            1.0 (i.e 0xFF) means fail.

  \n\t .z = same as .y
 */
class WRATHTextureFontTTF:public WRATHTextureFont
{
public:
  virtual
  ~WRATHTextureFontTTF();

  virtual
  character_data_type
  character_data(uint32_t glyph);

  virtual
  WRATHTextureChoice::texture_base::handle
  texture_binder(void)
  {
    return m_texture_binder;
  }

  virtual
  ivec2
  texture_size(void)
  {
    return m_texture_size;
  }

  virtual
  int
  new_line_height(void);

  //!\fn
  /*!
    Returns the FT_Face (a Freetype2 data structure)
    that generated this WRATHTextureFontTTF.
   */
  FT_Face
  ttf_face(void)
  {
    return m_ttf_face;
  }

    
  //!\fn
  /*!
    Checks if a WRATHTextureFontTTF from the specified file, face index
    and point size has been created, if so returns it, otherwise
    creates a new WRATHTextureFontTTF of those parameters and returns it.
    The font's resource name will be: WRATHTextureFontKey(name, face_index)
    where name is: "%s??%d" pfilename, psize. I.e. different point
    sizes are given different names.

    \param psize pointsize to use.
    \param pfilename filename of the file of the font
    \param face_index face index of the file to use.    
   */
  static
  WRATHTextureFontTTF*
  fetch_font(int psize, const std::string &pfilename, int face_index=0);
 

  //!\class point_type
  /*!
    For each glyph of a WRATHTextureFontTTF, there is a vectoral representation.
    A point_type gives the points of the outlines of a WRATHTextureFontTTF.
    The color indicates the source of the points as follows:
    \n\t (0xFF,0x00,0x00,0) {red} on outline control point
    \n\t (0x00,0xFF,0x00,1) {green} off outline quadratic control point
    \n\t (0x00,0x00,0xFF,2) {blue} off outline  cubic control point
  */
  class point_type:public WRATHInterleavedAttributes< vecN<GLshort,2>, vecN<GLubyte,4> >
  {
  public:
    typedef WRATHInterleavedAttributes< vecN<GLshort,2>, vecN<GLubyte,4> > base_class;
    enum
      {
        point_location=0,
        color_location=1,
      };

    enum point_classification
      {
        on_curve=0,
        conic_off_curve=1,
        cubic_off_curve=2,
      };

    point_type()
    {}

   
    point_type(const ivec2 &pos,
               enum point_classification cl)
    {
      static vecN<GLubyte,4> cols[3]=
        {
          vecN<GLubyte,4>(0xFF, 0x00, 0x00, on_curve),
          vecN<GLubyte,4>(0x00, 0xFF, 0x00, conic_off_curve),
          vecN<GLubyte,4>(0x00, 0x00, 0xFF, cubic_off_curve)
        };

      position().x()=pos.x();
      position().y()=pos.y();

      WRATHassert(cl<3);
      color()=cols[cl];
    }

   
    const vecN<GLshort,2>&
    position(void) const
    {
      return get<point_location>();
    }

    vecN<GLshort,2>&
    position(void)
    {
      return get<point_location>();
    }

    const vecN<GLubyte,4>&
    color(void) const
    {
      return get<color_location>();
    }

    vecN<GLubyte,4>&
    color(void) 
    {
      return get<color_location>();
    }

    enum point_classification
    classification(void) const
    {
      return static_cast<enum point_classification>(color().w());
    }

    template<unsigned int N>
    static
    void
    attribute_key(vecN<opengl_trait_value,N> &attrs)
    {
      base_class::attribute_key(attrs);
      if(N>=2)
        {
          attrs[1].m_normalized=GL_TRUE;
        }
    }
  };

  //!\fn
  /*!
    Returns all points of the font geometry.
   */
  const std::vector<point_type>&
  font_geometry(void);

  const std::vector<range_type<GLushort> >&
  glyph_outlines(uint32_t glyph);
  
  const std::string&
  debug_string_data(uint32_t glyph);


  /*!
    Get if next WRATHTextureFontTTF will
    use Mipmapping on the texture caches,
    off by default.
   */
  static
  bool
  use_mipmapping(void);

  /*!
    Set so that subsequent WRATHTextureFontTTF's
    will or will not use Mipmapping on the texture caches,
    off by default.
   */
  static
  void
  use_mipmapping(bool);

  static
  GLint
  texture_creation_width(void);

  static
  void
  texture_creation_width(GLint);


  int
  total_pixel_waste(void) const
  {
    return m_total_pixel_waste;
  }

  int
  total_pixel_use(void) const
  {
    return m_total_pixel_use;
  }

private:

  class per_character_data;

  static
  FT_Face
  load_face(const std::string &pfilename, int face_index=0);

  
  void
  generate_character(uint32_t);

  void
  resize_texture(int new_height);

  per_character_data&
  get_glyph(uint32_t);

  void
  flush_texture(void);
 
  
  WRATHTextureFontTTF(FT_Face pttfFace,  
                    const std::string &pname,
                    int pixel_height);

  typedef vecN<uint8_t,4> pixel_type; 
  enum texture_format_type
    {
      //      texture_format=GL_LUMINANCE_ALPHA,
      texture_format=GL_RGBA
    };

  class per_mipmap_per_character_data
  {
  public:
    per_mipmap_per_character_data(void):
      m_texture_position(-1,-1),
      m_texture_size(0,0)
    {}

    ivec2 m_texture_position, m_texture_size;
    std::vector<pixel_type> m_pixels;
  };

  class per_character_data
  {
  public:
    per_character_data(void):
      m_width(-1)
    {}

    void
    upload_data_to_texture(const ivec2 &total_size);
    
    character_data_type m_data;
    std::vector<range_type<GLushort> > m_point_indices;    
    std::vector<per_mipmap_per_character_data> m_mipmaps;
    int m_width;
    std::string m_debug_string_data;
  };

  class private_texture_binder:public WRATHTextureChoice::texture
  {
  public:
    private_texture_binder(WRATHTextureFontTTF *ttf);

    virtual
    void
    bind_texture(GLenum texture_unit);

  private:
    WRATHTextureFontTTF *m_ttf;
  };


  GLuint m_texture_name;  
  ivec2 m_texture_size;
  FT_Face m_ttf_face;
  int m_pixel_height;
  bool m_uses_mipmapping;
  WRATHTextureChoice::texture_base::handle m_texture_binder;

  int m_x, m_y;

  int m_current_line_max_height;

  std::map<uint32_t, per_character_data> m_character_data;
  std::set<uint32_t> m_dirty_characters;
  bool m_resize_required;

  int m_total_pixel_waste, m_total_pixel_use;
  
  std::vector<point_type> m_font_points;
};

#endif
