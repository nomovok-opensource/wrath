WRATH Variants
---------------------------------------
WRATH has 2 variants:
 -Qt
 -SDL

you can select which (or both) variants to build
by defining the environmental variable BUILDTARGETS:

(both SDL and Qt): #export BUILDTARGETS=qt sdl 
(SDL only): #export BUILDTARGETS=sdl 
(Qt only): #export BUILDTARGETS=qt 


These are built as _seperate_ libraries.
Both variants have the following dependencies:
- FreeType (libs and headers) 
- flex (see below about ngl)
- boost
- FontConfig (libs and headers)
- GL or GLES2 headers

The Qt variant requires Qt
The SDL variant requires SDL and SDL_image.

type make targets to see all available targets.


Shaders from resources
--------------------------------------
The class, WRATHShaderSourceResource, allows one to
have a shader fragment as a "resource", the Bash/Perl
frankenstein script to generate a .cpp file from shader 
source code is located at shell_scripts/create_cpp_hpp_from_file.sh,
which calls the Perl script shell_scripts/decode_file_chars_to_numbers.pl


About NGL
---------------------------------------

NGL is a machine generated header/source system to aid in
using and debugging GL. The files are generated from the
GL and/or GLES2 headers. Some platforms have different headers
of GLES2/gl2.h that are incompatible with the headers used
to generate ngl/ngl_gles2.cpp and ngl/ngl_gles2.hpp. To rectify
this case:
 - cd ngl
 - rm ngl_gles2.[ch]pp
 - make ngl_gles2

In addition, one can do the same for ngl_gl.[ch]pp but this is
not necessary. The generation of the files requires flex.


GL or GLES2
-------------------------------------
By default, the root makefile will do as follows:
 if x86 --> use GL
 if ARM --> use GLES2
this can be overridden from Makefile.settings, see
OVERRIDE_GL_TYPE. Additionally what version of the 
GL/GLES API is also set in Makefile.settings with
GL_VERSION


Boost
------------------------------------
WRATH makes optional use of boost::locale which is not present
in boost versions prior to 1.48. If you do not have a version of
boost of atleast 1.48, then within Makefile, set USE_BOOST_LOCALE to 0.
We strongly advise using atleast v1.48 of boost and using boost::locale
though.


Debug vs Release
---------------------------
The WRATH library and all demos not only have an SDL and Qt variants,
but also each such has a debug variant. The debug versions run horrow
slow but should be used if any issues arise on device.




