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
WRATHTextureFontFreeType_TMixSupport::PerMixClass&
WRATHTextureFontFreeType_TMix<T,S>::
datum(void)
{
  const std::type_info &type(typeid(WRATHTextureFontFreeType_TMix));
  return WRATHTextureFontFreeType_TMixSupport::datum(type);
}

template<typename T, typename S>
S*
WRATHTextureFontFreeType_TMix<T,S>::
create_minified_font(void)
{
  float fsz;
  int sz;
  WRATHTextureFont *r;
  
  fsz=static_cast<float>(this->pixel_size())/m_size_ratio;
  sz=static_cast<int>(fsz);
  
  r=S::fetch_font(sz, this->source_font());
  WRATHassert(dynamic_cast<S*>(r)!=NULL);
  
  return static_cast<S*>(r);
}



template<typename T, typename S>
T*
WRATHTextureFontFreeType_TMix<T,S>::
create_native_font(void)
{ 
  WRATHTextureFont *r;
  
  r=T::fetch_font(this->pixel_size(), this->source_font());
  WRATHassert(dynamic_cast<T*>(r)!=NULL);
  
  return static_cast<T*>(r);
}


template<typename T, typename S>
void
WRATHTextureFontFreeType_TMix<T,S>::
on_create_texture_page(void)
{
  m_new_page=true;
}


template<typename T, typename S>
void
WRATHTextureFontFreeType_TMix<T,S>::
common_init(void)
{
  m_page_tracker.connect(boost::bind(&WRATHTextureFontFreeType_TMix::on_create_texture_page, 
                                     this));

  m_texture_page_data_size=m_native_src->texture_page_data_size()
    + m_minified_src->texture_page_data_size();
  
  /*
    save:
    glyph_bottom_left
    of minified glyph: takes 2 floats.
  */
  m_glyph_custom_native_start=2;
  
  m_glyph_custom_minified_start=m_glyph_custom_native_start
    + m_native_src->glyph_custom_float_data_size();
  
  m_glyph_custom_float_data_size=m_glyph_custom_minified_start
    + m_minified_src->glyph_custom_float_data_size();
  
  m_glyph_glsl=WRATHTextureFontFreeType_TMixSupport::glyph_glsl(m_native_src,
                                                                m_minified_src,
                                                                &datum(),
                                                                m_glyph_custom_native_start,
                                                                m_glyph_custom_native_start,
                                                                m_glyph_custom_minified_start);
}

template<typename T, typename S>
WRATHTextureFont::glyph_data_type*
WRATHTextureFontFreeType_TMix<T,S>::
generate_character(WRATHTextureFont::glyph_index_type G)
{

  WRATHTextureFont::character_code_type C;
  C=this->character_code(G);
  
  const WRATHTextureFont::glyph_data_type &native_glyph(m_native_src->glyph_data(G));
  const WRATHTextureFont::glyph_data_type &minified_glyph(m_minified_src->glyph_data(G));
  
  if(native_glyph.font()==NULL or minified_glyph.font()==NULL)
    {
      WRATHTextureFont::glyph_data_type *ptr;
      ptr=WRATHNew WRATHTextureFont::glyph_data_type();
      ptr->font(this);
      return ptr;
    }
  
  int pg;
  std::vector<WRATHTextureChoice::texture_base::handle> R;
  
  R.resize(native_glyph.texture_binder().size()+minified_glyph.texture_binder().size());
  
  std::copy(native_glyph.texture_binder().begin(),
            native_glyph.texture_binder().end(),
            R.begin());
  std::copy(minified_glyph.texture_binder().begin(),
            minified_glyph.texture_binder().end(),
            R.begin()+native_glyph.texture_binder().size());
  
  
  
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
    .advance(native_glyph.advance())
    .bounding_box_size(native_glyph.bounding_box_size())
    .texel_values(native_glyph.texel_lower_left(),
                  native_glyph.texel_size())
    .origin(native_glyph.origin());
  
  glyph.sub_primitive_attributes().resize(native_glyph.sub_primitive_attributes().size());
  for(int i=0, end_i=native_glyph.sub_primitive_attributes().size(); i<end_i; ++i)
    {
      ivec2 rel;
      
      rel=native_glyph.sub_primitive_attributes()[i].m_texel_coordinates
        - native_glyph.texel_lower_left();
      
      glyph.sub_primitive_attributes()[i].set(glyph, rel);
    }
  glyph.sub_primitive_indices()=native_glyph.sub_primitive_indices();
  
  
  glyph.m_custom_float_data.resize(m_glyph_custom_float_data_size, 0.0f);

  /*
    pack the bottom left the minified glyph
    into glyph.m_custom_float_data[0--1]
   */
  glyph.m_custom_float_data[0]=minified_glyph.texel_lower_left().x();
  glyph.m_custom_float_data[1]=minified_glyph.texel_lower_left().y();

  /*
    pack the custom data from the native glyph next
   */
  std::copy(native_glyph.m_custom_float_data.begin(),
            native_glyph.m_custom_float_data.end(),
            glyph.m_custom_float_data.begin()+m_glyph_custom_native_start);

  /*
    and finally the custom data from the minified glyph 
   */
  std::copy(minified_glyph.m_custom_float_data.begin(),
            minified_glyph.m_custom_float_data.end(),
            glyph.m_custom_float_data.begin()+m_glyph_custom_minified_start);

  

  if(m_new_page)
    {
      std::vector<float> &data(m_page_tracker.custom_data(pg));
      m_new_page=false;

      data.resize(m_texture_page_data_size);
      /*
        we pack first the texture page data 
        of the native glyph, then texture
        page data of the minified glyph.
       */
      for(int i=0, endi=m_native_src->texture_page_data_size(); i<endi; ++i)
        {
          data[i]=m_native_src->texture_page_data(native_glyph.texture_page(), i);
        }
      for(int i=0, 
            j=m_native_src->texture_page_data_size(),
            endi=m_minified_src->texture_page_data_size();
          i<endi; ++i, ++j)
        {
          data[j]=m_minified_src->texture_page_data(minified_glyph.texture_page(), i);
        }
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
