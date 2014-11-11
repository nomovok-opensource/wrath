/*! 
 * \file WRATHLayer.cpp
 * \brief file WRATHLayer.cpp
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
#include "WRATHLayer.hpp"
#include "WRATHBaseItem.hpp"

/*
  Implementation overview:

   1) Recall that creating/fetching DataHandle's is handled
      by the class from which WRATHLayer inherits, WRATHLayerBase.
      As such WRATHLayer only needs to handle rendering and 
      hierarchy of WRATHLayer objects.

   2) The value of the modelview and projection matrix are triple buffered.
      The values of the previous simulation frame are copied to the
      values of the current simulation frame via the signal whose connection
      is stored in m_sim_connect. In addition, the fields m_child_order
      (indicating the child order of the WRATHLayer within it's parent),
      m_clip_drawer (handle to object to draw inside clipping) 
      are also triple buffered.

   3) There are _2_ seperate lists of children, one for the simulation
      thread and one for the rendering thread. Adding or removing a child
      first adds (or removes) it from the simulation list (m_children) and
      then an action is schedules to add (or remove) it from the render list
      (m_render_children). In addition, the location of a WRATHLayer within
      it's parent lists is also stored (for simulation: m_slot. for rendering:
      m_render_slot). We chose to NOT triple buffer because the lists can be
      non-trivial is size. Walking of m_list and reading/affecting m_parent 
      is mutex locked. Rendering is NOT mutex locked. As a consequence,
      the rendering cannot rely on m_parent to know it's parent.

   4) Deletion is thankfully merciful, calling on_place_on_deletion_list()
      to each child is performed, with the caveat that the children do
      not bother to remove themselves.
 
   5) since access of m_parent is mutex locked and it's value can change
      mid-render, the drawing methods have the argument of what WRATHLayer
      called the drawing method (NULL indicating that it is a root draw).
      The value of m_render_parent is set from this argument. The value
      of m_render_parent is reset back to NULL when the drawing routine
      returns, thus it can only be used during drawing!

   6) The main drawing routine is draw_implement. It operates in states:
      a) First it "opens" the stencil buffer is the WRATHLayer has any 
         clipped-in items or has a clip drawer (m_clip_drawer). This is
         action is done by WRATHLayer::push_clipping. Clipping tracking is 
         maintained by a stack defined by the type draw_state. There are
         2 opens of the clipping buffer: 1st by m_clip_drawer (if it is valid handle)
         and then by clip items. Weather or not a clip_drawer is active,
         the stack is pushed (but in such a way to indicate no additional clipping).
         Only if there are clipped-in items does the stack get pushed again.
         Pushing the stack non-trivially simply means drawing to the stencil
         buffer with stencilop set to increment stencil values and incrementing
         the stencil test value once done. 

      b) Then opaque items of the WRATHLayer are drawn
      c) Then children are recursed
      d) Then transparent items of the WRATHLayer are drawn
      e) Then the clipping stack is popped, undoing the operations of pushing the
         clipping. Popping the stack non-trivially just means drawing to the
         stencil buffer with stencil op to decrementing and setting the stencil
         test to one less once done drawing.
 
*/


namespace
{
  const float4x4&
  matrix(const WRATHLayer *v,
         enum WRATHLayer::matrix_type tp)
  {
    return (tp==WRATHLayer::projection_matrix)?
      v->current_render_transformation().m_composed_projection:
      v->current_render_transformation().m_composed_modelview;
  }
        
  
  void 
  disable_color_buffer_write(WRATHRawDrawData::DrawState &gl_state)
  {
    #ifdef HARMATTAN
    {
       /**
         N9's GLES2 implementation how do I hate thee.
         Doing glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE)
         also disables depth and stencil writes.
         
         So to get around one does:
          
          glEnable(GL_BLEND);
          glBlendFunc(GL_ZERO, GL_ONE);
          glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        but we also need to note the state vector,
        so what we do is that we make the "current"
        state change NULL making it so that the
        if there is a state change on the next drawing
        which is the same as the current one,
        it gets re-instated.
       **/
      if(gl_state.draw_active())
        {
          gl_state.gl_state_change(NULL);
        }
      glEnable(GL_BLEND);
      glBlendFunc(GL_ZERO, GL_ONE);
    }
    #else
    {
      WRATHunused(gl_state);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    }
    #endif
  }
        
