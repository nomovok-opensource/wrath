/*! 
 * \file WRATHQTImageSupport.hpp
 * \brief file WRATHQTImageSupport.hpp
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




#ifndef WRATH_HEADER_QT_IMAGE_SUPPORT_HPP_
#define WRATH_HEADER_QT_IMAGE_SUPPORT_HPP_

#include "WRATHConfig.hpp"
#include <QImage>
#include "WRATHImage.hpp"


/*! \addtogroup Qt
 * @{
 */

namespace WRATHQT
{          
  /*!\fn WRATHImage* create_image(const QImage&, const WRATHImage::ImageFormat&,
                                  const WRATHImage::WRATHImageID&, bool)
    Create and return a WRATHImage from a QImage.
    Will return NULL if the either of the dimensions 
    of the passed QImage exceed WRATHImage::texture_atlas_dimension().
    If the image format dictates to have mipmaps BUT
    to use manual mipmap generation, routine will
    also generate and set the higher LOD's.
    Does NOT need to be called from the same thread
    as the rendering thread.
    \param img QImage data source for WRATHImage.
    \param fmt image format for the created WRATHImage to have.
    \param ID resource ID to identify the created WRATHImage.
    \param use_unique_pixel_store Determines whether a unique pixel store is to be used.
   */
  WRATHImage*
  create_image(const QImage &img, 
               const WRATHImage::ImageFormat &fmt,
               const WRATHImage::WRATHImageID &ID,
               bool use_unique_pixel_store=false);

  /*!\fn WRATHImage* load_image(const std::string&, const WRATHImage::ImageFormat&,
                                const WRATHImage::WRATHImageID&, bool)
    Load a QImage via Qt from a specified file,
    then create a WRATHImage from that 
    QImage (via \ref create_image).
    If the QImage is null, (i.e empty) returns
    NULL.
    \param filename file name from which to load image data
    \param fmt image format for the created WRATHImage to have.
    \param ID resource ID to identify the created WRATHImage.
    \param use_unique_pixel_store Determines whether a unique pixel store is to be used.
   */
  WRATHImage*
  load_image(const std::string &filename,
             const WRATHImage::ImageFormat &fmt,
             const WRATHImage::WRATHImageID &ID,
             bool use_unique_pixel_store=false);


  /*!\fn WRATHImage* fetch_image(const WRATHImage::WRATHImageID&,
                                 const WRATHImage::ImageFormat&,
                                 bool)
    Checks if an image of the specified WRATHImageID
    is already resource managed and if so loads it.
    Otherwise, returns a new WRATHImage via
    \ref load_image(). Note: if there
    is no resource and if Qt loads an empty
    QImage for the filename will return NULL.
    \param fmt image format for the created WRATHImage to have.
    \param ID resource ID to identify the WRATHImage.
    \param use_unique_pixel_store Determines whether a unique pixel store is to be used.
   */
  WRATHImage*
  fetch_image(const WRATHImage::WRATHImageID &ID,
              const WRATHImage::ImageFormat &fmt,
              bool use_unique_pixel_store=false);
  

  /*!\fn respecify_sub_image(int, WRATHImage*, const QImage&, const ivec2&)
    Respecify a portion of a WRATHImage using a QImage. 
    Internally, the image data is repsecified
    with GL_RGBA and the image bits are produced with
    the return value from QGLWidget::convertToGLFormat().
    If the image_format() of the WRATHImage indictates 
    to have mipmaps BUT to use manual mipmap generation, 
    routine will also generate and set the higher LOD's.
    Does NOT need to be called from the same thread
    as the rendering thread.
    \param layer Layer index of the source image to respecify
    \param wrath_img WRATHImage to change a portion of the data of
    \param img QImage to use to change a portion of the WRATHImage
    \param min_corner bottom left corner relative to the dimensions
                      of the WRATHImage.
   */
  void
  respecify_sub_image(int layer, WRATHImage *wrath_img, const QImage &img,
                      const ivec2 &min_corner=ivec2(0,0));

  /*!\fn respecify_sub_image(WRATHImage*, const QImage&, const ivec2&)
    Convenience function for respecify_sub_image(int, WRATHImage*, const QImage&,
    const ivec2&) that assumes the first layer (0) to be respecified.
    \param wrath_img WRATHImage to change a portion of the data of
    \param img QImage to use to change a portion of the WRATHImage
    \param min_corner bottom left corner relative to the dimensions
                      of the WRATHImage.
   */
  inline
  void
  respecify_sub_image(WRATHImage *wrath_img, const QImage &img,
                      const ivec2 &min_corner=ivec2(0,0))
  {
    return respecify_sub_image(0, wrath_img, img, min_corner);
  }
}


/*! @} */

#endif
