/*! 
 * \file WRATHLayer.hpp
 * \brief file WRATHLayer.hpp
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


#ifndef __WRATH_LAYER_HPP__
#define __WRATH_LAYER_HPP__

#include "WRATHConfig.hpp"
#include "WRATHLayerBase.hpp"
#include "WRATHLayerClipDrawer.hpp"
#include "WRATHLayerIntermediateTransformation.hpp"
#include "WRATHLayerNodeValuePackerBase.hpp"

/*! \addtogroup Layer
 * @{
 */


/*!\class WRATHLayer
  A WRATHLayer builds on top of \ref WRATHLayerBase
  providing projection and model view matrix values
  which are triple buffered. In addition it
  provides a drawing method, a heirarchy of
  WRATHLayer and a clipping interface for specifying
  clipping.

  The tranformation hierarchy of WRATHLayer is as 
  follows:
   - modelview matrix M specified in the simulation thread
     (see \ref modelview_matrix, \ref simulation_matrix(),
      and \ref render_matrix())
   - a matrix composition mode for the modelview matrix
     (see \ref modelview_matrix, \ref simulation_composition_mode(),
      and \ref render_composition_mode())
   - a projection matrix P specified in the simulation thread
     (see \ref projection_matrix and \ref simulation_matrix())
   - a matrix composition mode for the projection matrix
     (see \ref projection_matrix, \ref simulation_composition_mode(),
      and \ref render_composition_mode())
   - in addition, for each of projection_matrix and modelview_matrix,
     a WRATHLayer may also have a \ref WRATHLayerIntermediateTransformation
     associated to it.
 
  Let parent(L) denote the parent WRATHLayer of a WRATHLayer L,
  
  Let finalM(L) denote the "effective" modelview matrix of L
  and let finalP(L) denote the "effective" projection matrix of L.
  finalP(L) and finalP(M) are computed as follows:

  \code
    mode=L->render_composition_mode(WRATHLayer::projection_matrix);
    modifier=L->render_transformation_modifier(WRATHLayer::projection_matrix);
    
    float4x4 M;
    M=L->render_matrix(WRATHLayer::projection_matrix);

    if(modifier!=NULL)
    {
      modifier->modify_matrix(M);
    }

    if(parent(L)==NULL or mode==WRATHLayer::use_this_matrix)
    {
        finalP(L)=M;
    }
    else //WRATHLayer::compose_matrix
    {
        finalP(L)=finalP( parent(L) ) * P(L)
    }
  \endcode
  and similarly  for finalM(L)

  \code
    
    mode=L->render_composition_mode(WRATHLayer::modelview_matrix)
    modifier=L->render_transformation_modifier(WRATHLayer::modelview_matrix);
    
    float4x4 M;
    M=L->render_matrix(WRATHLayer::modelview_matrix);

    if(modifier!=NULL)
    {
      modifier->modify_matrix(M);
    }

    if(parent(L)==NULL or mode==WRATHLayer::use_this_matrix)
    {
        finalM(L)=M;
    }
    else //WRATHLayer::compose_matrix
    {
        finalM(L)=finalM( parent(L) ) * M;
    }

    
  \endcode

  the value finalM(L), finalP(L) and their product
  are available in the rendering thread only during drawing, 
  via the method \ref current_render_transformation().
  
  The transformation for items of a WRATHLayer
  is as follows:

  Let N="final" transformation of the node
  (the node type is derived from WRATHLayerItemNodeBase
  that the item uses), then the transformation
  applied to a point p of the item is to be:

  \code
  finalP(L) * finalM(L) * N(p)
  \endcode

  The projection matrix is a projection matrix
  using the conventions/API of GL. In particular,
  a common matrix to use for 2D only is:\n

  \f[
     \left| \begin{array}{cccc}
    2/(r-l) & 0.0 & 0.0 &        -(r+l)/(r-l) \\
    0.0 & 2/(t-b) & 0.0 &        -(t+b)/(t-b) \\
    0.0 & 0.0     & -2.0/(f-n) & -(f+n)/(f-n) \\
    0.0 & 0.0     & 0.0        & 1.0  \end{array} \right|  
  \f]   

  typically: r=width, l=0, b=-height, t=0, n=-1, f=1.
  Note that the values of n and f have no affect on the
  normalized device co-ordinate. For the record, if
  w=1, (here foo_n is normalized device coordinates
  and foo_c is clip coordinates) then:  
  \n   \f$ x_c = x * 2/(r-l) - (r+l)/(r-l) \f$
  \n   \f$ w_c = 1 \f$
  \n thus \f$ x_n=x * 2/(r-l) - (r+l)/(r-l) \f$   

  For 3D, a common frustrum matrix is given by:\n

  \f[
     \left| \begin{array}{cccc}
     2n/(r-l) & 0.0 &  (r+l)/(r-l) & 0.0   \\
     0.0 & 2n/(t-b) &  (t+b)/(t-b) & 0.0   \\
     0.0 &   0.0    & -(f+n)/(f-n) & -2fn/(f-n) \\
     0.0 &   0.0    & -1.0         & 0.0  \end{array} \right|  
  \f]   
  

  \n Letting \f$ z=-1\f$ and \f$ w=1 \f$, we see that:
  \n \f$ x_c = 2n/(r-l) - (r+l)/(r-l) \f$
  \n \f$ w_c = +1 \f$
  \n Hence: \f$ x_n= x * 2n/(r-l) - (r+l)/(r-l) \f$  

  Thus to freely use a 3D transformation
  together with 2D drawing, just make
  l,r,t,b same as one would with an orthognal
  matrix and set z as -1 for the vertex
  attribute value and let n=1, this will
  guarantee that location on window is
  the same and that the primitive do
  not get near clipped.    
  
  In addition, WRATHLayer also supports clipping.
  There are 3 parts of clipping associated to a WRATHLayer
  performed in the following order:

  - 1) Clip in region as defined by a \ref WRATHLayerClipDrawer.
       If no WRATHLayerClipDrawer object is attached to the 
       WRATHLayer this step is skipped. The contents of
       all drawing of the WRATHLayer and its children
       are clipped to within the region drawn by the 
       WRATHLayerClipDrawer. In addition, a WRATHLayerClipDrawer
       can declare that the entire contents of the WRATHLayer
       is clipped, thus skipping the entire draw of a 
       WRATHLayer.
  
  - 2) If there are no items of the WRATHLayer
       where pass is WRATHDrawType::clip_inside_draw,
       this step is skipped. Otherwise, an addition test
       is added: contents of the WRATHLayer are also
       clipped to the UNION of all items with pass
       being WRATHDrawType::clip_inside_draw

  - 3) Items where pass is WRATHDrawType::clip_outside_draw
       give a set of occluders. The pixel of an item at P of a WRATHLayer
       are not drawn if for each occluder O, that pixel is infront
       of the pixel of the occluder O. If the occluder O does not
       have a pixel location at P, then it does not influence
       the clipping at pixel P. The typical use case is that the
       z-value of all drawn items of a WRATHLayer are behind the
       z-value of all occluders and thus the union of occluders 
       provides a "clip-out" region.
 */
