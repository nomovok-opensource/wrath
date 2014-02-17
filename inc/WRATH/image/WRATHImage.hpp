/*! 
 * \file WRATHImage.hpp
 * \brief file WRATHImage.hpp
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




#ifndef WRATH_HEADER_IMAGE_HPP_
#define WRATH_HEADER_IMAGE_HPP_


#include "WRATHConfig.hpp"
#include <stdint.h>
#include <boost/utility.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include "c_array.hpp"
#include "WRATHgl.hpp"
#include "vecN.hpp"
#include "vectorGL.hpp"
#include "WRATHNew.hpp"
#include "WRATHResourceManager.hpp"
#include "WRATHAtlas.hpp"
#include "WRATHTextureChoice.hpp"
#include "WRATHUniformData.hpp"

/*! \addtogroup Imaging
 * @{
 */


/*!\class WRATHImage
  A WRATHImage represents an image packed into a 
  GL texture, such a GL texture may have
  many images packed into it.

  WRATHImage is a resource managed object,
  i.e. that class has a WRATHResourceManager,
  see \ref WRATH_RESOURCE_MANAGER_DECLARE.
  A WRATHImage can be modified and created 
  from threads outside of the GL context, 
  however, it must only be deleted from 
  within the GL context. In particular the 
  resource manager may only be cleared from 
  within the GL context.
 */
class WRATHImage:public boost::noncopyable
{
public:
  /*!\typedef WRATHImageID
    A WRATHImageID is just an std::string.
  */
  typedef std::string WRATHImageID;

  /// @cond
  WRATH_RESOURCE_MANAGER_DECLARE(WRATHImage, WRATHImageID);
  /// @endcond

  /*!\class PixelImageFormat
    Data structure to specify incoming pixel 
    data, used in respify image API to specify
    the raw pixel data. Initializes as:
    - \ref m_pixel_data_format as GL_RGBA 
    - \ref m_pixel_type as GL_UNSIGNED_BYTE 
   */
  class PixelImageFormat
  {
  public:
    PixelImageFormat(void):
      m_pixel_data_format(GL_RGBA),
      m_pixel_type(GL_UNSIGNED_BYTE)
    {}

    /*!\var m_pixel_data_format
      Format of pixel data to glTexImage. 
      For GLES2 this is one of GL_RGBA, 
      GL_RGB, GL_LUMINANCE, GL_ALPHA 
      or GL_LUMINANCE_ALPHA, i.e. it specifies
      how many channels are provided.
     */
    GLenum m_pixel_data_format;

    /*!\var m_pixel_type
      Specifies the pixel type, for GLES2
      this is one of GL_UNSIGNED_BYTE, 
      GL_UNSIGNED_SHORT_5_6_5, 
      GL_UNSIGNED_SHORT_4_4_4_4, or
      GL_UNSIGNED_SHORT_5_5_5_1
     */
    GLenum m_pixel_type;
   
    /*!\fn PixelImageFormat& pixel_data_format(GLenum)
      Sets \ref m_pixel_data_format.
      \param v value to which to assign \ref m_pixel_data_format
     */
    PixelImageFormat&
    pixel_data_format(GLenum v)
    {
      m_pixel_data_format=v;
      return *this;
    }

    /*!\fn PixelImageFormat& pixel_type
      Sets \ref m_pixel_type.
      \param v value to which to assign \ref m_pixel_type
     */
    PixelImageFormat&
    pixel_type(GLenum v)
    {
      m_pixel_type=v;
      return *this;
    }

    /*!\fn int bytes_per_pixel
      Returns the number of bytes per 
      pixel needed when speciying image 
      data, depends only on m_pixel_type
      and m_pixel_data_format.
     */
    int
    bytes_per_pixel(void) const;
  };

  /*!\class ImageFormat
    Data structure to specify format of image data
    for a WRATHImage, For now we will punt and have 
    it specify the parameters to glTexImage
    directly.
   */
  class ImageFormat
  {
  public:
    /*!\fn ImageFormat
      Ctor to initialize member variables:
      - \ref m_internal_format as GL_RGBA
      - \ref m_magnification_filter as GL_LINEAR
      - \ref m_minification_filter as GL_LINEAR_MIPMAP_NEAREST
      - \ref m_automatic_mipmap_generation as true
      - \ref m_max_mip_level as -1
     */
    ImageFormat(void):
      m_internal_format(GL_RGBA),
      m_magnification_filter(GL_LINEAR),
      m_minification_filter(GL_LINEAR_MIPMAP_NEAREST),
      m_automatic_mipmap_generation(true),
      m_max_mip_level(-1)
    {}

    /*!\var m_internal_format
      Internal format to glTexImage, for GLES2 this
      is one of GL_RGBA, GL_RGB, GL_LUMINANCE,
      GL_ALPHA or GL_LUMINANCE_ALPHA, i.e. only
      specifies number of channels in GLES2.
     */
    GLenum m_internal_format;

    /*!\var m_pixel_format
      In GLES2, the values of the pixel
      format determine the real internal 
      format of a texture (i.e. float,
      byte, packed byte such as 565).
      The memeber m_pixel_format holds
      the 2 values that determine the
      internal format: 
      - \ref PixelImageFormat::m_pixel_data_format and
      - \ref PixelImageFormat::m_pixel_type
      However, under desktop GL, ONLY
      the internal format determines the format
      of the texture. Sighs.
     */
    PixelImageFormat m_pixel_format;

    /*!\var m_magnification_filter
      Specifies the magnification filter to use,
      one of GL_LINEAR or GL_NEAREST
     */
    GLenum m_magnification_filter;

    /*!\var m_minification_filter
      Specifies the minification filter to use,
      one of GL_NEAREST, GL_LINEAR, 
      GL_NEAREST_MIPMAP_NEAREST,
      GL_LINEAR_MIPMAP_NEAREST,
      GL_NEAREST_MIPMAP_LINEAR, or
      GL_LINEAR_MIPMAP_LINEAR
     */
    GLenum m_minification_filter;

    /*!\var m_automatic_mipmap_generation
      Only has affect if \ref m_minification_filter
      specifies that the texture data requires
      mipmaps. If true, whenever the image data
      has changed of a texture, on the next bind
      of the texture, glGenerateMipmap is called
      to regenerate the mipmaps. Note that this
      regeneration happens only at bind, but if one
      changes a small image regularly and it is
      on a large atlus, then a great deal of mipmap
      data is regenerated unnecessarily. Defualt
      value is true.
     */
    bool m_automatic_mipmap_generation;

    /*!\var m_max_mip_level m_max_mip_level
      If non-negative and IF the platform support specifying
      the maximum mipmap level (in GL for desktop this is
      accomplished via GL_TEXTURE_MAX_LEVEL and is a part
      of the GL specification, for GLES2 there is the extenstion
      GL_APPLE_texture_max_level where one may set 
      GL_TEXTURE_MAX_LEVEL_APPLE [which has the same value
      as GL_TEXTURE_MAX_LEVEL]). Initial value is -1.
     */
    int m_max_mip_level;
    
