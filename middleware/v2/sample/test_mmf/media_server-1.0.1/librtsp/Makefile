PWD:=$(shell pwd)
ROOT:=$(PWD)/../sdk

#--------------------------------Output------------------------------
# OUTTYPE: 0-exe, 1-dll, 2-static
#--------------------------------------------------------------------
OUTTYPE = 2
OUTFILE = librtsp.a

#-------------------------------Include------------------------------
#
# INCLUDES = $(addprefix -I,$(INCLUDES)) # add -I prefix
#--------------------------------------------------------------------
INCLUDES = . \
					./include \
					$(ROOT)/include \
					$(ROOT)/libhttp/include \
					../librtp/include \
					../libmpeg/include \
					../libflv/include \
					../libmov/include \
					../libmkv/include \
					$(PWD)/../avcodec/avbsf/include \
					$(PWD)/../avcodec/avcodec/include

#-------------------------------Source-------------------------------
#
#--------------------------------------------------------------------
SOURCE_PATHS = source source/client source/server source/utils source/sdp
SOURCE_FILES = $(foreach dir,$(SOURCE_PATHS),$(wildcard $(dir)/*.cpp))
SOURCE_FILES += $(foreach dir,$(SOURCE_PATHS),$(wildcard $(dir)/*.c))

SOURCE_FILES += $(ROOT)/source/digest/md5.c
SOURCE_FILES += $(ROOT)/source/base64.c

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
DEFINES = RTP_OVER_RTSP_TRY_TO_FIND_NEXT_PACKET FIX_DAHUA_AAC_FROM_G711

include ../gcc.mk
