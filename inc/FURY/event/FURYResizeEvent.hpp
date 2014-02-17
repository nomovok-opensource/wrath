/*! 
 * \file FURYResizeEvent.hpp
 * \brief file FURYResizeEvent.hpp
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


#ifndef FURY_RESIZE_EVENT_HPP_
#define FURY_RESIZE_EVENT_HPP_

#include "WRATHConfig.hpp"
#include "FURYEvent.hpp"

class FURYResizeEvent:
  public FURYEventT<FURYResizeEvent>
{
public:
  FURYResizeEvent(const ivec2 &pold_size,
                  const ivec2 &pnew_size):
    FURYEventT<FURYResizeEvent>(Resize),
    m_old_size(pold_size),
    m_new_size(pnew_size)
  {}

  const ivec2&
  new_size(void) const
  {
    return m_new_size;
  }

  const ivec2&
  old_size(void) const
  {
    return m_old_size;
  }


  virtual
  void
  log_event(std::ostream &ostr) const
  {
    ostr << "ResizeEvent[old_size=" 
         << m_old_size << ", new_size=" 
         << m_new_size << "]";
  }

private:
  ivec2 m_old_size, m_new_size;
};



#endif
