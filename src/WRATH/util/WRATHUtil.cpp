/*! 
 * \file WRATHUtil.cpp
 * \brief file WRATHUtil.cpp
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
#include <sstream>
#include <vector>
#include <dirent.h>
#include "WRATHassert.hpp" 
#include "WRATHUtil.hpp"
#include "ieeehalfprecision.h"
#include "WRATHNew.hpp"
#include "WRATHMutex.hpp"
#include "WRATHStaticInit.hpp"

std::string
WRATHUtil::
filename_extension(const std::string &S)
{
  std::string::size_type dot_position;
  dot_position=S.rfind('.');

  if(dot_position!=std::string::npos)
    {
      return S.substr(dot_position+1);
    }
  else
    {
      return std::string();
    }
}


std::string
WRATHUtil::
filename_fullpath(const std::string &S)
{
  #ifdef _WIN32
  {
    DWORD retval;
    char buffer[PATH_MAX+1];
    retval=GetFullPathNameA(S.c_str(), PATH_MAX+1, buffer, NULL);
    if(retval!=0)
      {
        std::string return_value(buffer);
        if(!return_value.empty() and *return_value.rbegin()!='\\')
          {
            ::DIR *dir_ptr(NULL);

            dir_ptr=::opendir(return_value.c_str());
            if(dir_ptr)
              {
                return_value.push_back('\\');
              }
          }
        return return_value;        
      }
    else
      {
        return std::string();
      }
  }
  #else
  {
    char buffer[PATH_MAX+1], *r;

    r=realpath(S.c_str(), buffer);
    if(r!=NULL)
      {
        std::string return_value(r);
        if(!return_value.empty() and *return_value.rbegin()!='/')
          {
            ::DIR *dir_ptr(NULL);

            dir_ptr=::opendir(return_value.c_str());
            if(dir_ptr)
              {
                return_value.push_back('/');
              }
          }
        return return_value;
      }
    else
      {
        return std::string();
      }
  }
  #endif
}

void
WRATHUtil::
convert_to_halfp_from_float_raw(void *dest, const void *src, int number_elements)
{
  singles2halfp(static_cast<uint16_t*>(dest), 
                static_cast<const uint32_t*>(src), 
                number_elements);
}
 
void
WRATHUtil::
convert_to_float_from_halfp_raw(void *dest, const void *src, int number_elements)
{
  halfp2singles(static_cast<uint32_t*>(dest), 
                static_cast<const uint16_t*>(src), 
                number_elements);
}


namespace
{
  class binomial_monster
  {
  public:

    binomial_monster()
    {
      WRATHLockMutex(m_mutex);
      m_values.resize(1, WRATHNew std::vector<int>());
      m_values[0]->resize(1);
      m_values[0]->operator[](0)=1;
      WRATHUnlockMutex(m_mutex);
    }

    ~binomial_monster()
    {
      WRATHLockMutex(m_mutex);
      for(std::vector< std::vector<int>* >::iterator 
            iter=m_values.begin(), end=m_values.end();
          iter!=end; ++iter)
        {
          std::vector<int> *ptr(*iter);
          WRATHDelete(ptr);
        }
      m_values.clear();
      WRATHUnlockMutex(m_mutex);
    }

    const_c_array<int>
    values(unsigned int A)
    {
      std::vector<int> *ptr;

      WRATHLockMutex(m_mutex);
      if(A>=m_values.size())
        {
          unsigned int oldA(m_values.size());
          
          m_values.resize(A+1);
          for(unsigned int n=oldA; n<=A; ++n)
            {
              m_values[n]=WRATHNew std::vector<int>();

              m_values[n]->resize(n+1);
              m_values[n]->operator[](0)=1;
              m_values[n]->operator[](n)=1;
              for(unsigned int k=1;k<n;++k)
                {
                  m_values[n]->operator[](k)=
                    m_values[n-1]->operator[](k-1)
                    + m_values[n-1]->operator[](k);
                }
            }
        }
      ptr=m_values[A];
      WRATHUnlockMutex(m_mutex);

      return const_c_array<int>(*ptr);
    }

  private:
    WRATHMutex m_mutex;
    std::vector< std::vector<int>* > m_values;
  };
}

const_c_array<int>
WRATHUtil::
BinomialCoefficients(int n)
{
  WRATHStaticInit();
  static binomial_monster R;

  WRATHassert(n>=0);
  return R.values(n);
}