class WRATHLayer:public WRATHLayerBase
{
public:
  /*!\enum matrix_composition_type
    Enumeration controlling how transformation
    and projection matrices are composed from
    the parent.
   */
  enum matrix_composition_type
    {
      /*!
        Compose the value with the 
        parent's WRATHLayer matrix value.
       */
      compose_matrix,

      /*!
        Use the matrix of this
        WRATHLayer without composing.
       */
      use_this_matrix,
    };

  /*!\enum matrix_type
    Enumeration to specify matrix
   */
  enum matrix_type
    {
      /*!
        Specifies to manipulate the projection matrix.
       */
      projection_matrix,

      /*!
        Specifies to manipulate the modelview matrix.
       */
      modelview_matrix
    };


  /*!\enum inherit_values_type
    Enumeration tag type.
   */
  enum inherit_values_type
    {
      /*!
        Enumeration tag that indicates
        for a child WRATHLayer to inherit
        its \ref WRATHLayerBase::sorter() 
        from it's parent.
       */
      inherit_values
    };

  /*!\typedef parent_change_signal
    Signal type for when a WRATHLayer changes it's parent,
    the paramter passed are 1st parameter is the old parent
    and the 2nd paramter is the new parent.
   */
  typedef boost::signals2::signal<void (WRATHLayer*, WRATHLayer*)> parent_change_signal;

  /*!\typedef child_add_signal
    Signal type for when a WRATHLayer had a child added to it.
   */
  typedef boost::signals2::signal<void (WRATHLayer*)> child_add_signal;
  
  /*!\typedef child_remove_signal
    Signal type for when a WRATHLayer had a child added to it.
   */
  typedef boost::signals2::signal<void (WRATHLayer*)> child_remove_signal;

