/*! 
 * \file WRATHTripleBufferEnabler.hpp
 * \brief file WRATHTripleBufferEnabler.hpp
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


#ifndef __WRATH_TRIPLE_BUFFER_ENABLER_HPP__
#define __WRATH_TRIPLE_BUFFER_ENABLER_HPP__

#include "WRATHConfig.hpp"
#include <list>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include "WRATHReferenceCountedObject.hpp"
#include "vecN.hpp"
#include "WRATHNew.hpp"
#include "WRATHassert.hpp"
#include "WRATHMutex.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\class WRATHTripleBufferEnabler
  The purpose of a WRATHTripleBufferEnabler is
  to coordinate between a presentation thread
  and a "logic" thread for the purpose of
  almost lockless triple buffering. 
  The essential idea is that a presenter
  gets the "last" state set that was finished
  by the logic thread. Rather than relying on
  locking large data structures (or many locks
  across many small data structures), an application
  has 3 stores for each value that is written to 
  by the "logic" thread and "read" by the "present" 
  thread. Which store is used for writing and
  reading is dictated by a WRATHTripleBufferEnabler
  as follows:

  Common code:
  \code
   SomeDataType MyState[3];
   WRATHTripleBufferEnabler::handle MyWRATHTripleBufferEnabler;

   void Draw(const SomeDataType &data);
   void RunSimulation(SomeDataType &out_data,
                      const SomeDataType &last_state_of_simulation);

  \endcode

  In the rendering thread:
  \code
  int read_index;

  MyWRATHTripleBufferEnabler->signal_begin_presentation_frame();
  read_index=MyWRATHTripleBufferEnabler->present_ID();

  //draw the content of MyStores[read_index]
  //and those values are guaranteed to not change
  Draw(MyState[read_index]);

  \endcode

  In the logic/simulation thread:
  \code
  


  int write_index;
  int last_write_index;

  write_index=MyWRATHTripleBufferEnabler->current_simulation_ID();
  last_write_index=MyWRATHTripleBufferEnabler->last_simulation_ID();

  //run simulation, can also use the last simulation values
  //computed to allow for incremental simulations:
  RunSimulation(MyState[write_index], MyState[last_write_index]);

  //now signal that simulation completed the operation
  //on MyState[write_index]:
  MyWRATHTripleBufferEnabler->signal_complete_simulation_frame();

  WRATHassert(write_index==MyWRATHTripleBufferEnabler->last_simulation_ID());
  WRATHassert(write_index!=MyWRATHTripleBufferEnabler->current_simulation_ID());

  \endcode
  
  Additionally, each of signal_begin_presentation_frame()
  and signal_complete_simulation_frame() emit signals at 4 different times
    - \ref WRATHTripleBufferEnabler::pre_update_no_lock emitted before the mutex is locked
    - \ref WRATHTripleBufferEnabler::pre_update_lock emitted within the mutex lock before updating the ID's
    - \ref WRATHTripleBufferEnabler::post_update_lock emitted within the mutex lock after updating the ID's
    - \ref WRATHTripleBufferEnabler::post_update_no_lock emitted after the mutex is uncloked 

 */
