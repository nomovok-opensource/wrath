/*! 
 * \file WRATHAtlas.cpp
 * \brief file WRATHAtlas.cpp
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
#include <algorithm>
#include "WRATHAtlas.hpp"


  



////////////////////////////////////////
// WRATHAtlas::tree_sorter methods
bool
WRATHAtlas::tree_sorter::
operator()(tree_base *lhs, tree_base *rhs) const
{
  /*
    we want to list the smallest "size" first
    to avoid splitting large elements
   */
  return lhs->area()<rhs->area();
}

///////////////////////////////////////////
// WRATHAtlas::local_rectangle methods
void
WRATHAtlas::local_rectangle::
build_parent_list(std::list<const tree_base*> &output) const
{
  const tree_base *p(m_tree);

  while(p!=NULL)
    {
      output.push_front(p);
      p=p->parent();
    }
}

//////////////////////////////////////
// WRATHAtlas::tree_base methods
WRATHAtlas::add_remove_return_value
WRATHAtlas::tree_base::
api_remove(const rectangle_handle *im)
{
  std::list<const tree_base*> parentage;
  const local_rectangle *p;

  p=dynamic_cast<const local_rectangle*>(im);
  WRATHassert(p!=NULL);
  
  p->build_parent_list(parentage);
  WRATHassert(!parentage.empty());
  WRATHassert(this==parentage.front());
  return remove(p, parentage);
}

//////////////////////////////////////
// WRATHAtlas::tree_node_without_children methods
WRATHAtlas::tree_node_without_children::
tree_node_without_children(const tree_base *pparent, 
                           freesize_tracker *tr,
                           const ivec2 &bl, const ivec2 &sz, 
                           local_rectangle *rect):
  tree_base(bl, sz, pparent, tr),
  m_rectangle(rect)
{
  if(m_rectangle!=NULL)
    {
      m_rectangle->m_tree=this;
    }
  update_tracking();
}


WRATHAtlas::tree_node_without_children::
~tree_node_without_children()
{
  if(m_rectangle!=NULL)
    {
      WRATHassert(m_rectangle->m_tree==this);
      WRATHDelete(m_rectangle);
    }
  clear_from_tracking();
}

WRATHAtlas::local_rectangle*
WRATHAtlas::tree_node_without_children::
data(void)
{
  return m_rectangle;
}

void
WRATHAtlas::tree_node_without_children::
clear_from_tracking(void)
{
  for(std::vector<freesize_map::iterator>::iterator
        iter=m_sorted_by_x_iters.begin(), end=m_sorted_by_x_iters.end();
      iter!=end; ++iter)
    {
      tracker()->m_sorted_by_x_size.erase(*iter);
    }

  for(std::vector<freesize_map::iterator>::iterator
        iter=m_sorted_by_y_iters.begin(), end=m_sorted_by_y_iters.end();
      iter!=end; ++iter)
    {
      tracker()->m_sorted_by_y_size.erase(*iter);
    }

  m_sorted_by_x_iters.clear();
  m_sorted_by_y_iters.clear();
}

void
WRATHAtlas::tree_node_without_children::
update_tracking_helper(int x, int y)
{
  freesize_map::iterator iter_x, iter_y;

  iter_x=tracker()->m_sorted_by_x_size.insert(freesize_map::value_type(x, this)); 
  m_sorted_by_x_iters.push_back(iter_x);
  
  iter_y=tracker()->m_sorted_by_y_size.insert(freesize_map::value_type(y, this)); 
  m_sorted_by_y_iters.push_back(iter_y);
}


void
WRATHAtlas::tree_node_without_children::
update_tracking(void)
{
  clear_from_tracking();
  if(m_rectangle==NULL)
    {
      update_tracking_helper(size().x(), size().y());
    }
  else
    {
      /*
        we have two possible ways to split,
        thus 2 entries:
       */
      update_tracking_helper(size().x(),
                             size().y()-m_rectangle->size().y());

      update_tracking_helper(size().x()-m_rectangle->size().x(),
                             size().y());
    }
}