  /*!\class draw_information
    An instance of a draw_information
    stores information of a 
    WRATHRawDrawData::draw_information
    and additional WRATHLayer statistics.
   */
  class draw_information:public WRATHRawDrawData::draw_information
  {
  public:
    /*!\fn draw_information
      Ctor.
     */
    draw_information(void):
      WRATHRawDrawData::draw_information(),
      m_layer_count(0)
    {}

    /*!\var m_layer_count  
      Number of \ref WRATHLayer objects drawn.
     */ 
    int m_layer_count;
  };

  /*!\class matrix_state
    A matrix_state is to set the values
    of the uniforms associated whose values are to come from
    the matrices of \ref current_render_transformation()
   */
  class matrix_state:public WRATHLayerBase::GLStateOfLayer
  {
  public:
    /*!\fn matrix_state
      Ctor, specifying the names of the uniforms
      \param projection_modelview name of the uniform representing the
                                  projection matrix times the modeview matrix. An empty string
                                  indicates to not append a uniform taking that value.
      \param modelview name of the uniform representing the modeview matrix. An empty string
                       indicates to not append a uniform taking that value.
      \param projection name of the uniform representing the projection matrix. An empty string
                        indicates to not append a uniform taking that value.
             
     */
    explicit
    matrix_state(const std::string &projection_modelview,
                 const std::string &modelview="",
                 const std::string &projection="");

    virtual
    void
    append_state(WRATHLayerBase *layer,
                 WRATHSubItemDrawState &sk) const;
    
    
  private:    
    std::string m_projection_modelview, m_modelview, m_projection;
  };

  /*!\fn WRATHLayer(const WRATHTripleBufferEnabler::handle&,
                    const WRATHLayerClipDrawer::handle&,
                    const WRATHDrawOrderComparer::handle)
    Ctor. Create a root WRATHLayer object.
    \param tr handle to a WRATHTripleBufferEnabler to
              which the users of the created object will
              sync. It is an error if the handle is not valid.
    \param pclipper handle to object to draw the WRATHLayer's clipping.
                    An invalid handle that the created WRATHLayer
                    does not have it's own clipping.
    \param sorter handle to WRATHDrawOrderComparer comparer
                  object which elements of created WRATHLayerBase
                  will be sorted (see \ref WRATHItemDrawState::m_force_draw_order)
                  An invalid handle indicates that the elements are
                  not sorted by a user defined sorting.
   */
  WRATHLayer(const WRATHTripleBufferEnabler::handle &tr,
             const WRATHLayerClipDrawer::handle &pclipper=
             WRATHLayerClipDrawer::handle(),
             const WRATHDrawOrderComparer::handle sorter
             =WRATHDrawOrderComparer::handle());

  /*!\fn WRATHLayer(WRATHLayer*, 
                    const WRATHLayerClipDrawer::handle&,
                    const WRATHDrawOrderComparer::handle)
  
    Ctor. Create a child WRATHLayer of a pre-existing WRATHLayer.
    The parent WRATHLayer owns the created child WRATHLayer, as
    such the created child WRATHLayer will be deleted when
    the parent WRATHLayer is.
    \param pparent parent WRATHLayer. The parent owns the child
                   WRATHLayer. Parent WRATHLayer's deallocate
                   their children when they themselves are 
                   deallocated.
    \param pclipper handle to object to draw the WRATHLayer's clipping.
                    An invalid handle that the created WRATHLayer
                    does not have it's own clipping.
    \param sorter handle to WRATHDrawOrderComparer comparer
                  object which elements of created WRATHLayerBase
                  will be sorted (see \ref WRATHItemDrawState::m_force_draw_order)
                  An invalid handle indicates that the elements are
                  not sorted by a user defined sorting.                   
   */
  WRATHLayer(WRATHLayer *pparent, 
             const WRATHLayerClipDrawer::handle &pclipper=
             WRATHLayerClipDrawer::handle(),
             const WRATHDrawOrderComparer::handle sorter
             =WRATHDrawOrderComparer::handle());

