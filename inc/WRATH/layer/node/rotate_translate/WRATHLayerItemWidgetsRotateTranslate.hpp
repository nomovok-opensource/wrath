/*! 
 * \file WRATHLayerItemWidgetsRotateTranslate.hpp
 * \brief file WRATHLayerItemWidgetsRotateTranslate.hpp
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


#ifndef __WRATH_LAYER_ITEM_ROTATE_TRANSLATE_WIDGET_HPP__
#define __WRATH_LAYER_ITEM_ROTATE_TRANSLATE_WIDGET_HPP__

#include "WRATHWidget.hpp"
#include "WRATHWidgetGenerator.hpp"
#include "WRATHLayer.hpp"
#include "WRATHLayerItemNodeRotateTranslate.hpp"
#include "WRATHLayerItemWidgets.hpp"
#include "WRATHLayerItemDrawerFactory.hpp"




/*! \addtogroup Layer
 * @{
 */
 

/*!\typedef WRATHLayerRotateTranslateWidgetGenerator
  Conveniance typedef using 
  - \ref WRATHLayerItemNodeRotateTranslate for base node type 
 */
typedef WRATHLayerItemWidget<WRATHLayerItemNodeRotateTranslate>::Generator WRATHLayerRotateTranslateWidgetGenerator;


/*!\typedef WRATHLayerRotateTranslateFamilySet
  Conveniance typedef using 
  - \ref WRATHLayerItemNodeRotateTranslate for base node type 
 */
typedef WRATHLayerItemWidget<WRATHLayerItemNodeRotateTranslate>::FamilySet WRATHLayerRotateTranslateFamilySet;

/*! @} */

#endif
