/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "media/engine/internal_encoder_factory.h"

#include <string>

#include "absl/strings/match.h"
#include "api/video_codecs/sdp_video_format.h"
#include "media/base/codec.h"
#include "media/base/media_constants.h"
#include "modules/video_coding/codecs/av1/libaom_av1_encoder.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "rtc_base/logging.h"

namespace webrtc {

std::vector<SdpVideoFormat> InternalEncoderFactory::SupportedFormats() {
  std::vector<SdpVideoFormat> supported_codecs;
  for (const webrtc::SdpVideoFormat& format : webrtc::SupportedH264Codecs()) {
    RTC_LOG(LS_INFO) << "Supported H264 codec: " << format.name;
    supported_codecs.push_back(format);
  }
  for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs()) {
    RTC_LOG(LS_INFO) << "Supported VP9 codec: " << format.name;
    supported_codecs.push_back(format);
  }
  RTC_LOG(LS_INFO) << "Supported VP8 codec: " << cricket::kVp8CodecName;
  supported_codecs.push_back(SdpVideoFormat(cricket::kVp8CodecName));
  if (kIsLibaomAv1EncoderSupported) {
    RTC_LOG(LS_INFO) << "Supported AV1 codec: " << cricket::kAv1CodecName;
    supported_codecs.push_back(SdpVideoFormat(cricket::kAv1CodecName));
  }
  return supported_codecs;
}

std::vector<SdpVideoFormat> InternalEncoderFactory::GetSupportedFormats()
    const {
  return SupportedFormats();
}

VideoEncoderFactory::CodecInfo InternalEncoderFactory::QueryVideoEncoder(
    const SdpVideoFormat& format) const {
  CodecInfo info;
  info.is_hardware_accelerated = false;
  info.has_internal_source = false;
  return info;
}

std::unique_ptr<VideoEncoder> InternalEncoderFactory::CreateVideoEncoder(
    const SdpVideoFormat& format) {
  if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
    return VP8Encoder::Create();
  if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
    return VP9Encoder::Create(cricket::VideoCodec(format));
  if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName))
    return H264Encoder::Create(cricket::VideoCodec(format));
  if (kIsLibaomAv1EncoderSupported &&
      absl::EqualsIgnoreCase(format.name, cricket::kAv1CodecName))
    return CreateLibaomAv1Encoder();
  RTC_LOG(LS_ERROR) << "Trying to created encoder of unsupported format "
                    << format.name;
  return nullptr;
}

}  // namespace webrtc
