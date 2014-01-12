/*! 
 * \file WRATHTextureFontUtil.cpp
 * \brief file WRATHTextureFontUtil.cpp
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
#include <boost/bind.hpp>
#include "WRATHMutex.hpp"
#include "WRATHTextureFontUtil.hpp"
#include "WRATHUtil.hpp"
#include "WRATHTriangulation.hpp"
#include "ostream_utility.hpp"

namespace
{
  ivec2
  compute_lowres_dimensions(const ivec2 &full_resolution, 
                            int min_quad_size)
  {
    ivec2 R;

    R=full_resolution/min_quad_size;

    if(R.x()*min_quad_size<full_resolution.x())
      {
        ++R.x();
      }

    if(R.y()*min_quad_size<full_resolution.y())
      {
        ++R.y();
      }
    return R;
  }

  class local_edge_type
  {
  public:
    int m_fixed;
    range_type<int> m_varying;
    int m_varying_index;

    ivec2 m_pt_start, m_pt_end;
    uint16_t m_start_index, m_end_index;

    local_edge_type(int fixed, int begin, int end, bool x_is_varying):
      m_fixed(fixed),
      m_varying( std::min(begin, end), std::max(begin,end) ),
      m_varying_index(x_is_varying?0:1),
      m_start_index(0),
      m_end_index(0)
    {
      m_pt_start[m_varying_index]=m_varying.m_begin;
      m_pt_end[m_varying_index]=m_varying.m_end;

      m_pt_start[1-m_varying_index]=m_pt_end[1-m_varying_index]=m_fixed;
    }

    uint16_t
    get_index(std::map<ivec2, uint16_t> &vert_map_index, 
              std::vector<ivec2> &verts, int varying)
    {
      std::map<ivec2, uint16_t>::iterator iter;
      ivec2 pt;
      uint16_t R;

      pt[m_varying_index]=varying;
      pt[1-m_varying_index]=m_fixed;
      

      iter=vert_map_index.find(pt);
      if(iter==vert_map_index.end())
        {
          R=verts.size();
          vert_map_index[pt]=R;
          verts.push_back(pt);
        }
      else
        {
          R=iter->second;
        }
      return R;
    }

    void
    set_indices(std::map<ivec2, uint16_t> &vert_map_index, 
                std::vector<ivec2> &verts)
    {
      m_start_index=get_index(vert_map_index, verts, m_varying.m_begin);
      m_end_index=get_index(vert_map_index, verts, m_varying.m_end);
    }

    bool
    operator<(const local_edge_type &rhs) const
    {
      if(m_fixed!=rhs.m_fixed)
        {
          return m_fixed<rhs.m_fixed;
        }
      else
        {
          return m_varying.m_begin<rhs.m_varying.m_begin;
        }
    }

                               
  };

  std::ostream&
  operator<<(std::ostream &str, const local_edge_type &E)
  {
    str << "\n\t[" << E.m_pt_start << ", " << E.m_pt_end << "](f=" << E.m_varying_index << ")";
    return str;
  }


  void
  collapse_edge_list(std::list<local_edge_type> &edge_list,
                     const boost::multi_array<int, 2> &edges_through_vertex)
  {
    //sort
    edge_list.sort();
    
    
    //collapse:
    /*
      however we cannot collapse edges which go through a pixel
      which is a cross
     */
    for(std::list<local_edge_type>::iterator iter=edge_list.begin();
        iter!=edge_list.end(); ++iter)
      {
        local_edge_type &E(*iter);
        std::list<local_edge_type>::iterator start_look(iter);
   
        ++start_look;
        std::list<local_edge_type>::iterator look_ahead(start_look);

        WRATHassert(edges_through_vertex[E.m_pt_end.x()][E.m_pt_end.y()]==2
               or edges_through_vertex[E.m_pt_end.x()][E.m_pt_end.y()]==4);

        WRATHassert(edges_through_vertex[E.m_pt_start.x()][E.m_pt_start.y()]==2
               or edges_through_vertex[E.m_pt_start.x()][E.m_pt_start.y()]==4);

        
        for(;look_ahead!=edge_list.end() and look_ahead->m_fixed==E.m_fixed 
              and look_ahead->m_varying.m_begin==E.m_varying.m_end
              and edges_through_vertex[E.m_pt_end.x()][E.m_pt_end.y()]==2;
            ++look_ahead)
          {
            E.m_varying.m_end=look_ahead->m_varying.m_end;
            E.m_pt_end=look_ahead->m_pt_end;
          }
        edge_list.erase(start_look, look_ahead);
        
      }

         
  }

  void
  set_indices(std::list<local_edge_type> &plist, 
              std::map<ivec2, uint16_t> &pmap, 
              std::vector<ivec2> &verts)
  {
    for(std::list<local_edge_type>::iterator iter=plist.begin(),
          end=plist.end(); iter!=end; ++iter)
      {
        iter->set_indices(pmap, verts);
      }
  }

  /*
    removes the edge e from map_iter->second,
    if after removal map-iter->second is empty, also erases map_iter.
    returns true if and only if it removed map_iter.
   */
  bool
  remove_edge(const local_edge_type *e, 
              std::map<uint16_t, std::set<const local_edge_type*> >::iterator map_iter, 
              std::map<uint16_t, std::set<const local_edge_type*> > &point_map_edge)
  {
    WRATHassert(map_iter->second.find(e)!=map_iter->second.end());
    map_iter->second.erase(e);
    if(map_iter->second.empty())
      {
        point_map_edge.erase(map_iter);
        return true;
      }
    return false;
  }

  
  /*
    returns 1 if both tile_covered[pt.x()][pt.y()] and tile_covered[pt.x()-1][pt.y()-1]
    are covered, 

    returns 0 if both tile_covered[pt.x()-1][pt.y()] and tile_covered[pt.x()][pt.y()-1]
    are covered, 

    otherwise returns 2.
   */
  int
  classify_tile_type(const boost::multi_array<bool, 2> &tile_covered,
                     ivec2 pt)
  {
    WRATHassert(pt.x()>0 and pt.y()>0);

    if(tile_covered[pt.x()][pt.y()] and tile_covered[pt.x()-1][pt.y()-1])
      {
        return 1;
      }
    else if(tile_covered[pt.x()-1][pt.y()] and tile_covered[pt.x()][pt.y()-1])
      {
        return 0;
      }
    else
      {
        return 2;
      }
  }

  /*
    Returns 0 if moving along the edge's varying coordinate
    from pt increases in value.
   */
  int
  classify_edge(const local_edge_type *e, 
                uint16_t coming_from_pt, 
                const std::vector<ivec2> &verts)
  {
    uint16_t going_to_pt;

    WRATHassert(e->m_start_index==coming_from_pt or e->m_end_index==coming_from_pt);

    going_to_pt=(e->m_start_index==coming_from_pt)?
      e->m_end_index:e->m_start_index;

    
    ivec2 comingFrom(verts[coming_from_pt]);
    ivec2 goingTo(verts[going_to_pt]);
    int delta;

    WRATHassert(comingFrom.x()==goingTo.x() or comingFrom.y()==goingTo.y());
    WRATHassert(comingFrom.x()!=goingTo.x() or comingFrom.y()!=goingTo.y());

    delta=goingTo.x()-comingFrom.x() + goingTo.y()-comingFrom.y();

    return (delta>0)?
      1:0;
  }

  /*
    given an edge "coming" into map_iter->first, get
    at end from map_iter->second that continues the
    outline.
   */
  const local_edge_type*
  choose_edge(std::map<uint16_t, std::set<const local_edge_type*> >::iterator map_iter,
              const local_edge_type *coming_edge, 
              const std::vector<ivec2> &verts, 
              const boost::multi_array<bool, 2> &tile_covered)
  {
    int set_size(map_iter->second.size());

    WRATHassert(set_size==1 or set_size==3);
    WRATHassert(coming_edge->m_start_index==map_iter->first
           or coming_edge->m_end_index==map_iter->first);

    if(set_size==1)
      {
        return *map_iter->second.begin();
      }
    else
      {
                
        /*
          get the two edges that are perpindicular
          and classify their types from the point,
          i.e. if they increase or decrease coordinate.
         */
        vecN<const local_edge_type*, 2> canidates;
        vecN<int, 2> candidate_types;
        int counter(0);

        for(std::set<const local_edge_type*>::iterator iter=map_iter->second.begin(),
              end=map_iter->second.end(); iter!=end; ++iter)
          {
            if( (*iter)->m_varying_index!=coming_edge->m_varying_index)
              {
                WRATHassert(counter<2);
                canidates[counter]=*iter;
                candidate_types[counter]=classify_edge(canidates[counter],
                                                       map_iter->first, verts);
                ++counter;
              }
          }

        WRATHassert(counter==2);
        WRATHassert(candidate_types[0]!=candidate_types[1]);

        /*
          Force so that candidate_types[0] is 0:
         */
        if(candidate_types[0]!=0)
          {
            std::swap(canidates[0], canidates[1]);
            candidate_types=ivec2(0,1);
          }

        /*
          The point shoud be so that exactly 2 tiles
          that touch the vertex are filled AND the tiles
          do not share an edge:
         */
        int tile_type, coming_edge_type;

        coming_edge_type=classify_edge(coming_edge, map_iter->first, verts);
        tile_type=classify_tile_type(tile_covered, verts[map_iter->first]);
        WRATHassert(tile_type!=2);



        /*
          Now for the quasi-trickiness:

          If the tile_type is 0, then the tile to
          the lower right and upper left are covered
          and thus we want an edge of the same 
          classification as returned by classify_edge().

          If the tile_type is 1, then the tile to the 
          upper right and bottom left are covered, 
          and thus we want an edge of the opposite
          classification as returned by classify_edge().

          The gluck of trickiness is that xor will help:

          int( bool(tile_type) xor bool(coming_edge_type))
          gives us the index into candidates to take
         */
        int canidate_index;

        canidate_index=int( bool(tile_type) xor bool(coming_edge_type) );
        return canidates[canidate_index];
      }
    
  }

  void
  create_outline(std::list< std::vector<uint16_t> > &outlines, 
                 std::map<uint16_t, std::set<const local_edge_type*> >::iterator map_iter, 
                 std::map<uint16_t, std::set<const local_edge_type*> > &point_map_edge,
                 const std::vector<ivec2> &verts, 
                 const boost::multi_array<bool, 2> &tile_covered)
  {
    
    WRATHassert(map_iter->second.size()==2 or map_iter->second.size()==4);

    uint16_t previous_pt(map_iter->first), next_pt, starting_pt(map_iter->first);
    const local_edge_type *current_edge(*map_iter->second.begin());

    outlines.push_back( std::vector<uint16_t>() );

    do
      {
        bool erased_map_iter;

        next_pt=(current_edge->m_start_index==previous_pt)?
          current_edge->m_end_index:
          current_edge->m_start_index;
        
        WRATHassert(next_pt!=previous_pt);

        //add the point from which the edge is coming
        outlines.back().push_back(previous_pt);

        //remove the edge from the point from which the edge is coming,
        //if the set of map_iter is empty, also erase map_iter
        erased_map_iter=remove_edge(current_edge, map_iter, point_map_edge);

        //get the point to which the edge is going
        map_iter=point_map_edge.find(next_pt);

        //remove the edge from the point to which the edge is going,
        //if the set of map_iter is empty, also erase map_iter
        erased_map_iter=remove_edge(current_edge, map_iter, point_map_edge);

        //choose the next edge to continue the loop.
        //if we returned to the starting point, then the
        //outline is done and we terminate the loop.
        if(next_pt==starting_pt)
          {
            WRATHassert(erased_map_iter);
            current_edge=NULL;
          }
        else
          {
            WRATHassert(!erased_map_iter);
            current_edge=choose_edge(map_iter, current_edge, verts, tile_covered);
          }
        WRATHunused(erased_map_iter);
        previous_pt=next_pt;
      }
    while(current_edge!=NULL);
  }

  
  void
  create_outlines(std::list< std::vector<uint16_t> > &outlines, 
                  const std::list<local_edge_type> &all_edges,
                  const std::vector<ivec2> &verts,
                  const boost::multi_array<bool, 2> &tile_covered)
  {
    /*
      realize the edges into connected components, i.e.
      identify the outlines from the list of edges.

      Basic idea: create a map keyed by the endpoints
      of the edges with values as lists of edges
      that have that endpoint. When building the outline
      we will need to also check the texel fill
      values for the cases of corners of quads meeting.

      The building of outlines will take place
      on the list of endpoint of the map. 
      We iterate through the entries of the map 
      as follows: choose an edge of the value
      and remove it. Then find the entry of it's 
      "other end point" in the map, remove it's 
      entry there from that entry. If the map
      has more than one element remaining choose
      the correct edge as according to tile_covered
      to continue walking the outline and remove
      the edge. If the point is the same as the starting
      point we have walked the outline. Repeat if
      the point has edges still left, otherwise,
      remove it and increment the iterator.
     
     */

    /*
      build map
     */
    std::map<uint16_t, std::set<const local_edge_type*> > point_map_edge;

    for(std::list<local_edge_type>::const_iterator iter=all_edges.begin(),
          end=all_edges.end(); iter!=end; ++iter)
      {
        const local_edge_type &E(*iter);
        point_map_edge[ E.m_start_index ].insert(&E);
        point_map_edge[ E.m_end_index ].insert(&E);        
      }


    /*
      now walk the outlines:
     */
    while(!point_map_edge.empty())
      {
        
        create_outline(outlines, point_map_edge.begin(),
                       point_map_edge,
                       verts, tile_covered); 
        
      }
  }

 
}

