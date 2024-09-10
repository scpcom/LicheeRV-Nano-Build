SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
        PARAM_FILE:=../../../Makefile.param
        include $(PARAM_FILE)
endif

PWD:=$(shell pwd)
ROOT:=$(PWD)/..

#--------------------------------Output------------------------------
# OUTTYPE: 0-exe, 1-dll, 2-static
#--------------------------------------------------------------------
OUTTYPE = 2
OUTFILE = libmaix_mmf.a

#-------------------------------Include------------------------------
#
# INCLUDES = $(addprefix -I,$(INCLUDES)) # add -I prefix
#--------------------------------------------------------------------
INCLUDES = . \
					$(MW_INC) \
					$(ISP_INC) \
					../../common/ \
					$(KERNEL_INC) \
					$(ROOT)

#-------------------------------Source-------------------------------
#
#--------------------------------------------------------------------
SOURCE_PATHS = .
#source source/client source/server source/utils source/sdp
SOURCE_FILES = $(foreach dir,$(SOURCE_PATHS),$(wildcard $(dir)/*.cpp))
SOURCE_FILES += $(foreach dir,$(SOURCE_PATHS),$(wildcard $(dir)/*.c))

#-----------------------------Library--------------------------------
#
# LIBPATHS = $(addprefix -L,$(LIBPATHS)) # add -L prefix
#--------------------------------------------------------------------
LIBPATHS =
ifdef RELEASE
# relase library path
LIBPATHS +=
else
LIBPATHS +=
endif

LIBS = 

STATIC_LIBS =

#-----------------------------DEFINES--------------------------------
#
# DEFINES := $(addprefix -D,$(DEFINES)) # add -L prefix
#--------------------------------------------------------------------
DEFINES = 

include gcc.mk
