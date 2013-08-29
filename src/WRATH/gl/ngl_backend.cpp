/*! 
 * \file ngl_backend.cpp
 * \brief file ngl_backend.cpp
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

#include <dlfcn.h>
#include <sstream>
#include "WRATHassert.hpp" 
#include "WRATHgl.hpp"
#include "vecN.hpp"
#include "WRATHStaticInit.hpp"



typedef std::ostream* ostream_ptr;

namespace
{
  ostream_ptr&
  the_stream(void)
  {
    WRATHStaticInit();
    static ostream_ptr R(&std::cerr);
    return R;
  }

  bool&
  the_log_bool(void)
  {
    WRATHStaticInit();
    static bool R(false);
    return R;
  }
}

bool
ngl_log_gl_commands(void)
{
  return the_log_bool();
}

void
ngl_log_gl_commands(bool v)
{
  the_log_bool()=v;
}

std::ostream*
ngl_LogStream(void)
{
  return the_stream();
}

void
ngl_LogStream(std::ostream *ptr)
{
  the_stream()=ptr;
}

void 
ngl_on_load_function_error(const char *fname)
{
  WRATHwarning("Unable to load function: \"" << fname << "\"");
}


const char* 
ngl_ErrorCheck(const char *call, const char *function_name, 
               const char *fileName, int line, 
               void* fptr)
{
  static std::string errorData;
  std::ostringstream str;
  int errorcode;
  int count;
 
  
  WRATHunused(call);
  WRATHunused(function_name);
  WRATHunused(fileName);
  WRATHunused(line);

  if(fptr==ngl_functionPointer(glGetError))
    {
      return NULL;
    }

  errorcode=glGetError();
  if(errorcode==GL_NO_ERROR and !ngl_log_gl_commands())
    {
      return NULL;
    }

  for(count=0; errorcode!=GL_NO_ERROR; ++count, errorcode=glGetError() )
    {
      if(count!=0)
        {
          str << ",";
        }

      switch(errorcode)
        {
        case GL_INVALID_ENUM:
          str << "GL_INVALID_ENUM ";
          break;
        case GL_INVALID_VALUE:
          str << "GL_INVALID_VALUE ";
          break;
        case GL_INVALID_OPERATION:
          str << "GL_INVALID_OPERATION ";
          break;
        case GL_OUT_OF_MEMORY:
          str << "GL_OUT_OF_MEMORY ";
          break; 

        default:
          str << "\n\tUnknown errorcode: 0x" << std::hex << errorcode;
        }
      
      
    }

  if(count==0)
    {
      str << "Post-Log(GL command returned)";
    }


  errorData=str.str();
  
  
  return errorData.c_str();
}


const char* 
ngl_preErrorCheck(const char *call, const char *function_name, 
                  const char *fileName, int line, 
                  void* fptr)
{
  WRATHunused(call);
  WRATHunused(function_name);
  WRATHunused(fileName);
  WRATHunused(line);
  WRATHunused(fptr);

  if(ngl_log_gl_commands())
    {
      return "Pre-Log";
    }

  return NULL;
}
