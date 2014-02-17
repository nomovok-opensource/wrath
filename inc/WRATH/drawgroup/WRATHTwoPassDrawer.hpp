/*! 
 * \file WRATHTwoPassDrawer.hpp
 * \brief file WRATHTwoPassDrawer.hpp
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




#ifndef WRATH_HEADER_TWO_PASS_DRAWER_HPP_
#define WRATH_HEADER_TWO_PASS_DRAWER_HPP_


#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include "WRATHCanvas.hpp"

/*! \addtogroup Group
 * @{
 */

/*!\class WRATHTwoPassDrawer
  Objects that are drawn with anti-aliasing
  are drawn in two passes when drawn opaquely:
  - Pass 1 draws those fragments that are considered "solid"
    with blending off and depth writes on
  - Pass 2 draws those fragmetns that are considererd
    not solid (i.e. the edges) with blending on
    and depth writes off.
  The first pass is for the portions 
  that are solid (i.e. no blending required), the
  second pass is for those portions that are transluscent,
  i.e. requiring blending and coming from anti-aliasing.  
  Objects that are rendered as transparent 
  only require a single pass.
 */
class WRATHTwoPassDrawer:boost::noncopyable
{
public:
  
  /*!\typedef ResourceKey
    Resource key type for WRATHTwoPassDrawer resource
    manager.
   */
  typedef std::string ResourceKey;

  /// @cond
  WRATH_RESOURCE_MANAGER_DECLARE(WRATHTwoPassDrawer, ResourceKey);
  /// @endcond

  /*!\enum drawing_pass_type
    Enumeration to enumerate the
    different passes of drawing
    with a WRATHTwoPassDrawer.
   */
  enum drawing_pass_type
    {
      /*!
        Opaque pass
       */
      opaque_draw_pass=0,

      /*!
        Transluscent pass for opaque
        object to draw anti-aliased
        portions.
       */
      transluscent_draw_pass=1,

      /*!
        Pure transluscent pass, used
        for drawing object that is 
        purely transluscent.
       */
      pure_transluscent=2
    };

  /*!\class DrawTypeSpecifier
    Provides an interface to specify the \ref 
    WRATHDrawType value for each drawing pass.
   */
  class DrawTypeSpecifier:
    public WRATHReferenceCountedObjectT<DrawTypeSpecifier>
  {
  public:
    /*!\fn WRATHDrawType draw_type
      To be implemented by a derived class
      to return the \ref WRATHDrawType for a drawing
      pass with the indicated "item_pass".
      Typical implementation will set \ref
      WRATHDrawType::m_value to item_pass.
      \param tp drawing pass of drawing item
      \param item_pass "drawing item pass" of item
     */
    virtual
    WRATHDrawType
    draw_type(enum drawing_pass_type tp, int item_pass) const=0;
  };
  

  /*!\fn WRATHTwoPassDrawer(WRATHItemDrawer*,  
                            WRATHItemDrawer*, 
                            WRATHItemDrawer*)
    Constructor. Resource name will be generated as an assemblage
    of the values passed.
    Note: it is _legal_ for ptranslucent_drawer to be NULL, in that case then opaque 
    does _NOT_ have a translucent pass, i.e. not anti-aliased.
    \param popaque_drawer pointer to drawer for opaque pass
    \param ptranslucent_drawer pointer to drawer for translucent pass
    \param ptranslucent_drawer_standalone pointer to drawer used for completely translucent item
                                         
   */
  WRATHTwoPassDrawer(WRATHItemDrawer *popaque_drawer,
                     WRATHItemDrawer *ptranslucent_drawer,
                     WRATHItemDrawer *ptranslucent_drawer_standalone);

  /*!\fn WRATHTwoPassDrawer(const ResourceKey&, WRATHItemDrawer*,
                            WRATHItemDrawer*,
                            WRATHItemDrawer*);
    Constructor. Note: it is _legal_ for ptranslucent_drawer to be NULL, in that case then opaque 
    does _NOT_ have a translucent pass, i.e. not anti-aliased.
    \param pname resource name to give to the created WRATHTwoPassDrawer.
    \param popaque_drawer pointer to drawer for opaque pass
    \param ptranslucent_drawer pointer to drawer for translucent pass
    \param ptranslucent_drawer_standalone pointer to drawer used for completely translucent item
   */
  WRATHTwoPassDrawer(const ResourceKey &pname,
                     WRATHItemDrawer *popaque_drawer,
                     WRATHItemDrawer *ptranslucent_drawer,
                     WRATHItemDrawer *ptranslucent_drawer_standalone);
  
