/*! 
 * \file WRATHLayerItemNodeBase.cpp
 * \brief file WRATHLayerItemNodeBase.cpp
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
#include "WRATHLayerItemNodeBase.hpp"

namespace
{
  class compare_child
  {
  public:
    explicit
    compare_child(const WRATHLayerItemNodeBase *q):
      m_q(q)
    {}

    bool
    operator()(const WRATHLayerItemNodeBase *lhs,
               const WRATHLayerItemNodeBase *rhs) const
    {
      return m_q->compare_children(lhs, rhs);
    }

    const WRATHLayerItemNodeBase *m_q;
  };
}


////////////////////////////////////////////////
// WRATHLayerItemNodeBase methods
const int WRATHLayerItemNodeBase::HierarchyNodeWalk;

WRATHLayerItemNodeBase::
WRATHLayerItemNodeBase(WRATHLayerItemNodeBase* p):
  m_tr(p->triple_buffer_enabler()),
  m_parent(NULL),
  m_root(p->m_root),
  m_is_dirty(false),
  m_child_order_is_dirty(false),
  m_hierarchy_walk_group_order(HierarchyNodeWalk)
{
  WRATHassert(p!=NULL);
  p->add_child(this);
}

WRATHLayerItemNodeBase::
WRATHLayerItemNodeBase(const WRATHTripleBufferEnabler::handle &r):
  m_tr(r),
  m_parent(NULL),
  m_root(this),
  m_is_dirty(false),
  m_child_order_is_dirty(false),
  m_hierarchy_walk_group_order(HierarchyNodeWalk)
{
  m_sig_walk=connect(WRATHTripleBufferEnabler::on_complete_simulation_frame, 
                     WRATHTripleBufferEnabler::pre_update_no_lock,
                     boost::bind(&WRATHLayerItemNodeBase::root_walk, this),
                     m_hierarchy_walk_group_order);
                     
}

WRATHLayerItemNodeBase::
~WRATHLayerItemNodeBase()
{
  //inform parent we no longer exist.
  if(m_parent!=NULL)
    {
      WRATHassert(*m_slot==this);
      m_parent->m_children.erase(m_slot);
      m_parent=NULL;
    }
 
  m_sig_walk.disconnect();
  for(std::list<WRATHLayerItemNodeBase*>::const_iterator 
        iter=m_children.begin(),
        end=m_children.end(); 
      iter!=end; ++iter)
    {
      WRATHLayerItemNodeBase *ptr(*iter);
      /*
        set ptr->m_parent as NULL
        so it does not remove itself from
        this->m_children and thus invalidate
        the iterator iter.
      */
      ptr->m_parent=NULL;
      WRATHDelete(ptr);
    }
}

void
WRATHLayerItemNodeBase::
hierarchy_walk_group_order_implement(int v)
{
  WRATHassert(m_parent==NULL);
  WRATHassert(m_root=this);

  if(v!=m_hierarchy_walk_group_order)
    {
      m_hierarchy_walk_group_order=v;
      m_sig_walk.disconnect();
      m_sig_walk=connect(WRATHTripleBufferEnabler::on_complete_simulation_frame, 
                         WRATHTripleBufferEnabler::pre_update_no_lock,
                         boost::bind(&WRATHLayerItemNodeBase::root_walk, this),
                         m_hierarchy_walk_group_order);
    }
  
}

WRATHLayerItemNodeBase*
WRATHLayerItemNodeBase::
previous_sibling_base(void) const
{
  if(m_parent==NULL)
    {
      return NULL;
    }
  WRATHassert(*m_slot==this);
  if(m_slot==m_parent->m_children.begin())
    {
      return NULL;
    }

  std::list<WRATHLayerItemNodeBase*>::iterator iter(m_slot);
  --iter;
  return *iter;
}

