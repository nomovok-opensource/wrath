wrath
=====

WRATH is a library to facilitate drawing user interfaces with GL in
an optimized fashion. It does not address window creation or event 
handling for that matter. It only handles drawing and creation of
objects that represent that drawing. To that end WRATH can be made
to work in a variety of toolkits that allow drawing with OpenGL or
OpenGL ES.

WRATH is a data based library; UI items and widgets by their
existence have their content presented to the screen. The items and
widgets themselves do NOT have a paint method. Instead, how
they are drawn and what they draw is data and that data is assembled
by WRATH into common units to reduce CPU load on drawing.

WRATH provides the following features for UI drawing
* text rendering and formatting
* image, gradients and brushes
* paths: filling and stroking
* transformation hierarchy system allowing for user defined transformation node types
* a set of classes that allows one to create new item types drawn with one's shaders
that will work on an transformation node type using the transformation hierarchy system
of WRATH
* a set of classes to apply brush (image, gradient, etc) linearly and non-linearly
to items

and more.
 