//////////////////////////////////////////////
// WRATHTextureFontUtil::TexturePageTracker methods
WRATHTextureFontUtil::TexturePageTracker::
~TexturePageTracker()
{
  for(std::vector<page_type*>::iterator iter=m_pages.begin(),
        end=m_pages.end(); iter!=end; ++iter)
    {
      WRATHDelete(*iter);
    }
}


int
WRATHTextureFontUtil::TexturePageTracker::
number_texture_pages(void) const
{
  WRATHAutoLockMutex(m_mutex);
  return m_pages.size();
}

const ivec2&
WRATHTextureFontUtil::TexturePageTracker::
texture_size(int pg) const
{
  WRATHAutoLockMutex(m_mutex);
  WRATHassert(pg>=0 and static_cast<unsigned int>(pg)<m_pages.size());
  return m_pages[pg]->texture_size();
}

const std::vector<float>&
WRATHTextureFontUtil::TexturePageTracker::
custom_data(int pg) const
{
  WRATHAutoLockMutex(m_mutex);
  WRATHassert(pg>=0 and static_cast<unsigned int>(pg)<m_pages.size());
  return m_pages[pg]->m_custom_data;
}


std::vector<float>&
WRATHTextureFontUtil::TexturePageTracker::
custom_data(int pg) 
{
  WRATHAutoLockMutex(m_mutex);
  WRATHassert(pg>=0 and static_cast<unsigned int>(pg)<m_pages.size());
  return m_pages[pg]->m_custom_data;
}

