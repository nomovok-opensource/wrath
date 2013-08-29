/*! 
 * \file WRATHCanvasHandle.cpp
 * \brief file WRATHCanvasHandle.cpp
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


#include "WRATHCanvasHandle.hpp"
void
WRATHCanvasHandle::
canvas_base(WRATHCanvas *p)
{
  if(p!=m_canvas)
    {
      m_dtor_connect.disconnect();
      m_canvas=p;
      if(p!=NULL)
        {
          WRATHCanvas *null_canvas(NULL);
          m_dtor_connect=p->connect_phased_delete(boost::bind(&WRATHCanvasHandle::canvas_base, 
                                                              this, null_canvas));
        }
    }
}
        