  void 
  enable_color_buffer_write(WRATHRawDrawData::DrawState &gl_state)
  {
    #ifdef HARMATTAN
    {
      if(gl_state.draw_active())
        {
          gl_state.gl_state_change(NULL);        
        }
      glDisable(GL_BLEND);
    }
    #else
    {
      WRATHunused(gl_state);
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
    #endif
  }

  class pvm_uniform:
    public WRATHUniformData::uniform_by_name_base
  {
  public:
    pvm_uniform(WRATHLayer *layer, const std::string &loc):
      WRATHUniformData::uniform_by_name_base(loc),
      m_layer(layer)
    {}

    virtual
    void
    set_uniform_value(GLint location)
    {
      WRATHglUniform(location, 
                     m_layer->current_render_transformation().m_composed_pvm);
    }

    WRATHLayer *m_layer;
  };

  class modelview_uniform:
    public WRATHUniformData::uniform_by_name_base
  {
  public:
    modelview_uniform(WRATHLayer *layer, const std::string &loc):
      WRATHUniformData::uniform_by_name_base(loc),
      m_layer(layer)
    {}

    virtual
    void
    set_uniform_value(GLint location)
    {
      WRATHglUniform(location,
                     m_layer->current_render_transformation().m_composed_modelview);
    }

    WRATHLayer *m_layer;
  };

  class projection_uniform:
    public WRATHUniformData::uniform_by_name_base
  {
  public:
    projection_uniform(WRATHLayer *layer, const std::string &loc):
      WRATHUniformData::uniform_by_name_base(loc),
      m_layer(layer)
    {}

    virtual
    void
    set_uniform_value(GLint location)
    {
      WRATHglUniform(location,
                     m_layer->current_render_transformation().m_composed_projection);
    }

    WRATHLayer *m_layer;
  };


}

/////////////////////////////////////
// WRATHLayer::child_sorter methods
bool
WRATHLayer::child_sorter::
operator()(WRATHLayer *lhs, WRATHLayer *rhs) const
{
  return lhs->m_child_order[lhs->present_ID()] < rhs->m_child_order[rhs->present_ID()];
}

//////////////////////////////////
// WRATHLayer::matrix_state uniforms
WRATHLayer::matrix_state::
matrix_state(const std::string &projection_modelview,
                           const std::string &modelview,
                           const std::string &projection):
  m_projection_modelview(projection_modelview),
  m_modelview(modelview),
  m_projection(projection)
{
}

void
WRATHLayer::matrix_state::
append_state(WRATHLayerBase *in_layer,
                WRATHSubItemDrawState &sk) const
{
  WRATHLayer *layer;

  /*
    NOTE: reading of current_render_matrix()
    and current_render_pvm() are _meant_ to
    be read from the rendering thread, hence
    the uniform classes only need a pointer
    to the WRATHLayer to get the job done.
   */
  layer=dynamic_cast<WRATHLayer*>(in_layer);
  if(layer!=NULL)
    {
      if(!m_projection_modelview.empty())
        {
          sk.add_uniform(WRATHNew pvm_uniform(layer, m_projection_modelview));
        }
      if(!m_modelview.empty())
        {
          sk.add_uniform(WRATHNew modelview_uniform(layer, m_modelview));
        }
      if(!m_projection.empty())
        {
          sk.add_uniform(WRATHNew projection_uniform(layer, m_projection));
        }
    }
}


/////////////////////////////////
// WRATHLayer::draw_state methods
WRATHLayer::draw_state::
draw_state(void)
{
  push_back(NULL, WRATHLayerClipDrawer::layer_unclipped, 0);
}

void
WRATHLayer::draw_state::
push_back(WRATHLayer *layer,
          const WRATHLayerClipDrawer::DrawStateElementClipping &cl,
          int stencil_value)
{
  if(layer==NULL)
    {
      WRATHassert(m_stack.empty());
      WRATHassert(stencil_value==0);
    }

  m_stack.push_back(draw_state_element());

  m_stack.back().m_clipping_mode=cl.m_clip_mode;
  m_stack.back().m_stencil_value=stencil_value;

  m_stack.back().m_write_z=false;
  m_stack.back().m_clipped=true;

  switch(m_stack.back().m_clipping_mode)
    {
    case WRATHLayerClipDrawer::layer_clipped_hierarchy:  
      m_stack.back().m_write_z=true;
      ++m_stack.back().m_stencil_value;   
      break;
  
    case WRATHLayerClipDrawer::layer_clipped_sibling: 
      ++m_stack.back().m_stencil_value;  
      break;

    case WRATHLayerClipDrawer::layer_unclipped:
      m_stack.back().m_clipped=false;
      break;

    default:
      break;
    }
  
  m_draw_stack.push_back(layer);
  if(layer!=NULL)
    {
      m_draw_stack.back().m_transformations=layer->current_render_transformation();
      m_draw_stack.back().m_clipping=cl;
    }

}

void
WRATHLayer::draw_state::
pop_back(void)
{
  WRATHassert(!m_stack.empty());
  WRATHassert(!m_draw_stack.empty());
  
  m_stack.pop_back();  
  m_draw_stack.pop_back();
}

/////////////////////////////////
// WRATHLayer methods
WRATHLayer::
WRATHLayer(const WRATHTripleBufferEnabler::handle &tr,
           const WRATHLayerClipDrawer::handle &pclipper,
           const WRATHDrawOrderComparer::handle sorter):
  WRATHLayerBase(tr, sorter),
  m_child_count(0),
  m_parent(NULL),
  m_root(this),
  m_render_parent(NULL),
  m_child_order(0), 
  m_clip_drawer(pclipper),
  m_visible(1),
  m_render_children_need_sorting(true)
{
  /*
    Note! it is post_update_no_lock so that we copy the matrix
    values from the just completed simulation frame to the next
    simulation frame.
   */
  m_sim_connect=connect(WRATHTripleBufferEnabler::on_complete_simulation_frame,
                        WRATHTripleBufferEnabler::post_update_no_lock,
                        boost::bind(&WRATHLayer::on_end_simulation_frame, this));

  
}

WRATHLayer::
WRATHLayer(WRATHLayer *pparent, 
           const WRATHLayerClipDrawer::handle &pclipper,
           const WRATHDrawOrderComparer::handle sorter):
  WRATHLayerBase(pparent->triple_buffer_enabler(), sorter),
  m_child_count(0),
  m_parent(NULL),
  m_root(this),
  m_render_parent(NULL),
  m_child_order(0), 
  m_clip_drawer(pclipper),
  m_visible(1),
  m_render_children_need_sorting(true)
{
  parent(pparent);
  /*
    Note! it is post_update_no_lock so that we copy the matrix
    values from the just completed simulation frame to the next
    simulation frame.
   */
  m_sim_connect=connect(WRATHTripleBufferEnabler::on_complete_simulation_frame,
                        WRATHTripleBufferEnabler::post_update_no_lock,
                        boost::bind(&WRATHLayer::on_end_simulation_frame, this));
}



WRATHLayer::
WRATHLayer(WRATHLayer *pparent, 
           enum WRATHLayer::inherit_values_type,
           const WRATHLayerClipDrawer::handle &pclipper):
  WRATHLayerBase(pparent->triple_buffer_enabler(),
                 pparent->sorter()),
  m_child_count(0),
  m_parent(NULL),
  m_root(this),
  m_render_parent(NULL),
  m_child_order(0), 
  m_clip_drawer(pclipper)
{
  parent(pparent);
  /*
    Note! it is post_update_no_lock so that we copy the matrix
    values from the just completed simulation frame to the next
    simulation frame.
   */
  m_sim_connect=connect(WRATHTripleBufferEnabler::on_complete_simulation_frame,
                        WRATHTripleBufferEnabler::post_update_no_lock,
                        boost::bind(&WRATHLayer::on_end_simulation_frame, this));
}



WRATHLayer::
~WRATHLayer()
{
}

void
WRATHLayer::
on_place_on_deletion_list(void)
{
  m_sim_connect.disconnect();

  if(m_parent!=NULL)
    {
      m_parent->remove_child(this);
    }
  WRATHassert(m_parent==NULL);

  WRATHLockMutex(m_mutex);
  for(std::list<WRATHLayer*>::iterator iter=m_children.begin(),
        end=m_children.end(); iter!=end; ++iter)
    {
      WRATHLayer *C(*iter);

      C->m_parent=NULL;
      WRATHPhasedDelete(C);
    }
  m_children.clear();
  WRATHUnlockMutex(m_mutex);

  WRATHLayerBase::on_place_on_deletion_list();
}


enum return_code
WRATHLayer::
parent(WRATHLayer *p)
{
  WRATHLayer *old_parent(NULL);

  if(p==this)
    {
      return routine_fail;
    }

  //scope the mutex lock.
  {
    WRATHAutoLockMutex(m_parent_mutex);
    if(m_parent==p)
      {
        return routine_success;
      }

    old_parent=m_parent;
    if(p!=NULL)
      {
        for(WRATHLayer *q=p->parent(); q!=NULL; q=q->parent())
          {
            if(q==this)
              {
                return routine_fail;
              }
          }
      }
    
    if(m_parent!=NULL)
      {
        m_parent->remove_child(this);
      }
    
    if(p!=NULL)
      {
        m_root=p->m_root;
        p->add_child(this);
      }
    else
      {
        m_root=this;
      }
    WRATHassert(p==m_parent);
  } //scope mutex lock

  m_parent_change_signal(old_parent, m_parent);
  return routine_success;
}


void
WRATHLayer::
add_child(WRATHLayer *child)
{
  {
    WRATHAutoLockMutex(m_mutex);
    WRATHAutoLockMutex(child->m_mutex);
    WRATHassert(child->m_parent==NULL);
    
    child->m_parent=this;
    add_child_implement(&m_children, child, &child->m_slot);
    
    schedule_rendering_action( boost::bind(&WRATHLayer::mark_render_sort_order_dirty,
                                           this));
    schedule_rendering_action( boost::bind(&WRATHLayer::add_child_implement,
                                           this, &m_render_children, 
                                           child, &child->m_render_slot));
  
    ++m_child_count;
  }

  m_child_add_signal(child);
}


void
WRATHLayer::
remove_child(WRATHLayer *child)
{
  {
    WRATHAutoLockMutex(m_mutex);
    WRATHAutoLockMutex(child->m_mutex);
    
    WRATHassert(child->m_parent==this);
    child->m_parent=NULL;
    remove_child_implement(&m_children, child, &child->m_slot);
    
    /*
      removing a child does NOT make the list ordering dirty because
      the child list is an std::list
    */
    schedule_rendering_action( boost::bind(&WRATHLayer::remove_child_implement,
                                           this, &m_render_children, 
                                           child, &child->m_render_slot));
    --m_child_count;
  }
  m_child_remove_signal(child);
}


void
WRATHLayer::
add_child_implement(std::list<WRATHLayer*> *ptr_array,
                    WRATHLayer *child, 
                    std::list<WRATHLayer*>::iterator *ptr_child_slot_value)
{
  std::list<WRATHLayer*> &array(*ptr_array);
  std::list<WRATHLayer*>::iterator &child_slot_value(*ptr_child_slot_value);

  child_slot_value=array.insert(array.end(), child);
}

void
WRATHLayer::
remove_child_implement(std::list<WRATHLayer*> *ptr_array,
                       WRATHLayer *child, 
                       std::list<WRATHLayer*>::iterator *ptr_child_slot_value)
{
  std::list<WRATHLayer*> &array(*ptr_array);
  std::list<WRATHLayer*>::iterator &child_slot_value(*ptr_child_slot_value);

  WRATHassert(*child_slot_value==child);
  WRATHunused(child);

  array.erase(child_slot_value);
  child_slot_value=array.end();
}

void
WRATHLayer::
set_render_matrices(const float4x4 *pre_modelview_matrix)
{
  //compute render matrix values:
  compute_render_matrix_value(m_current_render_transformation.m_composed_projection, projection_matrix);
  compute_render_matrix_value(m_current_render_transformation.m_composed_modelview, modelview_matrix);

  if(pre_modelview_matrix!=NULL)
    {
      m_current_render_transformation.m_composed_modelview=
        (*pre_modelview_matrix) * m_current_render_transformation.m_composed_modelview;
    }

  m_current_render_transformation.m_composed_pvm=
    m_current_render_transformation.m_composed_projection*m_current_render_transformation.m_composed_modelview;
  
}

void
WRATHLayer::
compute_render_matrix_value(float4x4 &output, enum matrix_type tp)
{
  int ID(present_ID());

  const per_matrix &RHS(m_matrices[tp][ID]);

  output=RHS.m_matrix;
  if(RHS.m_modifier.valid())
    {
      RHS.m_modifier->modify_matrix(output);
    }

  if(m_render_parent!=NULL and RHS.m_mode==compose_matrix)
    {
      const float4x4 &LHS(matrix(m_render_parent, tp));
      output=LHS*output;
    }
}

void
WRATHLayer::
visible(bool b)
{
  if(b)
    {
      __sync_fetch_and_or(&m_visible, 1);
    }
  else
    {
      __sync_fetch_and_and(&m_visible, 0);
    }
}

bool
WRATHLayer::
visible(void) 
{
  return __sync_fetch_and_or(&m_visible, 0)!=0;
}


void
WRATHLayer::
on_end_simulation_frame(void)
{
  int from(last_simulation_ID());
  int to(current_simulation_ID());

  m_matrices[projection_matrix][to]=m_matrices[projection_matrix][from];
  m_matrices[modelview_matrix][to]=m_matrices[modelview_matrix][from];
  m_clip_drawer[to]=m_clip_drawer[from];
  m_child_order[to]=m_child_order[from];
}

void
WRATHLayer::
child_order(int v)
{
  int oldv(m_child_order[current_simulation_ID()]);

  m_child_order[current_simulation_ID()]=v;

  WRATHAutoLockMutex(m_parent_mutex);
  if(v!=oldv and m_parent!=NULL)
    {
      schedule_rendering_action( boost::bind(&WRATHLayer::mark_render_sort_order_dirty,
                                             m_parent));
    
    }
}

void
WRATHLayer::
clear_and_draw(GLbitfield mask,
               const float4x4 *pre_modelview_matrix,
               draw_information *p)
{
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
  glStencilMask(~0);
  glClearStencil(0);

#if defined(WRATH_GL_VERSION)
  glClearDepth(1.0);
#else
  glClearDepthf(1.0f);
#endif

  glClear(mask);
  draw(pre_modelview_matrix, p);
}

void
WRATHLayer::
draw(const float4x4 *pre_modelview_matrix,
     draw_information *p)
{
  draw_state state_stack;
  draw_information R;

  if(p==NULL)
    {
      p=&R;
    }


  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
  glStencilMask(~0);

  glEnable(GL_STENCIL_TEST);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  WRATHRawDrawData::DrawState gl_state(WRATHMultiGLProgram::Selector(), p);

  gl_state.draw_begin();
  draw_implement(pre_modelview_matrix, state_stack, gl_state, *p, NULL);
  gl_state.draw_end();
}


void
WRATHLayer::
draw_implement(const float4x4 *pre_modelview_matrix,
               draw_state &state_stack,
               WRATHRawDrawData::DrawState &gl_state,
               draw_information &stats,
               WRATHLayer *from)
{
  bool have_clip_items(false);

  if(!visible())
    {
      return;
    }

  

  ++stats.m_layer_count;
  m_render_parent=from;

  //pre_modelview_matrix must be NULL if m_root is not this.
  WRATHassert(pre_modelview_matrix==NULL or m_render_parent==NULL);

  set_render_matrices(pre_modelview_matrix);



  /*
    push clipping
   */
  if(false==push_clipping(state_stack, have_clip_items, gl_state))
    {
      /*
        completely clipped, return immediately
        [also note that return false indicates
        that state_stack was not affected].
      */
      m_render_parent=NULL;
      return;
    }

  /*
    draw occluders with
    - color writes off
    - depth writes on   
    - depth test on (must this be on?)
   */
  if(!render_raw_datas(WRATHDrawType::clip_outside_draw).empty())
    {
      glDepthMask(GL_TRUE);
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);
      disable_color_buffer_write(gl_state);
      gl_state.selector(WRATHBaseItem::selector_non_color_draw());
      draw_render_items(gl_state, render_raw_datas(WRATHDrawType::clip_outside_draw) );
      gl_state.flush_draws();
    }

