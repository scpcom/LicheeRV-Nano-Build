SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../Makefile.param
     include $(PARAM_FILE)
endif

bin_mod_dir := $(realpath $(PWD)/../bin)
isp_chip_dir := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
isp_mod_dir := $(realpath $(PWD)/../isp)
isp_dir = $(isp_mod_dir)/$(isp_chip_dir)/isp
isp_bin_dir = $(isp_mod_dir)/$(isp_chip_dir)/isp_bin

SDIR = $(PWD)/src
BIN_SRCS := $(subst $(bin_mod_dir)/src,$(SDIR)/bin,$(wildcard $(bin_mod_dir)/src/*.c))
ISP_SRCS := $(subst $(isp_mod_dir)/common/clog,$(SDIR)/clog,$(wildcard $(isp_mod_dir)/common/clog/*.c))
ISP_SRCS += $(subst $(isp_dir)/src,$(SDIR)/isp,$(wildcard $(isp_dir)/src/*.c))
ISP_SRCS += $(subst $(isp_bin_dir)/src,$(SDIR)/isp_bin,$(isp_bin_dir)/src/isp_3a_bin.c $(isp_bin_dir)/src/isp_bin.c)

# 3rdparty lib
BIN_INCLUDE_PATH = $(PWD)/../bin/prebuilt/include

ifneq ($(findstring $(CHIP_ARCH), MARS PHOBOS CV181X CV180X SG200X),)
ISP_BIN_DIR = $(MW_PATH)/modules/isp/$(shell echo $(CHIP_ARCH) | tr A-Z a-z)/isp_bin/inc
else
$(error UNKNOWN chip architecture - $(CHIP_ARCH))
endif

INCS = -I$(MW_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VPU_INC) -I$(ISP_INC) -Iinclude
INCS += -I$(SYS_INC) -I$(BIN_INCLUDE_PATH)/cvi_json-c -I$(ISP_BIN_DIR) -I$(BIN_INCLUDE_PATH)/cvi_miniz
INCS += -I$(isp_mod_dir)/algo \
	-I$(isp_mod_dir)/common/clog \
	-I$(isp_mod_dir)/$(isp_chip_dir)/isp/inc \
	-I$(isp_mod_dir)/$(isp_chip_dir)/isp_algo/inc \
	-I$(bin_mod_dir)/include \
	-I$(isp_bin_dir)/inc
BIN_OBJS = $(BIN_SRCS:.c=.o)
BIN_DEPS = $(BIN_SRCS:.c=.d)
ISP_OBJS = $(ISP_SRCS:.c=.o)
ISP_DEPS = $(ISP_SRCS:.c=.d)

BIN_TARGET_A = $(MW_LIB)/libcvi_bin_light.a
BIN_TARGET_SO = $(MW_LIB)/libcvi_bin_light.so
ISP_TARGET_A = $(MW_LIB)/libisp_light.a
ISP_TARGET_SO = $(MW_LIB)/libisp_light.so

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
EXTRA_CFLAGS += -DDISABLE_PQBIN_JSON -DENABLE_ISP_C906L -DISP_LIGHT

.PHONY : clean all
all : $(BIN_TARGET_A) $(BIN_TARGET_SO) $(ISP_TARGET_A) $(ISP_TARGET_SO)


$(SDIR)/bin/%.o: $(bin_mod_dir)/src/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(SDIR)/clog/%.o: $(isp_mod_dir)/common/clog/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(SDIR)/isp/%.o: $(isp_dir)/src/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(SDIR)/isp_bin/%.o: $(isp_bin_dir)/src/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

bin_dirs:
	@mkdir -p $(SDIR)/bin

isp_dirs:
	@mkdir -p $(SDIR)/clog
	@mkdir -p $(SDIR)/isp
	@mkdir -p $(SDIR)/isp_bin

$(BIN_TARGET_A): bin_dirs $(BIN_OBJS)
	@$(AR) $(ARFLAGS) $(BIN_TARGET_A) $(BIN_OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(BIN_TARGET_A))
	@echo "$$AR_MRI" | $(AR) -M

$(BIN_TARGET_SO): bin_dirs $(BIN_OBJS)
	@$(LD) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $(BIN_TARGET_SO) --start-group $(BIN_OBJS) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(BIN_TARGET_SO))
	@$(STRIP) $(BIN_TARGET_SO)

$(ISP_TARGET_A): isp_dirs $(ISP_OBJS)
	@$(AR) $(ARFLAGS) $(ISP_TARGET_A) $(ISP_OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(ISP_TARGET_A))
	@echo "$$AR_MRI" | $(AR) -M

$(ISP_TARGET_SO): isp_dirs $(ISP_OBJS)
	@$(LD) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $(ISP_TARGET_SO) --start-group $(ISP_OBJS) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(ISP_TARGET_SO))
	@$(STRIP) $(ISP_TARGET_SO)

clean:
	@rm -f $(BIN_OBJS) $(BIN_DEPS) $(BIN_TARGET_A) $(BIN_TARGET_SO)
	@rm -f $(ISP_OBJS) $(ISP_DEPS) $(ISP_TARGET_A) $(ISP_TARGET_SO)

-include $(DEPS)