    /*!\fn ImageFormat& max_mip_level
      Sets \ref m_max_mip_level.
      \param v value to which to assign \ref m_max_mip_level
     */
    ImageFormat&
    max_mip_level(int v)
    {
      m_max_mip_level=v;
      return *this;
    }

    /*!\fn int bytes_per_pixel
      Returns the number of bytes per 
      pixel needed when speciying image 
      data, depends only on m_pixel_type
      and m_pixel_data_format.
     */
    int
    bytes_per_pixel(void) const
    {
      return m_pixel_format.bytes_per_pixel();
    }
    
    /*!\fn ImageFormat& internal_format
      Sets \ref m_internal_format.
      \param v value to which to assign \ref m_internal_format
     */
    ImageFormat&
    internal_format(GLenum v)
    {
      m_internal_format=v;
      return *this;
    }

    /*!\fn ImageFormat& pixel_format
      Sets \ref m_pixel_format.
      \param v value to which to assign \ref m_pixel_format
     */
    ImageFormat&
    pixel_format(PixelImageFormat v)
    {
      m_pixel_format=v;
      return *this;
    }

    /*!\fn ImageFormat& pixel_data_format
      Sets \ref PixelImageFormat::m_pixel_data_format of \ref m_pixel_format
      \param v value to which to assign 
     */
    ImageFormat&
    pixel_data_format(GLenum v)
    {
      m_pixel_format.m_pixel_data_format=v;
      return *this;
    }

    /*!\fn ImageFormat& pixel_type
      Sets \ref PixelImageFormat::m_pixel_type of \ref m_pixel_format
      \param v value to which to assign 
     */
    ImageFormat&
    pixel_type(GLenum v)
    {
      m_pixel_format.m_pixel_type=v;
      return *this;
    }

    /*!\fn ImageFormat& magnification_filter
      Sets \ref m_magnification_filter.
      \param v value to which to assign \ref m_magnification_filter
     */
    ImageFormat&
    magnification_filter(GLenum v)
    {
      m_magnification_filter=v;
      return *this;
    }

    /*!\fn ImageFormat& minification_filter
      Sets \ref m_minification_filter.
      \param v value to which to assign \ref m_minification_filter
     */
    ImageFormat&
    minification_filter(GLenum v)
    {
      m_minification_filter=v;
      return *this;
    }

    /*!\fn automatic_mipmap_generation
      Sets \ref m_automatic_mipmap_generation
      \param v value to which to assign \ref m_automatic_mipmap_generation
     */
    ImageFormat&
    automatic_mipmap_generation(bool v)
    { 
      m_automatic_mipmap_generation=v;
      return *this;
    }

    /*!\fn bool requires_mipmaps(GLenum)
      Conveniance function to determine
      if a valid enumeration value for
      GL_TEXTURE_MIN_FILTER implies that
      a texture requires mipmaps.
      \param v valid GL enumeration for for GL_TEXTURE_MIN_FILTER
     */
    static
    bool
    requires_mipmaps(GLenum v)
    {
      return v==GL_NEAREST_MIPMAP_NEAREST
        or v==GL_LINEAR_MIPMAP_NEAREST
        or v==GL_NEAREST_MIPMAP_LINEAR
        or v==GL_LINEAR_MIPMAP_LINEAR;
    }

    /*!\fn bool requires_mipmaps(void) const
      Equivalent to 
      \code
         requires_mipmaps(m_minification_filter)
      \endcode
      i.e. returns true if the minification filter
      implies that the texture requires mipmaps.
     */
    bool
    requires_mipmaps(void) const
    {
      return requires_mipmaps(m_minification_filter);
    }

    /*!\fn bool operator<(const ImageFormat&) const
      ImageFormat provided operator< so
      that the STL default comparison 
      function can be used for sorted
      containers.
      \param rhs ImageFormat to which to compare
     */
    bool
    operator<(const ImageFormat &rhs) const;

    /*!\fn bool platform_compare
      Under GLES2, the values of \ref m_pixel_format
      affect how the texture is stored, where as under
      OpenGL only \ref m_internal_format affects the
      storage format of the texture. Under GLES2,
      this routine is equivalent to \ref 
      bool operator<(const ImageFormat &) const. 
      Under OpenGL, this routine ignores 
      the values of \ref m_pixel_format.
      \param rhs ImageFormat to which to compare
     */
    bool
    platform_compare(const ImageFormat &rhs) const;

    /*!\fn bool operator==(const ImageFormat&) const
      ImageFormat provides operator==
      for equality tests.
     */
    bool
    operator==(const ImageFormat&) const;

    /*!\fn bool platform_equality
      Under GLES2, the values of \ref m_pixel_format
      affect how the texture is stored, where as under
      OpenGL only \ref m_internal_format affects the
      storage format of the texture. Under GLES2,
      this routine is equivalent to \ref 
      bool operator==(const ImageFormat&) const. 
      Under OpenGL, this routine ignores 
      the values of \ref m_pixel_format.
      \param rhs ImageFormat to which to compare
     */
    bool
    platform_equality(const ImageFormat &rhs) const;

    /*!\fn bool operator!=(const ImageFormat&) const
      ImageFormat provides operator!=
      for inequality tests.
     */
    bool
    operator!=(const ImageFormat &obj) const
    {
      return !operator==(obj);
    }
  };

  /*!\class ImageFormatComparer
    Comparison functor object for use by
    std to compare ImageFormat objects
    using \ref ImageFormat::platform_compare.
   */
  class ImageFormatComparer
  {
  public:
    /*!\fn operator()(const ImageFormat&, const ImageFormat&)
      Comparison operator mapping to ImageFormat::platform_compare()
      \param lhs left hand side of comparison (1st argument)
      \param rhs right hand side of comparison (2nd argument)
     */
    bool
    operator()(const ImageFormat &lhs,
               const ImageFormat &rhs) const
    {
      return lhs.platform_compare(rhs);
    }
  };

  /*!\class BoundarySize
    A BoundarySize specifies if a WRATHImage
    is to have a boundary (and where) and
    of what size. Boundary control is per-side,
    thus a WRATHImage can have boundaries on
    a selected subset of sides and can vary
    the thickness on each side too.
   */
  class BoundarySize
  {
  public:
    /*!\fn BoundarySize(void)
      Default ctor, initializing all values as 0.
     */
    BoundarySize(void):
      m_minX(0), m_maxX(0),
      m_minY(0), m_maxY(0)
    {}

    /*!\fn BoundarySize(int, int, int, int)
      Default ctor, initializing each of the 4 values
      \param pminX value to which to initialize \ref m_minX
      \param pmaxX value to which to initialize \ref m_maxX
      \param pminY value to which to initialize \ref m_minY
      \param pmaxY value to which to initialize \ref m_maxY
     */
    BoundarySize(int pminX, int pmaxX, int pminY, int pmaxY):
      m_minX(pminX), m_maxX(pmaxX),
      m_minY(pminY), m_maxY(pmaxY)
    {}

    /*!\fn BoundarySize(int)
      Default ctor, initializing each of the 4 values
      \param p value to which to initialize \ref m_minX, 
      \ref m_maxX, \ref m_minY and \ref m_maxY
     */
    explicit
    BoundarySize(int p):
      m_minX(p), m_maxX(p),
      m_minY(p), m_maxY(p)
    {}