  draw_content_pre_children(gl_state);

  /*
    sort children if needed:
   */
  if(m_render_children_need_sorting)
    {
      m_render_children.sort(child_sorter());
      m_render_children_need_sorting=false;
    }

  /*
    draw children
   */
  for(std::list<WRATHLayer*>::iterator iter=m_render_children.begin(),
        end=m_render_children.end(); iter!=end; ++iter)
    {
      //note: only the root WRATHLayer uses pre_modelview_matrix.
      (*iter)->draw_implement(NULL, state_stack, gl_state, stats, this);
    }

  draw_content_post_children(gl_state);

  /*
    restore clipping
   */
  pop_clipping(state_stack, have_clip_items, gl_state);

  m_render_parent=NULL;
}

void
WRATHLayer::
draw_render_items(WRATHRawDrawData::DrawState &gl_state,
                  const std::map<int, WRATHRawDrawData*> &items)
{
  for(std::map<int, WRATHRawDrawData*>::const_iterator 
        iter=items.begin(), end=items.end();
      iter!=end; ++iter)
    {
      iter->second->draw(gl_state);
    }
}

void
WRATHLayer::
draw_content_pre_children(WRATHRawDrawData::DrawState &gl_state)
{
  /*
    set:
    - depth writes on, depth test on
    - color buffer writes on, blending off

    Note: enable_color_buffer_write is called
    first because the HARMATTAN workaround
    futzes with the blending state.
   */
  enable_color_buffer_write(gl_state);
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glDisable(GL_BLEND);

  gl_state.selector(WRATHBaseItem::selector_draw());
  draw_render_items(gl_state, render_raw_datas(WRATHDrawType::opaque_draw));
  gl_state.flush_draws();

  glDepthFunc(GL_ALWAYS);
  gl_state.selector(WRATHBaseItem::selector_draw());
  draw_render_items(gl_state, render_raw_datas(WRATHDrawType::opaque_overdraw));
  gl_state.flush_draws();
}



