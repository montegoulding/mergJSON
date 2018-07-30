LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-16

LOCAL_MODULE := $(EXTERNAL_NAME)

LOCAL_SRC_FILES := mergJSON.c external.c jansson/src/dump.c jansson/src/error.c jansson/src/hashtable.c jansson/src/hashtable_seed.c jansson/src/load.c jansson/src/memory.c jansson/src/pack_unpack.c jansson/src/strbuffer.c jansson/src/strconv.c jansson/src/utf.c jansson/src/value.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/jansson/src \

LOCAL_LDFLAGS += -latomic -Wl,-u,getXtable

include $(BUILD_SHARED_LIBRARY)