    /*!\fn BoundarySize(const BoundarySize&, int)
      Ctor.
      \param obj BoundarySize from which to copy
      \param LOD -log2 of scaling factor, i.e.
                 all values are taken from obj bitshifted
                 to the right by LOD.
     */
    BoundarySize(const BoundarySize &obj,
                 int LOD):
      m_minX(obj.m_minX>>LOD), 
      m_maxX(obj.m_maxX>>LOD),
      m_minY(obj.m_minY>>LOD), 
      m_maxY(obj.m_maxY>>LOD)
    {}

    /*!\var m_minX
      Thickness of boundary on minX side
     */
    int m_minX;

    /*!\var m_maxX
      Thickness of boundary on maxX side
     */
    int m_maxX;

    /*!\var m_minY
      Thickness of boundary on minY side
     */
    int m_minY;

    /*!\var m_maxY
      Thickness of boundary on maxY side
     */
    int m_maxY;
  };

  /*!\class ImageFormatArray
    A WRATHImage may also be a layered,
    i.e. an array of same sized 2D pixel 
    data with each element of the array
    a potentially different ImageFormat.
    Internally, each layer is a part of
    a _different_ texture, with the image
    at the same exact location within
    each texture. An ImageFormatArray is
    essentially an array of ImageFormat POD's.
   */
  class ImageFormatArray
  {
  public:
    /*!\fn ImageFormatArray(void)
      Ctor. Default empty constructor.
     */
    ImageFormatArray(void)
    {}

    /*!\fn ImageFormatArray(const ImageFormat&)
      Ctor. Initializes the ImageFormatArray
      to be 1 layer of a specified ImageFormat
      \param fmt image format to use for the layer
     */
    ImageFormatArray(const ImageFormat &fmt):
      m_datum(1, fmt)
    {}

    /*!\fn ImageFormatArray(const std::vector<ImageFormat>&)
      Ctor. Initializes the ImageFormatArray
      to have multiple layers as specified by
      an array of ImageFormat objects.
      \param fmt array of ImageFormat object, the i'th
                 layer (as returned by format(int i) )
                 is initialized as fmt[i]
     */
    ImageFormatArray(const std::vector<ImageFormat> &fmt):
      m_datum(fmt)
    {}

    /*!\fn ImageFormatArray& format(unsigned int, const ImageFormat &)
      Set the named layer to the specified ImageFormat.
      \param i layer to assign, if size()<i, the
               new layers are added so that size() is 
               i+1 and all new layers added are initialized
               with fmt
      \param fmt ImageFormat to assign to i'th layer
     */
    ImageFormatArray&
    format(unsigned int i, const ImageFormat &fmt)
    {
      m_datum.resize(std::max(size_t(i+1), m_datum.size()), fmt);
      m_datum[i]=fmt;
      return *this;
    }

    /*!\fn const ImageFormat& format(unsigned int) const
      Return the ImageFormat used for the named layer.
      \param i layer to query
     */
    const ImageFormat&
    format(unsigned int i) const
    {
      return m_datum[i];
    }

    /*!\fn ImageFormat& format(unsigned int)
      Return the ImageFormat used for the named layer.
      \param i layer to query
     */
    ImageFormat&
    format(unsigned int i) 
    {
      return m_datum[i];
    }

    /*!\fn operator[](unsigned int) const
      Equivalent to
      \code
      format(i)
      \endcode
      \param i layer to query
     */
    const ImageFormat&
    operator[](unsigned int i) const
    {
      return m_datum[i];
    }

    /*!\fn operator[](unsigned int)
      Equivalent to
      \code
      format(i)
      \endcode
      \param i layer to query
     */
    ImageFormat&
    operator[](unsigned int i) 
    {
      return m_datum[i];
    }

    /*!\fn size(void) const
      Returns the number of layers specified.
     */
    unsigned int 
    size(void) const
    {
      return m_datum.size();
    }

    /*!\fn bool platform_compare
      Using ImageFormat::platform_compare(), return true
      if and only if this comes before a specified 
      ImageFormatArray.
      \param rhs ImageFormatArray to which to compare
     */
    bool
    platform_compare(const ImageFormatArray &rhs) const;

    /*!\fn bool platform_equality
      Using ImageFormat::platform_equality(), return true
      if and only if this is equivalent to a specified 
      ImageFormatArray.
      \param rhs ImageFormatArray to which to compare
     */
    bool
    platform_equality(const ImageFormatArray &rhs) const;

    /*!\fn operator<(const ImageFormatArray&) const
      Using ImageFormat::operator<(), return true
      if and only if this comes before a specified 
      ImageFormatArray.
      \param rhs ImageFormatArray to which to compare
     */
    bool
    operator<(const ImageFormatArray &rhs) const
    {
      return m_datum<rhs.m_datum;
    }
    
    /*!\fn operator==(const ImageFormatArray &) const
      Using ImageFormat::operator==(), return true
      if and only if this is equivalent to a specified 
      ImageFormatArray.
      \param rhs ImageFormatArray to which to compare
     */
    bool
    operator==(const ImageFormatArray &rhs) const
    {
      return m_datum==rhs.m_datum;
    }

  private:
    std::vector<ImageFormat> m_datum;
  };

  /*!\class ImageFormatArrayComparer
    Comparison functor object for use by
    std to compare ImageFormatArray objects
    using \ref ImageFormatArray::platform_compare.
   */
  class ImageFormatArrayComparer
  {
  public:
    /*!\fn operator()(const ImageFormatArray&, const ImageFormatArray&)
      Comparison operator mapping to ImageFormatArray::platform_compare()
      \param lhs left hand side of comparison (1st argument)
      \param rhs right hand side of comparison (2nd argument)
     */
    bool
    operator()(const ImageFormatArray &lhs,
               const ImageFormatArray &rhs) const
    {
      return lhs.platform_compare(rhs);
    }
  };

  /*!\enum UniquePixelStoreTag
    Enumeration tag type to indicate
    to construct a WRATHImage so that it
    is NOT a portion of a GL texture, but
    occupies an entire GL texture
   */
  enum UniquePixelStoreTag
    {
      /*!
        Only valid value for a UniquePixelStoreTag
       */
      UniquePixelStore
    };

  /*!\class TextureAllocatorHandle
    A TextureAllocatorHandle is a handle to a texture 
    allocator to allocate GL textures on which a
    WRATHImage may reside. The main purpose for
    partioning different WRATHImage object is for the 
    situation if a set of WRATHImage objects are guaranteed
    to have certain padding added and a rendering 
    assumes such padding. The underlying object
    handled by a TextureAllocatorHandle is reference
    counted. In particular if a WRATHImage exists
    which used that obejct to be created, it holds
    a reference to that texture allocator object.
   */
  class TextureAllocatorHandle
  {
  public:
    /*!\fn TextureAllocatorHandle
      Ctor. Initializes the handle as an invalid handle.
     */
    TextureAllocatorHandle(void)
    {}