void
WRATHLayer::
draw_content_post_children(WRATHRawDrawData::DrawState &gl_state)
{
  if(render_raw_datas(WRATHDrawType::transparent_draw).empty() 
     and render_raw_datas(WRATHDrawType::transparent_overdraw).empty())
    {
      return;
    }

  /*
    set:
    - depth writes off, depth test on
    - color buffer writes on, blending on

    Note: enable_color_buffer_write is called
    first because the HARMATTAN workaround
    futzes with the blending state.
   */
  enable_color_buffer_write(gl_state);
  glDepthMask(GL_FALSE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_BLEND);

  gl_state.selector(WRATHBaseItem::selector_draw());
  draw_render_items(gl_state, render_raw_datas(WRATHDrawType::transparent_draw));
  gl_state.flush_draws();

  glDisable(GL_DEPTH_TEST);
  gl_state.selector(WRATHBaseItem::selector_draw());
  draw_render_items(gl_state, render_raw_datas(WRATHDrawType::transparent_overdraw));
  gl_state.flush_draws();
}

void
WRATHLayer::
push_clipped_in_items(draw_state &state_stack, bool &have_clip_items,
                      WRATHRawDrawData::DrawState &gl_state)
{
  std::map<int, WRATHRawDrawData*>::const_iterator i, e;

  i=render_raw_datas(WRATHDrawType::clip_inside_draw).begin();
  e=render_raw_datas(WRATHDrawType::clip_inside_draw).end();

  have_clip_items=(i!=e);
  if(have_clip_items)
    {
      /*
        We need to flush the drawing because
        we are changing the framebuffer state
        in a very nasty way.
       */
      gl_state.flush_draws();

      const draw_state_element &v(state_stack.back());
      int current_stencil;
      current_stencil=v.m_stencil_value;

      state_stack.push_back(this, 
                            WRATHLayerClipDrawer::layer_clipped_sibling, 
                            current_stencil);
      
      /*
        Clip-in items do not care about the z-value,
        thus z-test is always but we do not affect 
        the values of the z-buffer either.
       */
      disable_color_buffer_write(gl_state);
      glStencilFunc(GL_EQUAL, current_stencil, ~0);
      glDepthMask(GL_FALSE);
      glDepthFunc(GL_ALWAYS);

      glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
      gl_state.selector(WRATHBaseItem::selector_non_color_draw());
      for(;i!=e; ++i)
        {
          i->second->draw(gl_state);
        }
      gl_state.flush_draws();

      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);  
      glStencilFunc(GL_EQUAL, state_stack.back().m_stencil_value, ~0);
    }
}

