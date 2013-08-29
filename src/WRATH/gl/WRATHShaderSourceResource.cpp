/*! 
 * \file WRATHShaderSourceResource.cpp
 * \brief file WRATHShaderSourceResource.cpp
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
#include "WRATHassert.hpp"
#include "WRATHShaderSourceResource.hpp"
#include "WRATHMutex.hpp"
#include "WRATHStaticInit.hpp"
#include <map>
#include <iostream>

namespace
{
  class ShaderSourceResourceHoard:boost::noncopyable
  {
  public:
    std::map<std::string, std::string> m_map;
    WRATHMutex m_mutex;
  };

  ShaderSourceResourceHoard&
  hoard(void)
  {
    WRATHStaticInit();
    static ShaderSourceResourceHoard R;
    return R;
  }
}



WRATHShaderSourceResource::
WRATHShaderSourceResource(const std::string &pname, 
                          const std::string &pshader_source_code)
{
  WRATHAutoLockMutex(hoard().m_mutex);
  if(hoard().m_map.find(pname)==hoard().m_map.end())
    {
      hoard().m_map[pname]=pshader_source_code;
    }
  else
    {
      WRATHwarning("WRATHShaderSourceResource: reusing resource name \""
                   << pname << "\"resource not added");
    }
}

const std::string&
WRATHShaderSourceResource::
retrieve_value(const std::string &pname)
{
  static std::string empty_string;
  std::map<std::string, std::string>::iterator iter;

  WRATHAutoLockMutex(hoard().m_mutex);  
  iter=hoard().m_map.find(pname);

  if(iter==hoard().m_map.end())
    {
      WRATHwarning("Failed to fetch internal shader \""
                   << pname << "\" returning empty string for shader code");
    }


  return (iter!=hoard().m_map.end())?
    iter->second:
    empty_string;

}
