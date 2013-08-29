/*! 
 * \file WRATHShape.hpp
 * \brief file WRATHShape.hpp
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


#ifndef __WRATH_SHAPE_HPP__
#define __WRATH_SHAPE_HPP__


#include "WRATHConfig.hpp"
#include <typeinfo>
#include <cstring>
#include "WRATHUtil.hpp"
#include "WRATHReferenceCountedObject.hpp"
#include "WRATHOutline.hpp"
#include "WRATHAttributeStore.hpp"
#include "WRATHAttributePacker.hpp"
#include "WRATHShaderSpecifier.hpp"



/*! \addtogroup Shape
 * @{
 */


/*!\class WRATHShape
  A WRATHShape is a collection of WRATHOutline's
  that specifies a 2D shape in the plane. 

  A WRATHShape stores payloads, keyed by
  payload type. A payload for a WRATHShape
  is set of processed data generated
  from the WRATHShape (for example tesellating
  the WRATHOutline objects of the WRATHShape.
  Payloads are fetched and created with the
  functions \ref fetch_payload() and \ref
  fetch_matching_payload(). Of critical
  importance is that a WRATHShape will only
  store one payload object per payload
  type. Additionally, whenever the WRATHShape
  object's geomtry is modified it clears
  it's payload storage. Hence, a user is
  guaranteed that payload fetched from a
  WRATHShape accurately reflect the current
  geometry of the WRATHShape.

  Do not change or query the same WRATHShape object
  from multiple threads without surrounding such
  calls with mutex locks.

  The requirement for a payload type P are as follows:
  - derived from WRATHReferenceCountedObjectT<P>
  - defines the (possibly empty) type PayloadParams which is copyable
  - the type PayloadParams defines the typedef PayloadType which is P,
    i.e. P::PayloadParams::PayloadType is P
  - defines the static function P::handle generate_payload(const WRATHShape<T>&, const P::PayloadParams &)
    which creates and returns a payload from the passed WRATHShape object
    and the passed parameters object
  - defines the static function P::handle generate_payload(const WRATHShape<T>&) which
    create and returns a payload object using the passed WRAThShape with
    default parameters. Additionally, if the creation of the payload type
    P requires other payloads, it will use \ref fetch_payload() instead of
    \ref fetch_matching_payload().

 */
template<typename T>
class WRATHShape:boost::noncopyable 
{
public:
  /*!\typedef outline_point
    Conveniance typedef
   */
  typedef typename WRATHOutline<T>::point outline_point;

  /*!\typedef position_type
    Conveniance typedef
   */
  typedef typename WRATHOutline<T>::position_type position_type;
 
  /*!\typedef control_point
    Conveniance typedef
   */
  typedef typename WRATHOutline<T>::control_point control_point;

  
  ~WRATHShape()
  {
    clear();
  }

  /*!\fn WRATHOutline<T>& current_outline
    Returns a reference to the current
    outline object.
   */
  WRATHOutline<T>&
  current_outline(void)
  {
    if(m_outlines.empty())
      {
        m_outlines.push_back(this->make_outline(0));
      }

    return *m_outlines.back();
  }

  /*!\fn void new_outline
    Add a new outline to the WRATHShape, the
    ID of the new outline will be the ID of
    the previous outline plus one.
   */
  void
  new_outline(void)
  {
    if(!current_outline().points().empty())
      {
        m_outlines.push_back(this->make_outline(m_outlines.size()));
        mark_dirty();
      }
  }

  /*!\fn void clear
    Delete all outlines of the WRATHShape.
   */
  void
  clear(void)
  {
    for(typename std::vector<WRATHOutline<T>*>::iterator iter=m_outlines.begin(),
          end=m_outlines.end(); iter!=end; ++iter)
      {
        WRATHDelete(*iter);
      }
    m_outlines.clear();
    mark_dirty();
  }
  
  
  /*!\fn const WRATHOutline<T>* outline
    Return's the named outline of this WRATHShape
    \param ID ID of outline to retrieve
   */
  const WRATHOutline<T>*
  outline(int ID) const
  {
    return m_outlines[ID];
  }

  /*!\fn WRATHShape& move_to
    Provided as a conveniance, equivalent to
    \code
    new_outline();
    current_outline() << v;
    \endcode
    \param v point at which to start a new outline
   */
  WRATHShape&
  move_to(const position_type &v)
  {
    new_outline();
    current_outline() << v;
    return *this;
  }

  /*!\fn WRATHShape& line_to
    Provided as a conveniance, equivalent to
    \code
    current_outline() << v;
    \endcode
    \param v point to which to connect a line
   */
  WRATHShape&
  line_to(const position_type &v)
  {
    current_outline() << v;
    return *this;
  }

  /*!\fn WRATHShape& quadratic_to
    Provided as a conveniance, equivalent to
    \code
    current_outline() << control_point(c) << v;
    \endcode
    \param c position of control point of quadratic Bezier curve
    \param v point at which to end the quadratic Bezier curve
   */
  WRATHShape&
  quadratic_to(const position_type &c, const position_type &v)
  {
    current_outline() << control_point(c) << v;
    return *this;
  }

