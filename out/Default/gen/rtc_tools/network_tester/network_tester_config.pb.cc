// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: network_tester_config.proto

#include "network_tester_config.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
// This is a temporary google only hack
#ifdef GOOGLE_PROTOBUF_ENFORCE_UNIQUENESS
#include "third_party/protobuf/version.h"
#endif
// @@protoc_insertion_point(includes)

namespace protobuf_network_5ftester_5fconfig_2eproto {
extern PROTOBUF_INTERNAL_EXPORT_protobuf_network_5ftester_5fconfig_2eproto ::google::protobuf::internal::SCCInfo<0> scc_info_NetworkTesterConfig;
}  // namespace protobuf_network_5ftester_5fconfig_2eproto
namespace webrtc {
namespace network_tester {
namespace config {
class NetworkTesterConfigDefaultTypeInternal {
 public:
  ::google::protobuf::internal::ExplicitlyConstructed<NetworkTesterConfig>
      _instance;
} _NetworkTesterConfig_default_instance_;
class NetworkTesterAllConfigsDefaultTypeInternal {
 public:
  ::google::protobuf::internal::ExplicitlyConstructed<NetworkTesterAllConfigs>
      _instance;
} _NetworkTesterAllConfigs_default_instance_;
}  // namespace config
}  // namespace network_tester
}  // namespace webrtc
namespace protobuf_network_5ftester_5fconfig_2eproto {
static void InitDefaultsNetworkTesterConfig() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::webrtc::network_tester::config::_NetworkTesterConfig_default_instance_;
    new (ptr) ::webrtc::network_tester::config::NetworkTesterConfig();
    ::google::protobuf::internal::OnShutdownDestroyMessage(ptr);
  }
  ::webrtc::network_tester::config::NetworkTesterConfig::InitAsDefaultInstance();
}

::google::protobuf::internal::SCCInfo<0> scc_info_NetworkTesterConfig =
    {{ATOMIC_VAR_INIT(::google::protobuf::internal::SCCInfoBase::kUninitialized), 0, InitDefaultsNetworkTesterConfig}, {}};

static void InitDefaultsNetworkTesterAllConfigs() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::webrtc::network_tester::config::_NetworkTesterAllConfigs_default_instance_;
    new (ptr) ::webrtc::network_tester::config::NetworkTesterAllConfigs();
    ::google::protobuf::internal::OnShutdownDestroyMessage(ptr);
  }
  ::webrtc::network_tester::config::NetworkTesterAllConfigs::InitAsDefaultInstance();
}

::google::protobuf::internal::SCCInfo<1> scc_info_NetworkTesterAllConfigs =
    {{ATOMIC_VAR_INIT(::google::protobuf::internal::SCCInfoBase::kUninitialized), 1, InitDefaultsNetworkTesterAllConfigs}, {
      &protobuf_network_5ftester_5fconfig_2eproto::scc_info_NetworkTesterConfig.base,}};

void InitDefaults() {
  ::google::protobuf::internal::InitSCC(&scc_info_NetworkTesterConfig.base);
  ::google::protobuf::internal::InitSCC(&scc_info_NetworkTesterAllConfigs.base);
}

}  // namespace protobuf_network_5ftester_5fconfig_2eproto
namespace webrtc {
namespace network_tester {
namespace config {

// ===================================================================

void NetworkTesterConfig::InitAsDefaultInstance() {
}
#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int NetworkTesterConfig::kPacketSendIntervalMsFieldNumber;
const int NetworkTesterConfig::kPacketSizeFieldNumber;
const int NetworkTesterConfig::kExecutionTimeMsFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

NetworkTesterConfig::NetworkTesterConfig()
  : ::google::protobuf::MessageLite(), _internal_metadata_(NULL) {
  ::google::protobuf::internal::InitSCC(
      &protobuf_network_5ftester_5fconfig_2eproto::scc_info_NetworkTesterConfig.base);
  SharedCtor();
  // @@protoc_insertion_point(constructor:webrtc.network_tester.config.NetworkTesterConfig)
}
NetworkTesterConfig::NetworkTesterConfig(const NetworkTesterConfig& from)
  : ::google::protobuf::MessageLite(),
      _internal_metadata_(NULL),
      _has_bits_(from._has_bits_) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::memcpy(&packet_send_interval_ms_, &from.packet_send_interval_ms_,
    static_cast<size_t>(reinterpret_cast<char*>(&execution_time_ms_) -
    reinterpret_cast<char*>(&packet_send_interval_ms_)) + sizeof(execution_time_ms_));
  // @@protoc_insertion_point(copy_constructor:webrtc.network_tester.config.NetworkTesterConfig)
}

void NetworkTesterConfig::SharedCtor() {
  ::memset(&packet_send_interval_ms_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&execution_time_ms_) -
      reinterpret_cast<char*>(&packet_send_interval_ms_)) + sizeof(execution_time_ms_));
}