WRATHAtlas::add_remove_return_value 
WRATHAtlas::tree_node_without_children::
add(local_rectangle *im)
{
  if(im->size().x()>size().x() or im->size().y()>size().y())
    {
      return add_remove_return_value(this, routine_fail);
    }

  if(m_rectangle==NULL)
    {
      //do not have a rect so we take it (and move it).
      m_rectangle=im;
      m_rectangle->m_tree=this;

      move_rectangle(m_rectangle, minX_minY());
      update_tracking();

      return add_remove_return_value(this, routine_success);
    }


  //we have a rectangle already, we need to check 
  //if we can split in such a way to take im:
  int dx, dy;
  bool split_y_works, split_x_works;

  dx=size().x() - m_rectangle->size().x();
  dy=size().y() - m_rectangle->size().y();

  split_y_works=(dy>=im->size().y());
  split_x_works=(dx>=im->size().x());

  if(!split_x_works and !split_y_works)
    {
      return add_remove_return_value(this, routine_fail);
    }
 
  if(split_x_works and split_y_works)
    {
      //choose a split that is nicest
      //by making the other split false.

      //whoever has the most room left over is the split.
      if(dx>dy)
        {
          split_y_works=false;
        }
      else
        {
          split_x_works=false;
        }
    }

  
  tree_base *new_node;
  add_remove_return_value R;
  
  //new_node will hold this->m_rectange:
  new_node=WRATHNew tree_node_with_children(this, split_x_works, split_y_works);
  //set m_rectangle to NULL since new_node "owns" it now,
  //the caller will delete this.
  m_rectangle=NULL;

  //add the new rectangle im to new_node:
  R=new_node->add(im);  
  WRATHassert(R.second==routine_success);

  if(R.first!=new_node)
    {
      WRATHDelete(new_node);
      new_node=R.first;
    }

  return add_remove_return_value(new_node, routine_success);
}

WRATHAtlas::add_remove_return_value 
WRATHAtlas::tree_node_without_children::
remove(const WRATHAtlas::local_rectangle *im,
       std::list<const tree_base*> &parent_list)
{
  WRATHassert(!parent_list.empty());
  if(parent_list.front()!=this)
    {
      return add_remove_return_value(this, routine_fail);
    }
  

  WRATHassert(m_rectangle==im);
  WRATHassert(im->m_tree==this);
  WRATHunused(im);
  WRATHDelete(m_rectangle);
  m_rectangle=NULL;
  update_tracking();

  return add_remove_return_value(this, routine_success);

}

bool
WRATHAtlas::tree_node_without_children::
empty(void)
{
  return m_rectangle==NULL;
}

////////////////////////////////////
// WRATHAtlas::tree_node_with_children methods
WRATHAtlas::tree_node_with_children::
tree_node_with_children(WRATHAtlas::tree_node_without_children *src, 
                        bool split_x, bool split_y):
  tree_base(src->minX_minY(), src->size(), src->parent(), src->tracker()),
  m_children(NULL, NULL, NULL)
{
  local_rectangle *R(src->data());
  WRATHassert(R!=NULL);

  m_children[2]=WRATHNew tree_node_without_children(this, src->tracker(),
                                                    R->minX_minY(), 
                                                    R->size(), R);  
  if(split_x)
    {
      m_children[0]
        =WRATHNew tree_node_without_children( this, src->tracker(),
                                              ivec2(minX_minY().x(),
                                                    minX_minY().y() + R->size().y()),
                                              ivec2(R->size().x(),
                                                    size().y() - R->size().y()) );

      m_children[1]
        =WRATHNew tree_node_without_children( this, src->tracker(),
                                              ivec2(minX_minY().x() + R->size().x(),
                                                    minX_minY().y()),
                                              ivec2(size().x() - R->size().x(),
                                                    size().y()) );
    }
  else
    {
      WRATHassert(split_y);
      WRATHunused(split_y);

      m_children[0]
        =WRATHNew tree_node_without_children( this, src->tracker(),
                                              ivec2(minX_minY().x() + R->size().x(),
                                                    minX_minY().y()),
                                              ivec2(size().x() - R->size().x(),
                                                    R->size().y()) );

      m_children[1]
        =WRATHNew tree_node_without_children( this, src->tracker(),
                                              ivec2(minX_minY().x(),
                                                    minX_minY().y() + R->size().y()),
                                              ivec2(size().x(),
                                                    size().y() - R->size().y()) );
    }

  std::sort(m_children.begin(), m_children.end(), tree_sorter());
}

WRATHAtlas::tree_node_with_children::
~tree_node_with_children()
{
  for(int i=0;i<3;++i)
    {
      WRATHassert(m_children[i]!=NULL);
      WRATHDelete(m_children[i]);
    }
}


WRATHAtlas::add_remove_return_value 
WRATHAtlas::tree_node_with_children::   
add(local_rectangle *im)
{
  add_remove_return_value R;

  for(int i=0;i<3;++i)
    {
      R=m_children[i]->add(im);
      if(R.second==routine_success)
        {
          if(R.first!=m_children[i])
            {
              WRATHDelete(m_children[i]);
              m_children[i]=R.first;
            }
          return add_remove_return_value(this, routine_success);
        }
    }

  return add_remove_return_value(this, routine_fail);
}

