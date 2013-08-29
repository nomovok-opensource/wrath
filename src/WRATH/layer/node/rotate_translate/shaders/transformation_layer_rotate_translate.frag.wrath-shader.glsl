/*! 
 * \file transformation_layer_rotate_translate.frag.wrath-shader.glsl
 * \brief file transformation_layer_rotate_translate.frag.wrath-shader.glsl
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




/*
  RotateTranslate does NOT do clipping!!
 */
void
discard_if_clipped(void)
{}

mediump float
discard_via_alpha(void)
{
  return 1.0;
}


