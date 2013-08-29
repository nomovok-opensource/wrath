/*! 
 * \file WRATHQTImageSupport.cpp
 * \brief file WRATHQTImageSupport.cpp
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
#include <QtOpenGL>
#include "WRATHgl.hpp"
#include "WRATHQTImageSupport.hpp"

namespace
{
  class GLBitsMaker
  {
  public:

    explicit
    GLBitsMaker(const QImage &img)
    {
      set(img);
    }

    void
    set(const QImage &img)
    {
      const QImage rgba_image(QGLWidget::convertToGLFormat(img));
      int scan_line_length(4*img.width());
      const uchar *bits(rgba_image.bits());
      
      m_bits_data.resize(scan_line_length*img.height());
      std::copy(bits, bits+m_bits_data.size(), m_bits_data.begin());
    }

        
    std::vector<uint8_t> m_bits_data;
  };
}

WRATHImage*
WRATHQT::
create_image(const QImage &img, 
             const WRATHImage::ImageFormat &fmt,
             const WRATHImage::WRATHImageID &ID,
             bool use_unique_pixel_store)
{
  vecN<uint32_t, 2> R(WRATHImage::texture_atlas_dimension());
  if(img.isNull()
     or static_cast<uint32_t>(img.width())>R.x()
     or static_cast<uint32_t>(img.height())>R.y())
    {
      return NULL;
    }

  WRATHImage *ptr;

  if(use_unique_pixel_store)
    {
      ptr=WRATHNew WRATHImage(ID, 
                              ivec2(img.width(), img.height()), 
                              fmt,
                              WRATHImage::UniquePixelStore);
    }
  else
    {

      ptr=WRATHNew WRATHImage(ID, 
                              ivec2(img.width(), img.height()), 
                              fmt);
    }

  respecify_sub_image(ptr, img, ivec2(0,0));
  return ptr;
}


void
WRATHQT::
respecify_sub_image(int layer, WRATHImage *kan_img,
                    const QImage &img,
                    const ivec2 &min_corner)
{
  if(img.isNull() or img.width()<=0 or img.height()<=0)
    {
      return;
    }

  GLBitsMaker bits(img);


  WRATHassert(kan_img!=NULL);
  kan_img->respecify_sub_image(layer, 0,
                               WRATHImage::PixelImageFormat()
                               .pixel_data_format(GL_RGBA)
                               .pixel_type(GL_UNSIGNED_BYTE),
                               bits.m_bits_data, 
                               min_corner,
                               ivec2(img.width(), img.height()), 
                               4);      
  
  if(!kan_img->image_format()[layer].m_automatic_mipmap_generation
     and kan_img->image_format()[layer].requires_mipmaps())
    {
      QImage current_image;
      const QImage *last_image(&img);
      
      for(int LOD=1, w=img.width()/2, h=img.height()/2; 
          w>0 and h>0; ++LOD, w>>=1, h>>=1)
        {
          current_image=last_image->scaled(w, h,
                                           Qt::IgnoreAspectRatio,
                                           Qt::SmoothTransformation);
          last_image=&current_image;
          bits.set(current_image);

          kan_img->respecify_sub_image(LOD,
                                       WRATHImage::PixelImageFormat()
                                       .pixel_data_format(GL_RGBA)
                                       .pixel_type(GL_UNSIGNED_BYTE),
                                       bits.m_bits_data, 
                                       ivec2(min_corner.x()>>LOD, min_corner.y()>>LOD),
                                       ivec2(w,h), 
                                       4);  
        }
    }
}
  

WRATHImage*
WRATHQT::
load_image(const std::string &filename,
           const WRATHImage::ImageFormat &fmt,
           const WRATHImage::WRATHImageID &ID,
           bool use_unique_pixel_store)
{
  QImage qim(filename.c_str());
  if(!qim.isNull())
    {
      return create_image(qim, fmt, ID, use_unique_pixel_store);
    }
  return NULL;
}

WRATHImage*
WRATHQT::
fetch_image(const WRATHImage::WRATHImageID &ID,
            const WRATHImage::ImageFormat &fmt,
            bool use_unique_pixel_store)
{
  WRATHImage *return_value;

  return_value=WRATHImage::retrieve_resource(ID);
  if(return_value==NULL)
    {
      return_value=load_image(ID, fmt, ID, use_unique_pixel_store);
    }
  return return_value;
}


