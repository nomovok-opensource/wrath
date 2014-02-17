/*! 
 * \file WRATHLayerItemWidgetsTranslate.hpp
 * \brief file WRATHLayerItemWidgetsTranslate.hpp
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


#ifndef WRATH_HEADER_LAYER_ITEM_TRANSLATE_WIDGET_HPP_
#define WRATH_HEADER_LAYER_ITEM_TRANSLATE_WIDGET_HPP_

#include "WRATHWidget.hpp"
#include "WRATHWidgetGenerator.hpp"
#include "WRATHLayer.hpp"
#include "WRATHLayerItemWidgets.hpp"
#include "WRATHLayerItemDrawerFactory.hpp"
#include "WRATHLayerItemNodeTranslate.hpp"
#include "WRATHLayerItemNodeLinearGradient.hpp"
#include "WRATHLayerItemNodeColorValue.hpp"



/*! \addtogroup Layer
 * @{
 */

 

/*!\typedef WRATHLayerTranslateWidgetGenerator
  Conveniance typedef using 
  - \ref WRATHLayerItemNodeTranslate for base node type 
 */
typedef WRATHLayerItemWidget<WRATHLayerItemNodeTranslate>::Generator WRATHLayerTranslateWidgetGenerator;

/*!\typedef WRATHLayerTranslateFamilySet
  Conveniance typedef using 
  - \ref WRATHLayerItemNodeTranslate for base node type 
 */
typedef WRATHLayerItemWidget<WRATHLayerItemNodeTranslate>::FamilySet WRATHLayerTranslateFamilySet;



/*! @} */

#endif
