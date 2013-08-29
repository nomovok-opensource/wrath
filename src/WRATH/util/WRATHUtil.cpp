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

namespace {

std::string
filename_fullpath(const std::string &name, std::list<std::string> &path_list)
{
  //get the path without the ".." and "." in it.
  std::string::const_iterator last_start, current;
  std::string path_piece;
  std::ostringstream retval;
  std::list<std::string>::iterator iter;
  bool is_absolute_path, is_directory;
  
  is_absolute_path=(!name.empty() and name[0]=='/');

  if(!is_absolute_path)
    {
      char *path(NULL);
      path=get_current_dir_name();

      if(path)
        {
          retval << "/" << get_current_dir_name() << "/" << name;
          free(path);
          return filename_fullpath(retval.str(), path_list);
        }      

      /*!
        get_current_dir_name() failed, so we silently
        do something icky: we pretend that name
        was an absolute path.
      */
    }

  is_directory=(!name.empty() and *name.rbegin()=='/');
  path_list.clear();
  
  last_start=current=name.begin();
  if(is_absolute_path)
    {
      while(current!=name.end() and 
            (*current=='/' or *current=='\\'))
        {
          ++last_start; 
          ++current;
        }
    }
  
  for(; current!=name.end(); ++current)
    {
      if('/'==*current or '\\'==*current)
        {
          //found a slash, add a string from the last slash to the current.
          if(last_start!=current)
            {
              path_piece=std::string(last_start,current);
              
              if(path_piece==".." and !path_list.empty() 
                 and path_list.back()!=".." and 
                 (is_absolute_path or path_list.back()!="."))
                  {
                    path_list.pop_back();
                  }
              else if(path_piece!="." 
                      or (!is_absolute_path and path_list.empty()))
                {
                  path_list.push_back(path_piece);
                }             
                last_start=current+1;
            }
          
        }
    }
  
  //and finally add the last bit past the last '/'
  if(last_start!=current)
    {
      path_piece=std::string(last_start,current);
      
      if(path_piece==".." and !path_list.empty() 
         and path_list.back()!=".." and 
           (is_absolute_path or path_list.back()!="."))
        {
          path_list.pop_back();
        }
      else if(path_piece!="." 
              or (!is_absolute_path and path_list.empty()))
        {
          path_list.push_back(path_piece);
        }             
    }
  
  retval.str("");
  
  WRATHassert(is_absolute_path);
  retval << "/";
    
  for(iter=path_list.begin(); iter!=path_list.end(); ++iter)
    {
      if(iter!=path_list.begin())
          {
            retval << "/";
          }
      retval << *iter;
    }
  
  if(is_directory and !path_list.empty())
    {
      retval << "/";
    }
  
  return retval.str();
}

}


std::string
WRATHUtil::
filename_fullpath(const std::string &S)
{
  std::list<std::string> path_list;
  return ::filename_fullpath(S, path_list);
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









