# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

LIB_SOURCES += $(call filelist, WRATHUniformData.cpp WRATHGLStateChange.cpp WRATHGLExtensionList.cpp WRATHBufferObject.cpp WRATHRawDrawData.cpp WRATHGLProgram.cpp WRATHMultiGLProgram.cpp WRATHBufferAllocator.cpp WRATHTextureChoice.cpp WRATHGPUConfig.cpp WRATHGLStateStack.cpp WRATHDrawCommandIndexBufferAllocator.cpp WRATHShaderSourceResource.cpp WRATHBufferBindingPoint.cpp ngl_backend.cpp ngl_backend_lib.cpp)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
