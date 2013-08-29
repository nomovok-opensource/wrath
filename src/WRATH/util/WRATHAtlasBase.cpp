/*! 
 * \file WRATHAtlasBase.cpp
 * \brief file WRATHAtlasBase.cpp
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
#include "WRATHAtlasBase.hpp"

////////////////////////////////////////
// WRATHAtlasBase methods
WRATHAtlasBase::
~WRATHAtlasBase()
{
  if(m_pixelstore!=NULL)
    {
      WRATHDelete(m_pixelstore);
    }
}

enum return_code
WRATHAtlasBase::
delete_rectangle(const WRATHAtlasBase::rectangle_handle *im)
{
  WRATHassert(im!=NULL);
  WRATHassert(im->atlas().valid());

  enum return_code R;
  handle H=im->atlas();
  
  /*
    this is silly and gross. The issue 
    that comes up here is that deleting
    a rectangle_handle makes a handle
    to the underlying WRATHAtlasBase
    object go out of scope. The issue
    comes up is what happens if we
    are deleting the last rectangle
    of that WRATHAtlasBase AND
    there are no more references active.
    Answer: the dtor of the WRATHAtlasBase
    derived object gets called within its
    implementation of remove_rectangle_implement(),
    which is a _BAD_ thing. To avoid this
    we create a temporary handle, H. 
    We call the remove_rectangle_implement
    through H, this way the dtor cannot get
    called since H is in scope. We save the
    return value and then return that value.
    When this function returns, then
    the WRATHAtlasBase object will get 
    deleted if H was the last reference.
   */
  R=H->remove_rectangle_implement(im);

  return R;
}


enum return_code
WRATHAtlasBase::
add_rectangles(const_c_array<ivec2> &dimensions,
               std::list<const rectangle_handle*> &out_rects)
{
  std::list<const rectangle_handle*> new_rects;
  bool all_succeeded(true);
  /*
    the right thing would be to allocate in decreasing
    order of size, but for now we be lazy..
   */
  for(const_c_array<ivec2>::iterator iter=dimensions.begin(),
        end=dimensions.end(); all_succeeded and iter!=end; ++iter)
    {
      const rectangle_handle *R;

      R=add_rectangle(*iter);
      if(R!=NULL)
        {
          new_rects.push_back(R);
        }
      else
        {
          all_succeeded=false;
        }
    }

  if(!all_succeeded)
    {
      for(std::list<const rectangle_handle*>::iterator iter=new_rects.begin(),
            end=new_rects.end(); iter!=end; ++iter)
        {
          delete_rectangle(*iter);
        }

      return routine_fail;
    }

  out_rects.splice(out_rects.end(), new_rects);

  return routine_success;
}