  /*!\fn WRATHLayer(WRATHLayer*, 
                    enum inherit_values_type,
                    const WRATHLayerClipDrawer::handle&)
    Ctor. Create a child WRATHLayer of a pre-existing WRATHLayer
    that inherits it's sorter (\ref WRATHLayerBase::sorter()) 
    from it's parent. The parent WRATHLayer owns the created 
    child WRATHLayer, as such the created child WRATHLayer 
    will be deleted when the parent WRATHLayer is.
    \param pparent parent WRATHLayer. The parent owns the child
                   WRATHLayer. Parent WRATHLayer's deallocate
                   their children when they themselves are 
                   deallocated.
    \param px must have value \ref inherit_values
    \param pclipper handle to object to draw the WRATHLayer's clipping.
                    An invalid handle that the created WRATHLayer
                    does not have it's own clipping.
    
   */
  WRATHLayer(WRATHLayer *pparent, 
             enum inherit_values_type px,
             const WRATHLayerClipDrawer::handle &pclipper=
             WRATHLayerClipDrawer::handle());


  virtual
  ~WRATHLayer();

  /*!\fn WRATHLayer* parent(void)
    Returns the parent WRATHLayer of this WRATHLayer.
    A NULL return value indicates that this WRATHLayer
    is a root WRATHLayer. 
   */
  WRATHLayer*
  parent(void)
  {
    WRATHAutoLockMutex(m_parent_mutex);
    return m_parent;
  }

  /*!\fn enum return_code parent(WRATHLayer*)
    Sets the parent WRATHLayer of this WRATHLayer,
    return routine_fail on failure and routine_success
    on success. Failure happens if the parent to be is
    a descendant of this. Method is thread safe
    and may be called from multiple threads 
    simutaneously.
    \param p new parent WRATHLayer
   */
  enum return_code
  parent(WRATHLayer *p);

  /*!\fn boost::signals2::connection connect_parent_change
    Connect to the signal emitted whenever the parent
    of this WRATHLayer changes. Signal is emitted
    _AFTER_ the the parent is set, and AFTER the
    add child and remove child signals have fired
    on the old and new parent.
    \param S slot to which to connect the signal
   */
  boost::signals2::connection
  connect_parent_change(const parent_change_signal::slot_type &S)
  {
    return m_parent_change_signal.connect(S);
  }
  
  /*!\fn boost::signals2::connection connect_child_add
    Connect to the signal emitted whenever a
    child is added to this WRATHLayer,
    the new child is passed in the signal.
    Signal is emitted AFTER the child is
    added.
    \param S slot to which to connect the signal
   */
  boost::signals2::connection
  connect_child_add(const child_add_signal::slot_type &S)
  {
    return m_child_add_signal.connect(S);
  }

  /*!\fn boost::signals2::connection connect_child_remove
    Connect to the signal emitted whenever a
    child is added to this WRATHLayer,
    the new child is passed in the signal.
    Signal is emitted AFTER the child is
    removed.
    \param S slot to which to connect the signal
   */
  boost::signals2::connection
  connect_child_remove(const child_remove_signal::slot_type &S)
  {
    return m_child_remove_signal.connect(S);
  }
  
  /*!\fn int child_order(void)
    Returns the child order for this WRATHLayer.
    Child WRATHLayer objects of a parent WRATHLayer
    are drawn by the parent WRATHLayer sorted by
    the return value of \ref child_order(). May 
    only be called from the simulation thread.
   */
  int
  child_order(void)
  {
    return m_child_order[current_simulation_ID()];
  }

  /*!\fn void child_order(int)
    Sets the child order for this WRATHLayer.
    Child WRATHLayer objects of a parent WRATHLayer
    are drawn by the parent WRATHLayer sorted by
    the return value of \ref child_order(). May 
    only be called from the simulation thread.
   */
  void
  child_order(int v);


  /*!\fn int number_children(void) const
    Returns the number of child WRATHLayer's
    of this WRATHLayer. May only be called 
    from the simulation thread.
   */
  int
  number_children(void) const
  {
    WRATHAutoLockMutex(m_mutex);
    return m_child_count;
  }

  /*!\fn WRATHLayer* root
    Returns the root ancestor WRATHLayer of this WRATHLayer
   */
  WRATHLayer*
  root(void)
  {
    return m_root;
  }

  /*!\fn const WRATHLayerClipDrawer::handle& simulation_clip_drawer(void) const
    The handle value of clip drawer of a WRATHLayer
    is triple buffered and is set from the
    simulation thread. This function returns
    the value as last set in the simulation 
    thread. May only be called from the
    simulation thread.
   */
  const WRATHLayerClipDrawer::handle&
  simulation_clip_drawer(void) const
  {
    return m_clip_drawer[current_simulation_ID()];
  }

