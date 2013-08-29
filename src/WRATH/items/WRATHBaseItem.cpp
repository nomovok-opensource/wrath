/*! 
 * \file WRATHBaseItem.cpp
 * \brief file WRATHBaseItem.cpp
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
#include "WRATHBaseItem.hpp"
#include "WRATHStaticInit.hpp"

namespace
{
  class BaseItemSelectorHoard:boost::noncopyable
  {
  public:

    WRATHMultiGLProgram::Selector m_depth_only;
    WRATHMultiGLProgram::Selector m_color_only;

    static
    BaseItemSelectorHoard&
    hoard(void)
    {
      WRATHStaticInit();
      static BaseItemSelectorHoard R;
      return R;
    }

  private:
    BaseItemSelectorHoard(void):
      m_depth_only(WRATHMultiGLProgram::macro_collection()
                   .add_macro("WRATH_DEPTH_STENCIL_ONLY_DRAW")),
      m_color_only(WRATHMultiGLProgram::macro_collection()
                   .add_macro("WRATH_POST_DEPTH_COLOR_ONLY_DRAW"))
      
                   
    {}
  };
}


WRATHMultiGLProgram::Selector
WRATHBaseItem::
selector_color_depth_draw(void)
{
  return WRATHMultiGLProgram::Selector();
}

WRATHMultiGLProgram::Selector
WRATHBaseItem::
selector_depth_stenicl_only_draw(void)
{
  return BaseItemSelectorHoard::hoard().m_depth_only;
}


WRATHMultiGLProgram::Selector
WRATHBaseItem::
selector_color_post_draw(void)
{
  return BaseItemSelectorHoard::hoard().m_color_only;
}
