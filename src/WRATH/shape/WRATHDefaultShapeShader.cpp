/*! 
 * \file WRATHDefaultShapeShader.cpp
 * \brief file WRATHDefaultShapeShader.cpp
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




#include "WRATHConfig.hpp"
#include "WRATHDefaultShapeShader.hpp"
#include "WRATHShaderBrushSourceHoard.hpp"

////////////////////////////////////////////
// WRATHDefaultShapeShader methods
const WRATHShaderSpecifier&
WRATHDefaultShapeShader::
shader_brush(const WRATHShaderBrush &brush, 
             enum WRATHBaseSource::precision_t prec)

{
  return shader_hoard().fetch(brush, prec);
}

const WRATHShaderBrushSourceHoard&
WRATHDefaultShapeShader::
shader_hoard(void)
{
  WRATHStaticInit();
  static WRATHShaderBrushSourceHoard R(WRATHGLShader::shader_source_collection()
				       .absorb_shader_stage(GL_VERTEX_SHADER, 
							    WRATHGLShader::shader_source()
                                                            .add_macro("WRATH_APPLY_BRUSH")
							    .add_source("shape.vert.wrath-shader.glsl", 
									WRATHGLShader::from_resource))

				       .absorb_shader_stage(GL_FRAGMENT_SHADER,
							    WRATHGLShader::shader_source()
                                                            .specify_extension("GL_OES_standard_derivatives",
                                                                               WRATHGLShader::enable_extension)
                                                            .add_macro("WRATH_APPLY_BRUSH")
							    .add_source("shape.frag.wrath-shader.glsl", 
									WRATHGLShader::from_resource) ));
						
  return R;
}

const WRATHShaderSpecifier&
WRATHDefaultShapeShader::
shader_simple(void)
{
  
  static WRATHShaderSpecifier R(WRATHGLShader::shader_source()
                                .add_source("shape.vert.wrath-shader.glsl", WRATHGLShader::from_resource),
                                WRATHGLShader::shader_source()
                                .add_source("shape.frag.wrath-shader.glsl", WRATHGLShader::from_resource));

  return R;
}

