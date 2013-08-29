/*! 
 * \file WRATHLayerNodeValuePackerTextureFixed.hpp
 * \brief file WRATHLayerNodeValuePackerTextureFixed.hpp
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


#ifndef __WRATH_LAYER_ITEM_UNIFORM_PACKER_TEXTURE_FIXED_HPP__
#define __WRATH_LAYER_ITEM_UNIFORM_PACKER_TEXTURE_FIXED_HPP__

#include "WRATHConfig.hpp"
#include "WRATHLayerNodeValuePackerBase.hpp"


/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerNodeValuePackerTextureFixed
  An implementation of WRATHLayerNodeValuePackerBase
  using a one texture to pack the per node value
  data. The values are available from
  to both the vertex and fragment shaders. Each value
  is packed as 8+8.8+8 fixed point, and thus each
  value is one RGBA8 lookup.
 */
class WRATHLayerNodeValuePackerTextureFixed:public WRATHLayerNodeValuePackerBase
{
public:

  /*!\fn WRATHLayerNodeValuePackerTextureFixed
    Ctor
    \param layer passed to ctor of \ref WRATHLayerNodeValuePackerBase
    \param payload passed to ctor of \ref WRATHLayerNodeValuePackerBase
    \param spec passed to ctor of \ref WRATHLayerNodeValuePackerBase    
   */ 
  explicit
  WRATHLayerNodeValuePackerTextureFixed(WRATHLayerBase *layer,
					const SpecDataProcessedPayload::const_handle &payload,
					const ProcessedActiveNodeValuesCollection &spec);

  virtual
  ~WRATHLayerNodeValuePackerTextureFixed();

  virtual
  void
  append_uniforms(WRATHSubItemDrawState &skey);


  /*!\fn const WRATHLayerNodeValuePackerBase::function_packet& functions(void)
    Returns a function packet for inserting the shader
    code, etc. 
   */
  static
  const WRATHLayerNodeValuePackerBase::function_packet&
  functions(void);

protected:

  virtual
  void
  phase_render_deletion(void);

private:
  WRATHTextureChoice::texture_base::handle m_texture;
};

/*! @} */

#endif
