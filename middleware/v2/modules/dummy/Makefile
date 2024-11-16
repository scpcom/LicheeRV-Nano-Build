SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../Makefile.param
     include $(PARAM_FILE)
endif

isp_chip_dir := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
isp_mod_dir := $(PWD)/../isp
isp_bin_dir = $(isp_mod_dir)/$(isp_chip_dir)/isp_bin

SDIR = $(PWD)/src
SRCS := $(wildcard $(SDIR)/a*.c)
SRCS += $(wildcard $(SDIR)/isp_*.c)
BIN_SRCS := $(wildcard $(SDIR)/cvi_*.c)
BIN_SRCS += $(wildcard $(isp_bin_dir)/src/*.c)

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
	-I$(isp_mod_dir)/$(isp_chip_dir)/isp_algo/inc \
	-I$(isp_bin_dir)/inc
OBJS = $(SRCS:.c=.o)
OBJS_ASM = $(SRCS_ASM:.S=.o)
DEPS = $(SRCS:.c=.d)
BIN_OBJS = $(BIN_SRCS:.c=.o)
BIN_DEPS = $(BIN_SRCS:.c=.d)

TARGET_A = $(MW_LIB)/libcvi_dummy.a
TARGET_SO = $(MW_LIB)/libcvi_dummy.so
BIN_TARGET_A = $(MW_LIB)/libcvi_bin_dummy.a
BIN_TARGET_SO = $(MW_LIB)/libcvi_bin_dummy.so

EXTRA_CFLAGS = $(INCS) -fpack-struct=8

EXTRA_CFLAGS += -DISP_BIN_COMMIT=\"$(shell git rev-parse --short HEAD)\"
gerrit_id=$(shell git log -n1 | grep Reviewed-on | tail -n1 | sed "s/.*\///g")
ifneq ($(gerrit_id), )
	EXTRA_CFLAGS += -DISP_BIN_GERRIT=\"$(gerrit_id)\"
endif

result=$(shell cd $(isp_bin_dir) ; ./CalPqBinMD5.sh)
ifeq ($(result),-1)
abort:
	$(error "md5 error: please check CalPqBinMD5.sh")
else
	EXTRA_CFLAGS += -DISP_BIN_MD5=\"$(result)\"
endif

EXTRA_CFLAGS += -DMMF_VERSION=\"$(shell git describe --always)\"
EXTRA_CFLAGS += -DSDK_VER=\"$(SDK_VER)\"
EXTRA_CFLAGS += -DISP_COMMIT=\"$(shell git rev-parse --short HEAD)\"
EXTRA_CFLAGS += -DISP_GERRIT=\"$(shell git log -n1 | grep Reviewed-on | tail -n1 | sed "s/.*\///g")\"
ifeq ($(MULTI_PROCESS_SUPPORT), 1)
EXTRA_CFLAGS += -DRPC_MULTI_PROCESS
endif

.PHONY : clean all
all : $(TARGET_A) $(TARGET_SO) $(BIN_TARGET_A) $(BIN_TARGET_SO)


$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(SDIR)/%.o: $(SDIR)/%.S
	$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(isp_bin_dir)/src/%.o: $(isp_bin_dir)/src/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS) $(OBJS_ASM)
	@$(AR) $(ARFLAGS) $(TARGET_A) $(OBJS) $(OBJS_ASM)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))
	@echo "$$AR_MRI" | $(AR) -M

$(TARGET_SO): $(OBJS) $(OBJS_ASM)
	@$(LD) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $(TARGET_SO) --start-group $(OBJS) $(OBJS_ASM) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))
	@$(STRIP) $(TARGET_SO)

$(BIN_TARGET_A): $(BIN_OBJS)
	@$(AR) $(ARFLAGS) $(BIN_TARGET_A) $(BIN_OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(BIN_TARGET_A))
	@echo "$$AR_MRI" | $(AR) -M

$(BIN_TARGET_SO): $(BIN_OBJS)
	@$(LD) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $(BIN_TARGET_SO) --start-group $(BIN_OBJS) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(BIN_TARGET_SO))
	@$(STRIP) $(BIN_TARGET_SO)

clean:
	@rm -f $(OBJS) $(OBJS_ASM) $(DEPS) $(TARGET_A) $(TARGET_SO)
	@rm -f $(BIN_OBJS) $(BIN_DEPS) $(BIN_TARGET_A) $(BIN_TARGET_SO)

-include $(DEPS)