  /*!\fn WRATHShape& cubic_to
    Provided as a conveniance, equivalent to
    \code
    current_outline() << control_point(c1)  << control_point(c2) << v;
    \endcode
    \param c1 position of first control point of cubic Bezier curve
    \param c2 position of second control point of cubic Bezier curve
    \param v point at which to end the cubic Bezier curve
   */
  WRATHShape&
  cubic_to(const position_type &c1, const position_type &c2, const position_type &v)
  {
    current_outline() << control_point(c1) << control_point(c2) << v;
    return *this;
  }

  /*!\fn int number_outlines
    Returns the number of outline of this WRATHShape
   */
  int 
  number_outlines(void) const
  {
    return m_outlines.size();
  }

  /*!\fn P::handle fetch_matching_payload
    Fetches a payload of type P created
    with the passed parameters. If the payload
    of type P does not exist or if the passed
    payload creation parameters do not match
    the passed parameters, then creates a new
    payload using the passed parameters and
    saves that as the payload of type P.
    Do not do fething of a payload on the
    same WRATHShape object across multiple
    threads without surrounding the call
    with mutex locking.
    The payload type P must satisfy the following
    - derived from WRATHReferenceCountedObjectT<P>
    - defines the type PayloadParams which is copyable
    - the type PayloadParams defines the typedef PayloadType which is P,
      i.e. P::PayloadParams::PayloadType is P
    - defines the static function P::handle generate_payload(const WRATHShape<T>&, const P::PayloadParams &)
      which creates and returns a payload from the passed WRATHShape object
      and the passed parameters object
    - defines the static function P::handle generate_payload(const WRATHShape<T>&) which
      create and returns a payload object using the passed WRAThShape with
      default parameters. Additionally, if the creation of the payload type
      P requires other payloads, it will use \ref fetch_payload() instead of
      \ref fetch_matching_payload().

    \tparam P Payload type.               
    \param params payload creation parameters for the payload type P 
   */
  template<typename P>
  typename P::handle
  fetch_matching_payload(typename P::PayloadParams const &params
                         =typename P::PayloadParams()) const
  {
    return this->fetch_payload_implement(type_tag<P>(), params, true);
  }

  /*!\fn P::handle fetch_payload
    Checks if a payload of type P exists,
    and if so returns it. Otherwise creates
    a payload of type P using the passed
    parameters to generate the payload.
    Do not do fetching of a payload on the
    same WRATHShape object across multiple
    threads without surrounding the call
    with mutex locking.
    The payload type P must satisfy the following
    - derived from WRATHReferenceCountedObjectT<P>
    - defines the (possibly empty) type PayloadParams which is copyable
    - the type PayloadParams defines the typedef PayloadType which is P,
      i.e. P::PayloadParams::PayloadType is P
    - defines the static function P::handle generate_payload(const WRATHShape<T>&, const P::PayloadParams &)
      which creates and returns a payload from the passed WRATHShape object
      and the passed parameters object
    - defines the static function P::handle generate_payload(const WRATHShape<T>&) which
      create and returns a payload object using the passed WRAThShape with
      default parameters. Additionally, if the creation of the payload type
      P requires other payloads, it will use \ref fetch_payload() instead of
      \ref fetch_matching_payload().
    \tparam P Payload type, see function description for requirements
   */
  template<typename P>
  typename P::handle
  fetch_payload(void) const
  {
    typename P::PayloadParams params;
    return this->fetch_payload_implement(type_tag<P>(), params, false);
  }

  /*!\fn const std::string& label(void) const
    Returns the label of the WRATHShape,
    the label of a WRATHShape is a user
    defined string used to identify the 
    WRATHShape uniquely, the default
    value is an empty string.
   */
  const std::string&
  label(void) const
  {
    return m_label;
  }

  /*!\fn void label(const std::string &)
    Sets the label of the WRATHShape,
    the label of a WRATHShape is a user
    defined string used to identify the 
    WRATHShape uniquely, the default
    value is an empty string.
    \param v value to which to set the label
   */
  void
  label(const std::string &v) 
  {
    m_label=v;
  }

private:
  class LocalOutlineType:public WRATHOutline<T>
  {
  public:
    LocalOutlineType(unsigned int pID, WRATHShape *master):
      WRATHOutline<T>(pID),
      m_master(master)
    {}

  protected:
    virtual
    void
    on_change(void)
    {
      m_master->mark_dirty();
    }

    WRATHShape *m_master;
  };