const_c_array<WRATHTextureChoice::texture_base::handle>
WRATHTextureFontUtil::TexturePageTracker::
texture_binder(int pg) const
{
  WRATHAutoLockMutex(m_mutex);
  WRATHassert(pg>=0 and static_cast<unsigned int>(pg)<m_pages.size());
  return m_pages[pg]->binders();
}

int
WRATHTextureFontUtil::TexturePageTracker::
get_page_number(WRATHImage *pImage)
{
  return get_page_number(pImage->atlas_size(),
                         pImage->texture_binders());
}

int
WRATHTextureFontUtil::TexturePageTracker::
get_page_number(WRATHImage *mainImage,
                const_c_array<WRATHImage*> additional_images)
{
  binder_array texes(mainImage->texture_binders().begin(),
                     mainImage->texture_binders().end());

  for(const_c_array<WRATHImage*>::iterator iter=additional_images.begin(),
        end=additional_images.end(); iter!=end; ++iter)
    {
      WRATHImage *r(*iter);

      for(const_c_array<WRATHTextureChoice::texture_base::handle>::iterator
            ii=r->texture_binders().begin(), ee=r->texture_binders().end(); ii!=ee; ++ii)
        {
          texes.push_back(*ii);
        }
    }
  return get_page_number_implement(mainImage->atlas_size(), 
                                   texes);
}