    /*!\fn bool valid
      Returns true if and only if this
      TextureAllocatorHandle is a valid
      handle to a texture allocator.
     */
    bool
    valid(void) const
    {
      return m_handle.valid();
    }

    /*!\fn void set_clear_bits
      The default for clearing a texture is to set
      all bits as zero. One can specify how the 
      textures of a layered image format (\ref
      ImageFormatArray) are cleared on a layer
      to layer basis.
      \param fmt ImageFormatArray to specify how to clear
      \param bits for each layer specified in fmt, an array
                  of bytes specifying the clear pixel value
                  (for example GL_RGBA8 would have an array
                  length of 4). Layers beyond the length
                  of bits will be cleared with zeros. Those
                  elements of bits that are empty will also
                  be cleared with zeros.
     */
    void
    set_clear_bits(const ImageFormatArray &fmt,
                   const_c_array<std::vector<uint8_t> > bits) const;

    /*!\fn void texture_atlas_dimension(uint32_t, uint32_t) const
      Sets the size used for the underlying
      textures of the texture alases for which
      each created WRATHImage resides. Changing
      this value only has affect on GL textures
      not yet created. Default value is 1024.
      
      A caller needs to make sure that
      this value is not larger than
      WRATHglGet<int>(GL_MAX_TEXTURE_SIZE),
      and if the platform does not support
      mipmaps for non-power of two textures,
      then the passed value must also
      be a power of 2. 
      
      This routine may be freely called 
      from multiple threads.

      \param vx value to which to set the texture atlas x-size
      \param vy value to which to set the texture atlas y-size
    */
    void
    texture_atlas_dimension(uint32_t vx, uint32_t vy) const;

    /*!\fn void texture_atlas_dimension(uint32_t) const
      Provided as a conveniance, equivalent to
      \code
      texture_atlas_dimension(v, v);
      \endcode
     */
    void
    texture_atlas_dimension(uint32_t v) const
    {
      texture_atlas_dimension(v, v);
    }

    /*!\fn vecN<uint32_t, 2> texture_atlas_dimension(void) const
      Returns the size used for the underlying
      textures of the texture alases for which
      each created WRATHImage resides. Changing
      this value only has affect on GL textures
      not yet created. Default value is 1024.
      This routine may be freely called from 
      multiple threads.
    */
    vecN<uint32_t, 2>
    texture_atlas_dimension(void) const;

    /*!\fn bool image_size_valid
      Returns true if the indicated image
      size is small enough to fit into
      one texture. Equivalent to testing
      both dimension seperately against
      texture_atlas_dimension().
      \param sz image size to query
    */
    bool
    image_size_valid(const ivec2 &sz) const
    {
      vecN<uint32_t, 2> S(texture_atlas_dimension());
      return sz.x()>=0 and sz.y()>=0 
        and static_cast<uint32_t>(sz.x())<=S.x()
        and static_cast<uint32_t>(sz.y())<=S.y();
    }  

    /*!\fn enum return_code allocate_multiple_images_on_same_page(const ImageFormatArray&, 
                                                                  const_c_array<ivec2>, 
                                                                  std::list<WRATHImage*>&)
      Allocate multiple WRATHImage objects (all _not_ tracked
      as a resource) so that each of the created WRATHImage
      objects is on the same texture atlas. Routine may
      fail if it is not possible to allocate all such
      images on one atlas.
      \param fmt Image format of teh WRATHImage set to allocate
      \param in_sizes size of each WRATHImage (each image will have
                      that their boundary size in each dimension is 0)
      \param out_images if call succeeds, created WRATHImage objects
                        are appended to out_images
     */
    enum return_code
    allocate_multiple_images_on_same_page(const ImageFormatArray &fmt,
                                          const_c_array<ivec2> in_sizes,
                                          std::list<WRATHImage*> &out_images);


    /*!\fn enum return_code allocate_multiple_images_on_same_page(const ImageFormatArray&,
                                                                  const_c_array<ivec2>,
                                                                  std::vector<WRATHImage*> &)
      Allocate multiple WRATHImage objects (all _not_ tracked
      as a resource) so that each of the created WRATHImage
      objects is on the same texture atlas. Routine may
      fail if it is not possible to allocate all such
      images on one atlas.
      \param fmt Image format of teh WRATHImage set to allocate
      \param in_sizes size of each WRATHImage (each image will have
                      that their boundary size in each dimension is 0)
      \param out_images if call succeeds, created WRATHImage objects
                        are appended to out_images
     */
    enum return_code
    allocate_multiple_images_on_same_page(const ImageFormatArray &fmt,
                                          const_c_array<ivec2> in_sizes,
                                          std::vector<WRATHImage*> &out_images);


    /*!\fn enum return_code allocate_multiple_images_on_same_page(const ImageFormatArray&,
                                                                  const_c_array<ivec2>,
                                                                  const BoundarySize &bd,
                                                                  std::vector<WRATHImage*> &)
      Allocate multiple WRATHImage objects (all _not_ tracked
      as a resource) so that each of the created WRATHImage
      objects is on the same texture atlas. Routine may
      fail if it is not possible to allocate all such
      images on one atlas.
      \param fmt Image format of teh WRATHImage set to allocate
      \param in_sizes size of each WRATHImage 
      \param bd boudnary size of each image
      \param out_images if call succeeds, created WRATHImage objects
                        are appended to out_images
     */
    enum return_code
    allocate_multiple_images_on_same_page(const ImageFormatArray &fmt,
                                          const_c_array<ivec2> in_sizes,
                                          const BoundarySize &bd,
                                          std::vector<WRATHImage*> &out_images);

    /*!\fn enum return_code allocate_multiple_images_on_same_page(const ImageFormatArray&,
                                                                  const_c_array<std::pair<ivec2, BoundarySize> >,
                                                                  std::vector<WRATHImage*> &)
      Allocate multiple WRATHImage objects (all _not_ tracked
      as a resource) so that each of the created WRATHImage
      objects is on the same texture atlas. Routine may
      fail if it is not possible to allocate all such
      images on one atlas.
      \param fmt Image format of teh WRATHImage set to allocate
      \param in_sizes size of each WRATHImage, .first holding the size
                      and .second holding the boundary size
      \param out_images if call succeeds, created WRATHImage objects
                        are appended to out_images
     */
    enum return_code
    allocate_multiple_images_on_same_page(const ImageFormatArray &fmt,
                                          const_c_array<std::pair<ivec2, BoundarySize> > in_sizes,
                                          std::vector<WRATHImage*> &out_images);

    /*!\class texture_consumption_data_type
      A texture_consumption_data_type gives statistics
      on how well a texture atlas utilizes a texture.
     */
    class texture_consumption_data_type
    {
    public:
      texture_consumption_data_type(void):
        m_number_texels(0),
        m_number_texels_used(0)
      {}

      /*!\var m_number_texels
        sum of number texels across all atlases
       */
      int m_number_texels;

      /*!\var m_number_texels_used
        sum of number texels used across all atlases
       */
      int m_number_texels_used;