void
WRATHLayer::
pop_clipped_in_items(draw_state &state_stack, bool have_clip_items,
                     WRATHRawDrawData::DrawState &gl_state)
{
  if(have_clip_items)
    {
      const draw_state_element &v(state_stack.back());

      /*
        We need to flush the drawing because
        we are changing the framebuffer state
        in a very nasty way.
       */
      gl_state.flush_draws();

      /*
        Restore the stencil buffer values. We do
        not affect z-values and do not care about them
        either.
       */

      disable_color_buffer_write(gl_state);

      glStencilFunc(GL_EQUAL, v.m_stencil_value, ~0);
      glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

      //depth func is set to always so that
      //regardless of what was drawn, the stencil
      //buffer gets decremented; but we do NOT
      //want to change the depth values, so
      //depth buffer is masked out.
      glDepthFunc(GL_ALWAYS);
      glDepthMask(GL_FALSE);

      gl_state.selector(WRATHBaseItem::selector_non_color_draw_cover());
      draw_render_items(gl_state, render_raw_datas(WRATHDrawType::clip_inside_draw));
      gl_state.flush_draws();

      state_stack.pop_back();

      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
      glStencilFunc(GL_EQUAL, state_stack.back().m_stencil_value, ~0);
    }
}


bool
WRATHLayer::
push_clipping(draw_state &state_stack, bool &have_clip_items,
              WRATHRawDrawData::DrawState &gl_state)
{
  WRATHLayerClipDrawer::DrawStateElementClipping cl(WRATHLayerClipDrawer::layer_unclipped);
  int current_stencil;
  const WRATHLayerClipDrawer::handle &clip_drawer(render_clip_drawer());

  if(clip_drawer.valid())
    {
      cl=clip_drawer->clip_mode(this, 
                                m_current_render_transformation,
                                state_stack.draw_stack());
    }

  if(cl.m_clip_mode==WRATHLayerClipDrawer::skip_layer)
    {
      return false;
    }

  
  /*
    Draw clipped in items first
   */
  push_clipped_in_items(state_stack, have_clip_items, gl_state);

  
  const draw_state_element &v(state_stack.back());
  current_stencil=v.m_stencil_value;

  
  /*
    RULE: we always, always push the clipping stack
    for the clip_drawer even if it is NULL.
   */
  state_stack.push_back(this, cl, current_stencil);

  if(state_stack.back().m_clipped)
    {
      WRATHassert(clip_drawer.valid());

      disable_color_buffer_write(gl_state);
      gl_state.draw_end();
      
      // pass stencil test only if stencil value equals
      // current render depth.
      glStencilFunc(GL_EQUAL, current_stencil, ~0);
      
      if(state_stack.back().m_write_z)
        {
          /*
            m_write_z true indicates that we are to 
            act as if we have our own private
            depth buffer, as such the drawing of the
            clip region will draw z values that
            need to pass the depth test.
          */
          glDepthMask(GL_TRUE);
          glDepthFunc(GL_LESS);
        }
      else
        {
          /*
            m_write_z false means that the 
            virtual draw order of out stuff is
            in the same as our parent's thus,
            we draw the clip region with the
            z-test always passing ANd we do
            NOT write to z-buffer.
           */
          glDepthMask(GL_FALSE);
          glDepthFunc(GL_ALWAYS);
        }

      // increment when both depth and stencil tests pass
      glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

      //now draw the region with the "usual z".
      clip_drawer->draw_region(false, state_stack.draw_stack().back(), state_stack.draw_stack());

      
      glStencilFunc(GL_EQUAL, state_stack.back().m_stencil_value, ~0);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

      if(state_stack.back().m_write_z)
        {
          //now draw the region but clearing the z-values,
          //use the stencil test only to get the correct pixels
          //touched
          glDepthMask(GL_TRUE);
          glDepthFunc(GL_ALWAYS);

          //draw the region so z written is 1.0 (clear value).
          clip_drawer->draw_region(true, state_stack.draw_stack().back(), state_stack.draw_stack());
        }

      gl_state.draw_begin();
    }


  return true;
}