int
WRATHTextureFontUtil::TexturePageTracker::
get_page_number(ivec2 ptexture_size, 
                const_c_array<WRATHTextureChoice::texture_base::handle> raw_key)
{
  binder_array key(raw_key.begin(), raw_key.end());
  return get_page_number_implement(ptexture_size, key);
}


int
WRATHTextureFontUtil::TexturePageTracker::
get_page_number_implement(ivec2 ptexture_size, binder_array &key)
{
  int R;

  WRATHLockMutex(m_mutex);
  
  map_type::iterator iter;

  iter=m_map.find(key);
  if(iter==m_map.end())
    {
      m_map[key]=m_pages.size();
      R=m_pages.size();
      m_pages.push_back(WRATHNew page_type(ptexture_size, 
                                           key));

      m_signal(R, 
               m_pages[R]->texture_size(),
               m_pages[R]->binders(),
               m_pages[R]->m_custom_data);
    }
  else
    {
      R=iter->second;
    }

  WRATHUnlockMutex(m_mutex);

  return R;
}




/////////////////////////////////////////
//WRATHTextureFontUtil::SubQuadProducer methods
WRATHTextureFontUtil::SubQuadProducer::
SubQuadProducer(const ivec2 &pfull_quad_resolution,
                int pmin_subquad_size):
  m_min_quad_size(std::max(1,pmin_subquad_size)),
  m_full_resolution(pfull_quad_resolution),
  m_lowres_resolution(compute_lowres_dimensions(m_full_resolution, m_min_quad_size)),
  m_tile_covered(boost::extents[m_lowres_resolution.x()][m_lowres_resolution.y()]),
  m_ready(true)
{
  std::fill(m_tile_covered.data(),
            m_tile_covered.data()+m_tile_covered.num_elements(),
            false);
}