      /*!\fn float utilization
        Returns the utilization percentage.
       */ 
      float
      utilization(void) const
      {
        return static_cast<float>(m_number_texels_used)/
          static_cast<float>(std::max(1,m_number_texels));
      }
    };

    /*!\fn texture_consumption_data_type texture_consumption(const ImageFormatArray&)
      Returns the texture memory comsumption for
      a particular format.
      \param fmt ImageFormatArray to query
     */
    texture_consumption_data_type
    texture_consumption(const ImageFormatArray &fmt);

    /*!\fn texture_consumption_data_type texture_consumption(void)
      Returns the total texture memory comsumption
     */
    texture_consumption_data_type
    texture_consumption(void);

  private:
    friend class WRATHImage;
    TextureAllocatorHandle(const WRATHReferenceCountedObject::handle &hnd):
      m_handle(hnd)
    {}

    WRATHReferenceCountedObject::handle m_handle;
  };


  /*!\fn WRATHImage(const WRATHImageID&, const ivec2&, const ImageFormatArray&, const BoundarySize&,
                    const TextureAllocatorHandle&)
    Ctor for a WRATHImage. Data store of the WRATHImage will
    be a portion of a GL texture. If either of the dimensions
    of the image size is larger than texture_atlas_dimension(),
    then the created WRATHImage will be invalid. 
    \param pname resource name of the WRATHImage.
    \param sz size of WRATHImage 
    \param fmt image format of the WRATHImage.
    \param pboundary_size size of each boundary in pixels,
                          a WRATHImage's storage is a rectangle of 
                          width=im.m_size.x()+pboundary_size.m_minX+pboundary_size.m_maxX
                          by height=im.m_size.y()+pboundary_size.m_minY+pboundary_size.m_maxY
                          The purpose of having a non-zero boundary
                          is for when one breaks a large image
                          into many images so that filtering
                          works at the edges of the subimages.
    \param texture_allocator referece to TextureAllocator that will
                             allocate the texture room for the WRATHImage.
   */
  WRATHImage(const WRATHImageID &pname, 
             const ivec2 &sz, const ImageFormatArray &fmt,
             const BoundarySize &pboundary_size=BoundarySize(),
             const TextureAllocatorHandle &texture_allocator=default_texture_allocator());

  /*!\fn WRATHImage(const ivec2&, const ImageFormatArray&, const BoundarySize&, const TextureAllocatorHandle&)
    Ctor for a WRATHImage that is NOT registered
    as a resource. Data store of the WRATHImage will
    be a portion of a GL texture. If either of the dimensions
    of the image size is larger than texture_atlas_dimension(),
    then the created WRATHImage will be invalid.
    \param sz size of WRATHImage 
    \param fmt image format of the WRATHImage
    \param pboundary_size size of each boundary in pixels,
                          a WRATHImage's storage is a rectangle of 
                          width=im.m_size.x()+pboundary_size.m_minX+pboundary_size.m_maxX
                          by height=im.m_size.y()+pboundary_size.m_minY+pboundary_size.m_maxY
                          The purpose of having a non-zero boundary
                          is for when one breaks a large image
                          into many images so that filtering
                          works at the edges of the subimages.
    \param texture_allocator referece to TextureAllocator that will
                             allocate the texture room for the WRATHImage.
   */
  WRATHImage(const ivec2 &sz, const ImageFormatArray &fmt,
             const BoundarySize &pboundary_size=BoundarySize(),
             const TextureAllocatorHandle &texture_allocator=default_texture_allocator());

  /*!\fn WRATHImage(const WRATHImageID&, 
                    const ivec2&, const ImageFormatArray&, 
                    enum UniquePixelStoreTag,
                    GLenum, GLenum)
    Ctor for a WRATHImage. Data store of the WRATHImage
    will be an entire GL texture of the same dimensions
    as indicated by the image data.
    \param pname resource name of the WRATHImage
    \param px must have value \ref UniquePixelStore
    \param sz size of WRATHImage 
    \param fmt image format of the WRATHImage
    \param texture_wrap_mode_s texture wrap mode to give to texture
                               for wrapping in s-texture coordinate 
                               (1st coordinate)
    \param texture_wrap_mode_t texture wrap mode to give to texture
                               for wrapping in t-texture coordinate 
                               (2nd coordinate)    
   */
  WRATHImage(const WRATHImageID &pname, 
             const ivec2 &sz, const ImageFormatArray &fmt,
             enum UniquePixelStoreTag px,
             GLenum texture_wrap_mode_s=GL_CLAMP_TO_EDGE,
             GLenum texture_wrap_mode_t=GL_CLAMP_TO_EDGE);

  /*!\fn WRATHImage(const ivec2&, const ImageFormatArray&, enum UniquePixelStoreTag, GLenum, GLenum)
    Ctor for a WRATHImage that is NOT registered
    as a resource. Data store of the WRATHImage
    will be an entire GL texture of the same dimensions
    as indicated by the image data. Can be
    called from another thread aside from the GL context's thread.
    \param sz size of WRATHImage 
    \param fmt image format of the WRATHImage
    \param px must have value \ref UniquePixelStore
    \param texture_wrap_mode_s texture wrap mode to give to texture
                               for wrapping in s-texture coordinate 
                               (1st coordinate)
    \param texture_wrap_mode_t texture wrap mode to give to texture
                               for wrapping in t-texture coordinate 
                               (2nd coordinate)    
   */
  WRATHImage(const ivec2 &sz, const ImageFormatArray &fmt,
             enum UniquePixelStoreTag px,
             GLenum texture_wrap_mode_s=GL_CLAMP_TO_EDGE,
             GLenum texture_wrap_mode_t=GL_CLAMP_TO_EDGE);

  /*!\fn WRATHImage(const WRATHImageID&, const ImageFormat&, GLuint, const ivec2&, const ivec2&)
    Ctor for a WRATHImage. Data store for the WRATHImage
    will come from an existing GL texture. Ctor does 
    NOT issue GL calls, as such may be called from a different
    thread than the thread of the GL context.   
    \param pname resource name of the WRATHImage
    \param fmt image format of the WRATHImage
    \param tex_name GL name of texture
    \param minXminY minX-minY corner (texel) that WRATHImage uses
    \param sz size in pixels
   */
  WRATHImage(const WRATHImageID &pname, const ImageFormat &fmt,
             GLuint tex_name, const ivec2 &minXminY, const ivec2 &sz);
  
  /*!\fn WRATHImage(const ImageFormat&, GLuint, const ivec2&, const ivec2&)
    Ctor for a WRATHImage that is NOT registered as a resource. 
    Data store for the WRATHImage
    will come from an existing GL texture. Ctor does 
    NOT issue GL calls, as such may be called from a different
    thread than the thread of the GL context.   
    \param fmt image format of the WRATHImage
    \param tex_name GL name of texture
    \param minXminY minX-minY corner (texel) that WRATHImage uses
    \param sz size in pixels
   */
  WRATHImage(const ImageFormat &fmt,
             GLuint tex_name, const ivec2 &minXminY, const ivec2 &sz);

  virtual
  ~WRATHImage();

