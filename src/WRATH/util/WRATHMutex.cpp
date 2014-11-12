/*! 
 * \file WRATHMutex.cpp
 * \brief file WRATHMutex.cpp
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
#include <iostream>
#include <errno.h>

#include "WRATHassert.hpp" 
#include "WRATHMutex.hpp"

namespace
{
  class MutexImplement:boost::noncopyable
  {
  public:
    MutexImplement(void)
    {
      int error_code;
      pthread_mutexattr_t mutex_attributes;
      
      error_code=pthread_mutexattr_init(&mutex_attributes);
      WRATHassert(error_code==0);
      
      #ifndef _WIN32
      {
        pthread_mutexattr_setprotocol(&mutex_attributes, PTHREAD_PRIO_NONE);
      }
      #endif

      #ifdef MUTEX_DEBUG
      {
        error_code=pthread_mutexattr_settype(&mutex_attributes, PTHREAD_MUTEX_ERRORCHECK);
        WRATHassert(error_code==0);
      }
      #endif
      
      error_code=pthread_mutex_init(&m_mutex, &mutex_attributes);
      WRATHassert(error_code==0);
      
      error_code=pthread_mutexattr_destroy(&mutex_attributes);
      WRATHassert(error_code==0);
    }

    ~MutexImplement(void)
    {
      pthread_mutex_destroy(&m_mutex);
    }

    int
    lock(void)
    {
      return pthread_mutex_lock(&m_mutex);
    }

    int
    unlock(void)
    {
      return pthread_mutex_unlock(&m_mutex);
    }

    pthread_mutex_t m_mutex;

  };


#if defined(MUTEX_DEBUG)
  void
  debug_error_code(int error_code, const char *file, int line)
  {
    switch(error_code)
      {
      default:
        WRATHwarning("Unknown Error code:" << error_code);
        break;
      case 0:
        break;
        
      case EINVAL:
        WRATHwarning("[" << file << ", " << line << "] EINVAL");
        break;
        
      case EAGAIN:
        WRATHwarning("[" << file << ", " << line << "] EAGAIN");
        break;
        
      case EDEADLK:
        WRATHwarning("[" << file << ", " << line << "] EDEADLK");
        break;

      case EPERM:
        WRATHwarning("[" << file << ", " << line << "] EPERM");
        break;
      }
  }
#endif

}

////////////////////////////
// WRATHMutex methods
WRATHMutex::
WRATHMutex(void)
#ifdef MUTEX_DEBUG
  :m_locked_from_file("??NotLocked??"),
   m_locked_from_line(-1)  
#endif
{
  /*
    Note that we use new/delete, this is because
    WRATHNew/WRATHDelete macro's under debug build
    invoke a mutex.
   */
  m_opaque=new MutexImplement();
}

WRATHMutex::
~WRATHMutex()
{
  MutexImplement *ptr;

  WRATHassert(m_opaque!=NULL);

  /*
    Note that we use new/delete, this is because
    WRATHNew/WRATHDelete macro's under debug build
    invoke a mutex.
   */
  ptr=reinterpret_cast<MutexImplement*>(m_opaque);
  delete ptr;

  m_opaque=NULL;
}

WRATHMutex&
WRATHMutex::
default_mutex(void)
{
  static WRATHMutex R;
  return R;
}

int
WRATHMutex::
internal_mutex_lock(void)
{
  MutexImplement *ptr;

  ptr=reinterpret_cast<MutexImplement*>(m_opaque);
  WRATHassert(ptr!=NULL);

  return ptr->lock();
}

int
WRATHMutex::
internal_mutex_unlock(void)
{
  MutexImplement *ptr;

  ptr=reinterpret_cast<MutexImplement*>(m_opaque);
  WRATHassert(ptr!=NULL);

  return ptr->unlock();
}

#ifdef MUTEX_DEBUG

void
WRATHMutex::
UnlockImplement(const char *file, int line) 
{
  pthread_t thread;
  int r;
  
  thread=pthread_self();

  WRATHassert(m_locked_from_line!=-1);
  WRATHassert(pthread_equal(m_locking_thread,thread));

  m_locked_from_file="??NotLocked??";
  m_locked_from_line=-1;

  

  r=internal_mutex_unlock();
  debug_error_code(r, file, line);
  
  WRATHassert(r==0);
}

void
WRATHMutex::
LockImplement(const char *file, int line) 
{
  int r;
  
  

  r=internal_mutex_lock();
  debug_error_code(r, file, line);
  
  WRATHassert(m_locked_from_line==-1);

  m_locked_from_file=file;
  m_locked_from_line=line;
  m_locking_thread=pthread_self();

  
  WRATHassert(r==0);
}

#endif

////////////////////////////////////
// WRATHThreadID methods
WRATHThreadID
WRATHThreadID::
create_thread(void* (*fptr)(void*), void *argument)
{
  WRATHThreadID return_value(no_init);
  int error_code;

  error_code=pthread_create(&return_value.m_id, NULL, fptr, argument);
  WRATHassert(error_code==0);
  WRATHunused(error_code);

  return return_value;
}

void*
WRATHThreadID::
wait_thread(WRATHThreadID id)
{
  int error_code;
  void *return_value(NULL);

  WRATHassert(!pthread_equal(id.m_id, pthread_self()));

  error_code=pthread_join(id.m_id, &return_value);
  WRATHassert(error_code==0);
  WRATHunused(error_code);

  return return_value;
}
