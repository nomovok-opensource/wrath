/*! 
 * \file WRATHMutex.hpp
 * \brief file WRATHMutex.hpp
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



#ifndef WRATH_HEADER_MUTEX_HPP_
#define WRATH_HEADER_MUTEX_HPP_

#include "WRATHConfig.hpp"
#include "WRATHassert.hpp"
#include <boost/utility.hpp>
#include <pthread.h>

/*! \addtogroup Utility
 * @{
 */

/*!\class WRATHMutex
  A WRATHMutex represent a simple
  non-recursive mutex lock. 
  Do NOT use the public methods
  of a WRATHMutex to lock and
  unlock, rather use the macros
  \ref WRATHLockMutex and \ref WRATHUnlockMutex.
  Those macro's provide extra debugging:
  file and line number of locking
  when MUTEX_DEBUG is defined.
 */
class WRATHMutex:boost::noncopyable
{
public:

  /*!\fn WRATHMutex
    Ctor, construct a mutex.
   */
  explicit 
  WRATHMutex(void);

  ~WRATHMutex();

  /*!\fn default_mutex
    Returns a default mutex, this mutex is used
    by \ref WRATHReferenceCountedObject when
    reference counting is locked by mutex but
    the mutex was not assigned at ctor.
   */
  static
  WRATHMutex&
  default_mutex(void);    

  ///@cond
  #ifdef MUTEX_DEBUG
    void
    LockImplement(const char *file, int line);
    
    void
    UnlockImplement(const char *file, int line);
    
    static
    void
    LockImplement(WRATHMutex &m, const char *file, int line)
    {
      m.LockImplement(file, line);
    }
    
    static
    void
    LockImplement(WRATHMutex *m, const char *file, int line)
    {
      WRATHassert(m!=NULL);
      m->LockImplement(file, line);
    }
    
    static
    void
    LockIfNonNULLImplement(WRATHMutex *m, const char *file, int line)
    {
      if(m!=NULL)
        {
          m->LockImplement(file, line);
        }
    }
    
    static
    void
    UnlockImplement(WRATHMutex *m, const char *file, int line)
    {
      WRATHassert(m!=NULL);
      m->UnlockImplement(file, line);
    }
    
    static
    void
    UnlockImplement(WRATHMutex &m, const char *file, int line)
    {
      m.UnlockImplement(file, line);
    }
    
    static
    void
    UnlockIfNonNULLImplement(WRATHMutex *m, const char *file, int line)
    {
      if(m!=NULL)
        {
          m->UnlockImplement(file, line);
        }
    }
    
  #else
    
    void
    LockImplement(void)
    {
      internal_mutex_lock();
    }
    
    void
    UnlockImplement(void) 
    {
      internal_mutex_unlock();
    }
    
    static
    void
    LockImplement(WRATHMutex &m)
    {
      m.LockImplement();
    }
    
    static
    void
    LockImplement(WRATHMutex *m)
    {
      WRATHassert(m!=NULL);
      m->LockImplement();
    }
    
    static
    void
    LockIfNonNULLImplement(WRATHMutex *m)
    {
      if(m!=NULL)
        {
          m->LockImplement();
        }
    }
    
    static
    void
    UnlockImplement(WRATHMutex *m)
    {
      WRATHassert(m!=NULL);
      m->UnlockImplement();
    }
    
    static
    void
    UnlockImplement(WRATHMutex &m)
    {
      m.UnlockImplement();
    }
    
    static
    void
    UnlockIfNonNULLImplement(WRATHMutex *m)
    {
      if(m!=NULL)
        {
          m->UnlockImplement();
        }
    }
    
  #endif  
    

  /*!\class AutoLock
    Do NOT use directly, use the macro WRATHAutoLockMutex
   */
  class AutoLock
  {
  public:
    #ifdef MUTEX_DEBUG

    AutoLock(WRATHMutex *m, const char *file, int line):
      m_mutex(m)
    {
      LockIfNonNULLImplement(m, file, line);
    }
    AutoLock(WRATHMutex &m, const char *file, int line):
      m_mutex(&m)
    {
      LockImplement(m, file, line);
    }

    ~AutoLock()
    {
      UnlockIfNonNULLImplement(m_mutex, "AutoUnlock", -1);
    }

    #else

    AutoLock(WRATHMutex *m):
      m_mutex(m)
    {
      LockIfNonNULLImplement(m);
    }
    AutoLock(WRATHMutex &m):
      m_mutex(&m)
    {
      LockImplement(m);
    }  

    ~AutoLock()
    {
      UnlockIfNonNULLImplement(m_mutex);
    }  
    #endif

  private:
    WRATHMutex *m_mutex;
  };

  /// @endcond

private:

  int
  internal_mutex_lock(void);

  int
  internal_mutex_unlock(void);


  void *m_opaque;

#ifdef MUTEX_DEBUG
  const char *m_locked_from_file;
  int m_locked_from_line;
  pthread_t m_locking_thread;
#endif


};



/*! @} */
#define WRATHAutoLockMutex_FUNC2(x,y,z) x##y##_C_##z
#define WRATHAutoLockMutex_FUNC1(x,y,z) WRATHAutoLockMutex_FUNC2(x,y,z)
#define WRATHAutoLockMutex_FUNC(x) WRATHAutoLockMutex_FUNC1(x, __COUNTER__, __LINE__)

/*! \addtogroup Utility
 * @{
 */

#ifdef MUTEX_DEBUG


