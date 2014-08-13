/*! 
 * \file WRATHSDLImageSupport.cpp
 * \brief file WRATHSDLImageSupport.cpp
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
#include "WRATHSDLImageSupport.hpp"

namespace
{
  class GLBitsMaker
  {
  public:

    explicit
    GLBitsMaker(const SDL_Surface *img, const WRATHImage::ImageFormat &fmt, bool flip)
    {
      if(!need_conversion(img, fmt))
        {
          set(const_cast<SDL_Surface*>(img), flip);
          return;
        }
   
      SDL_Surface* converted(NULL);
      
      // Get the source and target formats
      SDL_PixelFormat* src_fmt = img->format;
      WRATHImage::PixelImageFormat tgt_fmt_wrath = fmt.m_pixel_format;
      
      // Create an empty pixel format struct
      SDL_PixelFormat tgt_fmt;
      memset(&tgt_fmt, 0, sizeof(tgt_fmt));

      // Set the target format to correspond the source WRATHImage pixel format      
      tgt_fmt.palette = NULL; 
      tgt_fmt.BitsPerPixel = tgt_fmt_wrath.bytes_per_pixel() * 8;
      tgt_fmt.BytesPerPixel = tgt_fmt_wrath.bytes_per_pixel();
           
      // RGB
      if(tgt_fmt_wrath.bytes_per_pixel() == 1)
        {
          tgt_fmt.Rshift = 0;
          tgt_fmt.Gshift = 0;
          tgt_fmt.Bshift = 0;
          tgt_fmt.Rmask = 0x000000FF;
          tgt_fmt.Gmask = 0x000000FF;
          tgt_fmt.Bmask = 0x000000FF;
        }
      
      if(tgt_fmt_wrath.bytes_per_pixel() >= 3)
        {
          tgt_fmt.Rshift = 0;
          tgt_fmt.Gshift = 8;
          tgt_fmt.Bshift = 16;
          tgt_fmt.Rmask = 0x000000FF;
          tgt_fmt.Gmask = 0x0000FF00;
          tgt_fmt.Bmask = 0x00FF0000;
        }

      // RGBA
      if(tgt_fmt_wrath.bytes_per_pixel() == 4) 
        {
          tgt_fmt.Ashift = 24;
          tgt_fmt.Amask = 0xFF000000;
        }
       
      // Use SDL conversion to convert to correct format
      converted = SDL_ConvertSurface(const_cast<SDL_Surface*>(img), &tgt_fmt, SDL_SWSURFACE);
    
      // Set the bits to m_bits_data
      set(const_cast<SDL_Surface*>(converted), flip);
      
      SDL_FreeSurface(converted);
    }
        
    std::vector<uint8_t> m_bits_data;

  private:

    void
    set(SDL_Surface *img, bool flip)
    {
      if(img==NULL)
        {
          return;
        }

      // Store some useful variables
      SDL_LockSurface(img);
            
      int w(img->w), h(img->h);
      int pitch(img->pitch);
      int bytes_per_pixel(img->format->BytesPerPixel);

      const unsigned char *surface_data;
      surface_data=reinterpret_cast<const unsigned char*>(img->pixels);     

      // Resize the vector holding the bit data
      m_bits_data.resize(w*h*bytes_per_pixel);

      for(int y=0; y<h; ++y)
        {
          int source_y = (flip) ? h-1-y : y;

          for(int x=0; x<w; ++x)
            {
              int src_L, dest_L;
              
              src_L= source_y*pitch + x*bytes_per_pixel;
              dest_L= (y*w + x)*bytes_per_pixel;

              // Convert the pixel from surface data to Uint32
              for(int channel=bytes_per_pixel-1; channel>=0; --channel)
                {
                  m_bits_data[dest_L + channel] = surface_data[src_L + channel];
                }
            }
        }

      
      SDL_UnlockSurface(img);
    }
    
    bool
    need_conversion(const SDL_Surface *img, 
                    const WRATHImage::ImageFormat& fmt)
    {
      if( (img->format->BytesPerPixel != fmt.bytes_per_pixel() ) || // Different bpp
          (img->format->Rshift > img->format->Bshift)) // ABGR/BGR
        {
          return true;
        }
      
      return false;
    }
  };
}




WRATHImage*
WRATHSDL::
create_image(const SDL_Surface *img, 
             const WRATHImage::ImageFormat &fmt,
             const WRATHImage::WRATHImageID &ID,
             bool use_unique_pixel_store,
             enum y_flip_t flip)
{
  vecN<uint32_t, 2> R(WRATHImage::texture_atlas_dimension());

  if(img==NULL
     or static_cast<uint32_t>(img->w)>R.x()
     or static_cast<uint32_t>(img->h)>R.y())
    {
      return NULL;
    }

  WRATHImage *ptr;

  if(use_unique_pixel_store)
    {
      ptr=WRATHNew WRATHImage(ID, 
                              ivec2(img->w, img->h), 
                              fmt,
                              WRATHImage::UniquePixelStore);
    }
  else
    {

      ptr=WRATHNew WRATHImage(ID, 
                              ivec2(img->w, img->h), 
                              fmt);
    }

  respecify_sub_image(ptr, img, ivec2(0,0), flip);
  return ptr;
}

void
WRATHSDL::
respecify_sub_image(int layer,
                    WRATHImage *kan_img,
                    const SDL_Surface *img,
                    const ivec2 &min_corner,
                    enum y_flip_t flip)
{
  GLBitsMaker bits(img, kan_img->image_format().format(layer), flip==flip_y);

  WRATHassert(kan_img!=NULL);
  
  GLenum pdf = kan_img->image_format().format(layer).m_pixel_format.m_pixel_data_format;
  GLenum pt = kan_img->image_format().format(layer).m_pixel_format.m_pixel_type;
  
  kan_img->respecify_sub_image(layer, 0,
                               WRATHImage::PixelImageFormat()
                               .pixel_data_format(pdf)
                               .pixel_type(pt),
                               bits.m_bits_data, 
                               min_corner,
                               ivec2(img->w, img->h), 
                               1);      
  

}
  

WRATHImage*
WRATHSDL::
load_image(const std::string &filename,
           const WRATHImage::ImageFormat &fmt,
           const WRATHImage::WRATHImageID &ID,
           bool use_unique_pixel_store,
           enum y_flip_t flip)
{
  SDL_Surface *qim;
  qim=IMG_Load(filename.c_str());
  if(qim!=NULL)
    {
      return create_image(qim, fmt, ID, use_unique_pixel_store, flip);
    }
  return NULL;
}

WRATHImage*
WRATHSDL::
fetch_image(const WRATHImage::WRATHImageID &ID,
            const WRATHImage::ImageFormat &fmt,
            bool use_unique_pixel_store,
            enum y_flip_t flip)
{
  WRATHImage *return_value;

  return_value=WRATHImage::retrieve_resource(ID);
  if(return_value==NULL)
    {
      return_value=load_image(ID, fmt, ID, use_unique_pixel_store, flip);
    }
  return return_value;
}


