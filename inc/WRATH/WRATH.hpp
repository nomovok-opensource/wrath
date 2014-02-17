/*! 
 * \file WRATH.hpp
 * \brief file WRATH.hpp
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


/*
  NOTE: this is a source free header file,
  it contains documentation only.
 */

#ifndef WRATH_HEADER_HPP_
#define WRATH_HEADER_HPP_


/*!
  \defgroup Kernel Kernel
  \defgroup Utility Utility
  \defgroup GLUtility GLUtility
  \defgroup Group Group
  \defgroup Text Text
  \defgroup Imaging Imaging
  \defgroup Items Items
  \defgroup Widgets Widgets
  \defgroup Layer Layer
  \defgroup Qt Qt
  \defgroup SDL SDL
 */

/*!
  \mainpage WRATH Overview

  WRATH's purpose is to provide a high performance
  user interface rendering library. It focuses only 
  on rendering. It does not handle window creation,
  or for that matter GL context creation. WRATH 
  achieves it's high performance through batching.
  That batching allows for multiple items of a user
  interface to be drawn with a single draw call. 
  In a traditional UI renderer, each UI item of a 
  widget has a draw call which is executed whenever
  the widget is drawn. The result is that although
  the number of vertices and pixels processed are 
  relatively low, performance suffers. The main
  culprit is the simple well known fact that each
  draw call has significant overhead. Indeed, for
  600-800 Mhz ARM devices, one can expect to be unable
  to issue more than 100-200 calls per frame and maintain
  a refrest rate of 60 hertz, regardless of the total
  number of vertices processed and pixels affected.

  As a selling point, the rate of performance increase
  of GPU is several factors higher than the rate
  of increase of performance in CPU's. Indeed as an
  example, Apple claims that the iPad2 has 2X the CPU
  power than the iPad1, but 9X the GPU power. Examining
  Imagination Technologies GPU roadmap, NVIDIA's Tegra
  roadmap the factors of increase in horse power of GPU's
  far, far outpace CPU's.
  
  Please see \ref wrath_intro for an introduction to WRATH
  and see \ref wrath_examples_main for example code using WRATH.

 
  \section ModulOverview WRATH is conceptionally organized as follows

  \subsection AboutGroup Group
  See the documentation of the group \ref Group 
  for class specific details. \ref Group is the
  heart and soul of WRATH, it is the classes of
  Group that make batching of data easy. It provides
  an interface for a developer to create their
  own UI item types.   

  \subsection AboutLayer Layer
  See the documentation of the group \ref Layer 
  for class specific details. Layer represents
  an implementation of WRATHCanvas in addition
  to a framework to define transformation node
  type and node types from other node types
  holding additional data (for example color values).

  \subsection AboutUtil Utility
  See the documentation of the group \ref Utility
  for function and class specifics. Utility
  encompasses useful simple classes including fix-sized
  array classes (vecN), matrix classes, simple tags,
  a small number of "miscellaneous" utility classes
  and wrapper classes for multi-threaded programming.
  
  \subsection AboutGLUtility GLUtility
  See the documentation of the group \ref GLUtility
  for function and class specifics. GLUtility
  encompasses classes and functions to make programming
  using the GL API easier. These classes are the 
  "interface" to GL in WRATH for specifying data
  sources, data formats and GL state in WRATH. The 
  classes are used to specify GL state, data sources
  and draw calls to issue to GL.

  \subsection AboutKernel Kernel
  See the documentation of the group \ref Kernel 
  for class specific details. Kernel provides
  an interface to issue multiple GL draw calls
  to minimize GL state changes.

  \subsection AboutText Text
  See the documentation of the group \ref Text
  for class specific details. The items under
  \ref Text are for handling of text: creation
  of fonts, format and display of text.

  \subsection AboutImaging Imaging
  See the documentation of the group \ref Imaging
  for class specific details. The items under
  \ref Imaging are for handling images, gradients,
  and brushes. In addition defines attribute
  packer types for drawing imaged and non-imaged
  rectangles.

  \subsection AboutShape Shape
  See the documentation of the group \ref Shape
  for class specific details. The items under
  \ref Shape are for defining shapes (i.e. paths)
  for stroking and filling.  In addition defines 
  attribute packer types for drawing shapes
  both stroked and filled.

  \subsection AboutItems Items
  See the documentation of the group \ref Items 
  for class specific details. The classes in
  Items represent the code to create items independent
  of the transformation heirarchy to be applied to an 
  item.  

  \subsection AboutWidgets Widgets
  See the documentation of the group \ref Widgets 
  for class specific details. A widget is essentially
  a composite of a node and an item. The
  widget framework of WRATH is a set of template
  classes to allow for using items in \ref Items
  and user defined items in any node hierarchy type.

  \subsection AboutQT QT
  See the documentation of \ref Qt for details.
  The module QT provides an interface between
  Qt and WRATH, specifically to use Qt to perform font
  selection and to use QImage's as data sources for WRATH
  imaging data. This module is not a core part of WRATH
  and can be left out for applications that wish to NOT depend
  on Qt.

  \subsection AboutSDL SDL
  See the documentation of \ref SDL for details.
  This module provides image loading routines
  via SDL. This module is not a core part of WRATH
  and can be left out for applications that wish to 
  NOT depend on SDL.

 */

/*!\namespace WRATHQT
  WRATHQT namespace for Qt to WRATH interface. 
 */
namespace WRATHQT
{}

/*!\namespace WRATHSDL
  WRATHSDL namespace for SDL to WRATH interface. 
 */
namespace WRATHSDL
{}

/*!\namespace WRATHText
  WRATHText namespace for text data stream manipulators
  (like \ref WRATHText::font or shader \ref 
  WRATHText::font_shader)
 */
namespace WRATHText
{}


/*!\namespace WRATHMemory
  Namespace for memory allocation tracking.
  
  WRATHMemory provides object allocation and 
  object deallocation debugging when 
  WRATH_NEW_DEBUG is defined

  It also provids raw memory allocation
  (i.e. ala malloc) when WRATH_MALLOC_DEBUG
  is defined.

  See \ref WRATHNew, \ref WRATHDelete, \ref WRATHDelete_array,
  \ref WRATHmalloc, \ref WRATHrealloc and \ref WRATHfree.
 */
namespace WRATHMemory
{}

/*!\namespace WRATHUtil
  Namespace for various simple and commonly used global routines.
 */
namespace WRATHUtil
{}


{}



#endif
