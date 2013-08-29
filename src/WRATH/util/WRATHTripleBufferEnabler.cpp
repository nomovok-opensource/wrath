/*! 
 * \file WRATHTripleBufferEnabler.cpp
 * \brief file WRATHTripleBufferEnabler.cpp
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
#include "WRATHTripleBufferEnabler.hpp"

//////////////////////////////////////////
//WRATHTripleBufferEnabler::PhasedDeletedObject methods
WRATHTripleBufferEnabler::PhasedDeletedObject::
PhasedDeletedObject(const handle &tr):
  m_tr(tr)
{
  WRATHassert(m_tr.valid());
  
  #ifdef WRATH_NEW_DEBUG
    m_deletion_phase=-1;
    m_ptr=this;
    m_delete_at_file="Not Deleted";
    m_delete_at_line=-1;
  #endif
}


WRATHTripleBufferEnabler::PhasedDeletedObject::
~PhasedDeletedObject()
{
  #ifdef WRATH_NEW_DEBUG
    WRATHassert(m_deletion_phase==2);
    WRATHassert(m_ptr==this);
    m_ptr=NULL;
  #endif
}


void
WRATHTripleBufferEnabler::PhasedDeletedObject::
phased_delete_object(PhasedDeletedObject *ptr
#ifdef WRATH_NEW_DEBUG
                     , const char *file
                     , int line
#endif
                     )
{
  WRATHassert(ptr!=NULL);
  WRATHassert(ptr->m_tr.valid());
  
  #ifdef WRATH_NEW_DEBUG
    WRATHassert(ptr->m_ptr==ptr);
    WRATHassert(ptr->m_deletion_phase==-1);
    ptr->m_deletion_phase=0;

    
    ptr->m_delete_at_file=file;
    ptr->m_delete_at_line=line;

  #endif
    /*
      NOTE! We call on_place_on_deletion_list()
      BEFORE putting it on the deletion list.
      This is so that "recursive"/"dependent"
      destruction happen correctly, for example:
      Say object A has a pointer to object B.

      For non-phased deleted object we would have:

      (1) ~A called
      (2) ~B called by ~A 
      (3) ~B returns
      (4) ~A returns.

      Then in a phased deleted, we want this:

      A->on_place_on_deletion_list()
      B->on_place_on_deletion_list() called by A->on_place_on_deletion_list()
      delete B
      delete A

      i.e. A's dtor is called AFTER B's.

      On a side note, the final calling looks as follows:

      A->on_place_on_deletion_list()
      B->on_place_on_deletion_list()

      B->simulation_delete()
      A->simulation_delete()

      B->render_delete()
      A->render_delete()

      ~B
      ~A

      It is debatable if this is the correct thing to do
      The issue comes down to that having simulation_delete()
      and render_delete() intertwined is not a happy thing.

      The _ideal_ solution is to remove those
      methods, but typically objects need to do
      actions in certain threads only.

      The next best thing is that the calling order
      would be:

      A->on_place_on_deletion_list()

      B->on_place_on_deletion_list()
      B->simulation_delete()
      B->render_delete()      
      ~B

      A->simulation_delete()
      A->render_delete()
      ~A

      i.e. A's methods gets delayed until B is
      deleted.

      The way to implement this is that A's 
      deletion gets delayed by 1-round.
      In general, we sould like to know
      the depth of call stack of
      on_place_on_deletion_list(),
      that will tell us precisely how
      far to delay A. However, we would
      also need that a subsequent object
      C has all his actions delayed until
      A's is done! Requiring additional
      delay of C! I feel like I am thinking
      about emulating a stack with a linked
      list is the most retarded of fashions.
     */
    ptr->on_place_on_deletion_list();
  
    WRATHLockMutex(ptr->m_tr->m_phase_mutex);
    ptr->m_tr->m_phase0.push_back(ptr);  

  #ifdef WRATH_NEW_DEBUG
    ptr->m_tr->m_phase0.back().m_file=file;
    ptr->m_tr->m_phase0.back().m_line=line;
  #endif
  
  WRATHUnlockMutex(ptr->m_tr->m_phase_mutex);

  ptr->m_tr->schedule_simulation_action( boost::bind(&PhasedDeletedObject::simulation_delete,
                                                     ptr));
}

void
WRATHTripleBufferEnabler::PhasedDeletedObject::
simulation_delete(void)
{
  phase_simulation_deletion();
  #ifdef WRATH_NEW_DEBUG
     WRATHassert(m_deletion_phase==0);
     m_deletion_phase=1;
  #endif
  
  schedule_rendering_action( boost::bind(&PhasedDeletedObject::render_delete,
                                           this));
}


void
WRATHTripleBufferEnabler::PhasedDeletedObject::
render_delete(void)
{
  phase_render_deletion();
  #ifdef WRATH_NEW_DEBUG
     WRATHassert(m_deletion_phase==1);
     m_deletion_phase=2;
  #endif
}

////////////////////////////////////////
// WRATHTripleBufferEnabler methods
WRATHTripleBufferEnabler::
WRATHTripleBufferEnabler(void):
  m_purging(false),
  m_present_ID(0),
  m_last_simulation_ID(1),
  m_current_simulation_ID(2)
{}

WRATHTripleBufferEnabler::
~WRATHTripleBufferEnabler()
{}



