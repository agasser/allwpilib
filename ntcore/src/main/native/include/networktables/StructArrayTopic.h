// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <stdint.h>

#include <atomic>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <wpi/SmallVector.h>
#include <wpi/json_fwd.h>
#include <wpi/mutex.h>
#include <wpi/struct/Struct.h>

#include "networktables/NetworkTableInstance.h"
#include "networktables/Topic.h"
#include "ntcore_cpp.h"

namespace nt {

template <wpi::StructSerializable T>
class StructArrayTopic;

/**
 * NetworkTables struct-encoded value array subscriber.
 */
template <wpi::StructSerializable T>
class StructArraySubscriber : public Subscriber {
  using S = wpi::Struct<T>;

 public:
  using TopicType = StructArrayTopic<T>;
  using ValueType = std::vector<T>;
  using ParamType = std::span<const T>;
  using TimestampedValueType = Timestamped<ValueType>;

  StructArraySubscriber() = default;

  /**
   * Construct from a subscriber handle; recommended to use
   * StructTopic::Subscribe() instead.
   *
   * @param handle Native handle
   * @param defaultValue Default value
   */
  template <typename U>
#if __cpp_lib_ranges >= 201911L
    requires std::ranges::range<U> &&
                 std::convertible_to<std::ranges::range_value_t<U>, T>
#endif
  StructArraySubscriber(NT_Subscriber handle, U&& defaultValue)
      : Subscriber{handle},
        m_defaultValue{defaultValue.begin(), defaultValue.end()} {
  }

  /**
   * Get the last published value.
   * If no value has been published or the value cannot be unpacked, returns the
   * stored default value.
   *
   * @return value
   */
  ValueType Get() const { return Get(m_defaultValue); }

  /**
   * Get the last published value.
   * If no value has been published or the value cannot be unpacked, returns the
   * passed defaultValue.
   *
   * @param defaultValue default value to return if no value has been published
   * @return value
   */
  template <typename U>
#if __cpp_lib_ranges >= 201911L
    requires std::ranges::range<U> &&
             std::convertible_to<std::ranges::range_value_t<U>, T>
#endif
  ValueType Get(U&& defaultValue) const {
    return GetAtomic(std::forward<U>(defaultValue)).value;
  }

  /**
   * Get the last published value.
   * If no value has been published or the value cannot be unpacked, returns the
   * passed defaultValue.
   *
   * @param defaultValue default value to return if no value has been published
   * @return value
   */
  ValueType Get(std::span<const T> defaultValue) const {
    return GetAtomic(defaultValue).value;
  }

  /**
   * Get the last published value along with its timestamp
   * If no value has been published or the value cannot be unpacked, returns the
   * stored default value and a timestamp of 0.
   *
   * @return timestamped value
   */
  TimestampedValueType GetAtomic() const { return GetAtomic(m_defaultValue); }

  /**
   * Get the last published value along with its timestamp.
   * If no value has been published or the value cannot be unpacked, returns the
   * passed defaultValue and a timestamp of 0.
   *
   * @param defaultValue default value to return if no value has been published
   * @return timestamped value
   */
  template <typename U>
#if __cpp_lib_ranges >= 201911L
    requires std::ranges::range<U> &&
             std::convertible_to<std::ranges::range_value_t<U>, T>
#endif
  TimestampedValueType GetAtomic(U&& defaultValue) const {
    wpi::SmallVector<uint8_t, 128> buf;
    TimestampedRawView view = ::nt::GetAtomicRaw(m_subHandle, buf, {});
    if (view.value.size() == 0 || (view.value.size() % S::kSize) != 0) {
      return {0, 0, std::forward<U>(defaultValue)};
    }
    TimestampedValueType rv{view.time, view.serverTime, {}};
    rv.value.reserve(view.value.size() / S::kSize);
    for (auto in = view.value.begin(), end = view.value.end(); in != end;
         in += S::kSize) {
      rv.value.emplace_back(
          S::Unpack(std::span<const uint8_t, S::kSize>{in, in + S::kSize}));
    }
    return rv;
  }

  /**
   * Get the last published value along with its timestamp.
   * If no value has been published or the value cannot be unpacked, returns the
   * passed defaultValue and a timestamp of 0.
   *
   * @param defaultValue default value to return if no value has been published
   * @return timestamped value
   */
  TimestampedValueType GetAtomic(std::span<const T> defaultValue) const {
    wpi::SmallVector<uint8_t, 128> buf;
    TimestampedRawView view = ::nt::GetAtomicRaw(m_subHandle, buf, {});
    if (view.value.size() == 0 || (view.value.size() % S::kSize) != 0) {
      return {0, 0, {defaultValue.begin(), defaultValue.end()}};
    }
    TimestampedValueType rv{view.time, view.serverTime, {}};
    rv.value.reserve(view.value.size() / S::kSize);
    for (auto in = view.value.begin(), end = view.value.end(); in != end;
         in += S::kSize) {
      rv.value.emplace_back(
          S::Unpack(std::span<const uint8_t, S::kSize>{in, in + S::kSize}));
    }
    return rv;
  }

