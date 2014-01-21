/*! 
 * \file item.frag.glsl
 * \brief file item.frag.glsl
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


void
shader_main(void)
{
  // the transformation node GLSL code defines
  // this function which will issue a discard
  // if the node comptues the fragment to be 
  // discarded, if the node does not define
  // any clipping or if the node's clipping is
  // determined by another means (for example
  // using GL clip planes) then this function
  // is a no-op.
  discard_if_clipped();

  //minimalistic fragment shader
  wrath_FragColor=vec4(0.5, 0.0, 1.0, 1.0);
}
