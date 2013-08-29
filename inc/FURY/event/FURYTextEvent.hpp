/*! 
 * \file FURYTextEvent.hpp
 * \brief file FURYTextEvent.hpp
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


#ifndef __FURY_TEXT_EVENT_HPP__
#define __FURY_TEXT_EVENT_HPP__


#include "WRATHConfig.hpp"
#include <vector>

class FURYTextEvent:
  public FURYEventT<FURYTextEvent>
{
public:

  explicit
  FURYTextEvent(std::vector<uint32_t> &pvalues):
    FURYEventT<FURYTextEvent>(FURYEvent::Text)
  {
    std::swap(m_value, pvalues);
  }

  const std::vector<uint32_t>&
  values(void) const
  {
    return m_value;
  }

private:
  std::vector<uint32_t> m_value;
};

#endif