  /**
   * Get an array of all valid value changes since the last call to ReadQueue.
   * Also provides a timestamp for each value. Values that cannot be unpacked
   * are dropped.
   *
   * @note The "poll storage" subscribe option can be used to set the queue
   *     depth.
   *
   * @return Array of timestamped values; empty array if no valid new changes
   *     have been published since the previous call.
   */
  std::vector<TimestampedValueType> ReadQueue() {
    auto raw = ::nt::ReadQueueRaw(m_subHandle);
    std::vector<TimestampedValueType> rv;
    rv.reserve(raw.size());
    for (auto&& r : raw) {
      if (r.value.size() == 0 || (r.value.size() % S::kSize) != 0) {
        continue;
      }
      std::vector<T> values;
      values.reserve(r.value.size() / S::kSize);
      for (auto in = r.value.begin(), end = r.value.end(); in != end;
           in += S::kSize) {
        values.emplace_back(
            S::Unpack(std::span<const uint8_t, S::kSize>{in, in + S::kSize}));
      }
      rv.emplace_back(r.time, r.serverTime, std::move(values));
    }
    return rv;
  }

  /**
   * Get the corresponding topic.
   *
   * @return Topic
   */
  TopicType GetTopic() const {
    return StructArrayTopic<T>{::nt::GetTopicFromHandle(m_subHandle)};
  }

 private:
  ValueType m_defaultValue;
};

/**
 * NetworkTables struct-encoded value array publisher.
 */
template <wpi::StructSerializable T>
class StructArrayPublisher : public Publisher {
  using S = wpi::Struct<T>;

 public:
  using TopicType = StructArrayTopic<T>;
  using ValueType = std::vector<T>;
  using ParamType = std::span<const T>;

  using TimestampedValueType = Timestamped<ValueType>;

  StructArrayPublisher() = default;

  /**
   * Construct from a publisher handle; recommended to use
   * StructTopic::Publish() instead.
   *
   * @param handle Native handle
   */
  explicit StructArrayPublisher(NT_Publisher handle) : Publisher{handle} {}

  StructArrayPublisher(const StructArrayPublisher&) = delete;
  StructArrayPublisher& operator=(const StructArrayPublisher&) = delete;

  StructArrayPublisher(StructArrayPublisher&& rhs)
      : Publisher{std::move(rhs)},
        m_buf{std::move(rhs.m_buf)},
        m_schemaPublished{rhs.m_schemaPublished} {}

  StructArrayPublisher& operator=(StructArrayPublisher&& rhs) {
    Publisher::operator=(std::move(rhs));
    m_buf = std::move(rhs.m_buf);
    m_schemaPublished.clear();
    if (rhs.m_schemaPublished.test()) {
      m_schemaPublished.test_and_set();
    }
    return *this;
  }

  /**
   * Publish a new value.
   *
   * @param value value to publish
   * @param time timestamp; 0 indicates current NT time should be used
   */
  template <typename U>
#if __cpp_lib_ranges >= 201911L
    requires std::ranges::range<U> &&
             std::convertible_to<std::ranges::range_value_t<U>, T>
#endif
  void Set(U&& value, int64_t time = 0) {
    if (!m_schemaPublished.test_and_set()) {
      GetTopic().GetInstance().template AddStructSchema<T>();
    }
    m_buf.Write(std::forward<U>(value),
                [&](auto bytes) { ::nt::SetRaw(m_pubHandle, bytes, time); });
  }

  /**
   * Publish a new value.
   *
   * @param value value to publish
   * @param time timestamp; 0 indicates current NT time should be used
   */
  void Set(std::span<const T> value, int64_t time = 0) {
    m_buf.Write(value,
                [&](auto bytes) { ::nt::SetRaw(m_pubHandle, bytes, time); });
  }

  /**
   * Publish a default value.
   * On reconnect, a default value will never be used in preference to a
   * published value.
   *
   * @param value value
   */
  template <typename U>
#if __cpp_lib_ranges >= 201911L
    requires std::ranges::range<U> &&
             std::convertible_to<std::ranges::range_value_t<U>, T>
#endif
  void SetDefault(U&& value) {
    if (!m_schemaPublished.test_and_set()) {
      GetTopic().GetInstance().template AddStructSchema<T>();
    }
    m_buf.Write(std::forward<U>(value),
                [&](auto bytes) { ::nt::SetDefaultRaw(m_pubHandle, bytes); });
  }

