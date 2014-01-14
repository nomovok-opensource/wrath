/*! 
 * \file image-repeat-mode-functions.wrath-shader.glsl
 * \brief file image-repeat-mode-functions.wrath-shader.glsl
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
  WRATH_IMAGE_REPEAT_MODE_PREC needs to be defined
  to determine the precision.
*/
WRATH_IMAGE_REPEAT_MODE_PREC float
wrath_compute_simple(in WRATH_IMAGE_REPEAT_MODE_PREC float in_value)
{
  return in_value;
}

WRATH_IMAGE_REPEAT_MODE_PREC float
wrath_compute_repeat(in WRATH_IMAGE_REPEAT_MODE_PREC float in_value)
{
  return fract(in_value);
}

WRATH_IMAGE_REPEAT_MODE_PREC float
wrath_compute_clamp(in WRATH_IMAGE_REPEAT_MODE_PREC float in_value)
{
  return clamp(in_value, 0.0, 1.0);
}

WRATH_IMAGE_REPEAT_MODE_PREC float
wrath_compute_mirror_repeat(in WRATH_IMAGE_REPEAT_MODE_PREC float in_value)
{
  WRATH_IMAGE_REPEAT_MODE_PREC float v;

  v=2.0*fract(0.5*in_value);
  return 1.0-abs(1.0-v);
}

///////////////////////////////
// vec2 variations

WRATH_IMAGE_REPEAT_MODE_PREC vec2
wrath_compute_simple(in WRATH_IMAGE_REPEAT_MODE_PREC vec2 in_value)
{
  return in_value;
}

WRATH_IMAGE_REPEAT_MODE_PREC vec2
wrath_compute_repeat(in WRATH_IMAGE_REPEAT_MODE_PREC vec2 in_value)
{
  return fract(in_value);
}

WRATH_IMAGE_REPEAT_MODE_PREC vec2
wrath_compute_clamp(in WRATH_IMAGE_REPEAT_MODE_PREC vec2 in_value)
{
  return clamp(in_value, 0.0, 1.0);
}

WRATH_IMAGE_REPEAT_MODE_PREC vec2
wrath_compute_mirror_repeat(in WRATH_IMAGE_REPEAT_MODE_PREC vec2 in_value)
{
  WRATH_IMAGE_REPEAT_MODE_PREC vec2 v;

  v=2.0*fract(0.5*in_value);
  return vec2(1.0, 1.0) - abs( vec2(1.0,1.0) - v );
}