void
WRATHLayer::
pop_clipping(draw_state &state_stack, bool have_clip_items,
             WRATHRawDrawData::DrawState &gl_state)
{ 

  const draw_state_element &v(state_stack.back());
  const WRATHLayerClipDrawer::handle &clip_drawer(render_clip_drawer());

  if(v.m_clipped)
    {
      disable_color_buffer_write(gl_state);
      gl_state.draw_end();

      glStencilFunc(GL_EQUAL, v.m_stencil_value, ~0);
      glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

      glDepthFunc(GL_ALWAYS);
      if(state_stack.back().m_write_z)
        {
          /*
            if we wrote to z-buffer, we need
            to set the z-values as the clipping
            region indicates, think of the
            clipping region as a portal.

            TODO: 
              we should introduce a mode so that
              even if the z-values were not written
              we can cap the "portal" with the z
              write of clip_draw()
           */
          glDepthMask(GL_TRUE);
        }
      else
        {
          glDepthMask(GL_FALSE);
        }

      WRATHassert(clip_drawer.valid());
      clip_drawer->draw_region(false, state_stack.draw_stack().back(), state_stack.draw_stack());

      //make stencil op do nothing
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

      gl_state.draw_begin();
    }

  //restore stencil test
  state_stack.pop_back();
  glStencilFunc(GL_EQUAL, state_stack.back().m_stencil_value, ~0);

  pop_clipped_in_items(state_stack, have_clip_items, gl_state);
}
