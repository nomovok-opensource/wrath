/*! 
 * \file WRATHTextureFontDrawer.hpp
 * \brief file WRATHTextureFontDrawer.hpp
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




#ifndef WRATH_HEADER_TEXTURE_FONT_DRAWER_HPP_
#define WRATH_HEADER_TEXTURE_FONT_DRAWER_HPP_


#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include "WRATHTwoPassDrawer.hpp"
#include "WRATHTextureFont.hpp"
#include "WRATHGLStateChange.hpp"
#include "WRATHCanvas.hpp"

/*! \addtogroup Text
 * @{
 */

/*!\class WRATHTextureFontDrawer
  Fonts drawn via textures are drawn in 2 "passes".
  The first pass is for the portions of the glyph(s)
  that are solid (i.e. no blending required), the
  second pass is for those portions that are transluscent,
  i.e. requiring blending and coming from AA.  

  Text that is rendered as transparent only requires
  one pass.

  Shaders for text drawing have the texture page
  data (as returned by WRATHTextureFont::texture_page_data())
  fill a uniform array of floats, that uniform setter is 
  returned by opaque_pass_texture_page_data_uniform(),
  translucent_pass_texture_page_data_uniform(),
  translucent_only_texture_page_data_uniform()
  and texture_page_data_named_uniform().

  TODO: in truth we do not need to pass the actual
  drawers, just flags indicating if the named pass
  is active.
 */
class WRATHTextureFontDrawer:public WRATHTwoPassDrawer
{
public:

  /*!\fn WRATHTextureFontDrawer(const ResourceKey&, 
                                WRATHItemDrawer*,
                                WRATHItemDrawer*,
                                WRATHItemDrawer*);
    Constructor. 
    Note: it is _legal_ for ptranslucent_drawer to be NULL, in that case then opaque text
    does _NOT_ have a translucent pass, i.e. no AA.
    \param pname resource name to give to the created WRATHTextureFontDrawer.
    \param popaque_drawer pointer to drawer for opaque pass
    \param ptranslucent_drawer pointer to drawer for translucent pass
    \param ptranslucent_drawer_standalone pointer to drawer used for completely translucent text
   */
  WRATHTextureFontDrawer(const ResourceKey &pname,
                         WRATHItemDrawer *popaque_drawer,
                         WRATHItemDrawer *ptranslucent_drawer,
                         WRATHItemDrawer *ptranslucent_drawer_standalone);
  

  /*!\fn WRATHTextureFontDrawer(WRATHItemDrawer*, 
                                WRATHItemDrawer*,
                                WRATHItemDrawer*)
    Constructor. Resource name will be generated as an assemblage
    of the values passed.
    Note: it is _legal_ for ptranslucent_drawer to be NULL, in that case then opaque text
    does _NOT_ have a translucent pass, i.e. no AA.
    \param popaque_drawer pointer to drawer for opaque pass
    \param ptranslucent_drawer pointer to drawer for translucent pass
    \param ptranslucent_drawer_standalone pointer to drawer used for completely translucent text
   */
  WRATHTextureFontDrawer(WRATHItemDrawer *popaque_drawer,
                         WRATHItemDrawer *ptranslucent_drawer,
                         WRATHItemDrawer *ptranslucent_drawer_standalone);
  

  
  virtual
  ~WRATHTextureFontDrawer(); 

  /*!\fn WRATHUniformData::uniform_setter_base::handle opaque_pass_texture_page_data_uniform
    Returns the uniform handle for the texture 
    page data for the opaque pass for a named
    font and texture page. There is a unique
    uniform for each font-texture page pair
    \param pfont font 
    \param texture_page texture page
   */
  WRATHUniformData::uniform_setter_base::handle
  opaque_pass_texture_page_data_uniform(WRATHTextureFont *pfont, int texture_page)
  {
    return m_passes[opaque_draw_pass]->texture_page_data_uniform(pfont, texture_page);
  }

  /*!\fn WRATHUniformData::uniform_setter_base::handle translucent_pass_texture_page_data_uniform
    Returns the uniform handle for the texture
    page data for the translucent pass for a named
    font and texture page. There is a unique
    uniform for each font-texture page pair
    \param pfont font 
    \param texture_page texture page
   */
  WRATHUniformData::uniform_setter_base::handle
  translucent_pass_texture_page_data_uniform(WRATHTextureFont *pfont, int texture_page)
  {
    return (m_passes[transluscent_draw_pass]!=NULL)?
      m_passes[transluscent_draw_pass]->texture_page_data_uniform(pfont, texture_page):
      NULL;
  }

  /*!\fn WRATHUniformData::uniform_setter_base::handle translucent_only_texture_page_data_uniform
    Returns the uniform handle for the texture
    page data for the pure translucent drawer for a named
    font and texture page. There is a unique
    uniform for each font-texture page pair
    \param pfont font 
    \param texture_page texture page
   */
  WRATHUniformData::uniform_setter_base::handle
  translucent_only_texture_page_data_uniform(WRATHTextureFont *pfont, int texture_page)
  {
    return m_passes[pure_transluscent]->texture_page_data_uniform(pfont, texture_page);
  }

  /*!\fn WRATHUniformData::uniform_setter_base::handle texture_page_data_named_uniform
    Returns the uniform handle for the texture
    size for the named pass for a named
    font and texture page. There is a unique
    uniform for each font-texture page pair
    \param tp which pass
    \param pfont font 
    \param texture_page texture page
   */
  WRATHUniformData::uniform_setter_base::handle
  texture_page_data_named_uniform(enum drawing_pass_type tp, WRATHTextureFont *pfont, int texture_page)
  {
    return (m_passes[tp]!=NULL)?
      m_passes[tp]->texture_page_data_uniform(pfont, texture_page):
      NULL;
  }  
  

private:
  
  typedef std::pair<WRATHTextureFont*,int> map_key;
  typedef WRATHUniformData::uniform_setter_base::handle map_value;

  class per_type:boost::noncopyable
  {
  public:
    explicit
    per_type(void);

    ~per_type();

    WRATHUniformData::uniform_setter_base::handle
    texture_page_data_uniform(WRATHTextureFont *pfont, int texture_page);
  
  private:    
    std::map<map_key, map_value> m_map;
    WRATHMutex m_mutex;
  };

  per_type*
  named(enum drawing_pass_type tp) const
  {
    per_type *R;
    R=m_passes[tp];
    WRATHassert(R!=NULL);
    return R;
  }

  void
  init(WRATHItemDrawer *popaque_drawer,
       WRATHItemDrawer *ptranslucent_drawer,
       WRATHItemDrawer *ptranslucent_drawer_standalone);

  vecN<per_type*, 3> m_passes;
  ResourceKey m_resource_name;
  

};
/*! @} */

#endif
