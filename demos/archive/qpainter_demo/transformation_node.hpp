/*! 
 * \file transformation_node.hpp
 * \brief file transformation_node.hpp
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


#ifndef __TRANSFORMATION_NODE_HPP__
#define __TRANSFORMATION_NODE_HPP__

#include <QObject>
#include "simple_2d_transformation.hpp"

typedef Simple2DTransform TransformNodeType;

class TransformationNode:public QObject
{
  Q_OBJECT

public:

  TransformationNode(TransformationNode *parent=NULL);
  ~TransformationNode(void);

  void
  setValue(const TransformNodeType &v);

  //do NOT save the reference past setting/modifying the value!
  TransformNodeType&
  getRefValue(void);

  const TransformNodeType&
  getValue(void) const;

  const TransformNodeType&
  getGlobalValue(void);

  
private:
  
  void
  walk_update_values(void);

  TransformationNode *m_parent, *m_root;
  TransformNodeType m_local_value, m_global_value;
  bool m_dirty; 
};

#endif