  /*!\fn void simulation_clip_drawer(const WRATHLayerClipDrawer::handle&)
    The handle value of clip drawer of a WRATHLayer
    is triple buffered and is set from the
    simulation thread. This function sets
    the value. May only be called from the
    simulation thread.
    \param v handle to clip drawer to use, an invalid handle
             indicates to not have a clip drawer
   */
  void
  simulation_clip_drawer(const WRATHLayerClipDrawer::handle &v)
  {
    m_clip_drawer[current_simulation_ID()]=v;
  }

  /*!\fn const WRATHLayerClipDrawer::handle& render_clip_drawer(void) const
    The handle value of clip drawer of a WRATHLayer
    is triple buffered and is set from the
    simulation thread. Returns the clip drawer used. 
    This value is set from the simulation thread by of \ref
    simulation_clip_drawer(const WRATHLayerClipDrawer::handle&)
   */
  const WRATHLayerClipDrawer::handle&
  render_clip_drawer(void) const
  {
    return m_clip_drawer[present_ID()];
  }

  /*!\fn bool visible(void)
    Returns if this WRATHLayer is drawn.
    The value is not triple buffered
    and is set and returned via 
    atomic ops.
   */
  bool
  visible(void);

  /*!\fn void visible(bool)
    Sets if this WRATHLayer is drawn.
    The value is not triple buffered
    and is set and returned via 
    atomic ops.  
   */
  void
  visible(bool v);

  /*!\fn const float4x4& simulation_matrix(enum matrix_type)
    Returns the current value of the matrix
    as last set in the simulation thread,
    may only be called from the simulation
    thread.
    \param tp which matrix to query
   */
  const float4x4&
  simulation_matrix(enum matrix_type tp)
  {
    return m_matrices[tp][current_simulation_ID()].m_matrix;
  }

  /*!\fn void simulation_matrix(enum matrix_type, const float4x4&)
    Sets the current value of the matrix,
    may only be called from the simulation
    thread. Default value for each matrix 
    type is the identity matrix.
    \param tp which matrix to set
    \param v value to which to set the matrix
   */
  void
  simulation_matrix(enum matrix_type tp, const float4x4 &v)
  {
    m_matrices[tp][current_simulation_ID()].m_matrix=v;
  }

  /*!\fn enum matrix_composition_type simulation_composition_mode(enum matrix_type)
    Returns the current value of the matrix
    composition mode as last set in the 
    simulation thread, may only be called 
    from the simulation thread.
    \param tp which matrix to query
   */
  enum matrix_composition_type
  simulation_composition_mode(enum matrix_type tp)
  {
    return m_matrices[tp][current_simulation_ID()].m_mode;
  }

  /*!\fn void simulation_composition_mode(enum matrix_type, 
                                          enum matrix_composition_type)
  
    Sets the current value of the matrix
    composition mode, may only be called 
    from the simulation thread. Default value 
    for each matrix type is \ref compose_matrix.
    \param tp which matrix mode to set
    \param v value to which to set the mode
   */
  void
  simulation_composition_mode(enum matrix_type tp,
                              enum matrix_composition_type v)
  {
    m_matrices[tp][current_simulation_ID()].m_mode=v;
  }

  /*!\fn void simulation_transformation_modifier
    Sets the transformation modifier for 
    the stated matrix type. The defualt 
    value is not have a transformation
    modifier (i.e. NULL value). The 
    WRATHLayer does NOT take ownership
    of the intermediate transformation,
    as such it must stay in scope whenever
    the WRATHLayer uses it in the rendering
    thread.
    \param tp which matrix modifier to set
    \param hnd value to which to set the modifier
   */
  void
  simulation_transformation_modifier(enum matrix_type tp,
                                     WRATHLayerIntermediateTransformation::handle hnd)
  {
    m_matrices[tp][current_simulation_ID()].m_modifier=hnd;
  }

  /*!\fn const float4x4& render_matrix(enum matrix_type)
    Returns the matrix as used in the 
    rendering thread for drawing, may
    only be called from the rendering
    thread.
    \param tp which matrix to query
   */
  const float4x4&
  render_matrix(enum matrix_type tp)
  {
    return m_matrices[tp][present_ID()].m_matrix;
  }

  /*!\fn enum matrix_composition_type render_composition_mode
    Returns the matrix mode as used 
    in the  rendering thread for drawing, 
    may only be called from the rendering
    thread.
    \param tp which matrix modifier to query
   */
  enum matrix_composition_type
  render_composition_mode(enum matrix_type tp)
  {
    return m_matrices[tp][present_ID()].m_mode;
  }