  /*!\fn boost::signals2::connection connect_dtor
    The dtor of a WRATHImage emit's a signal, use this function
    to connect to that signal. The signal is emitted just before
    the WRATHImage is removed from the resource manager which
    in turn is before the underlying GL resources are marked
    as free.
   */
  boost::signals2::connection 
  connect_dtor(const boost::signals2::signal<void () >::slot_type &slot)
  {
    return m_dtor_signal.connect(slot);
  }

  /*!\fn const WRATHImageID& resource_name
    returns the resource name of this WRATHImage.
   */
  const WRATHImageID&
  resource_name(void) const 
  {
    return m_name;
  }

  /*!\fn void register_image
    Registers this WRATHImage if it was constructed unregistered
    to the WRATHImage resource manager. If the WRATHImage
    was already registered, changes the object's resource
    name and registers it under the new name.
   */
  void
  register_image(const WRATHImageID &pid);

  /*!\fn bool valid
    Returns true if the WRATHImage is valid,
    invalid WRATHImage's will WRATHassert in DEBUG
    on access of any of their methods
    and return NULL/Zero values in RELEASE.
   */
  bool
  valid(void) const
  {
    return m_location!=NULL;
  }

  /*!\fn const ImageFormatArray& image_format(void) const
    Returns the ImageFormatArray 
    of the WRATHImage.
   */
  const ImageFormatArray&
  image_format(void) const;

  /*!\fn const ImageFormat& image_format(int) const
    Returns the ImageFormat for 
    the named layer of the
    WRATHImage.
   */
  const ImageFormat&
  image_format(int layer) const
  {
    return image_format()[layer];
  }
  
  /*!\fn void respecify_sub_image(int, int, const PixelImageFormat&, std::vector<uint8_t>&, ivec2, ivec2, int)
    Respecify a portion of the image data of 
    the WRATHImage. This routine can be safely
    called from a seperate thread than the rendering
    thread. The image value update is issued to GL
    on the next time the return value of texture_binder()
    is bound.
    \param layer which layer of the WRATHImage to affect
    \param LOD level of detail, if not-zero, then
               the return value of image_format()
               must require mipmaps AND have not
               have automatic mipmap level generation.
    \param fmt format of the pixels
    \param raw_pixels new values for pixel data, to avoid
                      unneeded copies, the std::vector
                      is swapped with an empty std::vector,
                      (this is a pointer swap in STL),
                      as such, on return raw_pixels
                      will be empty.
    \param min_corner minY minX corner relative to
                      the WRATHImage, negative values
                      indicate affecting the texels
                      of the boundary of the WRATHImage.
    \param psize width and height of data of raw_pixels 
    \param raw_pixels new values for pixel data.
    \param scanline_alignment byte boundary that 
                              scan lines start on
   */
  void
  respecify_sub_image(int layer, int LOD,
                      const PixelImageFormat &fmt,
                      std::vector<uint8_t> &raw_pixels, 
                      ivec2 min_corner, 
                      ivec2 psize, 
                      int scanline_alignment=1);

  /*!\fn void respecify_sub_image(int, const PixelImageFormat&, std::vector<uint8_t>&, ivec2, ivec2, int)
    Provided as a conveniance, equivalent to
    \code
    respecify_sub_image(0, LOD, fmt, raw_pixels, min_corner, psize, scanline_alignment);
    \endcode
    \param LOD level of detail, if not-zero, then
               the return value of image_format()
               must require mipmaps AND have not
               have automatic mipmap level generation.
    \param fmt format of the pixels
    \param raw_pixels new values for pixel data, to avoid
                      unneeded copies, the std::vector
                      is swapped with an empty std::vector,
                      (this is a pointer swap in STL),
                      as such, on return raw_pixels
                      will be empty.
    \param min_corner minY minX corner relative to
                      the WRATHImage, negative values
                      indicate affecting the texels
                      of the boundary of the WRATHImage.
    \param psize width and height of data of raw_pixels 
    \param raw_pixels new values for pixel data.
    \param scanline_alignment byte boundary that 
                              scan lines start on
   */
  void
  respecify_sub_image(int LOD,
                      const PixelImageFormat &fmt,
                      std::vector<uint8_t> &raw_pixels, 
                      ivec2 min_corner, 
                      ivec2 psize, 
                      int scanline_alignment=1)
  {
    respecify_sub_image(0, LOD, fmt, raw_pixels, min_corner, psize, scanline_alignment);
  }

  /*!\fn void clear_sub_image
    Clears the pixels of the specified rectangle of the WRATHImage
    with a specified clear value.
    \param fmt format of the pixels
    \param min_corner minY minX corner relative to
                      the WRATHImage, negative values
                      indicate affecting the texels
                      of the boundary of the WRATHImage.
    \param psize width and height of data of raw_pixels 
    \param bits clear value to use for each pixel
   */
  void
  clear_sub_image(const ImageFormatArray &fmt,
                  const_c_array<std::vector<uint8_t> > bits,
                  ivec2 min_corner, 
                  ivec2 psize);
  
  
  /*!\fn void clear(void)
    Clears the contents of the WRATHImage.
    Function can be called from outside of the 
    GL context, The image value update is issued 
    to GL on the next time the return value of 
    texture_binder() is bound.
   */
  void
  clear(void);

  /*!\fn void clear(ivec2, ivec2)
    Clears the sub-region of the WRATHImage.
    Function can be called from outside of the 
    GL context, The image value update is issued 
    to GL on the next time the return value of 
    texture_binder() is bound.
    \param min_corner minY minX corner relative to
                      the WRATHImage, negative values
                      indicate affecting the texels
                      of the boundary of the WRATHImage.
    \param psize size in pixel clear region
   */
  void
  clear(ivec2 min_corner, ivec2 psize);
  
  /*!\fn const_c_array<WRATHTextureChoice::texture_base::handle> texture_binders
    Returns the texture binders that back the
    WRATHImage, one for each layer of the
    WRATHImage. Multiple WRATHImages can be backed
    by the same GL texture(s).
   */
  const_c_array<WRATHTextureChoice::texture_base::handle>
  texture_binders(void) const;

  /*!\fn WRATHTextureChoice::texture_base::handle texture_binder
    Returns the texture binder that backs the
    named layer of the WRATHImage. Multiple 
    WRATHImages can be backed by the same GL texture.
    \param layer layer to query
   */
  WRATHTextureChoice::texture_base::handle
  texture_binder(unsigned int layer=0) const;
 
  /*!\fn ivec2 minX_minY(void) const
    Returns the location of the
    minX_minY texel of the WRATHImage
    within the GL texture.
    DOES NOT return the position
    of the WRATHImage with the boundary data.
  */
  ivec2
  minX_minY(void) const
  {
    WRATHassert(valid());
    return (valid())?
      m_location->minX_minY() + ivec2(boundary_size().m_minX, 
                                      boundary_size().m_minY):
      ivec2(0,0);
  }

