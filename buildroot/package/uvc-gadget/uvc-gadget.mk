################################################################################
#
# uvc-gadget
#
################################################################################

UVC_GADGET_VERSION = 74522b25f982204c244357ca982a281d68352976
UVC_GADGET_SITE = $(call github,wlhe,uvc-gadget,$(UVC_GADGET_VERSION))

UVC_GADGET_MAKE_ENV += \
	CC="$(TARGET_CC)" \
	CXX="$(TARGET_CXX)" \
	CFLAGS="$(TARGET_CFLAGS) -I$(LINUX_DIR)/include -I$(LINUX_DIR)/arch/$(ARCH)/include" \
	CXXFLAGS="$(TARGET_CXXFLAGS) -I$(LINUX_DIR)/include -I$(LINUX_DIR)/arch/$(ARCH)/include" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	$(TARGET_MAKE_ENV) \
	KERNEL_DIR=$(LINUX_DIR) \
	ARCH=$(KERNEL_ARCH) \
	CROSS_COMPILE="$(TARGET_CROSS)"

define UVC_GADGET_BUILD_CMDS
	$(UVC_GADGET_MAKE_ENV) $(BR2_MAKE) -C $(@D)
endef

define UVC_GADGET_INSTALL_TARGET_CMDS
	mkdir -pv $(TARGET_DIR)/usr/bin/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/uvc-gadget $(TARGET_DIR)/usr/bin/
	mkdir -pv $(TARGET_DIR)/etc/init.d/
	cp -p ${@D}/uvc-gadget $(TARGET_DIR)/etc/init.d/uvc-gadget-server.elf
endef

$(eval $(generic-package))
