/*! 
 * \file WRATHReferenceCountedObject.cpp
 * \brief file WRATHReferenceCountedObject.cpp
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
#include "WRATHReferenceCountedObject.hpp"
#include "WRATHNew.hpp"
#include "WRATHMutex.hpp"
#include "WRATHatomic.hpp"


WRATHReferenceCountedObject::
WRATHReferenceCountedObject(void):
   m_reference_count(0),
   m_mutex(&WRATHMutex::default_mutex())
{}


WRATHReferenceCountedObject::
WRATHReferenceCountedObject(WRATHMutex *m):
   m_reference_count(0),
   m_mutex(m)
{}

WRATHReferenceCountedObject::
WRATHReferenceCountedObject(WRATHMutex &m):
   m_reference_count(0),
   m_mutex(&m) 
{}



WRATHReferenceCountedObject::
~WRATHReferenceCountedObject()
{
  WRATHassert(m_reference_count==0);
}


void
WRATHReferenceCountedObject::
increment(WRATHReferenceCountedObject *ptr)
{
  if(ptr!=NULL)
    {
      #ifdef WRATH_DISABLE_ATOMICS
        WRATHLockMutexIfNonNULL(ptr->m_mutex);
        ++ptr->m_reference_count;
        WRATHUnlockMutexIfNonNULL(ptr->m_mutex);
      #else
        /*
          do we really need such a strong sync as __ATOMIC_SEQ_CST ?
         */
        if(ptr->m_mutex!=NULL)
          {
            WRATHAtomicAddAndFetch(&ptr->m_reference_count, 1);
          }
        else
          {
            ++ptr->m_reference_count;
          }
      #endif

    }
}


void
WRATHReferenceCountedObject::
decrement(WRATHReferenceCountedObject *ptr)
{
  if(ptr!=NULL)
    {
      int v;

      #ifdef WRATH_DISABLE_ATOMICS
        WRATHLockMutexIfNonNULL(ptr->m_mutex);
        --ptr->m_reference_count;
        v=ptr->m_reference_count;
        WRATHUnlockMutexIfNonNULL(ptr->m_mutex);
      #else
        if(ptr->m_mutex!=NULL)
          {
            v=WRATHAtomicSubtractAndFetch(&ptr->m_reference_count, 1);
          }
        else
          {
            --ptr->m_reference_count;
            v=ptr->m_reference_count;
          }
      #endif

      WRATHassert(v>=0);
      if(v==0)
        {
          WRATHDelete(ptr);
        }
    }
}
