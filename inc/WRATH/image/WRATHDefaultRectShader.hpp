/*! 
 * \file WRATHDefaultRectShader.hpp
 * \brief file WRATHDefaultRectShader.hpp
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



/*! \addtogroup Imaging
 * @{
 */

#ifndef __WRATH_DEFAULT_RECT_SHADER_HPP__
#define __WRATH_DEFAULT_RECT_SHADER_HPP__

#include "WRATHConfig.hpp"
#include "WRATHBrush.hpp"
#include "WRATHShaderSpecifier.hpp"
#include "WRATHShaderBrushSourceHoard.hpp"

/*!\namespace WRATHDefaultRectShader
  Namespace to provide shaders used 
  by \ref WRATHDefaultRectAttributePacker
 */
namespace WRATHDefaultRectShader
{
  /*!\fn const WRATHShaderBrushSourceHoard& shader_hoard  
    Returns the WRATHShaderBrushSourceHoard for
    the default shading of rectangles.
   */
  const WRATHShaderBrushSourceHoard&
  shader_hoard(void);

  /*!\fn const WRATHShaderSpecifier& shader_brush(const WRATHShaderBrush &brush, 
                                                  enum WRATHBaseSource::precision_t)
     Provided as a conveniance, equivalent to
     \code
     shader_hoard().fetch(brush, prec);
     \endcode
  */
  const WRATHShaderSpecifier&
  shader_brush(const WRATHShaderBrush &brush, 
	       enum WRATHBaseSource::precision_t prec);
  
  /*!\fn const WRATHShaderSpecifier& shader_simple(void)
    Fetches (and if necessary generates) a 
    basic WRATHShaderSpecifier for filling
    rectangle with the solid color red
   */
  const WRATHShaderSpecifier&
  shader_simple(void);
};

/*! @} */

#endif