NetworkTesterConfig::~NetworkTesterConfig() {
  // @@protoc_insertion_point(destructor:webrtc.network_tester.config.NetworkTesterConfig)
  SharedDtor();
}

void NetworkTesterConfig::SharedDtor() {
}

void NetworkTesterConfig::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const NetworkTesterConfig& NetworkTesterConfig::default_instance() {
  ::google::protobuf::internal::InitSCC(&protobuf_network_5ftester_5fconfig_2eproto::scc_info_NetworkTesterConfig.base);
  return *internal_default_instance();
}


void NetworkTesterConfig::Clear() {
// @@protoc_insertion_point(message_clear_start:webrtc.network_tester.config.NetworkTesterConfig)
  ::google::protobuf::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  cached_has_bits = _has_bits_[0];
  if (cached_has_bits & 7u) {
    ::memset(&packet_send_interval_ms_, 0, static_cast<size_t>(
        reinterpret_cast<char*>(&execution_time_ms_) -
        reinterpret_cast<char*>(&packet_send_interval_ms_)) + sizeof(execution_time_ms_));
  }
  _has_bits_.Clear();
  _internal_metadata_.Clear();
}

bool NetworkTesterConfig::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::internal::LiteUnknownFieldSetter unknown_fields_setter(
      &_internal_metadata_);
  ::google::protobuf::io::StringOutputStream unknown_fields_output(
      unknown_fields_setter.buffer());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_output, false);
  // @@protoc_insertion_point(parse_start:webrtc.network_tester.config.NetworkTesterConfig)
  for (;;) {
    ::std::pair<::google::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(127u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional int32 packet_send_interval_ms = 1;
      case 1: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(8u /* 8 & 0xFF */)) {
          set_has_packet_send_interval_ms();
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &packet_send_interval_ms_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // optional float packet_size = 2;
      case 2: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(21u /* 21 & 0xFF */)) {
          set_has_packet_size();
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &packet_size_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // optional int32 execution_time_ms = 3;
      case 3: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(24u /* 24 & 0xFF */)) {
          set_has_execution_time_ms();
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &execution_time_ms_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:webrtc.network_tester.config.NetworkTesterConfig)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:webrtc.network_tester.config.NetworkTesterConfig)
  return false;
#undef DO_
}

void NetworkTesterConfig::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:webrtc.network_tester.config.NetworkTesterConfig)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  cached_has_bits = _has_bits_[0];
  // optional int32 packet_send_interval_ms = 1;
  if (cached_has_bits & 0x00000001u) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(1, this->packet_send_interval_ms(), output);
  }

  // optional float packet_size = 2;
  if (cached_has_bits & 0x00000002u) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(2, this->packet_size(), output);
  }

  // optional int32 execution_time_ms = 3;
  if (cached_has_bits & 0x00000004u) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(3, this->execution_time_ms(), output);
  }

  output->WriteRaw(_internal_metadata_.unknown_fields().data(),
                   static_cast<int>(_internal_metadata_.unknown_fields().size()));
  // @@protoc_insertion_point(serialize_end:webrtc.network_tester.config.NetworkTesterConfig)
}