  /**
   * Publish a default value.
   * On reconnect, a default value will never be used in preference to a
   * published value.
   *
   * @param value value
   */
  void SetDefault(std::span<const T> value) {
    m_buf.Write(value,
                [&](auto bytes) { ::nt::SetDefaultRaw(m_pubHandle, bytes); });
  }

  /**
   * Get the corresponding topic.
   *
   * @return Topic
   */
  TopicType GetTopic() const {
    return StructArrayTopic<T>{::nt::GetTopicFromHandle(m_pubHandle)};
  }

 private:
  wpi::StructArrayBuffer<T> m_buf;
  std::atomic_flag m_schemaPublished = ATOMIC_FLAG_INIT;
};

/**
 * NetworkTables struct-encoded value array entry.
 *
 * @note Unlike NetworkTableEntry, the entry goes away when this is destroyed.
 */
template <wpi::StructSerializable T>
class StructArrayEntry final : public StructArraySubscriber<T>,
                               public StructArrayPublisher<T> {
 public:
  using SubscriberType = StructArraySubscriber<T>;
  using PublisherType = StructArrayPublisher<T>;
  using TopicType = StructArrayTopic<T>;
  using ValueType = std::vector<T>;
  using ParamType = std::span<const T>;

  using TimestampedValueType = Timestamped<ValueType>;

  StructArrayEntry() = default;

  /**
   * Construct from an entry handle; recommended to use
   * StructTopic::GetEntry() instead.
   *
   * @param handle Native handle
   * @param defaultValue Default value
   */
  template <typename U>
#if __cpp_lib_ranges >= 201911L
    requires std::ranges::range<U> &&
                 std::convertible_to<std::ranges::range_value_t<U>, T>
#endif
  StructArrayEntry(NT_Entry handle, U&& defaultValue)
      : StructArraySubscriber<T>{handle, defaultValue},
        StructArrayPublisher<T>{handle} {
  }

  /**
   * Determines if the native handle is valid.
   *
   * @return True if the native handle is valid, false otherwise.
   */
  explicit operator bool() const { return this->m_subHandle != 0; }

  /**
   * Gets the native handle for the entry.
   *
   * @return Native handle
   */
  NT_Entry GetHandle() const { return this->m_subHandle; }

  /**
   * Get the corresponding topic.
   *
   * @return Topic
   */
  TopicType GetTopic() const {
    return StructArrayTopic<T>{::nt::GetTopicFromHandle(this->m_subHandle)};
  }

  /**
   * Stops publishing the entry if it's published.
   */
  void Unpublish() { ::nt::Unpublish(this->m_pubHandle); }
};

/**
 * NetworkTables struct-encoded value array topic.
 */
template <wpi::StructSerializable T>
class StructArrayTopic final : public Topic {
 public:
  using SubscriberType = StructArraySubscriber<T>;
  using PublisherType = StructArrayPublisher<T>;
  using EntryType = StructArrayEntry<T>;
  using ValueType = std::vector<T>;
  using ParamType = std::span<const T>;
  using TimestampedValueType = Timestamped<ValueType>;

  StructArrayTopic() = default;

  /**
   * Construct from a topic handle; recommended to use
   * NetworkTableInstance::GetStructTopic() instead.
   *
   * @param handle Native handle
   */
  explicit StructArrayTopic(NT_Topic handle) : Topic{handle} {}

  /**
   * Construct from a generic topic.
   *
   * @param topic Topic
   */
  explicit StructArrayTopic(Topic topic) : Topic{topic} {}

  /**
   * Create a new subscriber to the topic.
   *
   * <p>The subscriber is only active as long as the returned object
   * is not destroyed.
   *
   * @note Subscribers that do not match the published data type do not return
   *     any values. To determine if the data type matches, use the appropriate
   *     Topic functions.
   *
   * @param defaultValue default value used when a default is not provided to a
   *        getter function
   * @param options subscribe options
   * @return subscriber
   */
  template <typename U>
#if __cpp_lib_ranges >= 201911L
    requires std::ranges::range<U> &&
             std::convertible_to<std::ranges::range_value_t<U>, T>
#endif
  [[nodiscard]]
  SubscriberType Subscribe(
      U&& defaultValue, const PubSubOptions& options = kDefaultPubSubOptions) {
    return StructArraySubscriber<T>{
        ::nt::Subscribe(
            m_handle, NT_RAW,
            wpi::MakeStructArrayTypeString<T, std::dynamic_extent>(), options),
        defaultValue};
  }

