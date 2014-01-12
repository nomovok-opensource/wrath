/*  -*- C++ -*- */

/*! 
 * \file WRATHTextureFontFreeType_MixImplement.tcc
 * \brief file WRATHTextureFontFreeType_MixImplement.tcc
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



#if !defined(__WRATH_TEXTURE_FONT_FreeType_MIX_HPP__)  || defined(__WRATH_TEXTURE_FONT_FreeType_MIX_IMPLEMENT_TCC__)
#error "Direction inclusion of private header file WRATHTextureFontFreeType_MixImplement.tcc"
#endif


#define __WRATH_TEXTURE_FONT_FreeType_MIX_IMPLEMENT_TCC__


template<typename T, typename S>
WRATHTextureFont::glyph_data_type*
WRATHTextureFontFreeType_TMix<T,S>::
generate_character(WRATHTextureFont::glyph_index_type G)
{

  WRATHTextureFont::character_code_type C;
  C=this->character_code(G);
  
  const WRATHTextureFont::glyph_data_type &dist_gl(m_native_src->glyph_data(G));
  const WRATHTextureFont::glyph_data_type &cov_gl(m_minified_src->glyph_data(G));
  
  if(dist_gl.font()==NULL or cov_gl.font()==NULL)
    {
      WRATHTextureFont::glyph_data_type *ptr;
      ptr=WRATHNew WRATHTextureFont::glyph_data_type();
      ptr->font(this);
      return ptr;
    }
  
  int pg;
  std::vector<WRATHTextureChoice::texture_base::handle> R;
  
  R.resize(dist_gl.texture_binder().size()+cov_gl.texture_binder().size());
  
  std::copy(dist_gl.texture_binder().begin(),
            dist_gl.texture_binder().end(),
            R.begin());
  std::copy(cov_gl.texture_binder().begin(),
            cov_gl.texture_binder().end(),
            R.begin()+dist_gl.texture_binder().size());
  
  
  
  /*
    generating the texture page data is hackish;
    if a page is created, the member variable
    m_new_page is set to true; the upshot
    is that we can only generate one glyph 
    at a time. We lock the mutex just before
    resetting m_new_page to false and getting
    the page
   */
  WRATHAutoLockMutex(m_mutex);
  m_new_page=false;

  pg=m_page_tracker.get_page_number(ivec2(0,0), R);
  
  WRATHTextureFont::glyph_data_type *return_value(WRATHNew WRATHTextureFont::glyph_data_type());
  WRATHTextureFont::glyph_data_type &glyph(*return_value);
  
  glyph
    .font(this)
    .texture_page(pg)
    .character_code(C)
    .glyph_index(G)
    
    .advance(dist_gl.advance())
    .bounding_box_size(dist_gl.bounding_box_size())
    
    .texel_values(dist_gl.texel_lower_left(),
                  dist_gl.texel_size())
    .origin(dist_gl.origin());
  
  glyph.sub_primitive_attributes().resize(dist_gl.sub_primitive_attributes().size());
  for(int i=0, end_i=dist_gl.sub_primitive_attributes().size(); i<end_i; ++i)
    {
      ivec2 rel;
      
      rel=dist_gl.sub_primitive_attributes()[i].m_texel_coordinates
        - dist_gl.texel_lower_left();
      
      glyph.sub_primitive_attributes()[i].set(glyph, rel);
    }
  glyph.sub_primitive_indices()=dist_gl.sub_primitive_indices();
  
  
  glyph.m_custom_float_data
    .resize(dist_gl.m_custom_float_data.size() 
            + cov_gl.m_custom_float_data.size()
            + 4);

  /*
    pack the bottom left and size of the minified glyph
    into glyph.m_custom_float_data[0--3]
   */
  glyph.m_custom_float_data[0]=cov_gl.texel_lower_left().x();
  glyph.m_custom_float_data[1]=cov_gl.texel_lower_left().y();
  glyph.m_custom_float_data[2]=cov_gl.texel_size().x();
  glyph.m_custom_float_data[3]=cov_gl.texel_size().y();

  /*
    pack the custom data from the native glyph next
   */
  std::copy(dist_gl.m_custom_float_data.begin(),
            dist_gl.m_custom_float_data.end(),
            glyph.m_custom_float_data.begin()+4);

  /*
    and finally the custom data from the minified glyph 
   */
  std::copy(cov_gl.m_custom_float_data.begin(),
            cov_gl.m_custom_float_data.end(),
            glyph.m_custom_float_data.begin()+4+dist_gl.m_custom_float_data.size());

  

  if(m_new_page)
    {
      std::vector<float> &data(m_page_tracker.custom_data(pg));
      m_new_page=false;

      data.resize(m_texture_page_data_size);
      /*
        we pack first the texture page data 
        of the native glyph, then the ratio
        of the sizes between the glyphs
       */
      for(int i=0, endi=m_native_src->texture_page_data_size(); i<endi; ++i)
        {
          data[i]=m_native_src->texture_page_data(dist_gl.texture_page(), i);
        }
      for(int i=0, 
            j=m_native_src->texture_page_data_size(),
            endi=m_minified_src->texture_page_data_size();
          i<endi; ++i, ++j)
        {
          data[j]=m_minified_src->texture_page_data(cov_gl.texture_page(), i);
        }

      data.back()=m_size_ratio;
    }
  
  return return_value;
}


template<typename T, typename S>
WRATHTextureFont*
WRATHTextureFontFreeType_TMix<T,S>::
fetch_font(int native_psize, int minified_psize,
           const WRATHFontDatabase::Font::const_handle &fnt)
{
  //first try to fetch the font:
  std::ostringstream ostr;
  WRATHTextureFontFreeType_TMix *return_value;
  WRATHTextureFont *p;
  WRATHFreeTypeSupport::LockableFace::handle pface;
  
  ostr << typeid(WRATHTextureFontFreeType_TMix).name() << ", " 
       << minified_psize;
  
  WRATHTextureFontKey K(fnt, native_psize, ostr.str());
  
  p=WRATHTextureFont::retrieve_resource(K);
  return_value=dynamic_cast<WRATHTextureFontFreeType_TMix*>(p);
  
  if(return_value==NULL)
    {
      T *dist;
      S *covg;
      
      dist=dynamic_cast<T*>(T::fetch_font(native_psize, fnt));
      covg=dynamic_cast<S*>(S::fetch_font(minified_psize, fnt));
      
      if(dist!=NULL and covg!=NULL)
        {
          pface=WRATHFreeTypeSupport::load_face(fnt);        
          return_value=WRATHNew WRATHTextureFontFreeType_TMix(pface, dist, covg, K);
        }
    }
  
  WRATHassert(return_value!=NULL);
  return return_value;
}
