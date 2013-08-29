/*! 
 * \file wrath_demo_image_support.hpp
 * \brief file wrath_demo_image_support.hpp
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


#ifndef __WRATH_DEMO_IMAGE_SUPPORT_HPP__
#define __WRATH_DEMO_IMAGE_SUPPORT_HPP__

#include "WRATHConfig.hpp"


#ifdef WRATH_QT
#include "WRATHQTImageSupport.hpp"
namespace WRATHDemo
{
  using namespace WRATHQT;
}

#else

#include "WRATHSDLImageSupport.hpp"
namespace WRATHDemo
{
  using namespace WRATHSDL;
}
#endif

#endif
