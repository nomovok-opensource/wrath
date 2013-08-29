/*! 
 * \file WRATHLayerNodeValuePackerUniformArrays.hpp
 * \brief file WRATHLayerNodeValuePackerUniformArrays.hpp
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


#ifndef __WRATH_LAYER_ITEM_UNIFORM_PACKER_UNIFORM_ARRAYS_HPP__
#define __WRATH_LAYER_ITEM_UNIFORM_PACKER_UNIFORM_ARRAYS_HPP__

#include "WRATHConfig.hpp"
#include "WRATHLayerNodeValuePackerBase.hpp"


/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerNodeValuePackerUniformArrays
  An implementation of WRATHLayerNodeValuePackerBase
  using arrays of uniforms to pack the per node
  value data. The values are only available from
  the vertex shader.
 */
class WRATHLayerNodeValuePackerUniformArrays:public WRATHLayerNodeValuePackerBase
{
public:
  /*!\fn WRATHLayerNodeValuePackerUniformArrays
    Ctor
    \param layer passed to ctor of \ref WRATHLayerNodeValuePackerBase
    \param payload passed to ctor of \ref WRATHLayerNodeValuePackerBase
    \param spec passed to ctor of \ref WRATHLayerNodeValuePackerBase
   */
  WRATHLayerNodeValuePackerUniformArrays(WRATHLayerBase *layer,
					 const SpecDataProcessedPayload::const_handle &payload,
					 const ProcessedActiveNodeValuesCollection &spec);

  virtual
  ~WRATHLayerNodeValuePackerUniformArrays();

  /*!\fn unsigned int size_of_vec4_array(void)
    Returns the length of the uniform array that 
    WRATHLayerNodeValuePackerUniformArrays objects 
    use to pack the per-node values. The array is
    an array of _vec4_'s (not floats), as such if
    the number of per-node values is a multiple of 
    4, then the number of nodes supported by one
    WRATHLayerNodeValuePackerUniformArrays is
    this value. In general the number of nodes of a type
    N supported by a WRATHLayerNodeValuePackerUniformArrays 
    is given by (4*size_of_vec4_array())/K where K
    is the number of values per node rounded up to a multiple
    of 4. Only change the value before
    any WRATHBaseItem or WRATHLayerBase derived 
    objects are created to avoid inconsistent
    results. Default value is 200.
   */
  static
  unsigned int
  size_of_vec4_array(void);

  /*!\fn void size_of_vec4_array(unsigned int)
    Sets the length of the uniform array that 
    WRATHLayerNodeValuePackerUniformArrays objects 
    use to pack the per-node values. The array is
    an array of _vec4_'s (not floats), as such if
    the number of per-node values is a multiple of 
    4, then the number of nodes supported by one
    WRATHLayerNodeValuePackerUniformArrays is
    this value. In general the number of nodes of a type
    N supported by a WRATHLayerNodeValuePackerUniformArrays 
    is given by (4*size_of_vec4_array())/K where K
    is the number of values per node rounded up to a multiple
    of 4. Only change the value before
    any WRATHBaseItem or WRATHLayerBase derived 
    objects are created to avoid inconsistent
    results. Default value is 200.
   */
  static
  void
  size_of_vec4_array(unsigned int v);

  virtual
  void
  append_state(WRATHSubItemDrawState &skey);

  /*!\fn const WRATHLayerNodeValuePackerBase::function_packet& functions()
    function packet to be used that uses WRATHLayerNodeValuePackerUniformArrays
    to pack node values.
   */
  static
  const WRATHLayerNodeValuePackerBase::function_packet&
  functions(void);

protected:

  virtual
  void
  phase_render_deletion(void);

private:
  WRATHUniformData::uniform_by_name_base::handle m_uniform;
};



/*! @} */


#endif
