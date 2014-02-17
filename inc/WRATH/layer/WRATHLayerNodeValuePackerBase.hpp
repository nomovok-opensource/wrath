/*! 
 * \file WRATHLayerNodeValuePackerBase.hpp
 * \brief file WRATHLayerNodeValuePackerBase.hpp
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


#ifndef WRATH_HEADER_LAYER_ITEM_UNIFORM_PACKER_BASE_HPP_
#define WRATH_HEADER_LAYER_ITEM_UNIFORM_PACKER_BASE_HPP_

#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include "c_array.hpp"
#include "WRATHglShaderBits.hpp"
#include "WRATHShaderSpecifier.hpp"
#include "WRATHReferenceCountedObject.hpp"
#include "WRATHLayerBase.hpp"

/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerNodeValuePackerBase
  A WRATHLayerNodeValuePackerBase provides an interface
  to "pack" used per-node values into an array to be
  "sent" to GL for consumption. It defines an interface
  to name these values in GLSL and their source.
  Derived classes will implement the actual details of the GL calls
  to get that data to GL. A dervied class must still implement
  - append_state(WRATHSubItemDrawState &) to add to the GL state
    vector. The intention is that the objects appended will 
    use DataToGL::data_to_pack_to_GL() to send the per-node 
    values to GL through state added by append_state()

  In addition, a derived class must also implement the _static_ function
  - const WRATHLayerNodeValuePackerBase::function_packet& functions(void)
  to return the function packet to use the derived type.
 */
