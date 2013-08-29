/*! 
 * \file WRATHTwoPassDrawer.cpp
 * \brief file WRATHTwoPassDrawer.cpp
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


#include "WRATHGPUConfig.hpp"
#include "WRATHTwoPassDrawer.hpp"
#include "WRATHStaticInit.hpp"

namespace
{
  class DefaultDrawTypeSpecifier:public WRATHTwoPassDrawer::DrawTypeSpecifier
  {
  public:
    virtual
    WRATHDrawType
    draw_type(enum WRATHTwoPassDrawer::drawing_pass_type tp, int v) const
    {
      return (tp==WRATHTwoPassDrawer::opaque_draw_pass)?
        WRATHDrawType::opaque_pass(v):
        WRATHDrawType::transparent_pass(v);
    }
  };

  class ClipDrawTypeSpecifier:public WRATHTwoPassDrawer::DrawTypeSpecifier
  {
  public:
    ClipDrawTypeSpecifier(enum WRATHDrawType::draw_type_t tp):
      m_value(tp)
    {
      WRATHassert(tp==WRATHDrawType::clip_inside_draw
                  or tp==WRATHDrawType::clip_outside_draw);
    }

    virtual
    WRATHDrawType
    draw_type(enum WRATHTwoPassDrawer::drawing_pass_type, int v) const
    {
      return WRATHDrawType(v, m_value);
    }

  private:
    enum WRATHDrawType::draw_type_t m_value;
  };

  void
  kill_two_pass_drawer(WRATHTwoPassDrawer *p)
  {
    WRATHDelete(p);
  }
}

//////////////////////////////////////////
// WRATHTwoPassDrawer::per_item_drawer methods
WRATHTwoPassDrawer::per_item_drawer::
per_item_drawer(WRATHItemDrawer *p, 
                WRATHTwoPassDrawer *m)
{
  first=p;
  if(first!=NULL)
    {
      second=first->connect_dtor( boost::bind(kill_two_pass_drawer, m));
    }
}

//////////////////////////////////////////////////
// WRATHTwoPassDrawer methods
WRATH_RESOURCE_MANAGER_IMPLEMENT(WRATHTwoPassDrawer, 
                                 WRATHTwoPassDrawer::ResourceKey);

WRATHTwoPassDrawer::
WRATHTwoPassDrawer(const ResourceKey &pname,
                   WRATHItemDrawer *popaque_drawer,
                   WRATHItemDrawer *ptranslucent_drawer,
                   WRATHItemDrawer *ptranslucent_drawer_standalon):
  m_resource_name(pname),
  m_passes(per_item_drawer(popaque_drawer, this), 
           per_item_drawer(ptranslucent_drawer, this), 
           per_item_drawer(ptranslucent_drawer_standalon, this))
{
  resource_manager().add_resource(m_resource_name, this);
}

WRATHTwoPassDrawer::
WRATHTwoPassDrawer(WRATHItemDrawer *popaque_drawer,
                   WRATHItemDrawer *ptranslucent_drawer,
                   WRATHItemDrawer *ptranslucent_drawer_standalon):
  m_resource_name(generate_name(popaque_drawer, 
                                ptranslucent_drawer, 
                                ptranslucent_drawer_standalon)),
  m_passes(per_item_drawer(popaque_drawer, this), 
           per_item_drawer(ptranslucent_drawer, this), 
           per_item_drawer(ptranslucent_drawer_standalon, this))
{
  resource_manager().add_resource(m_resource_name, this);
}

WRATHTwoPassDrawer::
~WRATHTwoPassDrawer()
{
  m_dtor_signal();
  resource_manager().remove_resource(this);

  for(int i=0; i<3; ++i)
    {
      if(m_passes[i].first!=NULL)
        {
          m_passes[i].second.disconnect();
        }
    }
}


WRATHTwoPassDrawer::ResourceKey
WRATHTwoPassDrawer::
generate_name(WRATHItemDrawer *popaque_drawer,
              WRATHItemDrawer *ptranslucent_drawer,
              WRATHItemDrawer *ptranslucent_drawer_standalone)
{
  std::ostringstream ostr;

  ostr << popaque_drawer << ":"
       << popaque_drawer->resource_name()
       << ptranslucent_drawer << ":"
       << ptranslucent_drawer->resource_name()
       << ptranslucent_drawer_standalone << ":"
       << ptranslucent_drawer_standalone->resource_name();
  return ostr.str();    
}

WRATHGLStateChange::state_change::handle
WRATHTwoPassDrawer::
translucent_pass_state_change(void)
{
  WRATHStaticInit();
  static WRATHGLStateChange::state_change::handle v(WRATHNew WRATHGLStateChange::blend_state(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
  return v;
}

const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle&
WRATHTwoPassDrawer::
default_pass_specifier(void)
{
  WRATHStaticInit();
  static DrawTypeSpecifier::const_handle R(WRATHNew DefaultDrawTypeSpecifier());
  return R;
}

const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle&
WRATHTwoPassDrawer::
clip_pass_specifier(enum WRATHDrawType::draw_type_t tp)
{
  WRATHStaticInit();
  static DrawTypeSpecifier::const_handle ClipIn(WRATHNew ClipDrawTypeSpecifier(WRATHDrawType::clip_inside_draw));
  static DrawTypeSpecifier::const_handle ClipOut(WRATHNew ClipDrawTypeSpecifier(WRATHDrawType::clip_outside_draw));

  WRATHassert(tp==WRATHDrawType::clip_inside_draw
              or tp==WRATHDrawType::clip_outside_draw);

  return (tp==WRATHDrawType::clip_inside_draw)?
    ClipIn:
    ClipOut;
}
