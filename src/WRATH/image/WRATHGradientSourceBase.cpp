/*! 
 * \file WRATHGradientSourceBase.cpp
 * \brief file WRATHGradientSourceBase.cpp
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
#include "WRATHGradientSourceBase.hpp"




//////////////////////////////////////////////
// WRATHGradientSourceBasePrivate methods/classes
namespace WRATHGradientSourceBasePrivate
{
  class NonLinearFacade:public WRATHGradientSourceBase
  {
  public:
    NonLinearFacade(const WRATHGradientSourceBase *src):
      WRATHGradientSourceBase(WRATHGradientSourceBase::is_facade),
      m_src(src)
    {
      m_fully_non_linear_facade=this;
    }
    
    virtual
    enum interpolation_behaviour_t
    adjust_interpolation_behavior(enum interpolation_behaviour_t) const
    {
      return WRATHBaseSource::fully_nonlinear_computation;
    }

    virtual
    bool
    gradient_always_valid(void) const
    {
      return m_src->gradient_always_valid();
    }

  protected:
    virtual
    void
    add_shader_source_code_specify_interpolation_implementation(enum interpolation_behaviour_t ibt,
                                                                std::map<GLenum, WRATHGLShader::shader_source> &src,
                                                                enum precision_t prec,
                                                                const std::string &suffix) const
    {
      WRATHassert(ibt==WRATHBaseSource::fully_nonlinear_computation);
      m_src->add_shader_source_code_specify_interpolation_implementation(ibt, src, prec, suffix);
    }
    
  private:
    const WRATHGradientSourceBase *m_src;
  };

}

/////////////////////////////////////////////////
// WRATHGradientSourceBase methods
WRATHGradientSourceBase::
WRATHGradientSourceBase(void)
{
  m_fully_non_linear_facade=WRATHNew WRATHGradientSourceBasePrivate::NonLinearFacade(this);
}

WRATHGradientSourceBase::
WRATHGradientSourceBase(enum is_facade_t):
  m_fully_non_linear_facade(NULL)
{
  //NonLinearFacade ctor sets it
}
  
WRATHGradientSourceBase::
~WRATHGradientSourceBase()
{
  if(m_fully_non_linear_facade!=this)
    {
      WRATHDelete(m_fully_non_linear_facade);
    }
}


void
WRATHGradientSourceBase::
add_shader_source_code_implement(std::map<GLenum, WRATHGLShader::shader_source> &src,
                                 enum precision_t prec,
                                 const std::string &suffix) const
{
  enum interpolation_behaviour_t ibt;
  ibt=adjust_interpolation_behavior(linear_computation);
  add_shader_source_code_specify_interpolation(ibt, src, prec, suffix);
}

void
WRATHGradientSourceBase::
add_shader_source_code_specify_interpolation(enum interpolation_behaviour_t ibt,
                                             std::map<GLenum, WRATHGLShader::shader_source> &src,
                                             enum precision_t prec,
                                             const std::string &suffix) const
{
  WRATHassert(ibt==adjust_interpolation_behavior(ibt));

  add_shader_source_code_specify_interpolation_implementation(ibt, src, prec, suffix);
  if(suffix.empty())
    {
      WRATHGLShader::shader_source &vs(src[GL_VERTEX_SHADER]);
      WRATHGLShader::shader_source &fs(src[GL_FRAGMENT_SHADER]);
      switch(ibt)
         {
         case linear_computation:
           vs.add_macro("WRATH_LINEAR_GRADIENT");
           fs.add_macro("WRATH_LINEAR_GRADIENT");
           break;
           
         case nonlinear_computation:
           vs.add_macro("WRATH_NON_LINEAR_GRADIENT");
           fs.add_macro("WRATH_NON_LINEAR_GRADIENT");
           break;

         default://fall through
         case fully_nonlinear_computation:
           vs.add_macro("WRATH_NON_LINEAR_GRADIENT")
             .add_macro("WRATH_FULLY_NON_LINEAR_GRADIENT");
           fs.add_macro("WRATH_NON_LINEAR_GRADIENT")
             .add_macro("WRATH_FULLY_NON_LINEAR_GRADIENT");
           break;
         }
    }
}