  virtual
  ~WRATHTwoPassDrawer();  

  /*!\fn boost::signals2::connection connect_dtor
    The dtor of a WRATHTwoPassDrawer emit's a signal, use this function
    to connect to that signal. The signal is emitted just before
    the WRATHTwoPassDrawer is removed from the resource manager.
   */
  boost::signals2::connection 
  connect_dtor(const boost::signals2::signal<void () >::slot_type &slot)
  {
    return m_dtor_signal.connect(slot);
  }

  /*!\fn bool has_tranlucent_pass
    Returns true if and only if this 
    WRATHTwoPassDrawer has a tranlucent
    pass.
   */
  bool
  has_tranlucent_pass(void) const
  {
    return m_passes[transluscent_draw_pass].first!=NULL;
  }

  /*!\fn WRATHItemDrawer* opaque_pass_drawer
    Returns the \ref WRATHItemDrawer of
    the opaque pass.
   */
  WRATHItemDrawer*
  opaque_pass_drawer(void) const
  {
    return m_passes[opaque_draw_pass].first;
  }

  /*!\fn WRATHItemDrawer* translucent_pass_drawer
    Returns the drawer associated to the translucent
    pass, if this WRATHTwoPassDrawer does not have a
    translucent pass, returns NULL.
   */
  WRATHItemDrawer*
  translucent_pass_drawer(void) const
  {
    return m_passes[transluscent_draw_pass].first;
  }

  /*!\fn WRATHItemDrawer* translucent_only_drawer
    Returns the drawer associated to
    PURE translucent drawing. 
   */
  WRATHItemDrawer*
  translucent_only_drawer(void) const
  {
    return m_passes[pure_transluscent].first;
  }

  /*!\fn WRATHItemDrawer* drawer_named
    Returns the named drawer, will
    return NULL if this WRATHTwoPassDrawer
    does not posses the named drawer.
    \param tp enumeration to choose which drawer
   */
  WRATHItemDrawer*
  drawer_named(enum drawing_pass_type tp) const
  {
    return m_passes[tp].first;
  }

  /*!\fn const ResourceKey& resource_name
    returns the resource name of this WRATHTwoPassDrawer.
   */
  const ResourceKey&
  resource_name(void)
  {
    return m_resource_name;
  }
  
  /*!\fn WRATHGLStateChange::state_change::handle translucent_pass_state_change
    Returns a handle to the GL state
    change for the tranlucent pass
    drawing.
   */
  static
  WRATHGLStateChange::state_change::handle
  translucent_pass_state_change(void);

  /*!\fn const DrawTypeSpecifier::const_handle& default_pass_specifier
    Returns the "default" DrawTypeSpecifier. The default
    sets \ref WRATHDrawType::m_value as passed to
    \ref DrawTypeSpecifier::draw_type() and
    sets DrawTypeSpecifier::m_type as \ref WRATHDrawType::opaque_pass
    if the type passed is \ref opaque_draw_pass otherwise
    sets DrawTypeSpecifier::m_type as \ref WRATHDrawType::transparent_pass.
   */
  static
  const DrawTypeSpecifier::const_handle&
  default_pass_specifier(void);

  /*!\fn const DrawTypeSpecifier::const_handle& clip_pass_specifier
    Returns a DrawTypeSpecifier, which is suitable for
    specfiying that one wishes to use the item to 
    clipin or to clipout. 
    \param tp specifies weather item is to be used to clipin or
              clip out, must be one \ref WRATHDrawType::clip_inside_draw 
              or \ref WRATHDrawType::clip_outside_draw. The returned
              object will return WRATHDrawType(item_pass, tp)
              where item_pass as passed in \ref DrawTypeSpecifier::draw_type().
   */
  static
  const DrawTypeSpecifier::const_handle&
  clip_pass_specifier(enum WRATHDrawType::draw_type_t tp);

private:

  class per_item_drawer:
    public std::pair<WRATHItemDrawer*, boost::signals2::connection>
  {
  public:
    per_item_drawer(void)
    {
      first=NULL;
    }
    per_item_drawer(WRATHItemDrawer*, WRATHTwoPassDrawer*);
  };


  static
  ResourceKey
  generate_name(WRATHItemDrawer *popaque_drawer,
                WRATHItemDrawer *ptranslucent_drawer,
                WRATHItemDrawer *ptranslucent_drawer_standalone);

  ResourceKey m_resource_name;
  vecN<per_item_drawer, 3> m_passes;
  boost::signals2::signal<void () > m_dtor_signal;
};
/*! @} */

#endif

