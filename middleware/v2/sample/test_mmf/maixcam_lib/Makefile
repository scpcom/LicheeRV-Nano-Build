SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
        PARAM_FILE:=../../../Makefile.param
        include $(PARAM_FILE)
endif

LDFLAGS  = -shared -fPIC -Wl,--gc-sections -export-dynamic -L$(MW_LIB) -L$(MW_3RD_LIB) --sysroot $(SYSROOT) $(CVI_TARGET_PACKAGES_LIBDIR)

PWD:=$(shell pwd)
ROOT:=$(PWD)/..

#--------------------------------Output------------------------------
# OUTTYPE: 0-exe, 1-dll, 2-static
#--------------------------------------------------------------------
OUTTYPE = 1
OUTFILE = libmaixcam_lib.so

#-------------------------------Include------------------------------
#
# INCLUDES = $(addprefix -I,$(INCLUDES)) # add -I prefix
#--------------------------------------------------------------------
INCLUDES = . \
					./include \
					$(MW_INC) \
					$(ISP_INC) \
					../../common/ \
					$(KERNEL_INC) \
					$(SENSOR_LIST_INC) \
					$(ROOT)

#-------------------------------Source-------------------------------
#
#--------------------------------------------------------------------
SOURCE_PATHS = src
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

LIBS = stdc++fs

STATIC_LIBS = $(MW_LIB)/libsample.a $(MW_LIB)/libsns_full.a

#-----------------------------DEFINES--------------------------------
#
# DEFINES := $(addprefix -D,$(DEFINES)) # add -L prefix
#--------------------------------------------------------------------
DEFINES = 

include gcc.mk
