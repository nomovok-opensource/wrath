/*! 
 * \file WRATHItemGroup.hpp
 * \brief file WRATHItemGroup.hpp
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




#ifndef WRATH_HEADER_ITEM_GROUP_HPP_
#define WRATH_HEADER_ITEM_GROUP_HPP_

#include "WRATHConfig.hpp"
#include "WRATHRawDrawData.hpp"
#include "WRATHIndexGroupAllocator.hpp"
#include "WRATHItemDrawState.hpp"

/*! \addtogroup Group
 * @{
 */


 

/*!\class WRATHItemGroup
  A WRATHItemGroup represents a group of UI
  elements to draw with the WRATH batching
  system. 

  Creation and destruction of WRATHItemGroup 's
  is only performed by classed derived from
  WRATHCanvas in implementing \ref
  WRATHCanvas::create_implement().

  WRATHItemGroup derives from WRATHTripleBufferEnabler::PhasedDeletedObject,
  as such the object should never be deleted directly
  with \ref WRATHDelete. Rather, it should be phased deleted
  via \ref WRATHPhasedDelete.
 */
class WRATHItemGroup:
  public WRATHTripleBufferEnabler::PhasedDeletedObject
{
public:

  /*!\typedef DrawCall
    Specifies both the \ref WRATHDrawCallSpec and
    the \ref WRATHRawDrawData object in which to add 
    the draw call.   
   */
  typedef std::pair<WRATHRawDrawData*, WRATHDrawCallSpec> DrawCall;

  
  /*!\fn WRATHItemGroup(const WRATHIndexGroupAllocator::handle&,
                        const std::vector<DrawCall>&,
                        const WRATHCompiledItemDrawStateCollection&,
                        int)
    Create a WRATHItemGroup. 
    \param pindex_allocator handle to WRATHIndexGroupAllocator which stores index data
                            for the items of the WRATHItemGroup 
    \param spec array of WRATHDrawCallSpec, each element of the array is the draw
                call for a draw pass of the items of the WRATHItemGroup
    \param pitem_draw_state draw item state vector for all items of the WRATHItemGroup,
                            it is an error if pitem_draw_state.size() is not the same
                            as spec.size()
    \param pimplicit_store Specifies what value to pass to
                           WRATHAttributeStore::implicit_attribute_data()
                           where the implicit attribute data is stored
   */
  WRATHItemGroup(const WRATHIndexGroupAllocator::handle &pindex_allocator,
                 const std::vector<DrawCall> &spec,
                 const WRATHCompiledItemDrawStateCollection &pitem_draw_state,
                 int pimplicit_store);

  virtual
  ~WRATHItemGroup();

  /*!\fn const WRATHCompiledItemDrawStateCollection& item_draw_state
    Returns the "key" that was used to create
    the WRATHItemGroup (i.e that value which
    was passed in the contructor).
    Can be called from threads outside of the GL 
    context from multiple threads simultaneously.
    The actual GL state used to draw is the 
    return value augmented by state added by the 
    \ref WRATHCanvas derived object that created
    the \ref WRATHItemGroup.
   */
  const WRATHCompiledItemDrawStateCollection&
  item_draw_state(void) const
  {
    return m_key;
  }
  
  /*!\fn const WRATHAttributeStore::handle& attribute_store
    Returns a handle to the \ref WRATHAttributeStore 
    used to allocate attributes for items of this 
    WRATHItemGroup.
   */
  const WRATHAttributeStore::handle&
  attribute_store(void) const
  {
    return m_index_store->attribute_store();
  }

  /*!\fn unsigned int implicit_store
    Returns the index to feed to 
    WRATHAttributeStore::implicit_attribute_data()
    to fetch the buffer object storing the implicit 
    attributes used by this WRATHItemGroup.
   */
  unsigned int
  implicit_store(void) const
  {
    return m_implicit_store;
  }

  /*!\fn const WRATHIndexGroupAllocator::handle& index_store
    Returns WRATHIndexGroupAllocator used to allocate
    indices for items of this WRATHItemGroup.
   */
  const WRATHIndexGroupAllocator::handle&
  index_store(void)
  {
    return m_index_store;
  }
  
protected:

  virtual
  void
  phase_simulation_deletion(void);

private:
  WRATHIndexGroupAllocator::handle m_index_store;
  std::vector<WRATHRawDrawDataElement*> m_elements;
  WRATHCompiledItemDrawStateCollection m_key;
  unsigned int m_implicit_store;
};

/*! @} */



#endif
