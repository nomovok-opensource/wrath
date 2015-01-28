/*! 
 * \file ngl_backend_qt.cpp
 * \brief file ngl_backend_qt.cpp
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
#include <QtOpenGL>
#include <sstream>
#include "ngl_backend_lib.hpp"



void*
ngl_loadFunction(const char *name)
{
  void *return_value(NULL);
  const QGLContext *ctx;
  ctx=QGLContext::currentContext();
  if(ctx!=NULL)
    {
      return_value=(void*)ctx->getProcAddress(QString(name));
    }
 
  WRATHStaticInit();
  static NGLBackendLibEGL R;
  if(return_value==NULL)
    {
      return_value=R.load_function(name);
    }

  return return_value;
  
}