void
WRATHTripleBufferEnabler::
fire_signal(signal_t &sig)
{
  //if(!m_purging)
    {
      sig();
    }
}

WRATHTripleBufferEnabler::connect_t
WRATHTripleBufferEnabler::
connect(enum WRATHTripleBufferEnabler::signal_type tp,
        enum WRATHTripleBufferEnabler::signal_time tm,
        const WRATHTripleBufferEnabler::signal_t::slot_type &subscriber,
        int gp_order)
{
  WRATHassert(tp<number_signal_type_enums);
  WRATHassert(tm<number_signal_time_enums);

  return m_sigs[tp][tm].connect(gp_order, subscriber);
}


void
WRATHTripleBufferEnabler::
do_actions(std::list<base_action*> &actions,
           WRATHMutex &mutex)
{
  std::list<base_action*> ct;

  WRATHLockMutex(mutex);
  std::swap(actions, ct);
  WRATHUnlockMutex(mutex);

  for(std::list<base_action*>::iterator iter=ct.begin(),
        end=ct.end(); iter!=end; ++iter)
    {
      (*iter)->execute();
      WRATHDelete(*iter);
    }
}




void
WRATHTripleBufferEnabler::
signal_begin_presentation_frame(void)
{

  WRATHLockMutex(m_phase_mutex);
  m_phase2.splice(m_phase2.end(), m_phase1);
  WRATHUnlockMutex(m_phase_mutex);

  fire_signal(m_sigs[on_begin_presentation_frame][pre_update_no_lock]);

  WRATHLockMutex(m_mutex);
  fire_signal(m_sigs[on_begin_presentation_frame][pre_update_lock]);

  m_present_ID=m_last_simulation_ID;

  WRATHassert(m_current_simulation_ID!=m_last_simulation_ID);
  WRATHassert(m_current_simulation_ID!=m_present_ID);

  


  fire_signal(m_sigs[on_begin_presentation_frame][post_update_lock]);

  WRATHUnlockMutex(m_mutex);
  fire_signal(m_sigs[on_begin_presentation_frame][post_update_no_lock]);
  do_actions(m_render_actions, m_render_actions_mutex);
}


void
WRATHTripleBufferEnabler::
signal_complete_simulation_frame(void)
{
  

  /*
    Do pending deletes:
   */
  std::list<PhasedDeletedObjectEntry> ct;

  WRATHLockMutex(m_phase_mutex);
  std::swap(ct, m_phase2);
  WRATHUnlockMutex(m_phase_mutex);

  for(std::list<PhasedDeletedObjectEntry>::iterator
        iter=ct.begin(), end=ct.end();
      iter!=end; ++iter)
    {
      WRATHDelete(iter->m_object);
    }

  
  WRATHLockMutex(m_phase_mutex);
  m_phase1.splice(m_phase1.end(), m_phase0);
  WRATHUnlockMutex(m_phase_mutex);

  /*
    do pending actions
   */
  do_actions(m_simulation_actions, m_simulation_actions_mutex);



  
  fire_signal(m_sigs[on_complete_simulation_frame][pre_update_no_lock]);

  WRATHLockMutex(m_mutex);

  //pre-conditions:
  WRATHassert(m_current_simulation_ID!=m_last_simulation_ID);
  WRATHassert(m_current_simulation_ID!=m_present_ID);

  
  fire_signal(m_sigs[on_complete_simulation_frame][pre_update_lock]);

  const int local_table[3][3]=
    {
      {-1, 2, 1},// (0,0), (0,1), (0,2)
      { 2,-1, 0},// (1,0), (1,1), (1,2)
      { 1, 0,-1},// (2,0), (2,1), (2,2)
    };

  m_last_simulation_ID=m_current_simulation_ID;

  WRATHassert(m_last_simulation_ID>=0);
  WRATHassert(m_last_simulation_ID<3);
  WRATHassert(m_present_ID>=0);
  WRATHassert(m_present_ID<3);

  m_current_simulation_ID=local_table[m_last_simulation_ID][m_present_ID];

  WRATHassert(m_current_simulation_ID>=0);
  WRATHassert(m_current_simulation_ID<3);

  //post-conditions:
  WRATHassert(m_current_simulation_ID!=m_last_simulation_ID);
  WRATHassert(m_current_simulation_ID!=m_present_ID);
  
  fire_signal(m_sigs[on_complete_simulation_frame][post_update_lock]);

  WRATHUnlockMutex(m_mutex);
  fire_signal(m_sigs[on_complete_simulation_frame][post_update_no_lock]);
}



int
WRATHTripleBufferEnabler::
purge_cleanup(void)
{
  bool delops, rops;
  int return_value(0);

  m_purging=true;
  do
    {
      signal_complete_simulation_frame();
      signal_begin_presentation_frame();

      WRATHLockMutex(m_phase_mutex);
      delops=!m_phase0.empty() or !m_phase1.empty() or !m_phase2.empty();  
      WRATHUnlockMutex(m_phase_mutex);
      
      WRATHLockMutex(m_render_actions_mutex);
      WRATHLockMutex(m_simulation_actions_mutex);
      rops=!m_render_actions.empty() or !m_simulation_actions.empty();
      WRATHUnlockMutex(m_simulation_actions_mutex);
      WRATHUnlockMutex(m_render_actions_mutex);

      ++return_value;
    }
  while(delops or rops);


  return return_value;
}