  /**
   * Create a new subscriber to the topic.
   *
   * <p>The subscriber is only active as long as the returned object
   * is not destroyed.
   *
   * @note Subscribers that do not match the published data type do not return
   *     any values. To determine if the data type matches, use the appropriate
   *     Topic functions.
   *
   * @param defaultValue default value used when a default is not provided to a
   *        getter function
   * @param options subscribe options
   * @return subscriber
   */
  [[nodiscard]]
  SubscriberType Subscribe(
      std::span<const T> defaultValue,
      const PubSubOptions& options = kDefaultPubSubOptions) {
    return StructArraySubscriber<T>{
        ::nt::Subscribe(
            m_handle, NT_RAW,
            wpi::MakeStructArrayTypeString<T, std::dynamic_extent>(), options),
        defaultValue};
  }

  /**
   * Create a new publisher to the topic.
   *
   * The publisher is only active as long as the returned object
   * is not destroyed.
   *
   * @note It is not possible to publish two different data types to the same
   *     topic. Conflicts between publishers are typically resolved by the
   *     server on a first-come, first-served basis. Any published values that
   *     do not match the topic's data type are dropped (ignored). To determine
   *     if the data type matches, use the appropriate Topic functions.
   *
   * @param options publish options
   * @return publisher
   */
  [[nodiscard]]
  PublisherType Publish(const PubSubOptions& options = kDefaultPubSubOptions) {
    return StructArrayPublisher<T>{::nt::Publish(
        m_handle, NT_RAW,
        wpi::MakeStructArrayTypeString<T, std::dynamic_extent>(), options)};
  }

  /**
   * Create a new publisher to the topic, with type string and initial
   * properties.
   *
   * The publisher is only active as long as the returned object
   * is not destroyed.
   *
   * @note It is not possible to publish two different data types to the same
   *     topic. Conflicts between publishers are typically resolved by the
   *     server on a first-come, first-served basis. Any published values that
   *     do not match the topic's data type are dropped (ignored). To determine
   *     if the data type matches, use the appropriate Topic functions.
   *
   * @param properties JSON properties
   * @param options publish options
   * @return publisher
   */
  [[nodiscard]]
  PublisherType PublishEx(
      const wpi::json& properties,
      const PubSubOptions& options = kDefaultPubSubOptions) {
    return StructArrayPublisher<T>{::nt::PublishEx(
        m_handle, NT_RAW,
        wpi::MakeStructArrayTypeString<T, std::dynamic_extent>(), properties,
        options)};
  }

  /**
   * Create a new entry for the topic.
   *
   * Entries act as a combination of a subscriber and a weak publisher. The
   * subscriber is active as long as the entry is not destroyed. The publisher
   * is created when the entry is first written to, and remains active until
   * either Unpublish() is called or the entry is destroyed.
   *
   * @note It is not possible to use two different data types with the same
   *     topic. Conflicts between publishers are typically resolved by the
   *     server on a first-come, first-served basis. Any published values that
   *     do not match the topic's data type are dropped (ignored), and the entry
   *     will show no new values if the data type does not match. To determine
   *     if the data type matches, use the appropriate Topic functions.
   *
   * @param defaultValue default value used when a default is not provided to a
   *        getter function
   * @param options publish and/or subscribe options
   * @return entry
   */
  template <typename U>
#if __cpp_lib_ranges >= 201911L
    requires std::ranges::range<U> &&
             std::convertible_to<std::ranges::range_value_t<U>, T>
#endif
  [[nodiscard]]
  EntryType GetEntry(U&& defaultValue,
                     const PubSubOptions& options = kDefaultPubSubOptions) {
    return StructArrayEntry<T>{
        ::nt::GetEntry(m_handle, NT_RAW,
                       wpi::MakeStructArrayTypeString<T, std::dynamic_extent>(),
                       options),
        defaultValue};
  }

  /**
   * Create a new entry for the topic.
   *
   * Entries act as a combination of a subscriber and a weak publisher. The
   * subscriber is active as long as the entry is not destroyed. The publisher
   * is created when the entry is first written to, and remains active until
   * either Unpublish() is called or the entry is destroyed.
   *
   * @note It is not possible to use two different data types with the same
   *     topic. Conflicts between publishers are typically resolved by the
   *     server on a first-come, first-served basis. Any published values that
   *     do not match the topic's data type are dropped (ignored), and the entry
   *     will show no new values if the data type does not match. To determine
   *     if the data type matches, use the appropriate Topic functions.
   *
   * @param defaultValue default value used when a default is not provided to a
   *        getter function
   * @param options publish and/or subscribe options
   * @return entry
   */
  [[nodiscard]]
  EntryType GetEntry(std::span<const T> defaultValue,
                     const PubSubOptions& options = kDefaultPubSubOptions) {
    return StructArrayEntry<T>{
        ::nt::GetEntry(m_handle, NT_RAW,
                       wpi::MakeStructArrayTypeString<T, std::dynamic_extent>(),
                       options),
        defaultValue};
  }
};

}  // namespace nt