  class payload_hoard_entry_base:
    public WRATHReferenceCountedObjectT<payload_hoard_entry_base>
  {
  public:

    payload_hoard_entry_base():
      //no need for mutex locking on the counter...
      WRATHReferenceCountedObjectT<payload_hoard_entry_base>(NULL)
    {}

    template<typename P>
    typename P::handle
    get_handle(type_tag<P>, typename P::PayloadParams const &rhs,
               bool params_must_match) const
    {
      WRATHReferenceCountedObject::handle R;
      typename P::handle return_value;

      R=get_handle_implement(typeid(typename P::PayloadParams),
                             &rhs, params_must_match);

      if(R.valid())
        {
          return_value=R.dynamic_cast_handle<P>();
          WRATHassert(return_value.valid());
        }
      return return_value;
    }

    template<typename P>
    typename P::handle
    get_handle(type_tag<P>) const
    {
      WRATHReferenceCountedObject::handle R;
      typename P::handle return_value;

      R=get_handle_implement(typeid(P));
      if(R.valid())
        {
          return_value=R.dynamic_cast_handle<P>();
          WRATHassert(return_value.valid());
        }
      return return_value;
    }


    virtual
    WRATHReferenceCountedObject::handle
    get_handle_implement(const std::type_info &param_type,
                         const void *param_bytes,
                         bool params_must_match) const=0;

    virtual
    WRATHReferenceCountedObject::handle
    get_handle_implement(const std::type_info &payload_type) const=0;

  };

  template<typename PayloadType>
  class payload_hoard_entry:public payload_hoard_entry_base
  {
  public:
    typedef typename PayloadType::handle payload_handle;
    typedef typename PayloadType::PayloadParams PayloadParams;

    payload_hoard_entry(const PayloadParams &params,
                        const payload_handle &h):
      m_params(params),
      m_h(h)
    {}

    virtual
    WRATHReferenceCountedObject::handle
    get_handle_implement(const std::type_info &param_type,
                         const void *param_bytes,
                         bool params_must_match) const
    {
      WRATHassert(param_type==typeid(PayloadParams));
      WRATHunused(param_type);

      const PayloadParams *ptr(reinterpret_cast<const PayloadParams*>(param_bytes));
      if(params_must_match or *ptr==m_params)
        {
          return m_h;
        } 
      else
        {
          return payload_handle();
        }

    }

    virtual
    WRATHReferenceCountedObject::handle
    get_handle_implement(const std::type_info &payload_type) const
    {
      WRATHassert(payload_type==typeid(PayloadType));
      WRATHunused(payload_type);
      return m_h;
    }


  private:
    PayloadParams m_params;
    payload_handle m_h;
  };

  typedef WRATHUtil::TypeInfoSortable payload_hoard_key;
  typedef typename payload_hoard_entry_base::const_handle payload_hoard_value;
  typedef std::map<payload_hoard_key, payload_hoard_value> payload_hoard;
  typedef typename payload_hoard::iterator payload_iterator;
  typedef typename payload_hoard::value_type payload_entry;

   
  template<typename P>
  typename P::handle
  fetch_payload_implement(type_tag<P>, 
                          typename P::PayloadParams const &params,
                          bool params_must_match) const
  { 


    payload_iterator iter;

    iter=m_payloads.find(typeid(P));
    if(iter!=m_payloads.end())
      {
        const payload_hoard_value &V(iter->second);
        typename P::handle H;

        H=V->get_handle(type_tag<P>(), params, params_must_match);
        if(H.valid())
          {
            return H;
          }
        m_payloads.erase(iter);
      }

    typename P::handle H;

    if(params_must_match)
      {
        H=P::generate_payload(*this, params);
      }
    else
      {
        H=P::generate_payload(*this);
      }

    payload_hoard_value V;

    V=WRATHNew payload_hoard_entry<P>(params, H);
    m_payloads.insert( payload_entry(typeid(P), V) );

    return H;

  }



  WRATHOutline<T>*
  make_outline(unsigned int pID)
  {
    return WRATHNew LocalOutlineType(pID, this);
  }

  void
  mark_dirty(void)
  {
    m_payloads.clear();
  }

  std::vector<WRATHOutline<T>*> m_outlines;
  std::string m_label;

  /*
    keyed by payload type, values as (params, handles)
   */
  mutable payload_hoard m_payloads;
};


/*!\typedef WRATHShapeF
  Convenience typedef to WRATHShape\<float\>
 */
typedef WRATHShape<float> WRATHShapeF;

/*!\typedef WRATHShapeI
  Convenience typedef to WRATHShape\<int32_t\>
 */
typedef WRATHShape<int32_t> WRATHShapeI;

/*!\typedef WRATHShapeProcessorPayload 
  A WRATHShape specifies the raw defining
  data of a shape. Typically to draw shapes
  (be it filling or stroking or something else),
  that data needs to processed. A 
  WRATHShapeProcessorPayload represents a handle 
  to such processed data. Also note that
  a WRATHShape stores and creates payload
  objects, at most one per payload type, via
  the methods \ref WRATHShape<T>::fetch_payload()
  and \ref WRATHShape<T>::fetch_matching_payload()
*/
typedef WRATHReferenceCountedObject::handle_t<WRATHReferenceCountedObject> WRATHShapeProcessorPayload;


/*! @} */
#endif
