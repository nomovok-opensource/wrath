/*! 
 * \file WRATHSDLImageSupport.hpp
 * \brief file WRATHSDLImageSupport.hpp
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




#ifndef WRATH_HEADER_SDL_IMAGE_SUPPORT_HPP_
#define WRATH_HEADER_SDL_IMAGE_SUPPORT_HPP_

#include "WRATHConfig.hpp"
#include <SDL_image.h>
#include "WRATHImage.hpp"


/*! \addtogroup SDL
 * @{
 */

namespace WRATHSDL
{
  /*!\enum y_flip_t
    Enumeration describing weather or not to flip an image
    on load.
  */
  enum y_flip_t
    {
      /*!
        Flip the y-coordinate when loading
        data from an SDL_Surface. 
       */
      flip_y,

      /*!
        Dont't flip the y-coordinate when loading
        data from an SDL_Surface
       */
      dont_flip_y,
    };                  

  /*!\fn WRATHImage* create_image(const SDL_Surface*, const WRATHImage::ImageFormat&,
                                  const WRATHImage::WRATHImageID&,
                                  bool, enum y_flip_t)
    Create and return a WRATHImage from a SDL_Surface.
    Will return NULL if the either of the dimensions 
    of the passed SDL_Surface exceed WRATHImage::texture_atlas_dimension().
    If the image format dictates to have mipmaps BUT
    to use manual mipmap generation, routine will
    also generate and set the higher LOD's.
    Does NOT need to be called from the same thread
    as the rendering thread.
    \param img SDL_Surface data source for WRATHImage.
    \param fmt image format for the created WRATHImage to have.
    \param ID resource ID to identify the created WRATHImage.
    \param use_unique_pixel_store if true, the image is created on a private texture,
                                  i.e. the texture is not shared with other images
    \param flip determines weather or not to flip the input image
                data in the y-coordinate before sending it to
   */
  WRATHImage*
  create_image(const SDL_Surface *img, 
               const WRATHImage::ImageFormat &fmt,
               const WRATHImage::WRATHImageID &ID,
               bool use_unique_pixel_store=false,
               enum y_flip_t flip=flip_y);


  /*!\fn WRATHImage* load_image(const std::string&, const WRATHImage::ImageFormat&,
                                const WRATHImage::WRATHImageID&,
                                bool, enum y_flip_t)
    Load a SDL_Surface via SDL from a specified file,
    then create a WRATHImage from that 
    SDL_Surface (via \ref create_image).
    If the SDL_Surface is null, (i.e empty) returns
    NULL.

    \param filename file name from which to load image data
    \param fmt image format for the created WRATHImage to have.
    \param ID resource ID to identify the created WRATHImage.
    \param use_unique_pixel_store if true, the image is created on a private texture,
                                  i.e. the texture is not shared with other images
    \param flip determines weather or not to flip the input image
                data in the y-coordinate before sending it to
   */
  WRATHImage*
  load_image(const std::string &filename,
             const WRATHImage::ImageFormat &fmt,
             const WRATHImage::WRATHImageID &ID,
             bool use_unique_pixel_store=false,
             enum y_flip_t flip=flip_y);


  /*!\fn WRATHImage* fetch_image(const WRATHImage::WRATHImageID&,
                                 const WRATHImage::ImageFormat&,
                                 bool, enum y_flip_t)
  
    Checks if an image of the specified WRATHImageID
    is already resource managed and if so loads it.
    Otherwise, returns a new WRATHImage via
    \ref load_image(). Note: if there
    is no resource and if SDL loads an empty
    SDL_Surface for the filename will return NULL.
    
    \param ID resource ID to identify the WRATHImage.
    \param fmt image format for the created WRATHImage to have.
    \param use_unique_pixel_store if true, the image is created on a private texture,
                                  i.e. the texture is not shared with other images
    \param flip determines weather or not to flip the input image
                data in the y-coordinate before sending it to
   */
  WRATHImage*
  fetch_image(const WRATHImage::WRATHImageID &ID,
              const WRATHImage::ImageFormat &fmt,
              bool use_unique_pixel_store=false,
              enum y_flip_t flip=flip_y);
  

  /*!\fn void respecify_sub_image(int, WRATHImage*, const SDL_Surface*, 
                                  const ivec2&, enum y_flip_t)
    Respecify a portion of a WRATHImage using a SDL_Surface. 
    Internally, the image data is repsecified
    with GL_RGBA and the image bits are produced with
    the return value from QGLWidget::convertToGLFormat().
    If the image_format() of the WRATHImage indictates 
    to have mipmaps BUT to use manual mipmap generation, 
    routine will also generate and set the higher LOD's.
    Does NOT need to be called from the same thread
    as the rendering thread.
    
    Does not support converting to float and half float formats
    at the moment.
    \param layer Target layer in WRATHImage
    \param wrath_img WRATHImage to change a portion of the data of
    \param img SDL_Surface to use to change a portion of the WRATHImage
    \param min_corner minX-minY corner relative to the dimensions
                      of the WRATHImage.
    \param flip determines weather or not to flip the input image
                data in the y-coordinate before sending it to
   */
  void
  respecify_sub_image(int layer, WRATHImage *wrath_img, const SDL_Surface *img,
                      const ivec2 &min_corner=ivec2(0,0),
                      enum y_flip_t flip=flip_y);

  /*!\fn void respecify_sub_image(WRATHImage*, const SDL_Surface*, 
                                  const ivec2&, enum y_flip_t)
    Provided as a conveniance, equivalent to
    \code
    respecify_sub_image(0, kan_img, img, min_corner, flip);
    \endcode
    i.e. change a portion of layer 0 the image. 
    \param wrath_img WRATHImage to change a portion of the data of
    \param img SDL_Surface to use to change a portion of the WRATHImage
    \param min_corner minX-minY corner relative to the dimensions
                      of the WRATHImage.
    \param flip determines weather or not to flip the input image
                data in the y-coordinate before sending it to
   */
  inline
  void
  respecify_sub_image(WRATHImage *wrath_img, const SDL_Surface *img,
                      const ivec2 &min_corner=ivec2(0,0),
                      enum y_flip_t flip=flip_y)
  {
    return respecify_sub_image(0, wrath_img, img, min_corner, flip);
  }

}


/*! @} */

#endif