  /*!\fn WRATHLayerIntermediateTransformation::handle render_transformation_modifier
    Returns the transformation modifier
    as used in the rendering thread for drawing, 
    may only be called from the rendering
    thread.
    \param tp which matrix to query
   */
  WRATHLayerIntermediateTransformation::handle
  render_transformation_modifier(enum matrix_type tp)
  {
    return m_matrices[tp][present_ID()].m_modifier;
  }

  /*!\fn const WRATHLayerClipDrawer::DrawStateElementTransformations& current_render_transformation
    Returns the composited matrix, i.e.
    the matix value taking into account
    the matrix mode, may only be called
    from the rendering thread while rendering, 
    available for use typically by a \ref
    WRATHLayerClipDrawer.
   */
  const WRATHLayerClipDrawer::DrawStateElementTransformations&
  current_render_transformation(void) const 
  {
    return m_current_render_transformation;
  }
  
  /*!\fn WRATHLayer* current_render_parent
    Returns the WRATHLayer viewed as the parent
    WRATHLayer during rendering. May only be called
    from the rendering thread while rendering,
    available for use typically by a \ref
    WRATHLayerClipDrawer.
   */
  WRATHLayer*
  current_render_parent(void)
  {
    return m_render_parent;
  }

  /*!\fn void draw(const float4x4*, draw_information*)
    Draws this WRATHLayer and it's child WRATHLayer's.
    Does NOT clear any of the buffers before drawing
    but does restore writing to each of the buffers.
    The GL state vectors (including write masks
    any enables and blending mode) is undefined
    when draw(const float4x4*, draw_information*) 
    returns. See also \ref clear_and_draw(GLbitfield, const float4x4*, draw_information*) 
    \param pre_modelview_matrix if non-NULL have the modelview matrix transformation
                                if drawing "start" at the transformation pointed to 
                                by pre_modelview_matrix
    \param p if non-NULL, increments the pointee by
             the number of draw calls, state changes,
             etc. incurred from the draw call.
   */
  void
  draw(const float4x4 *pre_modelview_matrix=NULL,
       draw_information *p=NULL);

  /*!\fn void draw(draw_information*)
    Provided as a conveniance, equivalent to
    \code
    draw(NULL, p);
    \endcode
    \param p if non-NULL, increments the pointee by
             the number of draw calls, state changes,
             etc. incurred from the draw call.
   */
  void
  draw(draw_information *p)
  {
    draw(NULL, p);
  }

  /*!\fn void clear_and_draw(GLbitfield, const float4x4*, draw_information*)
    Clears the specified buffers and then executes draw().
    The GL state vectors (including write masks
    any enables and blending mode) is undefined
    when draw() returns.
    \param mask bitfield as fed to glClear() to specify which
                buffers to clear.
    \param pre_modelview_matrix if non-NULL have the modelview matrix transformation
                                if drawing "start" at the transformation pointed to 
                                by pre_modelview_matrix
    \param p if non-NULL, increments the pointee by
             the number of draw calls, state changes,
             etc. incurred from the draw call.
   */
  void
  clear_and_draw(GLbitfield mask=GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT,
                 const float4x4 *pre_modelview_matrix=NULL,
                 draw_information *p=NULL);

  /*!\fn void clear_and_draw(GLbitfield, draw_information*, const float4x4*)
    Equivalent to:
    \code
    clear_and_draw(mask, pre_modelview_matrix, p);
    \endcode
    \param mask bitfield as fed to glClear() to specify which
                buffers to clear.
    \param p if non-NULL, increments the pointee by
             the number of draw calls, state changes,
             etc. incurred from the draw call.
    \param pre_modelview_matrix if non-NULL have the modelview matrix transformation
                                if drawing "start" at the transformation pointed to 
                                by pre_modelview_matrix
   */
  void
  clear_and_draw(GLbitfield mask,
                 draw_information *p,
                 const float4x4 *pre_modelview_matrix=NULL)
  {
    clear_and_draw(mask, pre_modelview_matrix, p);
  }

  /*!\fn void clear_and_draw(draw_information*, const float4x4*)
    Equivalent to:
    \code
    clear_and_draw(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT, pre_modelview_matrix, p);
    \endcode
    \param p if non-NULL, increments the pointee by
             the number of draw calls, state changes,
             etc. incurred from the draw call.
    \param pre_modelview_matrix if non-NULL have the modelview matrix transformation
                                if drawing "start" at the transformation pointed to 
                                by pre_modelview_matrix
   */
  void
  clear_and_draw(draw_information *p,
                 const float4x4 *pre_modelview_matrix=NULL)
  {
    clear_and_draw(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT, 
                   pre_modelview_matrix, p);
  }