size_t NetworkTesterConfig::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:webrtc.network_tester.config.NetworkTesterConfig)
  size_t total_size = 0;

  total_size += _internal_metadata_.unknown_fields().size();

  if (_has_bits_[0 / 32] & 7u) {
    // optional int32 packet_send_interval_ms = 1;
    if (has_packet_send_interval_ms()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->packet_send_interval_ms());
    }

    // optional float packet_size = 2;
    if (has_packet_size()) {
      total_size += 1 + 4;
    }

    // optional int32 execution_time_ms = 3;
    if (has_execution_time_ms()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->execution_time_ms());
    }

  }
  int cached_size = ::google::protobuf::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void NetworkTesterConfig::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const NetworkTesterConfig*>(&from));
}

void NetworkTesterConfig::MergeFrom(const NetworkTesterConfig& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:webrtc.network_tester.config.NetworkTesterConfig)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  cached_has_bits = from._has_bits_[0];
  if (cached_has_bits & 7u) {
    if (cached_has_bits & 0x00000001u) {
      packet_send_interval_ms_ = from.packet_send_interval_ms_;
    }
    if (cached_has_bits & 0x00000002u) {
      packet_size_ = from.packet_size_;
    }
    if (cached_has_bits & 0x00000004u) {
      execution_time_ms_ = from.execution_time_ms_;
    }
    _has_bits_[0] |= cached_has_bits;
  }
}

void NetworkTesterConfig::CopyFrom(const NetworkTesterConfig& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:webrtc.network_tester.config.NetworkTesterConfig)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool NetworkTesterConfig::IsInitialized() const {
  return true;
}

void NetworkTesterConfig::Swap(NetworkTesterConfig* other) {
  if (other == this) return;
  InternalSwap(other);
}
void NetworkTesterConfig::InternalSwap(NetworkTesterConfig* other) {
  using std::swap;
  swap(packet_send_interval_ms_, other->packet_send_interval_ms_);
  swap(packet_size_, other->packet_size_);
  swap(execution_time_ms_, other->execution_time_ms_);
  swap(_has_bits_[0], other->_has_bits_[0]);
  _internal_metadata_.Swap(&other->_internal_metadata_);
}

::std::string NetworkTesterConfig::GetTypeName() const {
  return "webrtc.network_tester.config.NetworkTesterConfig";
}


// ===================================================================

void NetworkTesterAllConfigs::InitAsDefaultInstance() {
}
#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int NetworkTesterAllConfigs::kConfigsFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

NetworkTesterAllConfigs::NetworkTesterAllConfigs()
  : ::google::protobuf::MessageLite(), _internal_metadata_(NULL) {
  ::google::protobuf::internal::InitSCC(
      &protobuf_network_5ftester_5fconfig_2eproto::scc_info_NetworkTesterAllConfigs.base);
  SharedCtor();
  // @@protoc_insertion_point(constructor:webrtc.network_tester.config.NetworkTesterAllConfigs)
}
NetworkTesterAllConfigs::NetworkTesterAllConfigs(const NetworkTesterAllConfigs& from)
  : ::google::protobuf::MessageLite(),
      _internal_metadata_(NULL),
      _has_bits_(from._has_bits_),
      configs_(from.configs_) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  // @@protoc_insertion_point(copy_constructor:webrtc.network_tester.config.NetworkTesterAllConfigs)
}

void NetworkTesterAllConfigs::SharedCtor() {
}

NetworkTesterAllConfigs::~NetworkTesterAllConfigs() {
  // @@protoc_insertion_point(destructor:webrtc.network_tester.config.NetworkTesterAllConfigs)
  SharedDtor();
}

void NetworkTesterAllConfigs::SharedDtor() {
}

void NetworkTesterAllConfigs::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const NetworkTesterAllConfigs& NetworkTesterAllConfigs::default_instance() {
  ::google::protobuf::internal::InitSCC(&protobuf_network_5ftester_5fconfig_2eproto::scc_info_NetworkTesterAllConfigs.base);
  return *internal_default_instance();
}


void NetworkTesterAllConfigs::Clear() {
// @@protoc_insertion_point(message_clear_start:webrtc.network_tester.config.NetworkTesterAllConfigs)
  ::google::protobuf::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  configs_.Clear();
  _has_bits_.Clear();
  _internal_metadata_.Clear();
}

