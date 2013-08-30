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

    WRATHMultiGLProgram::Selector m_selector_non_color_draw;
    WRATHMultiGLProgram::Selector m_selector_color_draw_cover;
    WRATHMultiGLProgram::Selector m_selector_non_color_draw_cover;

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
      m_selector_non_color_draw(WRATHMultiGLProgram::macro_collection()
                                .add_macro("WRATH_NON_COLOR_DRAW")),
      m_selector_color_draw_cover(WRATHMultiGLProgram::macro_collection()
                                  .add_macro("WRATH_COVER_DRAW")),
      m_selector_non_color_draw_cover(WRATHMultiGLProgram::macro_collection()
                                      .add_macro("WRATH_NON_COLOR_DRAW")
                                      .add_macro("WRATH_COVER_DRAW"))
      
                   
    {}
  };
}


WRATHMultiGLProgram::Selector
WRATHBaseItem::
selector_draw(void)
{
  return WRATHMultiGLProgram::Selector();
}



WRATHMultiGLProgram::Selector
WRATHBaseItem::
selector_non_color_draw(void)
{
  return BaseItemSelectorHoard::hoard().m_selector_non_color_draw;
}



WRATHMultiGLProgram::Selector
WRATHBaseItem::
selector_color_draw_cover(void)
{
  return BaseItemSelectorHoard::hoard().m_selector_color_draw_cover;
}



WRATHMultiGLProgram::Selector
WRATHBaseItem::
selector_non_color_draw_cover(void)
{
  return BaseItemSelectorHoard::hoard().m_selector_non_color_draw_cover;
}