  /*!\fn void clear_and_draw(const float4x4 *, draw_information*) 
    Equivalent to:
    \code
    clear_and_draw(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT, pre_modelview_matrix, p);
    \endcode
    \param p if non-NULL, increments the pointee by
             the number of draw calls, state changes,
             etc. incurred from the draw call.
    \param pre_modelview_matrix if non-NULL have the modelview matrix transformation
                                if drawing "start" at the transformation pointed to 
                                by pre_modelview_matrix
   */
  void
  clear_and_draw(const float4x4 *pre_modelview_matrix,
                 draw_information *p=NULL)
  {
    clear_and_draw(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT, 
                   pre_modelview_matrix, p);
  }


protected:

  /*!\fn void draw_render_items
    Provided as a conveniance to worh with
    \ref WRATHLayerBase::render_raw_datas().
    For each WRATHRawDrawData object, call
    WRATHRawDrawData::draw() on the object
    \param gl_state object that tracks current GL state vector
    \param items container of WRATHRawDrawData objects
   */
  static
  void
  draw_render_items(WRATHRawDrawData::DrawState &gl_state,                    
                    const std::map<int, WRATHRawDrawData*> &items);

  /*!\fn void draw_content_pre_children
    To be optionally implemented by a derived class
    to draw the WRATHLayer content that is to be
    drawn BEFORE drawing the child WRATHLayer's.
    Default implementation draws opaque items
    once with 
    - color buffer writes on, blending off
    - depth test and depth buffer writes on
    \param gl_state object that tracks current GL state vector
   */
  virtual
  void
  draw_content_pre_children(WRATHRawDrawData::DrawState &gl_state);

  /*!\fn void draw_content_post_children
    To be optionally implemented by a derived class
    to draw the WRATHLayer content that is to be
    drawn AFTER drawing the child WRATHLayer's.
    Default implementation draws transparent items
    once with 
    - color buffer writes on, blending on
    - depth test on, depth buffer writes off
    \param gl_state object that tracks current GL state vector
   */
  virtual
  void
  draw_content_post_children(WRATHRawDrawData::DrawState &gl_state);
  
  virtual
  void
  on_place_on_deletion_list(void);

private:

  class child_sorter
  {
  public:
    bool
    operator()(WRATHLayer *lhs, WRATHLayer *rhs) const;
  };

  class per_matrix
  {
  public:
    per_matrix(void):
      m_mode(compose_matrix)
    {}

    float4x4 m_matrix;
    enum matrix_composition_type m_mode;
    WRATHLayerIntermediateTransformation::handle m_modifier;
  };
  
  class draw_state_element
  {
  public:
    int m_stencil_value;
    bool m_write_z;
    bool m_clipped;
    enum WRATHLayerClipDrawer::clip_mode_type m_clipping_mode;
  };

  class draw_state
  {
  public:
    explicit 
    draw_state(void);

    ~draw_state()
    {
      WRATHassert(m_stack.size()==1);
    }

    void
    push_back(WRATHLayer *layer,
              const WRATHLayerClipDrawer::DrawStateElementClipping &cl,
              int stencil_value);

    void
    pop_back(void);

    const draw_state_element&
    back(void) const
    {
      WRATHassert(!m_stack.empty());
      return m_stack.back();
    }

    const_c_array<WRATHLayerClipDrawer::DrawStateElement>
    draw_stack(void) const
    {
      return m_draw_stack;
    }

  private:
    std::vector<draw_state_element> m_stack;
    std::vector<WRATHLayerClipDrawer::DrawStateElement> m_draw_stack;
  };


  void
  add_child(WRATHLayer*);

  void
  remove_child(WRATHLayer*);

  void
  add_child_implement(std::list<WRATHLayer*> *array,
                      WRATHLayer *child, 
                      std::list<WRATHLayer*>::iterator *child_slot_value);

  void
  remove_child_implement(std::list<WRATHLayer*> *array,
                         WRATHLayer *child, 
                         std::list<WRATHLayer*>::iterator *child_slot_value);

 
  void
  draw_implement(const float4x4 *pre_modelview_transform,
                 draw_state &state_stack,
		 WRATHRawDrawData::DrawState &gl_state,
                 draw_information &stats,
                 WRATHLayer *from);

