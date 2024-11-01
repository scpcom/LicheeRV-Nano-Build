SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../Makefile.param
	include $(PARAM_FILE)
endif
include ../sample.mk

SDIR = $(PWD)
SRCS = $(wildcard $(SDIR)/*.c)
INCS = -I$(MW_INC) -I$(ISP_INC) -I../common/ -I$(KERNEL_INC) -I$(MW_INC)/linux -I$(SDIR)

CFLAGS += -I$(SDIR)/../test_mmf

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

TARGET = kvm_stream
ifeq ($(CONFIG_ENABLE_SDK_ASAN), y)
TARGET = kvm_stream_asan
endif

PKG_CONFIG_PATH = $(MW_PATH)/pkgconfig
REQUIRES = cvi_common cvi_sample
# cvi_ive

MW_LIBS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs --define-variable=mw_dir=$(MW_PATH) $(REQUIRES))

LIBS = $(MW_LIBS)
ifeq ($(MULTI_PROCESS_SUPPORT), 1)
DEFS += -DRPC_MULTI_PROCESS
LIBS += -lnanomsg
endif

EXTRA_CFLAGS = $(INCS) $(DEFS)
EXTRA_LDFLAGS = $(LIBS) -lpthread -lm -lini

MMF_LIBD = ../test_mmf/maix_mmf/release.linux

LIBS += -L$(MMF_LIBD)

# IVE_SUPPORT = 1
ifeq ($(IVE_SUPPORT), 1)
CFLAGS += -DIVE_SUPPORT

IVE_PATH = $(MW_PATH)/../install/soc_cv1835_wevb_0002a_spinand/tpu_64/cvitek_ive_sdk
EXTRA_CFLAGS += -I$(IVE_PATH)/include/ive
EXTRA_LDFLAGS += -L$(IVE_PATH)/lib -lcvi_ive_tpu-static

TPU_PATH = $(MW_PATH)/../install/soc_cv1835_wevb_0002a_spinand/tpu_64/cvitek_tpu_sdk
EXTRA_CFLAGS += -I$(TPU_PATH)/include
EXTRA_LDFLAGS += -L$(TPU_PATH)/lib -lcviruntime-static -lcvimath-static -lcvikernel-static -lcnpy -lglog -lz
endif

.PHONY : clean all
all: $(TARGET)

mmflibs:
	@$(MAKE) AR=$(AR) CC=$(CC) CXX=$(CXX) PLATFORM=linux RELEASE=1 -C ../test_mmf/maix_mmf/

clean_mmflibs:
	@$(MAKE) AR=$(AR) CC=$(CC) CXX=$(CXX) PLATFORM=linux RELEASE=1 -C ../test_mmf/maix_mmf/ clean

$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET): mmflibs $(COMM_OBJ) $(OBJS) $(ISP_OBJ) $(MW_LIB)/libvpu.a $(MW_LIB)/libsys.a
	@$(CXX) -o $@ -Wl,--start-group $(OBJS) $(COMM_OBJS) -lsys $(MW_LIB)/libsys.a -Wl,--end-group -lmaix_mmf $(ELFFLAGS) $(EXTRA_LDFLAGS)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CXX))] $(notdir $@)

clean: clean_mmflibs
	@rm -f $(OBJS) $(DEPS) $(COMM_OBJ) $(COMM_DEPS) $(TARGET)

-include $(DEPS)
