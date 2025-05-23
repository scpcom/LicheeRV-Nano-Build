SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../Makefile.param
     include $(PARAM_FILE)
endif

isp_chip_dir := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
isp_mod_dir := $(PWD)/../isp

SDIR = $(PWD)/src
SRCS := $(wildcard $(SDIR)/a*.c)
ALGO_SRCS := $(wildcard $(SDIR)/isp_*.c)
ALGO_SRCS += $(wildcard $(SDIR)/cvi_*.c)

# 3rdparty lib
BIN_INCLUDE_PATH = $(PWD)/../bin/prebuilt/include

ifneq ($(findstring $(CHIP_ARCH), MARS PHOBOS CV181X CV180X SG200X),)
ISP_BIN_DIR = $(MW_PATH)/modules/isp/$(shell echo $(CHIP_ARCH) | tr A-Z a-z)/isp_bin/inc
else
$(error UNKNOWN chip architecture - $(CHIP_ARCH))
endif

SRCS_ASM = $(wildcard $(SDIR)/*.S)
INCS = -I$(MW_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VPU_INC) -I$(ISP_INC) -Iinclude
INCS += -I$(SYS_INC) -I$(BIN_INCLUDE_PATH)/cvi_json-c -I$(ISP_BIN_DIR) -I$(BIN_INCLUDE_PATH)/cvi_miniz
INCS += -I$(isp_mod_dir)/algo \
	-I$(isp_mod_dir)/common/clog \
	-I$(isp_mod_dir)/$(isp_chip_dir)/isp/inc \
	-I$(isp_mod_dir)/$(isp_chip_dir)/isp_algo/inc
OBJS = $(SRCS:.c=.o)
OBJS_ASM = $(SRCS_ASM:.S=.o)
DEPS = $(SRCS:.c=.d)
ALGO_OBJS = $(ALGO_SRCS:.c=.o)
ALGO_DEPS = $(ALGO_SRCS:.c=.d)

TARGET_A = $(MW_LIB)/libcvi_dummy.a
TARGET_SO = $(MW_LIB)/libcvi_dummy.so
ALGO_TARGET_A = $(MW_LIB)/libisp_algo_dummy.a
ALGO_TARGET_SO = $(MW_LIB)/libisp_algo_dummy.so

EXTRA_CFLAGS = $(INCS) -fpack-struct=8

EXTRA_CFLAGS += -DMMF_VERSION=\"$(shell git describe --always)\"
EXTRA_CFLAGS += -DSDK_VER=\"$(SDK_VER)\"
EXTRA_CFLAGS += -DISP_COMMIT=\"$(shell git rev-parse --short HEAD)\"
EXTRA_CFLAGS += -DISP_GERRIT=\"$(shell git log -n1 | grep Reviewed-on | tail -n1 | sed "s/.*\///g")\"
ifeq ($(MULTI_PROCESS_SUPPORT), 1)
EXTRA_CFLAGS += -DRPC_MULTI_PROCESS
endif

.PHONY : clean all
all : $(TARGET_A) $(TARGET_SO) $(ALGO_TARGET_A) $(ALGO_TARGET_SO)


$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(SDIR)/%.o: $(SDIR)/%.S
	$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS) $(OBJS_ASM)
	@$(AR) $(ARFLAGS) $(TARGET_A) $(OBJS) $(OBJS_ASM)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))
	@echo "$$AR_MRI" | $(AR) -M

$(TARGET_SO): $(OBJS) $(OBJS_ASM)
	@$(LD) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $(TARGET_SO) --start-group $(OBJS) $(OBJS_ASM) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))
	@$(STRIP) $(TARGET_SO)

$(ALGO_TARGET_A): $(ALGO_OBJS)
	@$(AR) $(ARFLAGS) $(ALGO_TARGET_A) $(ALGO_OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(ALGO_TARGET_A))
	@echo "$$AR_MRI" | $(AR) -M

$(ALGO_TARGET_SO): $(ALGO_OBJS)
	@$(LD) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $(ALGO_TARGET_SO) --start-group $(ALGO_OBJS) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(ALGO_TARGET_SO))
	@$(STRIP) $(ALGO_TARGET_SO)

clean:
	@rm -f $(OBJS) $(OBJS_ASM) $(DEPS) $(TARGET_A) $(TARGET_SO)
	@rm -f $(ALGO_OBJS) $(ALGO_DEPS) $(ALGO_TARGET_A) $(ALGO_TARGET_SO)

-include $(DEPS)