  bool
  push_clipping(draw_state &state_stack, bool &have_clip_items,
		WRATHRawDrawData::DrawState &gl_state);

  void
  push_clipped_in_items(draw_state &state_stack, bool &have_clip_items,
			WRATHRawDrawData::DrawState &gl_state);

  void
  pop_clipping(draw_state &state_stack, bool have_clip_items,
	       WRATHRawDrawData::DrawState &gl_state);

  void
  pop_clipped_in_items(draw_state &state_stack, bool have_clip_items,
		       WRATHRawDrawData::DrawState &gl_state);

  void
  on_end_simulation_frame(void);
  
  void
  compute_render_matrix_value(float4x4 &output, enum matrix_type);

  void
  set_render_matrices(const float4x4 *pre_modelview_transform);
  
  void
  mark_render_sort_order_dirty(void)
  {
    m_render_children_need_sorting=true;
  }

  /*
    m_children and m_slot are protected
    behind m_mutex
   */
  mutable WRATHMutex m_mutex, m_parent_mutex;
  std::list<WRATHLayer*> m_children;  
  std::list<WRATHLayer*>::iterator m_slot;
  int m_child_count;

  WRATHLayer *m_parent, *m_root, *m_render_parent;
  vecN<int, 3> m_child_order;
  vecN<WRATHLayerClipDrawer::handle, 3> m_clip_drawer;

  vecN<vecN<per_matrix, 3>, 2> m_matrices;
  WRATHTripleBufferEnabler::connect_t m_sim_connect;

  parent_change_signal m_parent_change_signal;
  child_add_signal m_child_add_signal;
  child_remove_signal m_child_remove_signal;

  /*
    +1=visible
    0=non-visible
   */
  int m_visible;

  /*
    effective matrix values only valid during rendering!
   */
  WRATHLayerClipDrawer::DrawStateElementTransformations m_current_render_transformation;


  /*
    we use schedule_rendering_action to update
    m_render_children and m_render_slot
   */
  bool m_render_children_need_sorting;
  std::list<WRATHLayer*> m_render_children;  
  std::list<WRATHLayer*>::iterator m_render_slot;
};


/*!\class WRATHLayerItemDrawer
  A WRATHLayerItemDrawer inherits from WRATHLayerNodeValuePackerBase::Drawer,
  thus the template paremeter determines how per-node values
  are handled. In addition, it also adds a 
  WRATHLayer::matrix_state object to it's
  additional uniforms (see \ref WRATHLayerBase::DrawerBase::add_GLStateOfLayer() )
  with the default name "clip_matrix_layer". I.e., it adds a mat4 GLSL 
  uniform whose value is the value of the projection-modelview
  matrix of the layer of the items being drawn.

  \tparam NodePacker must be derived from WRATHLayerNodeValuePackerBase
                     and have a ctor of the form 
                     \code NodePacker(WRATHLayerBase*,  const WRATHLayerNodePackerBase::SpecDataProcessedPayload::const_handle &, const WRATHLayerNodePackerBase::ProcessedActiveNodeValuesCollection &)
                     \endcode
 */
template<typename NodePacker>
class WRATHLayerItemDrawer:public WRATHLayerNodeValuePackerBase::Drawer<NodePacker>
{
public:
  /*!\fn WRATHLayerItemDrawer
    Ctor. Create a WRATHLayerItemDrawer using a passed WRATHMultiGLProgram
    that can have the indicated number of items per call using the
    per-node values specified
    \param pr WRATHMultiGLProgram to do the drawing
    \param ppayload payload passed along to the node packer type.
    \param spec specifies the per-item uniform names, etc
    \param pvm_name name of the GLSL uniform in the GLSL code that stores
                    the projection*modelview matrix of the WRATHLayer the 
                    drawn items are within
   */
  WRATHLayerItemDrawer(WRATHMultiGLProgram *pr,
                       const WRATHLayerNodeValuePackerBase::SpecDataProcessedPayload::const_handle &ppayload,
                       const WRATHLayerNodeValuePackerBase::ProcessedActiveNodeValuesCollection &spec,
                       const std::string &pvm_name="clip_matrix_layer"):
    WRATHLayerNodeValuePackerBase::Drawer<NodePacker>(pr, ppayload, spec)
  {
    WRATHLayer::matrix_state *u;
    u=WRATHNew WRATHLayer::matrix_state(pvm_name);
    this->add_GLStateOfLayer(u);
  }
};



/*! @} */

#endif
