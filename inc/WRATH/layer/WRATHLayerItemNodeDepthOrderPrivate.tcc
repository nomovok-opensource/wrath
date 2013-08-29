// -*- C++ -*-

/*! 
 * \file WRATHLayerItemNodeDepthOrderPrivate.tcc
 * \brief file WRATHLayerItemNodeDepthOrderPrivate.tcc
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


#if !defined(__WRATH_LAYER_ITEM_NODE_DEPTH_ORDER_HPP__) || defined(__WRATH_LAYER_ITEM_NODE_DEPTH_ORDER_PRIVATE__)
#error "Direction inclusion of private header file WRATHLayerItemNodeDepthOrderPrivate.tcc" 
#endif

#define __WRATH_LAYER_ITEM_NODE_DEPTH_ORDER_PRIVATE__

template<enum WRATHLayerItemNodeDepthType::depth_order_t pz_order_type, 
         typename T, 
         typename normalizer_type=WRATHUtil::normalizer<int16_t> >
class WRATHLayerItemNodeDepthOrder;

/*
  Template classes in C++ are a pain. One cannot define
  a template class and only specialize a few functions of
  the class, instead on must do the entire class for
  each specialization. The typical way around this is that
  one then creates bases classes that only "do" the functions
  one wishes to specialize and from there use those classes
  in the "real" template class. Things get much, much ickier
  when those functions need to touch private and/or protected
  stuff from the main template class. C++: doing in 500 lines
  of icky hard to read tempalte code what a handfule of parentesis 
  in LISP will do for you.
 */
namespace WRATHLayerItemNodeDepthOrderPrivate
{
  /*
   */
  template<enum WRATHLayerItemNodeDepthType::depth_order_t, 
           typename T, 
           typename normalizer_type>
  class z_order_helper
  {};

  template<typename T, typename normalizer_type>
  class z_order_helper<WRATHLayerItemNodeDepthType::flat_ordering, T, normalizer_type>
  {
  public:
    typedef int global_z_order_type;

  private:
    friend class WRATHLayerItemNodeDepthOrder<WRATHLayerItemNodeDepthType::flat_ordering, T, normalizer_type>;
    typedef WRATHLayerItemNodeDepthOrder<WRATHLayerItemNodeDepthType::flat_ordering, T, normalizer_type> Node;

    void
    register_parent_changes(Node*)
    {}

    void
    note_order_change(Node*)
    {}
    
    void
    compute_z_value(Node *pthis)
    {
      pthis->m_global_z=pthis->m_local_z;
    }

    void
    global_z_order_consumes_slot(bool)
    {}

    bool
    global_start_z(global_z_order_type)
    {
      return false;
    }
  };

  template<typename T, typename normalizer_type>
  class z_order_helper<WRATHLayerItemNodeDepthType::hierarchical_ordering, T, normalizer_type>
  {
  public:
    typedef typename normalizer_type::type global_z_order_type;

  private:
    friend class WRATHLayerItemNodeDepthOrder<WRATHLayerItemNodeDepthType::hierarchical_ordering, T, normalizer_type>;
    typedef WRATHLayerItemNodeDepthOrder<WRATHLayerItemNodeDepthType::hierarchical_ordering, T, normalizer_type> Node;

    z_order_helper(void):
      m_start(normalizer_type::max_value - 1), // the -1 to stay away from +1.0
      m_counter(0),
      m_consumes(true)
    {}

    void
    register_parent_changes(Node *pthis)
    {
      pthis->connect_parent_changed(boost::bind(&z_order_helper::note_order_change,
                                                this, pthis));
      pthis->connect_parent_changed(boost::bind(&z_order_helper::parent_changed,
                                                this, pthis, _1));
    }

    void
    parent_changed(Node *pthis, WRATHLayerItemNodeBase *old_parent)
    {
      if(pthis->parent()==NULL)
        {
          Node *p;

          WRATHassert(dynamic_cast<Node*>(old_parent)!=NULL);
          p=static_cast<Node*>(old_parent);
          m_start=p->root()->m_z_order_helper.m_start;
        }
    }

    void
    note_order_change(Node *pthis)
    {
      bool r1, r2;
      Node *next, *prev;
      Node *parent;

      parent=pthis->parent();
      if(parent==NULL)
        {
          return;
        }

      prev=pthis->previous_sibling();
      next=pthis->next_sibling();

      /*
        we want to know if they are out of order, that is
        why it is "and compare(elements reversed)"
       */
      r1=(prev!=NULL and pthis->compare_children(pthis, prev));
      r2=(next!=NULL and pthis->compare_children(next, pthis));
      if(r1 or r2)
        {
          parent->mark_child_ordering_dirty();
          pthis->mark_dirty();
        }
    }

    void
    compute_z_value(Node *pthis)
    {
      /*
        the node walking is parent first then children in
        order they are sorted. The children are _reverse_
        sorted by m_local_z, thus more negative values
        come later. Since more negative is infront, 
        the counter is decremented for each child.
      */
      if(pthis->parent()!=NULL)
        {
          pthis->m_global_z=pthis->root()->m_z_order_helper.m_counter;
          if(m_consumes)
            {
              --pthis->root()->m_z_order_helper.m_counter;
            }
        }
      else
        {
         
          pthis->m_z_order_helper.m_counter=m_start;
          pthis->m_global_z=pthis->m_z_order_helper.m_counter;
          if(m_consumes)
            {
              --pthis->m_z_order_helper.m_counter;
            }
        }
    }
    
    void
    global_z_order_consumes_slot(bool v)
    {
      m_consumes=v;
    }

    bool
    global_start_z(global_z_order_type v)
    {
      bool r(m_start!=v);
      m_start=v;
      return r;
    }
   
    global_z_order_type m_start, m_counter;
    bool m_consumes;
  };
};
