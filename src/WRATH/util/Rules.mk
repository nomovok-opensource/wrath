# Begin standard header
sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)
# End standard header

LIB_SOURCES += $(call filelist,  WRATHReferenceCountedObject.cpp WRATHUtil.cpp WRATHPolynomial.cpp WRATHNew.cpp WRATHAtlas.cpp WRATHAtlasBase.cpp WRATH2DRigidTransformation.cpp WRATHResourceManager.cpp WRATHmalloc.cpp WRATHMutex.cpp WRATHTripleBufferEnabler.cpp WRATHStateStream.cpp WRATHStaticInit.cpp)

# Begin standard footer
d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
# End standard footer
