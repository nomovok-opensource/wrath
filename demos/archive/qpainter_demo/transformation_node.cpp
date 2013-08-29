/*! 
 * \file transformation_node.cpp
 * \brief file transformation_node.cpp
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


#include <QtGlobal> 
#include "transformation_node.hpp"

TransformationNode::
TransformationNode(TransformationNode *parent):
  QObject(parent),
  m_parent(parent),
  m_root( parent!=NULL?parent->m_root:this),
  m_dirty(false)
{}

TransformationNode::
~TransformationNode()
{}

void
TransformationNode::
setValue(const TransformNodeType &v)
{
  m_root->m_dirty=true;
  m_local_value=v;
}


TransformNodeType&
TransformationNode::
getRefValue(void)
{
  m_root->m_dirty=true;
  return m_local_value;
}


const TransformNodeType&
TransformationNode::
getValue(void) const
{
  return m_local_value;
}

const TransformNodeType&
TransformationNode::
getGlobalValue(void)
{
  if(m_root->m_dirty)
    {
      m_root->walk_update_values();
    }
  Q_ASSERT(!m_dirty);
  return m_global_value;
}


void
TransformationNode::
walk_update_values(void)
{
  m_dirty=false;
  if(m_parent!=NULL)
    {
      Q_ASSERT(!m_parent->m_dirty);
      m_global_value=m_parent->m_global_value * m_local_value;
    }
  else
    {
      m_global_value=m_local_value;
    }

  //now for each child that is a TrasnformationNode:
  for(QObjectList::const_iterator iter=children().begin(),
        end=children().end(); iter!=end; ++iter)
    {
      TransformationNode *child;

      child=qobject_cast<TransformationNode*>(*iter);
      if(child!=NULL)
        {
          child->walk_update_values();
        }
    }
}
