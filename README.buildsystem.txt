WRATH buildsystem
=================

Everything is built from the toplevel Makefile in the same directory
with this document. For information about available targets, run

 make targets


Modifying WRATH library build process
=====================================

All directories contain a file named Rules.mk. It contains make code
that declares what files and directories should be processed from that
particular directory. No other files have any knowledge about them,
nor do they need it.

Rules.mk files need to have certain code in the beginning and end of
them. This is marked in the existing Rules.mk files with "Begin/End
standard header" and "Begin/End standard footer".

The variable $(d) contains the full path to the directory being
processed. All filename references MUST use it, because the "current
working directory" of make stays as the toplevel root directory. There
is a function "filelist" that can be called to prepend the path $(d)
to a list of filenames. Example:

 LIB_SOURCES += $(call filelist, foo.cpp bar.cpp quz.cpp)

Adding a subdirectory to be processed requires two lines. The variable
$(dir) needs to be set to that subdirectory path (with $(d) again,
like above) and then $(dir)/Rules.mk is included. Example:

 dir := $(d)/subdir
 include $(dir)/Rules.mk

Rules.mk files should only need to modify certain variables. Any other
code inside a Rules.mk file is an indication of a probable
bug. Variables available are:

INCLUDES: Contains -I flags used when building WRATH libraries, and
demos. Typical usage: INCLUDES += -I$(d)

LIB_SOURCES: Contains source files for building WRATH
libraries. Typical usage: LIB_SOURCES += $(call filelist, foo.cpp
bar.cpp)

buildtargetname_LIB_SOURCES: Contains source files specific for the
`buildtargetname' version of WRATH libraries. Typical usage like
above. Example: SDL_LIB_SOURCES += $(call filelist, sdl_frob.cpp). The
naming convention is ultimately up to the buildtarget-specific
Makefile snippets, nothing else touches the variable.

SHADERS: Contains .glsl files that are to be converted to .glsl.cpp
and .glsl.hpp files.


Modifying demos build process
=============================

Everything stated in the above section still applies, just variable
names change.

DEMOS_INCLUDES: Contains -I flags common to all demos. demos/Rules.mk
adds the various common/ directories to this variable. The total set
of -I flags used when building a demo are INCLUDES (from wrath library
building, above), DEMOS_INCLUDES, demo-specific_CPPFLAGS (below).

DEMOS: Contains the names of all demos. Typical usage: DEMOS +=
text-viewer. The buildtarget-specific Makefile snippets operate on
this list to their liking.

Demo-specific variables:

foobar_SOURCES: If foobar is added to DEMOS, this variable contains
the source files used to build it. ****IMPORTANT**** This variable
MUST be defined with :=, not =. Example: foobar_SOURCES := $(call
filelist, foobar.cpp)

For the demo's sources list, you can use $(COMMON_DEMO_SOURCES) to
include commonly used .cpp files. It is defined in
demos/common/Rules.mk.

foobar_CPPFLAGS: Extra preprocessor flags (-I flags, -D flags, etc) specific
to this demo.

foobar_CXXFLAGS: Extra compiler flags specific to this demo.

foobar_CXXFLAGS_RELEASE: Extra compiler flags specific to this demo,
used when building for release.

foobar_CXXFLAGS_DEBUG: Extra compiler flags specific to this demo,
used when building for debug.

CPPFLAGS and CXXFLAGS are separated because only CPPFLAGS is used when
generating dependencies.

foobar_LDFLAGS: Extra linker flags specific to this demo.


Build targets
=============

The main Makefile declares a variable BUILDTARGETS that lists targets
to build against. For example, sdl qt. This variable is settable from
environment or the make command line as well. If the string `foobar'
is included in the list, the file

 Makefile.foobar.pre

is included and processed before collecting variable values from
subdirectories, and file

 Makefile.foobar.post

is included and processed after.

The main interface that these makefile snippets use is adding new
targets, as well as adding prerequisites to the `release-all' and
`debug-all' rules. The files to build are communicated to the .post
files with LIB_SOURCES (see above) and buildtarget-specific variables.

The snippets can declare rules that depend on object files (.o) in the
`release' or `debug' directories, which will be built using the main
helper machinery, from .cpp or .c files in matching paths with
`release' or `debug' stripped. They can also add dependency filenames
(.d) in the `release' and `debug' directories to the variable DEPS,
which will get included and generated from matching .cpp or .c files
like with .o files. Duplicate filenames in DEPS get discarded safely.

The variables RELEASE_SHADERS and DEBUG_SHADERS will be defined to
shader .cpp filenames, and the snippets must orchestrate those to be
built, it will not be done with any automation.


Example:

release-all: my-own-demo

my-own-demo: release/demos/my-own/my.o

DEPS += release/.depend/demos/my-own/my.d

See Makefile.sdl.pre and Makefile.sdl.post for a more thorough example
and useful machinery.



Implementation notes
====================

Main inspiration for this build system is the document Recursive Make
Considered Harmful[1]. Implementation style for subdirectory
processing is directly copied.

Only GNU make can process this system. GNU make specific features
include, but not limited to, simply-expanded flavor of variables
(assignment using := instead of =).



References:

 1) http://miller.emu.id.au/pmiller/books/rmch/