WRATHAtlas::add_remove_return_value 
WRATHAtlas::tree_node_with_children::   
remove(const local_rectangle *im,
       std::list<const tree_base*> &parent_list)
{
  WRATHassert(!parent_list.empty());  
  if(parent_list.front()!=this)
    {
      return add_remove_return_value(this, routine_fail);
    }

  parent_list.pop_front();
  tree_base *null_tree_base(NULL);
  add_remove_return_value R(null_tree_base, routine_fail);
  int delete_index(3);

  for(int i=0; i<3 and R.second==routine_fail; ++i)
    {
      WRATHassert(m_children[i]!=NULL);

      R=m_children[i]->remove(im, parent_list);      
      if(R.second==routine_success)
        {
          delete_index=i;
        }
    }

  WRATHassert(R.second==routine_success);
  WRATHassert(delete_index<3);
  if(R.first!=m_children[delete_index])
    {
      WRATHDelete(m_children[delete_index]);
      m_children[delete_index]=R.first;
    }

  //now check the situation, if all children are
  //empty and thus we can reform to be a 
  //tree_node_without_children
  if(empty())
    {
      tree_node_without_children *ptr;
      
      ptr=WRATHNew tree_node_without_children(parent(), tracker(),
                                              minX_minY(), size());
      return add_remove_return_value(ptr, routine_success);
    }
  else
    {
      return add_remove_return_value(this, routine_success);
    }

}

bool
WRATHAtlas::tree_node_with_children::   
empty(void)
{
  return m_children[0]->empty()
    and m_children[1]->empty()
    and m_children[2]->empty();
}

//////////////////////////////////////
// WRATHAtlas::freesize_tracker methods
bool
WRATHAtlas::freesize_tracker::
fast_check(ivec2 psize)
{

  return !m_sorted_by_x_size.empty()
    and m_sorted_by_x_size.rbegin()->first>=psize.x()
    and !m_sorted_by_y_size.empty()
    and m_sorted_by_y_size.rbegin()->first>=psize.y();

  /*
    the fast check can cull quickly, but
    it does not guarnantee that an element
    can be fit, to that would require
    checking if the sets
    [iterx, m_sorted_by_x_size.end())
    and
    [itery, m_sorted_by_y_size.end())

    where iterx=m_sorted_by_x_size.lower_bound(psize.x())
    and itery=m_sorted_by_y_size.lower_bound(psize.y())

    intersect as sets of pointers sorted by pointer
    (not value). Such a check would induce
    one to sort, which is O(nlogn),
    which is likely much more than just walking
    the tree structure.

    The fast test is O(1).
   */
}
     

//////////////////////////////////////
// WRATHAtlas methods
WRATHAtlas::
WRATHAtlas(const ivec2 &dimensions, WRATHPixelStore *ppixelstore):
  WRATHAtlasBase(ppixelstore),
  m_root(NULL)
{
  m_root=WRATHNew tree_node_without_children(NULL, &m_tracker,
                                             ivec2(0,0), 
                                             dimensions, NULL);   
}


WRATHAtlas::
~WRATHAtlas()
{
  WRATHassert(m_root->empty());
  WRATHDelete(m_root);
}

ivec2
WRATHAtlas::
size(void) const
{
  return m_root->size();
}

void
WRATHAtlas::
clear(void)
{
  ivec2 dimensions(m_root->size());

  WRATHLockMutex(m_mutex);
  WRATHDelete(m_root);
  m_root=WRATHNew tree_node_without_children(NULL, &m_tracker,
                                             ivec2(0,0), dimensions, NULL);   
  WRATHUnlockMutex(m_mutex);
}

const WRATHAtlas::rectangle_handle*
WRATHAtlas::
add_rectangle(const ivec2 &dimensions)
{
  local_rectangle *return_value(NULL);
  

  WRATHLockMutex(m_mutex);
  if(m_tracker.fast_check(dimensions))
    {
      add_remove_return_value R;

      return_value=WRATHNew local_rectangle(this, dimensions);

      if(dimensions.x()>0 and dimensions.y()>0)
        {
          //attempt to add the rect:
          R=m_root->add(return_value);
      
          if(R.second==routine_success)
            {
              if(R.first!=m_root)
                {
                  WRATHDelete(m_root);
                  m_root=R.first;
                }
            }
          else
            {
              WRATHDelete(return_value);
              return_value=NULL;
            }  
        }
    }
  WRATHUnlockMutex(m_mutex);

  return return_value;
}


enum return_code
WRATHAtlas::
remove_rectangle_implement(const rectangle_handle *im)
{
  add_remove_return_value R;

  WRATHassert(im->atlas()==this);

  if(im->size().x()<=0 or im->size().y()<=0)
    {
      WRATHDelete(const_cast<rectangle_handle*>(im));
      return routine_success;
    }
  else
    {
      
      WRATHLockMutex(m_mutex);  
      R=m_root->api_remove(im);
      
      if(R.second==routine_success and R.first!=m_root)
        {
          WRATHDelete(m_root);
          m_root=R.first;
        }
  
      WRATHUnlockMutex(m_mutex);
      return R.second;
    }
}
