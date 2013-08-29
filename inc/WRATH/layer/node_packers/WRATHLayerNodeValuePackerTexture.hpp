/*! 
 * \file WRATHLayerNodeValuePackerTexture.hpp
 * \brief file WRATHLayerNodeValuePackerTexture.hpp
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


#ifndef __WRATH_LAYER_ITEM_UNIFORM_PACKER_TEXTURE_HPP__
#define __WRATH_LAYER_ITEM_UNIFORM_PACKER_TEXTURE_HPP__

#include "WRATHConfig.hpp"
#include "WRATHLayerNodeValuePackerBase.hpp"


/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerNodeValuePackerTexture
  An implementation of WRATHLayerNodeValuePackerBase
  using a one texture to pack the per node
  value data. The values are available from
  to both the vertex and fragment shaders.

  This class cannot be used directly by WRATHLayerItemDrawerFactory,
  use the derived classes WRATHLayerNodeValuePackerTextureMediump  
  and WRATHLayerNodeValuePackerTextureHighp. 
 */
class WRATHLayerNodeValuePackerTexture:public WRATHLayerNodeValuePackerBase
{
public:

  /*!\enum texture_packing_type
    Enumeration to specify the floating point texture type  
    a WRATHLayerNodeValuePackerTexture will use: 
    - fp16 (essentially mediump)
    - fp32 (essentially highp)
   */
  enum texture_packing_type
    {
      /*!
        Indicates to use a half floating
        point texture. Using this texture 
        forces that the floating point data
        needs to converted from float to
        half-float.
       */
      fp16_texture,

      /*!
        Indicates to use a full floating
        point texture. Using this texture
        format format avoid format conversion
        at the cost that the texture uses
        twice as much memory.
       */
      fp32_texture,
    };

  /*!\enum texture_channel_type
    Enumeration to describe the number of channels
    per texel.
   */
  enum texture_channel_type
    {
      /*!
	4 channel texture, thus
	4 node values per texel
       */
      four_channel_texture,

      /*!
	2 channel texture, thus
	2 node values per texel
       */
      two_channel_texture,

      /*!
	1 channel texture, thus
	1 node values per texel
       */
      one_channel_texture,
    };

  /*!\fn WRATHLayerNodeValuePackerTexture
    Ctor
    \param layer passed to ctor of \ref WRATHLayerNodeValuePackerBase
    \param payload passed to ctor of \ref WRATHLayerNodeValuePackerBase
    \param spec passed to ctor of \ref WRATHLayerNodeValuePackerBase    
   */ 
  WRATHLayerNodeValuePackerTexture(WRATHLayerBase *layer,
				   const SpecDataProcessedPayload::const_handle &payload,
				   const ProcessedActiveNodeValuesCollection &spec);

  virtual
  ~WRATHLayerNodeValuePackerTexture();

  virtual
  void
  append_state(WRATHSubItemDrawState &skey);


  /*!\fn const WRATHLayerNodeValuePackerBase::function_packet& functions
    Returns a function packet for inserting the shader
    code, etc to use a WRATHLayerNodeValuePackerTexture to pack
    per node values.
    \param type specifies to use half-float (fp16_texture)
                or full-floating (fp32_texture) point texture
    \param ch specifies the number of node values per texel
   */
  static
  const WRATHLayerNodeValuePackerBase::function_packet&
  functions(enum texture_packing_type type, 
	    enum texture_channel_type ch=four_channel_texture);

protected:

  virtual
  void
  phase_render_deletion(void);

private:
  WRATHTextureChoice::texture_base::handle m_texture;
  
};

/*!\class WRATHLayerNodeValuePackerTextureT
  Template class based upon WRATHLayerNodeValuePackerTexture
  where the template parameter determines the number of value 
  per texel and the precision of the texture and to use for 
  storing the per node values.
  \tparam type specifies to use half-float (\ref WRATHLayerNodeValuePackerTexture::fp16_texture)
               or full-floating (\ref WRATHLayerNodeValuePackerTexture::fp32_texture) point texture
  \tparam ch specifies the number of node values per texel
 */ 
template<enum WRATHLayerNodeValuePackerTexture::texture_packing_type type,
	 enum WRATHLayerNodeValuePackerTexture::texture_channel_type ch=WRATHLayerNodeValuePackerTexture::four_channel_texture>
class WRATHLayerNodeValuePackerTextureT:public WRATHLayerNodeValuePackerTexture
{
public:

  /*!\fn WRATHLayerNodeValuePackerTextureT
    Ctor
    \param layer passed to ctor of \ref WRATHLayerNodeValuePackerBase
    \param payload passed to ctor of \ref WRATHLayerNodeValuePackerBase
    \param spec passed to ctor of \ref WRATHLayerNodeValuePackerBase    
   */ 
  WRATHLayerNodeValuePackerTextureT(WRATHLayerBase *layer,
                                    const SpecDataProcessedPayload::const_handle &payload,
                                    const ProcessedActiveNodeValuesCollection &spec):
    WRATHLayerNodeValuePackerTexture(layer, payload, spec)
  {}

  /*!\fn const WRATHLayerNodeValuePackerBase::function_packet& functions
    Returns a function packet for inserting the shader
    code, etc to use a WRATHLayerNodeValuePackerTextureT to pack
    per node values. Equivalent to
    \code
    WRATHLayerNodeValuePackerTexture::functions(type, ch)
    \endcode
   */
  static
  const WRATHLayerNodeValuePackerBase::function_packet&
  functions(void)
  {
    return WRATHLayerNodeValuePackerTexture::functions(type, ch);
  }
};


/*!\typedef WRATHLayerNodeValuePackerTextureFP16
  A WRATHLayerNodeValuePackerTextureFP16 is suitable to 
  be consumed as the uniform packer type for 
  WRATHLayerItemDrawerFactory. It packs the per-node values
  into a half-float point texture (fp16, same essentially
  as mediump precision).
 */
typedef WRATHLayerNodeValuePackerTextureT<WRATHLayerNodeValuePackerTexture::fp16_texture> WRATHLayerNodeValuePackerTextureFP16;

/*!\typedef WRATHLayerNodeValuePackerTextureFP32
  A WRATHLayerNodeValuePackerTextureFP32 is suitable to 
  be consumed as the uniform packer type for 
  WRATHLayerItemDrawerFactory. It packs the per-node values
  into a float point texture (fp32, same essentially
  as highp precision).
 */
typedef WRATHLayerNodeValuePackerTextureT<WRATHLayerNodeValuePackerTexture::fp32_texture> WRATHLayerNodeValuePackerTextureFP32;



/*! @} */


#endif