  /*!\fn ivec2 minX_minY(int) const
    Returns the location of the
    minX_minY texel of the WRATHImage
    within the GL texture of a specified
    LOD (mipmap). DOES NOT return the position
    of the WRATHImage with the boundary data.
    \param LOD LevelOfDetail, level 0 indicates
               the base image and level N
               indicates the N'th mipmap
  */
  ivec2
  minX_minY(int LOD) const
  {
    ivec2 bl(minX_minY());
    LOD=std::max(LOD, 0);
    return ivec2(bl.x()>>LOD, bl.y()>>LOD);
  }
  
  /*!\fn ivec2 size(void) const
    Returns the size of the WRATHImage
    (not including the boundary).
  */
  ivec2
  size(void) const
  {
    return (valid())?
      m_location->size() - ivec2(boundary_size().m_minX+boundary_size().m_maxX,
                                 boundary_size().m_minY+boundary_size().m_maxY):
      ivec2(0,0);
  }

  /*!\fn ivec2 size(int) const
    Returns the size of the WRATHImage
    (not including the boundary) of a
    specified LOD(mipmap).
    \param LOD LevelOfDetail, level 0 indicates
               the base image and level N
               indicates the N'th mipmap
  */
  ivec2
  size(int LOD) const
  {
    return size_lod(size(), LOD);
  }

  /*!\fn ivec2 minX_minY_boundary(void) const
    Returns the location within the
    underlying GL texture of the
    minY minX of the boundary.
  */
  ivec2
  minX_minY_boundary(void) const
  {
    WRATHassert(valid());
    return (valid())?
      m_location->minX_minY():
      ivec2(0,0);
  }

  /*!\fn ivec2 minX_minY_boundary(int) const
    Returns the location within the
    underlying GL texture of the
    minY minX of the boundary of
    a specified LOD.
    \param LOD LevelOfDetail, level 0 indicates
               the base image and level N
               indicates the N'th mipmap
   */
  ivec2
  minX_minY_boundary(int LOD) const
  {
    ivec2 bl(minX_minY_boundary());
    LOD=std::max(LOD, 0);
    return ivec2(bl.x()>>LOD, bl.y()>>LOD);
  }

  /*!\fn ivec2 size_including_boundary(void) const
    Returns the size of the WRATHImage
    including the boundary.
  */
  ivec2
  size_including_boundary(void) const
  {
    return (valid())?
      m_location->size():
      ivec2(0,0);
  }

  /*!\fn ivec2 size_including_boundary(int) const
    Returns the size of the WRATHImage
    including the boundary of a specified
    LOD.
    \param LOD LevelOfDetail, level 0 indicates
               the base image and level N
               indicates the N'th mipmap
  */
  ivec2
  size_including_boundary(int LOD) const
  {
    return size_lod(size_including_boundary(), LOD);
  }  
  
  /*!\fn const vec2& minX_minY_texture_coordinate
    Returns the texture coordinate of the
    center of the texel of the minY minX 
    of the WRATHImage. Does NOT include the 
    boundary.
    \param add_central_offset if true returns the
                              texture coordinate of 
                              the center of the 
                              minY minX texel.
  */  
  const vec2&
  minX_minY_texture_coordinate(bool add_central_offset=true) const
  {
    return (add_central_offset)?
      m_minX_minY_texture_coordinate[0]:
      m_minX_minY_texture_coordinate[1];
  }

  /*!\fn const vec2& maxX_maxY_texture_coordinate
    Returns the texture coordinate of the
    center of the texel of the maxY maxX 
    of the WRATHImage. Does NOT include the 
    boundary.
    \param add_central_offset if true returns the
                              texture coordinate of 
                              the center of the 
                              maxY maxX texel.
  */  
  const vec2&
  maxX_maxY_texture_coordinate(bool add_central_offset=true) const
  {
    return (add_central_offset)?
      m_maxX_maxY_texture_coordinate[0]:
      m_maxX_maxY_texture_coordinate[1];
  }

  /*!\fn const BoundarySize& boundary_size
    Returns the size of the boudnary of
    the WRATHImage. The purpose of having 
    a non-zero boundary is for when one 
    breaks a large image into many images 
    so that filtering works at the edges 
    of the subimages. 
   */
  const BoundarySize&
  boundary_size(void) const
  {
    return m_boundary_size;
  }

  /*!\fn bool uses_same_atlas
    Returns true of this WRATHImage and
    the WRATHImage im use the same
    atlas, i.e. the same GL texture(s).
    \param im WRATHImage to which to compare
   */
  bool
  uses_same_atlas(const WRATHImage *im) const
  {
    return (im!=NULL 
            and im->valid()
            and valid()
            and im->m_location->atlas()==m_location->atlas());
  }

  /*!\fn ivec2 atlas_size
    Returns the size of the underlying texture
    that the WRATHImage is on. 
   */
  ivec2
  atlas_size(void) const;
  
  /*!\fn GLuint texture_atlas_glname
    Returns the GL id's of the textures
    of the named layer on which this 
    WRATHImage layer is allocated.
    \param layer layer of WRATHImage to query
   */
  GLuint
  texture_atlas_glname(unsigned int layer=0) const;

  /*!\fn const_c_array<GLuint> texture_atlas_glnames
    Returns the GL id's of the texture
    of the atlas on which this WRATHImage
    is allocated.
   */
  const_c_array<GLuint>
  texture_atlas_glnames(void) const;

  /*!\fn void bind_texture_to_fbo
    Binds the texture used by this
    WRATHImage to the named attachment
    point to the FBO target GL_FRAMEBUFFER.
    May only be called in a thread with a
    current GL context which is in the 
    same share group that created the 
    underlying GL texture of this WRATHImage.
    \param layer layer of the WRATHImage to affect
    \param attachment_loc attachment point to which 
                          to attach the texture, i.e.
                          the 2nd argument of
                          glFramebufferTexture2D.
                         
   */
  void
  bind_texture_to_fbo(GLenum attachment_loc,
                      unsigned int layer=0) const;

  /*!\fn void copy_from_framebuffer
    The WRATHImage wrapper to glCopyTexSubImage*
    family of functions. Copies the values
    from the currently bound framebuffer into
    the WRATHImage. May only be called in a thread 
    with a current GL context which is in the 
    same share group that created the 
    underlying GL texture of this WRATHImage.
    \param location location within the WRATHImage to 
                    which to copy
    \param screen_location location within the curent 
                           framebuffer from which to copy
    \param width_height width (in .x()) and height (in .y())
                        of rectangle from which to copy
    \param layer to which layer to copy
   */
  void
  copy_from_framebuffer(ivec2 location, 
                        ivec2 screen_location,
                        ivec2 width_height,
                        unsigned int layer=0);

  
  /*!\fn const TextureAllocatorHandle& default_texture_allocator
    Returns the default TextureAllocator
    used by WRATHImage to allocate
    texture data.
   */
  static
  const TextureAllocatorHandle&
  default_texture_allocator(void);
  
