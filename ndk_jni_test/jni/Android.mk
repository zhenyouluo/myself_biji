LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    :=1 
LOCAL_SRC_FILES :=1.c 

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_EXECUTABLE)

