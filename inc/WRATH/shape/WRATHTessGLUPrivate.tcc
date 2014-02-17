// -*- C++ -*-

/*! 
 * \file WRATHTessGLUPrivate.tcc
 * \brief file WRATHTessGLUPrivate.tcc
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



#if !defined(WRATH_HEADER_TESS_GLU_HPP_) || defined(WRATH_HEADER_TESS_GLU_PRIVATE_TCC_)
#error "Direction inclusion of private header file WRATHTessGLUPrivate.tcc" 
#endif


#define WRATH_HEADER_TESS_GLU_PRIVATE_TCC_

class WRATHTessGLU;

namespace WRATHTessGLUPrivate
{
  class polygon_element
  {
  public:
    polygon_element(WRATHTessGLU *t, void *p):
      m_tess(t), m_polygon(p)
    {}

    WRATHTessGLU *m_tess;
    void *m_polygon;
  };
}