WRATHLayerItemNodeBase*
WRATHLayerItemNodeBase::
next_sibling_base(void) const
{
  if(m_parent==NULL)
    {
      return NULL;
    }
  WRATHassert(*m_slot==this);

  std::list<WRATHLayerItemNodeBase*>::iterator iter(m_slot);
  ++iter;

  if(iter==m_parent->m_children.end())
    {
      return NULL;
    }
  return *iter;
}


void
WRATHLayerItemNodeBase::
add_child(WRATHLayerItemNodeBase *c)
{
  WRATHassert(c->m_parent==NULL);

  c->m_slot=m_children.insert(m_children.end(), c);
  c->m_parent=this;
  if(c->m_root!=m_root)
    {
      c->recurse_set_root(m_root);
    }
  WRATHassert(c->m_root==m_root);  
  mark_dirty();
}

void
WRATHLayerItemNodeBase::
remove_child(WRATHLayerItemNodeBase *c)
{
  WRATHassert(c->m_parent==this);
  WRATHassert(*(c->m_slot)==c);

  m_children.erase(c->m_slot);
  c->m_parent=NULL;
  c->m_slot=m_children.end();
  if(c->m_root!=c)
    {
      c->recurse_set_root(c);
    }
  WRATHassert(c->m_root==c);

}

enum return_code
WRATHLayerItemNodeBase::
parent_base(WRATHLayerItemNodeBase *p)
{
  if(p!=m_parent)
    {
      WRATHLayerItemNodeBase *old_parent(m_parent);

      if(p!=NULL and p->triple_buffer_enabler()!=triple_buffer_enabler())
        {
          return routine_fail;
        }
      /*
        if p is a decendant of this, then p cannot
        be a parent of this since that would make
        a cycle.

        QUESTION: should we skip this step and just
        assume that a user will not mess this up?
        Checiking changes this function from O(1)
        to O(node depth) operation.
       */
      for(WRATHLayerItemNodeBase *q=p; q!=NULL; q=q->m_parent)
        {
          if(q==this)
            {
              return routine_fail;
            }
        }

      //save m_hierarchy_walk_group_order from m_root,
      //if the new parent is NULL, then this will be a new root
      //and will inherit the value from the original root
      m_hierarchy_walk_group_order=m_root->m_hierarchy_walk_group_order;
      if(m_parent!=NULL)
        {
          m_parent->remove_child(this);
        }
      WRATHassert(m_parent==NULL);

      if(p!=NULL)
        {
          p->add_child(this);
        }

      mark_dirty();
      WRATHassert(m_parent==p);
      m_parent_changed_signal(old_parent);
    }
  return routine_success;
}

void
WRATHLayerItemNodeBase::
recurse_set_root(WRATHLayerItemNodeBase *r)
{
  m_root=r;
  m_sig_walk.disconnect();

  if(m_root==this)
    {
      m_sig_walk=connect(WRATHTripleBufferEnabler::on_complete_simulation_frame, 
                         WRATHTripleBufferEnabler::pre_update_no_lock,
                         boost::bind(&WRATHLayerItemNodeBase::root_walk, this),
                         m_hierarchy_walk_group_order);
    }

  for(std::list<WRATHLayerItemNodeBase*>::iterator 
        iter=m_children.begin(),
        end=m_children.end(); 
      iter!=end; ++iter)
    {
      (*iter)->recurse_set_root(r);
    }
        
}

void
WRATHLayerItemNodeBase::
walk_hierarchy(void)
{
  if(m_child_order_is_dirty)
    {
      m_child_order_is_dirty=false;
      m_children.sort(compare_child(this));
    }

  for(std::list<WRATHLayerItemNodeBase*>::const_iterator 
        iter=m_children.begin(),
        end=m_children.end(); 
      iter!=end; ++iter)
    {
      WRATHLayerItemNodeBase *ptr(*iter);
      
      ptr->compute_values();
      ptr->walk_hierarchy();
    }
}


void
WRATHLayerItemNodeBase::
root_walk(void)
{
  WRATHassert(m_root==this);
  if(m_is_dirty)
    {
      compute_values();
      walk_hierarchy();
      m_is_dirty=false;
    }

}