void
WRATHTextureFontUtil::SubQuadProducer::
mark_texel(int x, int y)
{
  ivec2 p(x,y);
  
  p=lowres_coordinate(p);
  if(!m_tile_covered[p.x()][p.y()])
    {
      m_list_of_covered_tiles.push_back(p);
      m_tile_covered[p.x()][p.y()]=true;
      m_ready=false;
    }
}

void
WRATHTextureFontUtil::SubQuadProducer::
flush(void) const
{
  if(m_ready)
    {
      return;
    }

  m_ready=true;
  
  /*
    compute the active edges 
   */
  std::list<local_edge_type> horizontal_edges, vertical_edges;
  boost::multi_array<int, 2> edges_through_vertex(boost::extents[m_lowres_resolution.x()+1][m_lowres_resolution.y()+1]);

  std::fill(edges_through_vertex.data(),
            edges_through_vertex.data()+edges_through_vertex.num_elements(),
            0);
  
  for(std::list<ivec2>::const_iterator iter=m_list_of_covered_tiles.begin(),
        end=m_list_of_covered_tiles.end(); iter!=end; ++iter)
    {
      ivec2 pt(*iter);

      //left side check:
      if(pt.x()==0 or !m_tile_covered[pt.x()-1][pt.y()])
        {
          //fixed value is x, varying is y:
          vertical_edges.push_back( local_edge_type(pt.x(), pt.y(), pt.y()+1, false) );
          ++edges_through_vertex[pt.x()][pt.y()];
          ++edges_through_vertex[pt.x()][pt.y()+1];
        }

      //right side check:
      if(pt.x()+1==m_lowres_resolution.x() or !m_tile_covered[pt.x()+1][pt.y()])
        {
          //fixed value is x, varying is y:
          vertical_edges.push_back( local_edge_type(pt.x()+1, pt.y(), pt.y()+1, false) );
          ++edges_through_vertex[pt.x()+1][pt.y()];
          ++edges_through_vertex[pt.x()+1][pt.y()+1];
        }

      //below:
      if(pt.y()==0 or !m_tile_covered[pt.x()][pt.y()-1])
        {
          //fixed value is y, varying is x:
          horizontal_edges.push_back( local_edge_type(pt.y(), pt.x(), pt.x()+1, true) );
          ++edges_through_vertex[pt.x()][pt.y()];
          ++edges_through_vertex[pt.x()+1][pt.y()];
        }

      //above
      if(pt.y()+1==m_lowres_resolution.y() or !m_tile_covered[pt.x()][pt.y()+1])
        {
          //fixed value is y, varying is x:
          horizontal_edges.push_back( local_edge_type(pt.y()+1, pt.x(), pt.x()+1, true) );
          ++edges_through_vertex[pt.x()][pt.y()+1];
          ++edges_through_vertex[pt.x()+1][pt.y()+1];
        }
      
    }

 


  /*
    Sort and merge the edges
   */
  collapse_edge_list(vertical_edges, edges_through_vertex);
  collapse_edge_list(horizontal_edges, edges_through_vertex);

  
  /*
    Save required vertices in addition realize
    the indices of the edges.
   */
  std::map<ivec2, uint16_t> vert_map_index;
  

  m_attributes.clear();
  set_indices(vertical_edges, vert_map_index, m_attributes);
  set_indices(horizontal_edges, vert_map_index, m_attributes);
  WRATHassert(m_attributes.size()==vert_map_index.size());  


  /*
    create outline lists from the edge lists, note that
    m_attributes[I] is a lowres coordinate right now.
   */
  std::list<local_edge_type> all_edges;
  std::list< std::vector<uint16_t> > outlines;

  all_edges.splice(all_edges.begin(), horizontal_edges);
  all_edges.splice(all_edges.begin(), vertical_edges);
  create_outlines(outlines, all_edges, m_attributes, m_tile_covered);
  

  for(std::vector<ivec2>::iterator iter=m_attributes.begin(),
        end=m_attributes.end(); iter!=end; ++iter)
    {
      *iter=fullres_coordinate(*iter);
    }
    
  /*
    Generate a triangulation.
   */
  WRATHTriangulationI triangulator;

  for(std::list< std::vector<uint16_t> >::iterator iter=outlines.begin(),
        end=outlines.end(); iter!=end; ++iter)
    {

      std::vector<ivec2>::const_reference (std::vector<ivec2>::*access)(std::vector<ivec2>::size_type) const;
      access=&std::vector<ivec2>::operator[];

      /*
        Note that & to take address. Without the &, boost::bind
        would _copy_ the array.
       */
      triangulator.add_outline(iter->begin(), iter->end(),
                               boost::bind(access, &m_attributes, _1));
     

    }
  
  /*
    absorb index data, note that we use the even-odd rule.
    This is because our outlines are not really oriented.
  */  
  m_indices.resize(triangulator.even_odd_rule_triangulation().size());
  std::copy(triangulator.even_odd_rule_triangulation().begin(),
            triangulator.even_odd_rule_triangulation().end(),
            m_indices.begin());
}



GLint
WRATHTextureFontUtil::
effective_texture_creation_size(int R, bool force_pow2)
{
  /*
    GLint MX;
    MX=WRATHglGet<GLint>(GL_MAX_TEXTURE_SIZE);
    if(force_power2_texture())
    {
      MX=WRATHUtil::floor_power_2(MX);
    }
    
    R=std::min(MX, R);
  */

  if(force_pow2)
    {
      R=WRATHUtil::ceiling_power_2(R);
    }
  return R;
}
