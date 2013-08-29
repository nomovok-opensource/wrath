/*! 
 * \file WRATHRectAttributePacker.hpp
 * \brief file WRATHRectAttributePacker.hpp
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




#ifndef __WRATH_IMAGE_ATTRIBUTE_PACKER_HPP__
#define __WRATH_IMAGE_ATTRIBUTE_PACKER_HPP__

#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include "c_array.hpp"
#include "WRATHResourceManager.hpp"
#include "WRATHCanvas.hpp"
#include "WRATHAttributePacker.hpp"
#include "WRATHAbstractDataSink.hpp"

/*! \addtogroup Imaging
 * @{
 */



/*!\class WRATHRectAttributePacker
  A WRATHRectAttributePacker defines
  an interface to pack attribute data 
  for drawing a WRATHRect on behalf of
  WRATHRectItem. The underlying
  assumption is that to display an
  image requires 4 attributes/vertices
  so that the image is drawn as a
  quad. Dervied classes are used by WRATHRectItem
  and to pack attribute data for rect display.
 */
class WRATHRectAttributePacker:public WRATHAttributePacker
{
public:
  /*!\fn WRATHRectAttributePacker(const ResourceKey&, iterator, iterator)
    Ctor. Specifies the resource name of the attribute packer
    and the names of each attribute as an STL range.
    The number of attributes is then std::distance(begin,end)
    and the attribute of index I has value begin+I.

    \param pname resource name to identify the WRATHRectAttributePacker
    \param begin iterator to name of attribure #0
    \param end iterator to one past the name of the last attribute
   */
  template<typename iterator>
  WRATHRectAttributePacker(const ResourceKey &pname,
                           iterator begin, iterator end):
    WRATHAttributePacker(pname, begin, end)
  {}

  /*!\fn WRATHRectAttributePacker(const ResourceKey &pname, const std::vector<std::string>&)
    Ctor. Specifies the resource name of the attribute packer
    and the names of each attribute as an STL range.
    The number of attributes is then std::distance(begin,end)
    and the attribute of index I has value *(begin+I).
    \tparam iterator to string type
    \param pname resource name to identify the WRATHAttributePacker
    \param pattribute_names names of the attributes, value at index 0 
                            will be for attribute #0 in GL
   */
  WRATHRectAttributePacker(const ResourceKey &pname,
                           const std::vector<std::string> &pattribute_names):
    WRATHAttributePacker(pname, pattribute_names)
  {}

  virtual
  ~WRATHRectAttributePacker(void)
  {}
  
  /*!\fn void set_attribute_data(WRATHAbstractDataSink&, 
                                 int,
                                 const WRATHReferenceCountedObject::handle&,
                                 const WRATHStateBasedPackingData::handle&) const  
    Packs the attribute data to draw a rectangle. 
    The attribute data is to be 4 vertices packed
    in the order: minx_miny, minx_maxy, maxy_maxy, maxx_miny.
    \param sink WRATHAbstractSink to which to pack the attribute data
    \param attr_location location within sink to place attribute data
    \param rect handle of data describing the rectangle from which to compute/create attribute data 
    \param h handle to hold additional immutable state that affects packing
   */
  void
  set_attribute_data(WRATHAbstractDataSink &sink,
                     int attr_location,
                     const WRATHReferenceCountedObject::handle &rect,
                     const WRATHStateBasedPackingData::handle &h) const
  {
    set_attribute_data_implement(sink, attr_location, rect, h);
  }
  
  /*!\fn void set_attribute_data(WRATHCanvas::DataHandle, 
                                 int,
                                 const WRATHReferenceCountedObject::handle&,
                                 const WRATHStateBasedPackingData::handle&) const  
    Packs the attribute data to draw a rectangle. 
    The attribute data is to be 4 vertices packed
    in the order: minx_miny, minx_maxy, maxy_maxy, maxx_miny.
    Provided as a conveniance, equivalent to
    \code
    WRATHAttributeStore::DataSink sink(item_group.attribute_store()->data_sink());
    set_attribute_data(sink, attr_location, rect, h);
    \endcode
    \param item_group location to which to pack the attribute data
    \param attr_location location within sink to place attribute data
    \param rect handle of data describing the rectangle from which to compute/create attribute data 
    \param h handle to hold additional immutable state that affects packing
   */
  void
  set_attribute_data(WRATHCanvas::DataHandle item_group,
                     int attr_location,
                     const WRATHReferenceCountedObject::handle &rect,
                     const WRATHStateBasedPackingData::handle &h) const
  {
    WRATHAttributeStore::DataSink sink(item_group.attribute_store()->data_sink());
    set_attribute_data(sink, attr_location, rect, h);
  }

  /*!\fn void attribute_key 
    To be implemented by a derived class to
    fetch the attribute key.
    \param attrib_key WRATHAttributeStoreKey to which to set
   */
  virtual
  void
  attribute_key(WRATHAttributeStoreKey &attrib_key) const=0;
              
protected:
  /*!\fn set_attribute_data_implement(WRATHAbstractDataSink&, 
                                      int,
                                      const WRATHReferenceCountedObject::handle&,
                                      const WRATHStateBasedPackingData::handle&)  
    To be implemented by a dervied class to
    pack the attribute data to draw a rectangle. 
    The attribute data is to be 4 vertices packed
    in the order: minx_miny, minx_maxy, maxx_maxy, maxx_miny.
    \param sink WRATHAbstractSink to which to pack the attribute data
    \param attr_location location within sink to place attribute data
    \param rect handle of data describing the rectangle from which to compute/create attribute data 
    \param h handle to hold additional immutable state that affects packing
   */
  virtual
  void
  set_attribute_data_implement(WRATHAbstractDataSink &sink,
                               int attr_location,
                               const WRATHReferenceCountedObject::handle &rect,
                               const WRATHStateBasedPackingData::handle &h) const=0;


};
/*! @} */


#endif
