# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../../android-webrtc.mk

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libwebrtc_h264
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
	h264_record.cpp \
    H264_dec.cc \
    h264.cc

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    '-D__STDC_CONSTANT_MACROS'

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/../interface \
    $(LOCAL_PATH)/../../.. \
    $(LOCAL_PATH)/../../../interface \
    $(LOCAL_PATH)/../../main/include \
    $(LOCAL_PATH)/../../main/source/utility/include \
    $(LOCAL_PATH)/../../main/source/utility \
    $(LOCAL_PATH)/../../../common_video/source \
    $(LOCAL_PATH)/../../../common_video/interface \
    $(LOCAL_PATH)/../../../common_video/source/libyuv/include \
    $(LOCAL_PATH)/../../../../system_wrappers/interface \
    $(LOCAL_PATH)/../../../../module/videojpegyuv/include \
    $(LOCAL_PATH)/../../../../third_party/libx264/libx264_android/include \
	$(LOCAL_PATH)/../../../../third_party/ffmpeg/ffmpeg-android-bin/include \
    $(LOCAL_PATH)/include
        
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