bool NetworkTesterAllConfigs::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::internal::LiteUnknownFieldSetter unknown_fields_setter(
      &_internal_metadata_);
  ::google::protobuf::io::StringOutputStream unknown_fields_output(
      unknown_fields_setter.buffer());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_output, false);
  // @@protoc_insertion_point(parse_start:webrtc.network_tester.config.NetworkTesterAllConfigs)
  for (;;) {
    ::std::pair<::google::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(127u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // repeated .webrtc.network_tester.config.NetworkTesterConfig configs = 1;
      case 1: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(10u /* 10 & 0xFF */)) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessage(
                input, add_configs()));
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:webrtc.network_tester.config.NetworkTesterAllConfigs)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:webrtc.network_tester.config.NetworkTesterAllConfigs)
  return false;
#undef DO_
}

void NetworkTesterAllConfigs::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:webrtc.network_tester.config.NetworkTesterAllConfigs)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // repeated .webrtc.network_tester.config.NetworkTesterConfig configs = 1;
  for (unsigned int i = 0,
      n = static_cast<unsigned int>(this->configs_size()); i < n; i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      1,
      this->configs(static_cast<int>(i)),
      output);
  }

  output->WriteRaw(_internal_metadata_.unknown_fields().data(),
                   static_cast<int>(_internal_metadata_.unknown_fields().size()));
  // @@protoc_insertion_point(serialize_end:webrtc.network_tester.config.NetworkTesterAllConfigs)
}

size_t NetworkTesterAllConfigs::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:webrtc.network_tester.config.NetworkTesterAllConfigs)
  size_t total_size = 0;

  total_size += _internal_metadata_.unknown_fields().size();

  // repeated .webrtc.network_tester.config.NetworkTesterConfig configs = 1;
  {
    unsigned int count = static_cast<unsigned int>(this->configs_size());
    total_size += 1UL * count;
    for (unsigned int i = 0; i < count; i++) {
      total_size +=
        ::google::protobuf::internal::WireFormatLite::MessageSize(
          this->configs(static_cast<int>(i)));
    }
  }

  int cached_size = ::google::protobuf::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void NetworkTesterAllConfigs::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const NetworkTesterAllConfigs*>(&from));
}

void NetworkTesterAllConfigs::MergeFrom(const NetworkTesterAllConfigs& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:webrtc.network_tester.config.NetworkTesterAllConfigs)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  configs_.MergeFrom(from.configs_);
}

void NetworkTesterAllConfigs::CopyFrom(const NetworkTesterAllConfigs& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:webrtc.network_tester.config.NetworkTesterAllConfigs)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool NetworkTesterAllConfigs::IsInitialized() const {
  return true;
}

void NetworkTesterAllConfigs::Swap(NetworkTesterAllConfigs* other) {
  if (other == this) return;
  InternalSwap(other);
}
void NetworkTesterAllConfigs::InternalSwap(NetworkTesterAllConfigs* other) {
  using std::swap;
  CastToBase(&configs_)->InternalSwap(CastToBase(&other->configs_));
  swap(_has_bits_[0], other->_has_bits_[0]);
  _internal_metadata_.Swap(&other->_internal_metadata_);
}

::std::string NetworkTesterAllConfigs::GetTypeName() const {
  return "webrtc.network_tester.config.NetworkTesterAllConfigs";
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace config
}  // namespace network_tester
}  // namespace webrtc
namespace google {
namespace protobuf {
template<> GOOGLE_PROTOBUF_ATTRIBUTE_NOINLINE ::webrtc::network_tester::config::NetworkTesterConfig* Arena::CreateMaybeMessage< ::webrtc::network_tester::config::NetworkTesterConfig >(Arena* arena) {
  return Arena::CreateInternal< ::webrtc::network_tester::config::NetworkTesterConfig >(arena);
}
template<> GOOGLE_PROTOBUF_ATTRIBUTE_NOINLINE ::webrtc::network_tester::config::NetworkTesterAllConfigs* Arena::CreateMaybeMessage< ::webrtc::network_tester::config::NetworkTesterAllConfigs >(Arena* arena) {
  return Arena::CreateInternal< ::webrtc::network_tester::config::NetworkTesterAllConfigs >(arena);
}
}  // namespace protobuf
}  // namespace google

// @@protoc_insertion_point(global_scope)