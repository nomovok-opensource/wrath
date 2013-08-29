ngl_gl.[ch]pp and ngl_gles2.[ch]pp are machine generated files.
The build process automatically generates them. The files are generated
by filtering and scanning the GL/GLES2 header files.

Comment: the GLES2 header files define a number of functions
with different argument types which are compatible in terms
of pushing values om the stack (namely GLint vs GLenum, etc),
but have incompatible function pointer types. If ngl_gles2.cpp
(or for that matter for builds using desktop GL ngl_gl.cpp)
fails to compile, issue make clean and make in the main project directory
to regenerate the files.