class WRATHLayerNodeValuePackerBase:
  public WRATHLayerBase::GLStateOfNodeCollection
{
public:
  /*!\class ActiveNodeValue
    Because not all per-node values are used by each
    shader stage, the location of the node-value
    in the data to send to GL is NOT the same as
    the source node-index (i.e. \ref m_source_index).
    An ActiveNodeValue records the source and the
    location within that array to send to GL.
   */
  class ActiveNodeValue
  {
  public:
    /*!\fn ActiveNodeValue
      Ctor, intializes \ref m_source_index
      and \ref m_offset as -1
     */ 
    ActiveNodeValue(void):
      m_source_index(-1),
      m_offset(-1)
    {}

    /*!\fn const std::string& label(void) const
      Returns the first label of the ActiveNodeValue.
      If \ref m_labels is empty, then asserts on 
      debug.
     */
    const std::string&
    label(void) const
    {
      WRATHassert(!m_labels.empty());
      return *m_labels.begin();
    }

    /*!\var m_source_index
      Specifies the source index from 
      WRATHLayerItemNodeBase::extract_values()
      that the per-node value takes as the value 
     */
    int m_source_index;

    /*!\var m_offset
      Offset into \ref DataToGL::data_to_pack_to_GL()
      where the value is stored.
     */
    int m_offset;

    /*!\var m_labels
      Set of labels for the per-node value as it is 
      to appear in GLSL.
     */
    std::set<std::string> m_labels;
  };

  /*!\class ActiveNodeValues
    An ActiveNodeValues is a collection
    of ActiveNodeValue objects for a fixed
    shader stage. Essentially it is an std::map
    of ActiveNodeValue objects keyed by 
    \ref ActiveNodeValue::m_source_index.
   */
  class ActiveNodeValues
  {
  public:

    /*!\typedef map_type
      The std::map underneath. Keyed by node-indices
      (i.e values as in \ref ActiveNodeValue::m_source_index)
      with values as ActiveNodeValue objects. 
      In particular, 
      \code 
      map_type[I].m_source_index==I 
      \endcode
      for all I when I is within the map.
     */
    typedef std::map<int, ActiveNodeValue> map_type;

    /*!\class Filter
      A Filter determines if a ActiveNodeValue from
      one ActiveNodeValues is to be absorbed by
      a another ActiveNodeValues object via absorb().
     */
    class Filter:
      public WRATHReferenceCountedObjectT<Filter>
    {
    public:
      /*!\fn bool absorb_active_node_value
        To be implemented by a derived class to return
        true if and only if the passed ActiveNodeValue
        is to be added.
       */
      virtual
      bool
      absorb_active_node_value(const ActiveNodeValue&) const
      {
        return true;
      }
    };
    
    /*!\fn ActiveNodeValues
      Ctor to initialize ActiveNodeValues object
     */
    ActiveNodeValues(void);

    /*!\fn ActiveNodeValues& add_source(int, const std::string&)
      Mark a per-node value as active with
      a label.
      \param idx source index, i.e. a value for
                 \ref ActiveNodeValue::m_source_index
      \param label GLSL name for the node value
     */
    ActiveNodeValues&
    add_source(int idx, const std::string &label);

    /*!\fn const map_type& entries(void) const
      Returns the entries of the underlying map.
     */
    const map_type&
    entries(void) const
    {
      return m_data;
    }

    /*!\fn int number_active(void) const
      Returns the number of distinct node-values active
     */
    int
    number_active(void) const
    {
      return m_data.size();
    }
    
    /*!\fn bool node_value_active(int) const
      Returns true if a per-node value is active.
      \param source_index index into node value 
                          (\ref ActiveNodeValue::m_source_index)
                          to query if active
     */ 
    bool
    node_value_active(int source_index) const
    {
      return source_index>=0 
        and m_permutation_array.size()>static_cast<unsigned int>(source_index)
        and m_permutation_array[source_index]!=-1;
    }

    /*!\fn bool contains(const ActiveNodeValues&) const
      Returns true if each element active in 
      the passed ActiveNodeValues, obj, is
      also active in this ActiveNodeValues.
      NOTE: only tests for active value 
      source indices, it does NOT test if 
      their names in GLSL will be the same.
      \param obj ActiveNodeValues to which to test against
     */
    bool
    contains(const ActiveNodeValues &obj) const;

    /*!\fn bool same(const ActiveNodeValues&) const
      Returns true if the active node values
      in the passed ActiveNodeValues, obj, is
      exactly the same as the active node
      value in this.
      NOTE: only tests for active value 
      source indices, it does NOT test if 
      their names in GLSL will be the same.x
      \param obj ActiveNodeValues to which to test against
     */
    bool
    same(const ActiveNodeValues &obj) const;

    /*!\fn ActiveNodeValues& absorb(const ActiveNodeValues&, const Filter::const_handle&)
      Absorb all entries of a different
      \ref ActiveNodeValues object into this
      \ref ActiveNodeValues object.
      \param obj NodeValues object from which to
                 absorb entries
      \param hnd handle to a Filter object, if the handle
                 is not valid then all ActiveNodeValue objects
                 from obj are absorbed.
     */
    ActiveNodeValues&
    absorb(const ActiveNodeValues &obj,
           const Filter::const_handle &hnd=Filter::const_handle());

    /*!\fn int one_plus_highest_index
      Returns the one plus highest node value index
      (i.e. the highest value across all
      ActiveNodeValue objects of
      ActiveNodeValue::m_source_index).
     */
    int
    one_plus_highest_index(void) const
    {
      return m_permutation_array.size();
    }

  private:
    map_type::iterator
    fetch_source_iterator(unsigned int source_index);

    /*
      WRATHLayerNodeValuePackerBase needs access to m_permutation_array
     */
    friend class WRATHLayerNodeValuePackerBase;
    map_type m_data;
    std::vector<int> m_permutation_array;
  };

  /*!\class ActiveNodeValuesCollection 
    A ActiveNodeValuesCollection is wrapper over a
    std::map keyed by shader stage with values as
    ActiveNodeValues objects
   */
  class ActiveNodeValuesCollection
  {
  public:
    /*!\typedef map_type
      Typedef for the underlying map holding the data.
      The key value is a GL enumeration for a shader
      stage (for example GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
      and the value is a an ActiveNodeValues object
      listing the active node values and GLSL labels.
     */
    typedef std::map<GLenum, ActiveNodeValues> map_type;
    
    /*!\fn ActiveNodeValuesCollection
      Ctor to initialize ActiveNodeValuesCollection.
     */
    ActiveNodeValuesCollection(void):
      m_one_plus_highest_index(0)
    {}
    
    /*!\fn ActiveNodeValuesCollection& add_source(int, const std::string&, GLenum)
      Add a node value source for a shader stage.
      \param idx source index of the node value, i.e. a valid
                 entry for \ref ActiveNodeValue::m_source_index
      \param label GLSL label for the node value, i.e. 
                   an entry for \ref ActiveNodeValue::m_labels
      \param shader_stage GL enumeration for shader stage,
                          for example GL_VERTEX_SHADER or
                          GL_FRAGMENT_SHADER.
     */
    ActiveNodeValuesCollection&
    add_source(int idx, const std::string &label, GLenum shader_stage)
    {
      m_one_plus_highest_index=std::max(m_one_plus_highest_index, idx+1);
      m_entries[shader_stage].add_source(idx, label);
      return *this;
    }

    /*!\fn ActiveNodeValuesCollection& absorb(const ActiveNodeValues&, 
                                              GLenum, 
                                              const ActiveNodeValues::Filter::const_handle&)
      Adds all entries from a ActiveNodeValues object
      to the specifier shader stage.
      \param obj ActiveNodeValues object from which to take entries
      \param shader_stage shader stage (i.e. GL_VERTEX_SHADER, 
                          GL_FRAGMENTS_SHADER, etc) to which to add 
      \param hnd handle to a Filter object, if the handle
                 is not valid then all ActiveNodeValue objects
                 from obj are absorbed.
                          
     */
    ActiveNodeValuesCollection&
    absorb(const ActiveNodeValues &obj, GLenum shader_stage, 
           const ActiveNodeValues::Filter::const_handle &hnd=ActiveNodeValues::Filter::const_handle())
    {
      m_entries[shader_stage].absorb(obj, hnd);
      m_one_plus_highest_index=std::max(m_one_plus_highest_index, obj.one_plus_highest_index());
      return *this;
    }

    /*!\fn ActiveNodeValuesCollection& absorb(const ActiveNodeValuesCollection&, GLenum, 
                                              const ActiveNodeValues::Filter::const_handle&)
      Adds all entries from a ActiveNodeValuesCollection object
      to the specifier shader stage. Essentially checks if
      entries() has the key for the named shader stage and if
      so then calls absorb(const ActiveNodeValues &, GLenum)
      for the map value.
      \param obj ActiveNodeValuesCollection object from which to take entries
      \param shader_stage shader stage (i.e. GL_VERTEX_SHADER, 
                          GL_FRAGMENTS_SHADER, etc) to which to add 
      \param hnd handle to a Filter object, if the handle
                 is not valid then all ActiveNodeValue objects
                 from obj are absorbed.
     */
    ActiveNodeValuesCollection&
    absorb(const ActiveNodeValuesCollection &obj, GLenum shader_stage, 
           const ActiveNodeValues::Filter::const_handle &hnd=ActiveNodeValues::Filter::const_handle());

    /*!\fn const map_type& entries(void) const
      Returns a const-reference to the underlying
      map holding the values.
     */
    const map_type&
    entries(void) const
    {
      return m_entries;
    }

    /*!\fn bool active_entry(GLenum) const
      Returns true if there a per-node value
      for the indicated shader shage.
      \param shader_stage GL enumeration for shader stage to query
     */
    bool
    active_entry(GLenum shader_stage) const;

    /*!\fn int one_plus_highest_index
      Returns the one plus highest node value index
      (i.e. the highest value across all
      ActiveNodeValues objects of
      ActiveNodeValues::one_plus_highest_index()).
     */
    int
    one_plus_highest_index(void) const
    {
      return m_one_plus_highest_index;
    }

  private:
    map_type m_entries;
    int m_one_plus_highest_index;
  };
  
  /*!\class NodeDataPackParameters
    A NodeDataPackParameters object determines how
    data is to be packed into an array to
    be sent to GL. 
   */
  class NodeDataPackParameters
  {
  public:
    /*!\enum data_packing_type
      Enumeration to describe if the data
      extracted from the WRATHLayerItemNodeBase
      derived objects is packed node major
      or index major.
    */
    enum data_packing_type
      {
        /*!
          The data is packed into the array
          so that values within a node are
          continuously stored within the array,
          i.e. if each node has
          N vec4 values, then the values
          are packed as follows:\n\n
          
          \n Node[0].value[0], Node[0].value[1], ...., Node[0].value[N-1]
          \n Node[1].value[0], Node[1].value[1], ...., Node[1].value[N-1]
          \n...
          \n Node[NumberNodes-1].value[0], Node[NumberNodes-1].value[1], ...., Node[NumberNodes-1].value[N-1]
        */
        packed_by_node,
        
        /*!
          The data is packed into the array
          so that values from all nodes are
          continously stored with the array,
          i.e. if each node has
          N vec4 values, then the values
          are packed as follows:\n\n
          
          \n Node[0].value[0], Node[1].value[0], ...., Node[NumberNodes-1].value[0]
          \n Node[0].value[1], Node[1].value[1], ...., Node[NumberNodes-1].value[1]
          \n Node[0].value[2], Node[1].value[2], ...., Node[NumberNodes-1].value[2]
          \n...
          \n Node[0].value[N-1], Node[1].value[N-1], ...., Node[NumberNodes-1].value[N-1]
        */
        packed_by_value
      };

    /*!\fn NodeDataPackParameters(void)
      Ctor. Initializes \ref m_float_alignment as 4
      and \ref m_packing_type as \ref packed_by_node
     */
    NodeDataPackParameters(void):
      m_float_alignment(4),
      m_packing_type(packed_by_node)
    {}

    /*!\fn NodeDataPackParameters(int, enum data_packing_type)
      Ctor. 
      \param pfloat_alignment value to which to initialize m_float_alignment
      \param ppacking_type value to which to initialize m_packing_type
     */
    NodeDataPackParameters(int pfloat_alignment,
                           enum data_packing_type ppacking_type):
      m_float_alignment(pfloat_alignment),
      m_packing_type(ppacking_type)
    {}

    /*!\fn bool operator<(const NodeDataPackParameters &) const
      Comparison operator for NodeDataPackParameters objects.
      \param rhs object to which to compare
     */
    bool
    operator<(const NodeDataPackParameters &rhs) const
    {
      return m_float_alignment<rhs.m_float_alignment
        or (m_float_alignment==rhs.m_float_alignment and m_packing_type<rhs.m_packing_type);
    }

    /*!\var m_float_alignment
      For each "row" of packing per-node values, the data is 
      padded per row so the legnth is a multiple of the float 
      alignment. For example a return value of 1 means no padding, 
      where as a return value of 4 means pad the row length to 4
      (which by doing so allows one to view it as array of vec4's).
    */
    int m_float_alignment;
    
    /*!\var m_packing_type
      Enumeration value to determine if the packing of
      the array is by node or by source value.
     */
    enum data_packing_type m_packing_type;
  };

  /*!\class NodeDataPackParametersCollection
    A NodeDataPackParametersCollection 
    provides an interface to specify
    how create the packing data for
    a specific shader stage. Additionally
    it provides an interface where different
    shader stages can be specified to use
    the same arrays to send to GL.
   */
  class NodeDataPackParametersCollection
  {
  public:
    /*!\class packing_group
      A packing_group embodies the idea
      of a group of shader stages that share
      the exact same array to send to GL.
     */
    class packing_group
    {
    public:
      /*!\fn packing_group
        Ctor. Initialize the packing_group
        to use the default packing_group
        (see \ref NodeDataPackParametersCollection::default_packing_group()).
       */
      packing_group(void):
        m_index(0)
      {}

      /*!\fn bool operator<(packing_group) const
        Comparison operator for sorting.
        \param rhs value to which to compare
       */
      bool
      operator<(packing_group rhs) const
      {
        return m_index < rhs.m_index;
      }

    private:
      friend class NodeDataPackParametersCollection;

      explicit
      packing_group(unsigned int v):
        m_index(v)
      {}

      unsigned int m_index;
    };

    /*!\fn NodeDataPackParametersCollection
      Default ctor.
     */
    NodeDataPackParametersCollection(void):
      m_values(1)
    {}

    /*!\fn packing_group default_packing_group
      Returns a handle to the default packing group.
     */
    packing_group
    default_packing_group(void) const
    {
      return packing_group();
    }

    /*!\fn packing_group add_packing_group
      Create a new packing group, returning a handle to it.
      It is an ERROR to use a packing_group returned
      by one NodeDataPackParametersCollection in a
      different NodeDataPackParametersCollection.
      \param v packing parameters for the GL array
     */
    packing_group
    add_packing_group(const NodeDataPackParameters &v)
    {
      int return_value(m_values.size());
      m_values.push_back(v);
      return packing_group(return_value);
    }

    /*!\fn void set_shader_packer
      Set the packing group for a particular shader stage.
      \param shader_stage which shader stage (for example GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
      \param ID packing_group must be a value returned by add_packing_group() or default_packing_group() 
     */
    void
    set_shader_packer(GLenum shader_stage, packing_group ID)
    {
      WRATHassert(ID.m_index<m_values.size());
      m_map[shader_stage]=ID;
    }
    
    /*!\fn packing_group get_shader_packer
      Returns the packing_group for a shader stage. If the value
      was never set by \ref set_shader_packer(), then returns
      default_packing_group().
     */
    packing_group
    get_shader_packer(GLenum shader_stage) const
    {
      std::map<GLenum, packing_group>::const_iterator iter;
      iter=m_map.find(shader_stage);
      return (iter!=m_map.end())?
        iter->second:
        packing_group();
    }

    /*!\fn const NodeDataPackParameters& packer_set_parameters(packing_group) const
      Returns the packing parameters for a packing group as const reference 
      for reading
      \param ID packing_group must be a value returned by add_packing_group() or default_packing_group() 
     */
    const NodeDataPackParameters&
    packer_set_parameters(packing_group ID) const
    {
      return m_values[ID.m_index];
    }

    /*!\fn NodeDataPackParameters& packer_set_parameters(packing_group) 
      Returns the packing parameters for a packing group as
      a reference for writing
      \param ID packing_group must be a value returned by add_packing_group() or default_packing_group() 
     */
    NodeDataPackParameters&
    packer_set_parameters(packing_group ID) 
    {
      return m_values[ID.m_index];
    }

  private:
    /*
      array of NodeDataPackParameters specifying
      how the data is to be packed.
     */
    std::vector<NodeDataPackParameters> m_values;
    /*
      given a shader stage, get an index into
      m_values. If two shader stages map
      to the same index, then they will use
      the same GL data array.
     */
    std::map<GLenum, packing_group> m_map;
  };

  
  /*!\class SpecDataProcessedPayload
    Classed derived from WRATHLayerNodeValuePackerBase
    may need to know more about how the WRATHMultiGLProgram
    is affected, that data is encapsulated in an object
    derived from WRATHReferenceCountedObject.
  */
  class SpecDataProcessedPayload:
    public WRATHReferenceCountedObjectT<SpecDataProcessedPayload>
  {
  public:
    /*!\fn SpecDataProcessedPayload
      Ctor. Initializes \ref m_number_slots as 256
     */
    explicit
    SpecDataProcessedPayload(void):
      m_number_slots(256)
    {}

    /*!\var m_number_slots
      Gives the maximum number of nodes that
      can be packed into on GL draw call,
      i.e. the number of slots per call
      available. Initial value is 256. Made
      public so that derived classes can
      change it freely.
     */
    int m_number_slots;

    /*!\var m_packer_parameters
      Specifies the packing parameters, default
      is that the shader stages all use a common
      array to send to GL packed with the
      default packing group, \ref 
      NodeDataPackParametersCollection::default_packing_group()
     */
    NodeDataPackParametersCollection m_packer_parameters;
  };

  /*!\class ProcessedActiveNodeValuesCollection
    A ProcessedActiveNodeValuesCollection represents taking
    the data of a \ref NodeDataPackParametersCollection
    and \ref ActiveNodeValuesCollection together to
    produce the data necessary to decide how to
    pack data to GL shaders. One should view this
    as a quasi-internal class that is passed to
    ones WRATHLayerNodeValuePackerBase derived object
    constructor to pass to the constructor of
    WRATHLayerNodeValuePackerBase.

    The class creats an array of (NodeDataPackParameters, ActiveNodeValues)
    object pairs. Each entry of the array represents a different
    data array to send to GL (i.e. \ref DataToGL::data_to_pack_to_GL().
   */
  class ProcessedActiveNodeValuesCollection
  {
  public:
    /*!\fn void set
      Set the ProcessedActiveNodeValuesCollection for
      a given NodeDataPackParametersCollection, 
      ActiveNodeValuesCollection and active shader stages.
      This function clears this ProcessedActiveNodeValuesCollection
      object then sets is data from the parameters.
      \param parameters specifies the group partioning of packing
                        for the shader stages. Elements in the same
                        partition share the same data to send to GL
                        (see \ref DataToGL::data_to_pack_to_GL())
      \param input specifies, on a per shader level, what node values
                   need to be packed.
      \param active_shader_stages specifies what shader stages will
                                  have per-node value sent to them,
                                  the values of the map give a filter
                                  for the stage. A lack of a filter
                                  indicates to take none from that
                                  stage. NOTE the difference from
                                  ActiveNodeValues::absorb() and
                                  ActiveNodeValuesCollection::absorb()
                                
     */
    void
    set(const NodeDataPackParametersCollection &parameters,
        const ActiveNodeValuesCollection &input,
        const std::map<GLenum, ActiveNodeValues::Filter::const_handle> &active_shader_stages);

    
    /*!\fn int number_indices
      Returns the number of (NodeDataPackParametersCollection, ActiveNodeValuesCollection)
      pairs within this ProcessedActiveNodeValuesCollection. When set() is called,
      the number of elements is essentially the number of packing groups
      needed for all the shader stages passed in set(), i.e a packing group
      is considered active if one of the shaders within it are listed
      as an active shader. The number of (NodeDataPackParametersCollection, ActiveNodeValuesCollection)
      pairs is then just the number of active packing groups.
     */
    int
    number_indices(void) const 
    { 
      return m_values.size();
    }

    /*!\fn const NodeDataPackParameters& packer_parameters
      Returns the packing parameters for the named index 
      of the internal array of (NodeDataPackParametersCollection, ActiveNodeValuesCollection)
      pairs.
      \param pindex index of packing group, 0<= pindex < number_indices()
     */
    const NodeDataPackParameters&
    packer_parameters(int pindex) const
    {
      return m_values[pindex].first;
    }

    /*!\fn const ActiveNodeValues& active_node_values
      Returns the active node values for the named index.
      Given an active packing group, let A be the union
      of all active node values for all shader stages
      that are considered active. The return value is 
      that A.
      \param pindex inde of packing group,  0<= pindex < number_indices()
     */
    const ActiveNodeValues&
    active_node_values(int pindex) const
    {
      return m_values[pindex].second;
    }

    /*!\fn const ActiveNodeValuesCollection& original_data
      Returns a copy of the original ActiveNodeValuesCollection
      passed to set()
     */
    const ActiveNodeValuesCollection&
    original_data(void) const
    {
      return m_original_collection;
    }

    /*!\fn const std::map<GLenum, int>& shader_entries
      Returns an std::map, keyed by shader stage to indices
      of the (NodeDataPackParametersCollection, ActiveNodeValuesCollection)
      array. Only those shaders in the set passed in set()
      are in the map. The values of the map are index values
      to be used as arguments to packer_parameters() 
      and active_node_values().
     */
    const std::map<GLenum, int>&
    shader_entries(void) const
    {
      return m_index_for_stage;
    }

  private:
    typedef NodeDataPackParametersCollection::packing_group packing_group;
    typedef std::pair<NodeDataPackParameters, ActiveNodeValues> data_type;

    ActiveNodeValuesCollection m_original_collection;
    std::map<GLenum, int> m_index_for_stage;
    std::vector<data_type> m_values;
  };

  /*!\class Drawer
    A Drawer implements WRATHLayerBase::DrawerBase
    through it's template parameter NodePacker,
    which is to be a derived class of WRATHLayerNodeValuePackerBase.
    \tparam NodePacker must be derived from WRATHLayerNodeValuePackerBase
                       and have a ctor of the form \code  NodePacker(WRATHLayerBase*, const SpecDataProcessedPayload::const_handle &, const ActiveNodeValuesCollection &) \endcode
   */
  template<typename NodePacker>
  class Drawer:public WRATHLayerBase::DrawerBase
  {
  public:
    /*!\fn Drawer
      Ctor. Create a WRATHLayerUnformPackerDrawer using a passed WRATHMultiGLProgram
      that can have the indicated number of items per call using the
      per-node values specified
      \param pr WRATHMultiGLProgram to do the drawing
      \param ppayload handle to payload of data that is to be used to generate
                      the object that packer per-node values to GL
      \param spec specifies the per-node names, etc
    */
    Drawer(WRATHMultiGLProgram *pr,
           const SpecDataProcessedPayload::const_handle &ppayload,
           const ProcessedActiveNodeValuesCollection &spec):
      WRATHLayerBase::DrawerBase(pr),
      m_payload(ppayload),
      m_spec(spec)
    {}
    
    virtual
    GLStateOfNodeCollection*
    allocate_node_packet(WRATHLayerBase *layer) const
    {
      return WRATHNew NodePacker(layer, m_payload, m_spec);
    }
    
    virtual
    unsigned int
    number_slots(void) const
    {
      return m_payload->m_number_slots;
    }
    
  private:
    SpecDataProcessedPayload::const_handle m_payload;
    ProcessedActiveNodeValuesCollection m_spec;
  };
  
  /*!\class function_packet
    A function_packet provides an interface for functions that depend
    on a WRATHLayerNodeValuePackerBase type but not the object itself.
    These functions append to the data that creates a WRATHMultiGLProgram,
    including shader source code, bind actions etc.
   */
  class function_packet:public boost::noncopyable
  {
  public:

    /*!\typedef SpecDataProcessedPayload
      Conveniance typedef
     */
    typedef WRATHLayerNodeValuePackerBase::SpecDataProcessedPayload SpecDataProcessedPayload;

    /*!\typedef ActiveNodeValue
      Conveniance typedef ActiveNodeValue
     */
    typedef WRATHLayerNodeValuePackerBase::ActiveNodeValue ActiveNodeValue;

    /*!\typedef ActiveNodeValues
      Conveniance typedef ActiveNodeValues
     */
    typedef WRATHLayerNodeValuePackerBase::ActiveNodeValues ActiveNodeValues;

    /*!\typedef ActiveNodeValuesCollection
      Conveniance typedef ActiveNodeValuesCollection
     */
    typedef WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection ActiveNodeValuesCollection;

    /*!\typedef NodeDataPackParametersCollection
      Conveniance typedef NodeDataPackParametersCollection
     */        
    typedef WRATHLayerNodeValuePackerBase::NodeDataPackParametersCollection NodeDataPackParametersCollection;

    /*!\typedef ProcessedActiveNodeValuesCollection
      Conveniance typedef ProcessedActiveNodeValuesCollection
     */
    typedef WRATHLayerNodeValuePackerBase::ProcessedActiveNodeValuesCollection ProcessedActiveNodeValuesCollection;

    virtual
    ~function_packet() {}

    
    /*!\fn SpecDataProcessedPayload::handle create_handle(const ActiveNodeValuesCollection &)
      To be implemented by a derived class to create and
      return a SpecDataProcessedPayload object using
      the passed ActiveNodeValuesCollection object to
      help initialize it. The returned handle should set
      both \ref SpecDataProcessedPayload::m_number_slots
      and \ref SpecDataProcessedPayload::m_packer_parameters
      correctly for the node packing. Changes of
      \ref SpecDataProcessedPayload::m_packer_parameters 
      in subsequent calls using the handle are _ignored_.
      Subsequent changes to \ref SpecDataProcessedPayload::m_number_slots
      are NOT ignored.

      Note that a single payload is made for many 
      WRATHLayerNodeValuePackerBase objects. Essentially,
      a single payload is made each unique 
      (WRATHShaderSpecifier, WRATHLayerItemNodeBase::node_function_packet, function_packet)
      triple.
      \param spec ActiveNodeValuesCollection defining active node values
     */
    virtual
    SpecDataProcessedPayload::handle
    create_handle(const ActiveNodeValuesCollection &spec) const=0;

    /*!\fn void append_fetch_code
       To be implemented by a derived class to
       append to a shader source code macros that 
       fetch the uniform values. The macros are:
       \code
       fetch_node_value(X) 
       \endcode
       to "get" a vec4 where X is from \ref ActiveNodeValue::m_labels.
       In addition, to define the GLSL function pre_fetch_node_values()
       which performs any steps required for the fetch_node_value macro
       to work in GLSL.
       \param src WRATHGLShader::shader_source to which to add the code 
       \param shader_stage shader stage of the shader code
       \param node_values specifies the per-node values
       \param hnd handle to payload created by create_handle()
       \param index_name GLSL string that is used to get the index of the node
     */
    virtual
    void
    append_fetch_code(WRATHGLShader::shader_source &src,
                      GLenum shader_stage,
                      const ActiveNodeValues &node_values,
                      const SpecDataProcessedPayload::handle &hnd,
                      const std::string &index_name) const=0;

    /*!\fn void add_actions
      To be implemented by a derived class
      to add on bind actions and program initializers. 
      This is called _AFTER_ the on bind actions and
      program initializers from the WRATHShaderSpecifier
      are added. 
     */
    virtual
    void
    add_actions(const SpecDataProcessedPayload::handle& /*payload*/,
                const ProcessedActiveNodeValuesCollection& /*spec*/,
                WRATHShaderSpecifier::ReservedBindings& /*reserved_bindings*/,
                WRATHGLProgramOnBindActionArray& /*actions*/,
                WRATHGLProgramInitializerArray& /*initers*/) const=0;

    /*!\fn bool supports_per_node_value
      To be implemented by a derived class to indicate
      if the WRATHLayerNodeValuePackerBase supports
      having per node values in the named shader type.
      \param shader_type valid GL enumeration for the named shader type
      (for unextended GLES2 and GLES3, must be one
      of GL_VERTEX_SHADER or GL_FRAGMENT_SHADER).
    */
    virtual
    bool
    supports_per_node_value(GLenum shader_type) const=0;
  };

  /*!\class DataToGL
    DataToGL encapsulates the data to send to GL
    for a fixed shader stage.
   */
  class DataToGL
  {
  public:
    /*!\fn WRATHLayerNodeValuePackerBase* parent
      The WRATHLayerNodeValuePackerBase object that
      holds the actual data. Once that object goes out 
      of scope, then this DataToGL is invalid and all 
      function calls are undefined and likely crashing
      behavior.
     */ 
    WRATHLayerNodeValuePackerBase*
    parent(void) const;

    /*!\fn enum NodeDataPackParameters::data_packing_type packing_type
      Returns the data packing type, which is 
      set at ctor of the WRATHLayerNodeValuePackerBase
    */
    enum NodeDataPackParameters::data_packing_type 
    packing_type(void) const;

    /*!\fn int float_alignment
      Returns the data packing alignment, which is 
      set at ctor of the WRATHLayerNodeValuePackerBase
    */
    int
    float_alignment(void) const;

    /*!\fn const_c_array<float> data_to_pack_to_GL
      Returns the data to pack to GL for the 
      specified shader stage as visible
      from the rendering thread. Must only be
      called from the rendering thread. The
      expectation is that a class derived from
      WRATHLayerNodeValuePackerBase will
      use the return value of data_to_pack_to_GL()
      as source data for a GL call (to set for
      example an array of uniforms, or a texture,
      or a buffer object, etc). The data will be 
      packed as according to \ref packing_type()
      whose return value is set at ctor.
    */
    const_c_array<float>
    data_to_pack_to_GL(void) const;
    
    /*!\fn int number_slots_to_pack_to_GL
      Returns one plus the highest slot ID in use.
    */
    int 
    number_slots_to_pack_to_GL(void) const;
    
    /*!\fn const_c_array<float> data_to_pack_to_GL_restrict
      Provided as a conveniance.
      If the packing_type() is \ref NodeDataPackParameters::packed_by_node
      returns the array of data_to_pack_to_GL() but restricted
      to the node range [0, number_slots_to_pack_to_GL() ).
      However if the packing type is \ref 
      NodeDataPackParameters::packed_by_value
      then just returns the entire array
    */
    const_c_array<float>
    data_to_pack_to_GL_restrict(void) const;

    /*!\fn bool non_empty 
      Returns true if the array of data_to_pack_to_GL()
      is non-empty.
     */
    bool
    non_empty(void) const;

  private:
    explicit
    DataToGL(const void *ptr):
      m_actual_data(ptr)
    {}

    friend class WRATHLayerNodeValuePackerBase;    
    const void *m_actual_data;
  };


  /*!\fn WRATHLayerNodeValuePackerBase
    Ctor.
    \param layer WRATHLayerBase to which the packer is tied
    \param payload used to specify the maximum number of slots and
                   what per-node uniforms are in use (which in turn creates
                   the permutation array)
    \param spec holds how to pack data to send to GL and what shader stages
                use each such array packing.             
   */
  explicit
  WRATHLayerNodeValuePackerBase(WRATHLayerBase *layer,
                                const SpecDataProcessedPayload::const_handle &payload,
                                const ProcessedActiveNodeValuesCollection &spec);

  virtual
  ~WRATHLayerNodeValuePackerBase();

  virtual
  void
  assign_slot(int slot, WRATHLayerItemNodeBase* h, int highest_slot);
   
  /*!\fn const SpecDataProcessedPayload::const_handle& payload
    Returns the payload as specified in the ctor.
   */
  const SpecDataProcessedPayload::const_handle&
  payload(void) const
  {
    return m_payload;
  }
  
  /*!\fn int number_slots_to_pack_to_GL
    Returns one plus the highest slot ID in use.
  */
  int
  number_slots_to_pack_to_GL(void);

  /*!\fn DataToGL data_to_gl
    Returns a DataToGL object (which is just a handle)
    for the named shader stage. If the named shader stage
    does not have any per-node values, returns a DataToGL
    object which indicates to pack no data to GL.
   */
  DataToGL
  data_to_gl(GLenum shader_stage);

  /*!\fn DataToGL data_to_gl_indexed
    Returns a DataToGL object (which is just a handle)
    for an index of the ProcessedActiveNodeValuesCollection
    object passed in the ctor. An invalid index value
    returns a DataToGL which indicates to pack no data to GL.
   */
  DataToGL
  data_to_gl_indexed(unsigned int idx);

protected:

  virtual
  void
  on_place_on_deletion_list(void);

private:

  class per_packer_datum
  {
  public:

    per_packer_datum(WRATHLayerNodeValuePackerBase *pparent,
                     const ActiveNodeValues &used_per_node_values,
                     const NodeDataPackParameters &packing_params,
                     int one_plus_highest_index);

    /*
      no shader stage, etc indicates a per_shader_stage
      packer that does nothing.
     */
    explicit
    per_packer_datum(WRATHLayerNodeValuePackerBase *pparent);



    WRATHLayerNodeValuePackerBase *m_parent;   

    /*
      Almost the same as m_payload->permutation_array()
      except that -1 has been replaced by indices
      starting at m_payload->number_active() to
      m_payload->permutation_array().size().
    */
    std::vector<int> m_permutation_array;
    enum NodeDataPackParameters::data_packing_type m_packing_type;

    int m_float_alignment;
    int m_padded_row_size_in_floats;
    int m_overflow_padding;
    int m_number_active;

    /*
      m_data_to_pack_to_GL_padded is padded with 
      extra floats to allow for the extra room
      needed for when WRATHLayerItemNodeBase
      dervied classes pack values that are NOT
      is use.
    */
    vecN<std::vector<float>, 3> m_data_to_pack_to_GL_padded;
    vecN<const_c_array<float>, 3> m_data_to_pack_to_GL;

    /*
      only needed if packing is packed_by_value
    */
    std::vector<float> m_pack_work_room;

    void
    pack_data(int number_slots);
    
  };
 
  void
  pack_data(void);


  SpecDataProcessedPayload::const_handle m_payload;
  int m_highest_slot;
  vecN<int, 3> m_number_slots_to_pack_to_GL;

  WRATHMutex m_nodes_mutex;
  std::vector<WRATHLayerItemNodeBase*> m_nodes;
  WRATHTripleBufferEnabler::connect_t m_sim_signal;

  std::vector<per_packer_datum*> m_packers;
  per_packer_datum m_empty_packer;
  std::map<GLenum, int> m_packers_by_shader;
};



/*! @} */


#endif
