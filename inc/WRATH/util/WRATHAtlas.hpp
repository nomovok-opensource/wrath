/*! 
 * \file WRATHAtlas.hpp
 * \brief file WRATHAtlas.hpp
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




#ifndef WRATH_HEADER_ATLAS_HPP_
#define WRATH_HEADER_ATLAS_HPP_

#include "WRATHConfig.hpp"
#include <map>
#include "WRATHAtlasBase.hpp"

/*! \addtogroup Utility
 * @{
 */


/*!\class WRATHAtlas
  A WRATHAtlas represents an allocation
  of rectangles within a rectangle, the purpose
  is to use such an allocation for packing
  multiple images into one texture.
  A WRATHAtlas enforces no restrictions
  on rectangles' sizes or positions. Internally,
  a WRATHAtlas successively partions
  the rectangular region as rectangles are
  allocated.
 */
class WRATHAtlas:public WRATHAtlasBase
{
public:

  /*!\fn WRATHAtlas
    Constructs a WRATHAtlas, passing the dimensions and
    a pixel store. The created WRATHAtlas OWNS the
    passed WRATHPixelStore and will delete it.
    \param dimensions dimension of the texture atlas, this is then the return value to size().
    \param ppixelstore pixel store associated to the WRATHAtlas
   */
  explicit
  WRATHAtlas(const ivec2 &dimensions, WRATHPixelStore *ppixelstore);

  virtual
  ~WRATHAtlas();

  virtual
  const rectangle_handle*
  add_rectangle(const ivec2 &dimension);
  
  virtual
  void
  clear(void);

  /*!\fn ivec2 size
    Returns the size of the \ref WRATHAtlas,
    i.e. the value passed as dimensions
    in WRATHAtlas().
   */ 
  ivec2
  size(void) const;

protected:

  virtual
  enum return_code
  remove_rectangle_implement(const rectangle_handle *im);

private:
  /*
    Tree structure to construct the texture atlas,
    basic idea is very simple: walk the tree until one finds
    a node where the image can fit. 

    if .second is routine_fail, then the routine
    failed.
   
    If .first of the return value of add or remove
    is not the same as the object, then the return value 
    represents a new child and the old object should be deleted.

    if .first of the return value of add or remove
    is the same as the object, then the routine succeeded
    and the object should not be deleted.
   */
  class tree_base;
  class tree_node_without_children;
  class freesize_tracker;
  typedef std::pair<tree_base*, enum return_code> add_remove_return_value;
  typedef std::multimap<int, tree_node_without_children*> freesize_map;

  class local_rectangle:public rectangle_handle
  {
  public:
    local_rectangle(const handle &p, const ivec2 &psize):
      rectangle_handle(p, psize),
      m_tree(NULL)
    {}

    tree_base *m_tree;

    void
    build_parent_list(std::list<const tree_base*> &output) const;
  };

  class tree_sorter
  {
  public:
    bool
    operator()(tree_base *lhs, tree_base *rhs) const;

  };

  
  class tree_base
  {
  public:

    tree_base(const ivec2 &bl, const ivec2 &sz,
              const tree_base *pparent,
              freesize_tracker *tr):
      m_minX_minY(bl), m_size(sz),
      m_parent(pparent),
      m_tr(tr)
    {}

    virtual
    ~tree_base(void)
    {}
    
    const ivec2&
    size(void) const
    {
      return m_size;
    }

    int
    area(void) const
    {
      return m_size.x()*m_size.y();
    }
    
    const ivec2&
    minX_minY(void) const
    {
      return m_minX_minY;
    }

    const tree_base*
    parent(void) const
    {
      return m_parent;
    }

    virtual
    add_remove_return_value
    add(local_rectangle*)=0;

    /*
      Idea:

      parent_list.front()==this, otherwise
      abort.

      For recursive calls, pop_front() parent
      list to next element..
     */
    virtual
    add_remove_return_value
    remove(const local_rectangle*, 
           std::list<const tree_base*> &parent_list)=0;

    add_remove_return_value
    api_remove(const rectangle_handle *im);

    virtual
    bool
    empty(void)=0;

    freesize_tracker*
    tracker(void)
    {
      return m_tr;
    }


  private:
    ivec2 m_minX_minY, m_size;
    const tree_base *m_parent;
    freesize_tracker *m_tr;
  };

  //a tree_node_without_children represents
  //a Node which has NO child Node's
  //but may or maynot have a rectangle.
  class tree_node_without_children:public tree_base
  {
  public:
    tree_node_without_children(const tree_base *pparent, 
                               freesize_tracker *tr,
                               const ivec2 &bl, const ivec2 &sz, 
                               local_rectangle *rect=NULL);
    ~tree_node_without_children();
    
    virtual
    add_remove_return_value
    add(local_rectangle*);

    virtual
    add_remove_return_value
    remove(const local_rectangle*,
           std::list<const tree_base*>&);

    virtual
    bool
    empty(void);

    local_rectangle*
    data(void);

  private:
    void
    update_tracking(void);
    
    void
    update_tracking_helper(int x, int y);

    void
    clear_from_tracking(void);

    local_rectangle *m_rectangle;
    std::vector<freesize_map::iterator> m_sorted_by_x_iters;
    std::vector<freesize_map::iterator> m_sorted_by_y_iters;
  };

  //a tree node with children has _3_ children.
  //they are spawned when a tree_node_wihout_children
  //has a rectangle added but it already has a rectangle.
  class tree_node_with_children:public tree_base
  {
  public:
    tree_node_with_children(tree_node_without_children *src, 
                            bool split_x, bool split_y);
    ~tree_node_with_children();
    
    virtual
    add_remove_return_value
    add(local_rectangle*);

    virtual
    add_remove_return_value
    remove(const local_rectangle*,
           std::list<const tree_base*>&);

    virtual
    bool
    empty(void);

  private:
    vecN<tree_base*,3> m_children;
  };

  class freesize_tracker
  {
  public:

    bool
    fast_check(ivec2 psize);

    freesize_map m_sorted_by_x_size;
    freesize_map m_sorted_by_y_size;
  };



  freesize_tracker m_tracker;
  WRATHMutex m_mutex;
  tree_base *m_root;
};
/*! @} */

#endif
