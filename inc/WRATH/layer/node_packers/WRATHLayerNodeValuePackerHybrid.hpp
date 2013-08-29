/*! 
 * \file WRATHLayerNodeValuePackerHybrid.hpp
 * \brief file WRATHLayerNodeValuePackerHybrid.hpp
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


#ifndef __WRATH_LAYER_ITEM_UNIFORM_PACKER_HYBRID_HPP__
#define __WRATH_LAYER_ITEM_UNIFORM_PACKER_HYBRID_HPP__

#include "WRATHConfig.hpp"
#include "WRATHLayerNodeValuePackerBase.hpp"

/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerNodeValuePackerHybrid
  An implementation of WRATHLayerNodeValuePackerBase
  that supports packing node values for a vertex
  and fragment shader using the template parameter
  classes for each stage.
  \tparam VertexPacker a WRATHLayerNodeValuePackerBase derived type
                       that support packing node values into the
                       vertex shader
  \tparam FragmentPacker a WRATHLayerNodeValuePackerBase derived type
                         that support packing node values into the
                         fragment shader                       
 */
template<typename VertexPacker, typename FragmentPacker>
class WRATHLayerNodeValuePackerHybrid:public WRATHLayerNodeValuePackerBase
{
public:
  /*!\fn WRATHLayerNodeValuePackerHybrid
    Ctor
    \param layer passed to ctor of \ref WRATHLayerNodeValuePackerBase
    \param payload passed to ctor of \ref WRATHLayerNodeValuePackerBase
    \param spec passed to ctor of \ref WRATHLayerNodeValuePackerBase
   */
  WRATHLayerNodeValuePackerHybrid(WRATHLayerBase *layer,
                                  const SpecDataProcessedPayload::const_handle &payload,
                                  const ProcessedActiveNodeValuesCollection &spec);


  virtual
  void
  append_state(WRATHSubItemDrawState &skey);

  virtual
  void
  assign_slot(int slot, WRATHLayerItemNodeBase* h, int highest_slot);

  /*!\fn const WRATHLayerNodeValuePackerBase::function_packet& functions()
    function packet to be used that uses WRATHLayerNodeValuePackerHybrid
    to pack node values.
   */
  static
  const WRATHLayerNodeValuePackerBase::function_packet&
  functions(void);

protected:

  virtual
  void
  on_place_on_deletion_list(void);
  

private:

  VertexPacker *m_vertex_packer;
  FragmentPacker *m_fragment_packer;
};

#include "WRATHLayerNodeValuePackerHybridImplement.tcc"

/*! @} */

#endif