  /*!\fn TextureAllocatorHandle create_texture_allocator(bool, ivec2, GLenum, GLenum)
    Create and return a handle to a new texture allocator object.
    \param memeset_zero_texture_data if set to true, when creating a new
                                       GL texture, will set all pixel data
                                       of that new texture to 0.
    \param texture_dimension size of each texture created by the
                             texture allocator
    \param texture_wrap_mode_s texture wrap mode to give to texture
                               for wrapping in s-texture coordinate 
                               (1st coordinate) for all textures allocated
                               by the returned texture allocator
    \param texture_wrap_mode_t texture wrap mode to give to texture
                               for wrapping in t-texture coordinate 
                               (2nd coordinate) for all textures allocated
                               by the returned texture allocator   
   */
  static
  TextureAllocatorHandle
  create_texture_allocator(bool memeset_zero_texture_data=false,
                           ivec2 texture_dimension=ivec2(1024, 1024),
                           GLenum texture_wrap_mode_s=GL_CLAMP_TO_EDGE,
                           GLenum texture_wrap_mode_t=GL_CLAMP_TO_EDGE);


  /*!\fn TextureAllocatorHandle create_texture_allocator(bool, int, GLenum, GLenum)
    Create and return a handle to a new texture allocator object.
    \param memeset_zero_texture_data if set to true, when creating a new
                                       GL texture, will set all pixel data
                                       of that new texture to 0.
    \param texture_dimension size of each texture created by the
                             texture allocator
    \param texture_wrap_mode_s texture wrap mode to give to texture
                               for wrapping in s-texture coordinate 
                               (1st coordinate) for all textures allocated
                               by the returned texture allocator
    \param texture_wrap_mode_t texture wrap mode to give to texture
                               for wrapping in t-texture coordinate 
                               (2nd coordinate) for all textures allocated
                               by the returned texture allocator   
   */
  static
  TextureAllocatorHandle
  create_texture_allocator(bool memeset_zero_texture_data,
                           int texture_dimension,
                           GLenum texture_wrap_mode_s=GL_CLAMP_TO_EDGE,
                           GLenum texture_wrap_mode_t=GL_CLAMP_TO_EDGE)
  {
    return create_texture_allocator(memeset_zero_texture_data,
                                    ivec2(texture_dimension, texture_dimension),
                                    texture_wrap_mode_s,
                                    texture_wrap_mode_t);
  }



  /*!\fn void texture_atlas_dimension(uint32_t, uint32_t)
    Provided as a conveniance, equivalent to
    \code
    default_texture_allocator().texture_atlas_dimension(vx, vy)
    \endcode
    \param vx value to which to set the texture atlas x-size
    \param vy value to which to set the texture atlas y-size
   */
  static
  void
  texture_atlas_dimension(uint32_t vx, uint32_t vy)
  {
    default_texture_allocator().texture_atlas_dimension(vx, vy);
  }

  /*!\fn void texture_atlas_dimension(uint32_t)
    Provided as a conveniance, equivalent to
    \code
    default_texture_allocator().texture_atlas_dimension(v, v)
    \endcode
    \param v value to which to set the texture atlas size
   */
  static
  void
  texture_atlas_dimension(uint32_t v)
  {
    default_texture_allocator().texture_atlas_dimension(v);
  }

  /*!\fn vecN<uint32_t, 2> texture_atlas_dimension(void)
    Provided as a conveniance, equivalent to
    \code
    default_texture_allocator().texture_atlas_dimension()
    \endcode
   */
  static
  vecN<uint32_t, 2>
  texture_atlas_dimension(void)
  {
    return default_texture_allocator().texture_atlas_dimension();
  }

  /*!\fn bool image_size_valid
    Provided as a conveniance, equivalent to
    \code
    default_texture_allocator().image_size_valid(sz)
    \endcode
    \param sz image size to query
   */
  static
  bool
  image_size_valid(const ivec2 &sz)
  {
    return default_texture_allocator().image_size_valid(sz);
  }

  /*!\fn bool uses_same_atlases
    Returns true if a range of WRATHImage objects
    use the same texture atlases
    \tparam iterator iterator type to WRATHImage*
    \param begin iterator to 1st WRATHImage pointer in the range
    \param end iterator to 1 past last WRATHImage pointer in the range 
   */
  template<typename iterator>
  static
  bool
  uses_same_atlases(iterator begin, iterator end)
  {
    bool return_value(true);
    if(begin!=end)
      {
        WRATHPixelStore *first_value((*begin)->pixel_store_object());
        for(++begin; begin!=end and return_value; ++begin)
          {
            /*
              The underlying pixel store object of the atlas
              of m_location holds what textures to use, thus
              we need only check that the pixel store object
              is the same.
             */
            return_value=( first_value==(*begin)->pixel_store_object() );
          }
      }
    return return_value;
  }

private:

  static
  ivec2
  size_lod(const ivec2 in_sz, int LOD)
  {
    ivec2 sz(in_sz);
    int mm;

    LOD=std::max(LOD, 0);

    sz.x()>>=LOD;
    sz.y()>>=LOD;
    
    mm=(sz.x()>0 and sz.y()>0)?
      1:0;

    sz.x()=std::max(mm, sz.x());
    sz.y()=std::max(mm, sz.y());

    return sz;
  }

  void
  init(const ivec2 &sz, const ImageFormatArray &fmt,
       GLenum texture_wrap_mode_s,
       GLenum texture_wrap_mode_t);

  void
  init(const WRATHImage::ImageFormat &im,
       GLuint tex_name, const ivec2 &bl, const ivec2 &sz);

  void
  init(const ivec2 &sz, const ImageFormatArray &fmt,
       const TextureAllocatorHandle &tex_allocator);

  /*
    private ctor for creating an array of WRATHImage
    objects on the same atlas
   */
  WRATHImage(const WRATHAtlas::rectangle_handle *rect,
             const BoundarySize &bd);
  

  vec2
  compute_maxX_maxY_texture_coordinate(bool add_central_offset);

  vec2
  compute_minX_minY_texture_coordinate(bool add_central_offset);

  void
  compute_texture_coordinates(void);

  WRATHPixelStore*
  pixel_store_object(void) const
  {
    return (m_location!=NULL)?
      m_location->atlas()->pixelstore():
      NULL;
  }

  void
  clear_implement(ivec2 bl_corner_texture, ivec2 psize);

  BoundarySize m_boundary_size;
  const WRATHAtlas::rectangle_handle *m_location;

  /*
    [0] --> add_central_offset is true
    [1] --> don't add central offset
   */
  vecN<vec2, 2> m_minX_minY_texture_coordinate;
  vecN<vec2, 2> m_maxX_maxY_texture_coordinate;
  vecN<vec2, 2> m_size_texture_coordinate;

  WRATHImageID m_name;
  bool m_on_manager;
  boost::signals2::signal<void () > m_dtor_signal;
};

/*!\fn std::ostream& operator<<(std::ostream &, const WRATHImage::BoundarySize &)
  Overload for << to print the values of a \ref WRATHImage::BoundarySize 
  to an std::ostream.
  \param ostr stream to which to print
  \param obj value to print
 */ 
inline
std::ostream&
operator<<(std::ostream &ostr, const WRATHImage::BoundarySize &obj)
{
  ostr << "[ minX:" << obj.m_minX
       << " maxX:" << obj.m_maxX
       << " minY:" << obj.m_maxY
       << " maxY:" << obj.m_minY
       << "]";
  return ostr;
}


/*! @} */


#endif