/*!\def WRATHLockMutex
  Use the WRATHLockMutex macro to lock a WRATHMutex.
  Under debug build it will also record the file
  and line number of the when the lock appeared.
  A WRATHMutex is a non-recursive mutex, as such
  it will WRATHassert on successive locking within the
  same thread. See also \ref WRATHLockMutexIfNonNULL, \ref
  WRATHAutoLockMutex, \ref WRATHUnlockMutex and
  \ref WRATHUnlockMutexIfNonNULL.
  \param X pointer or reference to a WRATHMutex to lock
 */
#define WRATHLockMutex(X) WRATHMutex::LockImplement(X, __FILE__, __LINE__)

/*!\def WRATHLockMutexIfNonNULL
  Similar to WRATHLockMutex, the the argument _must_ be pointer.
  Checks if X is non-NULL, and if so locks the mutex pointed
  to by X. See also \ref WRATHLockMutex, \ref
  WRATHAutoLockMutex, \ref WRATHUnlockMutex and
  \ref WRATHUnlockMutexIfNonNULL.
  \param X (potentially NULL) pointer to WRATHMutex to lock
 */
#define WRATHLockMutexIfNonNULL(X) WRATHMutex::LockIfNonNULLImplement(X, __FILE__, __LINE__)

/*!\def WRATHAutoLockMutex
  Effectively creates a uniquely named object on the stack 
  that at construction locks a mutex and at deconstruction 
  unlocks the mutex. See also \ref WRATHLockMutexIfNonNULL, 
  \ref WRATHLockMutex, \ref WRATHUnlockMutex and
  \ref WRATHUnlockMutexIfNonNULL.
  \param X WRATHMutex to auto-lock-unlock, maybe a pointer or
           reference. A NULL pointer results in a no-op
 */
#define WRATHAutoLockMutex(X) WRATHMutex::AutoLock WRATHAutoLockMutex_FUNC(WRATH_wrath_autolocked_mutex)(X, __FILE__, __LINE__)

/*!\def WRATHUnlockMutex
  Use the WRATHUnlockMutex macro to unlock a WRATHMutex.
  A WRATHMutex is a non-recursive mutex, as such
  it will WRATHassert on successive locking within the
  same thread. See also \ref WRATHLockMutexIfNonNULL, \ref
  WRATHAutoLockMutex, \ref WRATHLockMutex and
  \ref WRATHUnlockMutexIfNonNULL.
  \param X pointer or reference to a WRATHMutex to unlock
 */
#define WRATHUnlockMutex(X) WRATHMutex::UnlockImplement(X, __FILE__, __LINE__)


/*!\def WRATHUnlockMutexIfNonNULL
  Similar to WRATHUnlockMutex, the the argument _must_ be pointer.
  Checks if X is non-NULL, and if so unlocks the mutex pointed
  to by X. See also \ref WRATHLockMutexIfNonNULL, \ref
  WRATHAutoLockMutex, \ref WRATHUnlockMutex and
  \ref WRATHLockMutex.
  \param X (potentially NULL) pointer to WRATHMutex to unlock
 */
#define WRATHUnlockMutexIfNonNULL(X) WRATHMutex::UnlockIfNonNULLImplement(X, __FILE__, __LINE__)

#else

#define WRATHLockMutex(X) WRATHMutex::LockImplement(X)
#define WRATHLockMutexIfNonNULL(X) WRATHMutex::LockIfNonNULLImplement(X)
#define WRATHUnlockMutex(X) WRATHMutex::UnlockImplement(X)
#define WRATHUnlockMutexIfNonNULL(X) WRATHMutex::UnlockIfNonNULLImplement(X)
#define WRATHAutoLockMutex(X) WRATHMutex::AutoLock WRATHAutoLockMutex_FUNC(WRATH_wrath_autolocked_mutex)(X)
#endif









/*!\class WRATHThreadID
  A WRATHThreadID is used to identify 
  the current thread and if two threads
  are same or different.
 */
class WRATHThreadID
{
public:
  /*!\fn WRATHThreadID  
    Default ctor, initializes the WRATHThreadID
    as the ID of the running thread, in terms
    of pthread's, pthread_self().
   */
  WRATHThreadID(void)
  {
    m_id=pthread_self();
  }
  
  /*!\fn bool operator==(WRATHThreadID) const
    Returns true if and only if this
    WRATHThreadID and obj refer to the
    same thread.
   */
  bool
  operator==(WRATHThreadID obj) const
  {
    return pthread_equal(m_id, obj.m_id);
  }

  /*!\fn bool operator!=(WRATHThreadID) const
    Returns true if and only if this
    WRATHThreadID and obj refer to 
    different threads.
   */
  bool
  operator!=(WRATHThreadID obj) const
  {
    return !pthread_equal(m_id, obj.m_id);
  }

  /*!\fn WRATHThreadID create_thread
    Spawn a thread, immediately calls
    the thread, returns the thread ID
    of the spawned thread. This function
    is analgous to pthread_create().
    \param fptr function of the thread
    \param argument value to pass to fptr on execution.
   */
  static
  WRATHThreadID
  create_thread(void* (*fptr)(void*), void *argument);
  
  /*!\fn void* wait_thread
    Wait for a thread to complete,
    this function is analgous to 
    pthread_join(). Returns the return 
    value of the function of the thread.
    \param id WRATHThreadID of thread to wait for.
   */
  static
  void*
  wait_thread(WRATHThreadID id);

private:

  enum no_init_type
    {
      no_init
    };

  /*
    Special private ctor to have an uninitialized 
    thread ID.
   */
  WRATHThreadID(enum no_init_type)
  {}

  pthread_t m_id;
};
/*! @} */


#endif
