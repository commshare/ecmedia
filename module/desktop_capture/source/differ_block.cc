/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "differ_block.h"

#include <string.h>

//#include "build/build_config.h"
#include "differ_block_sse2.h"
#include "../system_wrappers/include/cpu_features_wrapper.h"

namespace yuntongxunwebrtc {

int BlockDifference_C(const uint8_t* image1,
                      const uint8_t* image2,
                      int stride) {
  int width_bytes = kBlockSize * kBytesPerPixel;

  for (int y = 0; y < kBlockSize; y++) {
    if (memcmp(image1, image2, width_bytes) != 0)
      return 1;
    image1 += stride;
    image2 += stride;
  }
  return 0;
}

int BlockDifference(const uint8_t* image1, const uint8_t* image2, int stride) {
  static int (*diff_proc)(const uint8_t*, const uint8_t*, int) = NULL;

  if (!diff_proc) {
#if defined(ARCH_CPU_ARM_FAMILY) || defined(ARCH_CPU_MIPS_FAMILY)
    // For ARM and MIPS processors, always use C version.
    // TODO(hclam): Implement a NEON version.
    diff_proc = &BlockDifference_C;
#else
    bool have_sse2 = WebRtc_GetCPUInfo(kSSE2) != 0;
    // For x86 processors, check if SSE2 is supported.
    if (have_sse2 && kBlockSize == 32) {
      diff_proc = &BlockDifference_SSE2_W32;
    } else if (have_sse2 && kBlockSize == 16) {
      diff_proc = &BlockDifference_SSE2_W16;
    } else {
      diff_proc = &BlockDifference_C;
    }
#endif
  }

  return diff_proc(image1, image2, stride);
}

}  // namespace yuntongxunwebrtc