class WRATHTripleBufferEnabler:
  public WRATHReferenceCountedObjectT<WRATHTripleBufferEnabler>
{
public:
  /*!\typedef signal_t
    Conveniance typedef for the signal type.
   */
  typedef boost::signals2::signal<void ()> signal_t;

  /*!\typedef connect_t
    Conveniance typedef for the connection type.
   */
  typedef boost::signals2::connection connect_t;

  /*!\enum signal_time
    The methods signal_begin_presentation_frame()
    and signal_complete_simulation_frame() each emit
    4 signals (in the boost::signals2 sense) that
    to which one can connect, the enumeration type
    signal_time enumerates the when of each signal.
    The operations performed are as follows:
    - 1) emit \ref pre_update_no_lock signal
    - 2) lock mutex
    - 3) emit \ref pre_update_lock signal
    - 4) updata ID's
    - 5) emit \ref post_update_lock signal
    - 6) unlock mutex
    - 7) emit \ref post_update_no_lock signal
   */
  enum signal_time
    {
      /*!
        Emitted before the mutex lock where
        the ID's are updated.
       */
      pre_update_no_lock=0,

      /*!
        Emitted after the mutex lock but before
        the ID's are updated. Slots connected
        to a signal of this time should be
        fast since the operation takes place
        within the mutex lock of the 
        WRATHTripleBufferEnabler.
       */
      pre_update_lock,

      /*!
        Emitted after the the ID's are updated
        but before the mutex is unlocked. 
        Slots connected to a signal of this 
        time should be fast since the operation 
        takes place within the mutex lock of the 
        WRATHTripleBufferEnabler.
       */
      post_update_lock,
      
      /*!
        Emitted after the mutex is unlocked.
       */
      post_update_no_lock,

      /*!
        Enumeration to indicate the number
        of different enumeration values
        for \ref signal_time. NOT a valid
        value for the enumeration.
       */
      number_signal_time_enums
    };

  /*!\enum signal_type
    Enumeration to describe which of the two
    operations signal_begin_presentation_frame()
    or signal_complete_simulation_frame()
   */
  enum signal_type
    {
      /*!
        Associated to signal_begin_presentation_frame().
       */
      on_begin_presentation_frame=0,

      /*!
        Associated to signal_complete_simulation_frame().
       */
      on_complete_simulation_frame,

      /*!
        Enumeration to indicate the number
        of different enumeration values
        for \ref signal_type. NOT a valid
        value for the enumeration.
       */
      number_signal_type_enums
    };


  /*!\class PhasedDeletedObject
    When objects are accessed in
    both the rendering and simulation
    threads, deletion of such objects
    needs to be done with care.
    As a general rule of thumb,
    if an object is referenced both
    in the rendering and simulation
    threads, then the object should be
    derived from PhasedDeletedObject.
    Deletion of PhasedDeletedObject 
    is done through the macro
    \ref WRATHPhasedDelete.
    A PhasedDeletedObject deletion
    is carried out in phases:
    - 1) it is placed on a list of objects
         and it's on_place_on_deletion_list()
         method is called _immediately_
    - 2) in the simulation thread, the object's
         phase_simulation_deletion() method is called.
    - 3) in rendering thread, the object's
         phase_render_deletion() method is called.
    - 4) in the simulation thread, the object
         is finally deleted (i.e. it's deconstructor
         is called and it's memory is freed).

     Note that ALWAYS: phase_simulation_deletion() is
     called before phase_render_deletion() and that
     on_place_on_deletion_list() may be called from 
     any thread.

     Objects derived from PhasedDeletedObject MUST
     be phased deleted, use the macro \ref WRATHPhasedDelete
     to delete them. Also note WRATHPhasedDelete is
     the same as \ref WRATHDelete for objects not
     derived from PhasedDeletedObject.

     Notes:
     - Any simulation actions created before the macro
       PhasedDeletedObject is invoked are called before
       phase_simulation_deletion(). Any simulation actions
       added after the macro PhasedDeletedObject is invoked 
       are called after phase_simulation_deletion(). 
     - Any rendering actions created before phase_simulation_deletion()
       is called are invoked before the call to phase_render_deletion().
       Any rendering actions created after phase_simulation_deletion()
       is called are called after phase_render_deletion().
     - A developer needs to be hyper aware of dtor order,
       careless use of phased deleted objects can invert 
       deconstruction order. The most obvious issue is for
       hierarchies where an object has child phased delete
       objects. If the parent object does not call WRATHPhasedDelete
       on it's child objects at on_place_on_deletion_list(),
       then if there are subsequent objects deleted before
       the simulation runs again, the dtor order can become
       corrupted. You have been warned!

   */
  class PhasedDeletedObject:boost::noncopyable
  {
  public:
    /*!\fn PhasedDeletedObject(const handle&)
      Ctor. 
      \param tr handle to \ref WRATHTripleBufferEnabler object
                to which to sync; handle must be valid.
     */
    explicit
    PhasedDeletedObject(const handle &tr);

    virtual
    ~PhasedDeletedObject();

    /*!\fn const handle& triple_buffer_enabler  
      Returns a handle to the WRATHTripleBufferEnabler that this
      object uses for triple buffer coordinating.
    */
    const handle& 
    triple_buffer_enabler(void) const
    {
      WRATHassert(this!=NULL);
      WRATHassert(m_ptr!=NULL);
      return m_tr;
    }

    /*!\fn connect_t connect  
      Provided as a conveniance, equivalent to
      \code
      triple_buffer_enabler()->connect(tp, tm, subscriber, gp_order)
      \endcode
      see \ref 
      WRATHTripleBufferEnabler::connect(enum signal_type,enum signal_time, const signal_t::slot_type&, int)
      \param tp signal type to which to hook
      \param tm time specification
      \param subscriber functor to call
      \param gp_order Calling of slots from a specific
                      (signal_type, signal_time) order
                      can be specified by setting gp_order,
                      slots connected with a lower gp_order 
                      are guaranteed to be called before slots 
                      connected with a higher (for a fixed 
                      signal_type-signal_time pair). Slots
                      in the same gp_order have no guarantee
                      of which is called first.
    */
    connect_t
    connect(enum signal_type tp,
            enum signal_time tm,
            const signal_t::slot_type &subscriber,
            int gp_order=0) const
    {
      return m_tr->connect(tp, tm, subscriber, gp_order);
    }

    /*!\fn void schedule_rendering_action   
      Provided as a conveniance, equivalent to
      \code
      triple_buffer_enabler()->schedule_rendering_action(v)
      \endcode
      see \ref 
      WRATHTripleBufferEnabler::schedule_rendering_action(const T&)
      \tparam T copyable functor object providing operator(void) method
      \param v action to schedule
     */
    template<typename T>
    void
    schedule_rendering_action(const T &v) const
    {
      m_tr->schedule_rendering_action<T>(v);
    }
  
    /*!\fn void schedule_simulation_action  
      Provided as a conveniance, equivalent to
      \code
      triple_buffer_enabler()->schedule_simulation_action(v)
      \endcode
      see \ref 
      WRATHTripleBufferEnabler::schedule_simulation_action(const T&)
      \tparam T copyable functor object providing operator(void) method
      \param v functor object providing the action to execute
     */
    template<typename T>
    void
    schedule_simulation_action(const T &v) const
    {
      m_tr->schedule_simulation_action<T>(v);
    }

    /*!\fn int present_ID
      Provided as a conveniance, equivalent to
      \code
      triple_buffer_enabler()->present_ID()
      \endcode
      see \ref WRATHTripleBufferEnabler::present_ID(void)
     */
    int
    present_ID(void) const
    {
      return m_tr->present_ID();
    }

    /*!\fn int current_simulation_ID
      Provided as a conveniance, equivalent to
      \code
      triple_buffer_enabler()->current_simulation_ID()
      \endcode
      see \ref WRATHTripleBufferEnabler::current_simulation_ID(void)
     */
    int
    current_simulation_ID(void) const
    {
      return m_tr->current_simulation_ID();
    }

    /*!\fn int last_simulation_ID
      Provided as a conveniance, equivalent to
      \code
      triple_buffer_enabler()->last_simulation_ID()
      \endcode
      see \ref WRATHTripleBufferEnabler::last_simulation_ID(void)
     */
    int
    last_simulation_ID(void) const
    {
      return m_tr->last_simulation_ID();
    }

#ifdef WRATH_NEW_DEBUG

    /*!\fn implement_phase_deleted(T*, boost::true_type, const char*, int)
      Do not use! Implementation function for
      macro WRATHPhasedDelete.
     */
    template<typename T>
    static
    void
    implement_phase_deleted(T *ptr, boost::true_type, 
                            const char *file, int line)
    {      
      WRATHMemory::object_deletion_message(ptr, file, line, false);
      phased_delete_object(ptr, file, line);
    }

    /*!\fn implement_phase_deleted(T*, boost::false_type, const char*, int)
      Do not use! Implementation function for
      macro WRATHPhasedDelete.
     */
    template<typename T>
    static
    void
    implement_phase_deleted(T *ptr, boost::false_type, 
                            const char *file, int line)
    {
      WRATHMemory::object_deletion_message(ptr, file, line, true);
      ::delete ptr;
    }

    /*!\fn implement_phase_deleted(T*, const char*, int)
      Do not use! Implementation function for
      macro WRATHPhasedDelete.
     */
    template<typename T>
    static
    void
    implement_phase_deleted(T *ptr, const char *file, int line)
    {
      implement_phase_deleted(ptr, 
                              typename boost::is_base_of<PhasedDeletedObject, T>::type(),
                              file, line);
    }
        

#else
    /*!\fn implement_phase_deleted(T*, boost::true_type)
      Do not use! Implementation function for
      macro WRATHPhasedDelete.
     */
    template<typename T>
    static
    void
    implement_phase_deleted(T *ptr, boost::true_type)
    {
      phased_delete_object(ptr);
    }

    /*!\fn implement_phase_deleted(T*, boost::false_type)
      Do not use! Implementation function for
      macro WRATHPhasedDelete.
     */
    template<typename T>
    static
    void
    implement_phase_deleted(T *ptr, boost::false_type)
    {
      ::delete ptr;
    }

    /*!\fn implement_phase_deleted(T*)
      Do not use! Implementation function for
      macro WRATHPhasedDelete.
     */
    template<typename T>
    static
    void
    implement_phase_deleted(T *ptr)
    {
      implement_phase_deleted(ptr, 
                              typename boost::is_base_of<PhasedDeletedObject, T>::type());
    }
#endif
    
  protected:
    /*!\fn void on_place_on_deletion_list
      To be optionally implemented by a derived 
      class. Is called when the object is placed 
      for delayed deletion (i.e. passed to the
      macro PhasedDeletedObject). May be called
      from any thread.
     */
    virtual
    void
    on_place_on_deletion_list(void)
    {}

    /*!\fn void phase_simulation_deletion
      To be optionally implemented by a derived 
      class. Called from the simulation thread 
      _AFTER_ on_place_on_deletion_list().
     */
    virtual
    void
    phase_simulation_deletion(void)
    {}

    /*!\fn void phase_render_deletion
      To be optionally implemented by a derived 
      class. Called from the rendering thread 
      _AFTER_ phase_simulation_deletion().
     */
    virtual
    void
    phase_render_deletion(void)
    {}

  private:
    
    void
    simulation_delete(void);

    void
    render_delete(void);

    #ifdef WRATH_NEW_DEBUG
       /*
         -1= alive
         0=on phase0
         1=on phase1
         2=on phase2
       */
       int m_deletion_phase;
       PhasedDeletedObject *m_ptr;
       
       const char *m_delete_at_file;
       int m_delete_at_line;
 
       static
       void
       phased_delete_object(PhasedDeletedObject *ptr, 
                            const char *file, int line);
     #else
       static
       void
       phased_delete_object(PhasedDeletedObject *ptr);
     #endif

    handle m_tr;
  };

 
  /*!\fn WRATHTripleBufferEnabler 
    Default ctor.
   */
  WRATHTripleBufferEnabler(void);

  ~WRATHTripleBufferEnabler();

  /*!\fn signal_complete_simulation_frame
    Call in the simulation thread (and only
    the simulation thread!) to indicate
    that the simulation is finished running
    a single frame and that the values
    are ready to be consumed by the presentation
    thread.
   */
  void
  signal_complete_simulation_frame(void);
  
  /*!\fn signal_begin_presentation_frame
    Call in the presentation thread (and only
    the presentation thread) _before_ presenting
    data.
   */
  void
  signal_begin_presentation_frame(void);

  /*!\fn connect_t connect(enum signal_type, enum signal_time, const signal_t::slot_type&, int)
    Add a slot to a named signal, returning
    the connect_t object.
    \param tp signal type to which to hook
    \param tm time specification
    \param subscriber functor to call
    \param gp_order Calling order of slots from a specific
                    (signal_type, signal_time) 
                    can be specified by setting gp_order,
                    slots connected with a lower gp_order 
                    are guaranteed to be called before slots 
                    connected with a higher (for a fixed 
                    signal_type-signal_time pair). Slots
                    in the same gp_order have no guarantee
                    of which is called first.
   */
  connect_t
  connect(enum signal_type tp,
          enum signal_time tm,
          const signal_t::slot_type &subscriber,
          int gp_order=0);

  /*!\fn int present_ID
    Returns the ID for the data to be presented
    by the presentation thread. May only be
    called from the presentation thread.
   */
  int
  present_ID(void)
  {
    return m_present_ID;
  }

  /*!\fn int last_simulation_ID
    Returns the ID for the data that was 
    last completed by the simulation thread.
    May only be called from the simulation 
    thread.
   */
  int
  last_simulation_ID(void)
  {
    return m_last_simulation_ID;
  }

  /*!\fn int current_simulation_ID
    Returns the ID for the data that the
    simulation thread is computing, may
    only be called from the simulation
    thread.
   */
  int
  current_simulation_ID(void)
  {
    return m_current_simulation_ID;
  }
  
  /*!\fn number_complete_simulation_frame_calls
    Returns the total number times \ref
    signal_complete_simulation_frame() has been called.
    May be called from both the rendering
    and simulation thread.
   */
  int
  number_complete_simulation_frame_calls(void)
  {
    WRATHAutoLockMutex(m_counter_lock);
    return m_number_complete_simulation_frame_calls;
  }

  /*!\fn number_begin_presentation_frame_calls
    Returns the total number times \ref
    signal_begin_presentation_frame() has been called.
    May be called from both the rendering
    and simulation thread. 
   */
  int
  number_begin_presentation_frame_calls(void)
  {
    WRATHAutoLockMutex(m_counter_lock);
    return m_number_begin_presentation_frame_calls;
  }

  /*!\fn number_complete_simulation_calls_since_last_begin_presentation_frame
    Returns the number times \ref signal_complete_simulation_frame()
    has been called since the last call to \ref signal_begin_presentation_frame().
   */
  int
  number_complete_simulation_calls_since_last_begin_presentation_frame(void)
  {
    WRATHAutoLockMutex(m_counter_lock);
    return m_number_complete_simulation_calls_since_last_begin_presentation_frame;
  }

  /*!\fn number_begin_presentation_calls_since_last_simulation_complete_frame
    Returns the number times \ref signal_begin_presentation_frame()
    has been called since the last call to \ref signal_complete_simulation_frame().
   */
  int
  number_begin_presentation_calls_since_last_simulation_complete_frame(void)
  {
    WRATHAutoLockMutex(m_counter_lock);
    return m_number_begin_presentation_calls_since_last_simulation_complete_frame;
  }

  /*!\fn void schedule_rendering_action(const T&)
    Actions added via schedule_rendering_action()
    are executed at end of the next call to 
    \ref signal_begin_presentation_frame(void),
    i.e. after \ref present_ID() is updated.
    \tparam T functor class that must be copyable and provides
              the method operator() to execute its action(s)
    \param v functor object to execute
   */
  template<typename T>
  void
  schedule_rendering_action(const T &v)
  {
    base_action *ptr;

    ptr=create_action<T>(v);
    WRATHLockMutex(m_render_actions_mutex);
    m_render_actions.push_back(ptr);
    WRATHUnlockMutex(m_render_actions_mutex);
  }

  /*!\fn void schedule_simulation_action(const T&)  
    Actions added via schedule_simulation_action()
    are executed at the beginning of the next call to 
    \ref signal_complete_simulation_frame(void),
    i.e. BEFORE the values \ref current_simulation_ID()
    and \ref last_simulation_ID() are updated.
    \tparam T functor class that must be copyable and provides
              the method operator() to execute its action(s)
    \param v functor object to execute
  */
  template<typename T>
  void
  schedule_simulation_action(const T &v)
  {
    base_action *ptr;

    ptr=create_action<T>(v);
    WRATHLockMutex(m_simulation_actions_mutex);
    m_simulation_actions.push_back(ptr);
    WRATHUnlockMutex(m_simulation_actions_mutex);
  }

  /*!\fn int purge_cleanup
    To be called _after_ the simulation and
    rendering threads have joined to
    delete any remaining phased-deleted objects
    and to any GL scheduled rendering or
    simulation actions. Internally, simply
    calls signal_complete_simulation_frame() and
    signal_begin_presentation_frame() unitl there
    are no rendering or simulation actions to
    execute and no phased delete object to delete. 
    Returns the number of signal_complete_simulation_frame()/
    signal_begin_presentation_frame() pairs executed 
    for cleanup
   */
  int
  purge_cleanup(void);

private:

  /*
    private class magic to allow one to use boost::bind
    to name operations to be executed
   */
  class base_action
  {
  public:
    virtual
    ~base_action(void)
    {}

    virtual
    void
    execute(void)=0;
  };

  template<typename T>
  class action:public base_action
  {
  public:
    T m_value;

    action(const T &v):
      m_value(v)
    {}

    virtual
    void
    execute(void)
    {
      m_value();
    }
  };


  template<typename T>
  static
  base_action*
  create_action(const T &v)
  {
    return WRATHNew action<T>(v);
  }

  
  class PhasedDeletedObjectEntry
  {
  public:
    
    PhasedDeletedObject *m_object;
    #ifdef WRATH_NEW_DEBUG
      const char *m_file;
      int m_line;
    #endif

    PhasedDeletedObjectEntry(PhasedDeletedObject *obj):
      m_object(obj)
      #ifdef WRATH_NEW_DEBUG
      ,m_file("NoFile")
      ,m_line(-1)
      #endif
    {}
  };


  static
  void
  do_actions(std::list<base_action*> &actions,
             WRATHMutex &mutex);

  
  void
  fire_signal(signal_t &sig);

  std::list<PhasedDeletedObjectEntry> m_phase0, m_phase1, m_phase2;
  std::list<base_action*> m_render_actions;
  std::list<base_action*> m_simulation_actions;  
  vecN<vecN<signal_t, number_signal_time_enums>, number_signal_type_enums> m_sigs;

  bool m_purging;
  WRATHMutex m_mutex;
  WRATHMutex m_phase_mutex;
  WRATHMutex m_render_actions_mutex;
  WRATHMutex m_simulation_actions_mutex;
  int m_present_ID;
  int m_last_simulation_ID;
  int m_current_simulation_ID;

  WRATHMutex m_counter_lock;
  int m_number_complete_simulation_frame_calls;
  int m_number_begin_presentation_frame_calls;
  int m_number_complete_simulation_calls_since_last_begin_presentation_frame;
  int m_number_begin_presentation_calls_since_last_simulation_complete_frame;
};



/*!\def WRATHPhasedDelete
   WRATHPhasedDelete maps to \ref WRATHDelete for objects not derived
   from WRATHTripleBufferEnabler::PhasedDeletedObject otherwise
   begins phased deletion of the object, see \ref 
   WRATHTripleBufferEnabler::PhasedDeletedObject
   \param ptr obejct to phased delete or directly delete.
 */

#ifdef WRATH_NEW_DEBUG
#define WRATHPhasedDelete(ptr) do { \
    WRATHTripleBufferEnabler::PhasedDeletedObject::implement_phase_deleted(ptr, __FILE__, __LINE__); } while(0)

#else
#define WRATHPhasedDelete(ptr) do { \
    WRATHTripleBufferEnabler::PhasedDeletedObject::implement_phase_deleted(ptr); } while(0)

#endif

/*! @} */

#endif
