//  -*- C++ -*-

/*! 
 * \file WRATHWidgetGeneratorImplement.tcc
 * \brief file WRATHWidgetGeneratorImplement.tcc
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


/*
  Should only be included from WRATHWidgetGenerator.hpp
 */

#if !defined(WRATH_HEADER_WIDGET_GENERATOR_HPP_) || defined(WRATH_HEADER_WIDGET_GENERATOR_IMPLEMENT_TCC_)
#error "Direction inclusion of private header file WRATHWidgetGeneratorImplement.tcc"
#endif
#define WRATH_HEADER_WIDGET_GENERATOR_IMPLEMENT_TCC_

////////////////////////////////////////
// WRATHWidgetGeneratorT methods
template<typename pFamilySet>
WRATHWidgetGeneratorT<pFamilySet>::
WRATHWidgetGeneratorT(Canvas *pCanvas,
                      NodeHandle &proot_widget, 
                      int &pz):
  m_z(pz),
  m_default_text_item_pass(0),
  m_default_rect_item_pass(0),
  m_default_stroke_item_pass(0),
  m_default_fill_item_pass(0),
  m_default_stroke_item_aa(WRATHWidgetGenerator::shape_opaque_non_aa),
  m_default_fill_item_aa(WRATHWidgetGenerator::shape_opaque_non_aa)
{
  NodeWidget *ptr(proot_widget.widget());

  if(ptr==NULL)
    {
      ptr=WRATHNew NodeWidget(pCanvas);
      proot_widget.widget(ptr);
    }
  else if(ptr->canvas()!=pCanvas)
    {
      ptr->canvas(pCanvas);
    }

  m_stack.push_back(ptr);
}


template<typename pFamilySet>
template<typename T>
void
WRATHWidgetGeneratorT<pFamilySet>::
pre_treat_widget_implement(T* &q, NodeWidget *n)
{
  if(q==NULL)
    {
      return;
    }
  
  if(q->canvas()!=n->canvas())
    {
      q->canvas(n->canvas());
    }
  
  if(q->template parent_node<Node>()!=n->node())
    {
      q->parent_widget(n);
    }
}

template<typename pFamilySet>
void
WRATHWidgetGeneratorT<pFamilySet>::
push_widget_create_if_needed(NodeWidget *&widget_ptr)
{
  pre_treat_widget(widget_ptr);
  
  if(widget_ptr==NULL)
    {
      widget_ptr=WRATHNew NodeWidget(current());
    }
    
  push_widget(widget_ptr);

  if(Node::z_order_type==WRATHLayerItemNodeDepthType::hierarchical_ordering)
    {
      /*
        only needed when node type's z-ordering is hierarchical
      */
      --m_z;
      widget_ptr->z_order(m_z);
      widget_ptr->global_z_order_consumes_slot(false);
    }

  ++m_counters.m_number_nodes;
}

template<typename pFamilySet>
void
WRATHWidgetGeneratorT<pFamilySet>::
push_node(NodeHandle &smart_widget)
{
  NodeWidget *p(smart_widget.widget());
    
  push_widget_create_if_needed(p);
  smart_widget.widget(p);
}

template<typename pFamilySet>
template<typename WidgetHandle>
void
WRATHWidgetGeneratorT<pFamilySet>::
update_generic(WidgetHandle &widget, NodeWidget *n, int &z)
{
  typename WidgetHandle::Widget *p(widget.widget());

  WRATHassert(p!=NULL);
  
  if(p->canvas()!=n->canvas())
    {
      p->canvas(n->canvas());
    }
  
  if(p!=NULL and p->template parent_node<Node>()!=n->node())
    {
      p->parent_widget(n);
    }
  
    
  --z;
  p->z_order(z);

  
  ++m_counters.m_number_items;
}

template<typename pFamilySet>
template<typename Widget,
         typename WidgetPropertySetter,
         typename WidgetCreator>
void
WRATHWidgetGeneratorT<pFamilySet>::
add_generic_implement(Widget *&widget_ptr,
                      const WidgetPropertySetter &P,
                      const WidgetCreator &C,
                      NodeWidget *n,
                      int &z)
{
  z--;
  ++m_counters.m_number_items;

  pre_treat_widget_implement(widget_ptr, n);

  if(widget_ptr==NULL)
    {
      widget_ptr=C(n);
      ++m_counters.m_number_contructed_items;
    }
  
  
  P(widget_ptr);
  
  widget_ptr->z_order(z);
}


template<typename pFamilySet>
template<typename WidgetHandle,
         typename WidgetPropertySetter,
         typename WidgetCreator>
void
WRATHWidgetGeneratorT<pFamilySet>::
add_generic(WidgetHandle &smart_widget,
            const WidgetPropertySetter &P,
            const WidgetCreator &C,
            NodeWidget *n,
            int &z)
{
  typename WidgetHandle::Widget *p(smart_widget.widget());

  add_generic_implement(p, P, C, n, z);
  smart_widget.widget(p);
}


template<typename pFamilySet>
void
WRATHWidgetGeneratorT<pFamilySet>::
push_canvas_node_implement(DrawnCanvas &canvas)
{
  typename DrawnCanvas::Widget *p(canvas.widget());
  
  pre_treat_widget(p);
  if(p==NULL)
    {
      p=WRATHNew typename DrawnCanvas::Widget(current());
    }

  p->properties()->contents()->child_order(m_stack.back().m_number_child_canvases++);
  canvas.widget(p);

  m_z--;
  p->z_order(m_z);
  ++m_counters.m_number_canvases;
  /*
    now all we need to do is push
    the empty_widget element of p:
   */
  m_stack.push_back(p->empty_widget());
  m_stack.back().m_canvas=canvas.widget();

  WRATHassert(p->empty_widget()->canvas()==p->properties()->contents());
}

template<typename pFamilySet>
void
WRATHWidgetGeneratorT<pFamilySet>::
pop_node(void)
{
  
  if(m_stack.back().m_canvas!=NULL)
    {
      --m_z;
      for(typename std::list<Node*>::const_iterator 
            iter=m_stack.back().m_canvas->clip_out_items().begin(),
            end=m_stack.back().m_canvas->clip_out_items().end();
          iter!=end; ++iter)
        {
          (*iter)->z_order(m_z);
        }
      --m_z;
    }

  m_stack.pop_back();
  WRATHassert(!m_stack.empty());
}


  
