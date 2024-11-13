/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <mariana-trench/CallKind.h>
#include <mariana-trench/Method.h>
#include <mariana-trench/OriginSet.h>
#include <mariana-trench/PointerIntPair.h>
#include <mariana-trench/Position.h>

namespace marianatrench {

/**
 * Represents a "next hop" in a trace.
 *
 * The `callee_port` is the port to the next method in the trace, or
 * `Root::Kind::Leaf` for a leaf frame.
 *
 * `call_kind`. See `CallKind`.
 *
 * `callee` is the next method in the trace. This is `nullptr` for a leaf frame.
 *
 * `call_position` is the position of the call to the `callee`. This is
 * `nullptr` for a leaf frame. This can be non-null for leaf frames inside
 * issues, to describe the position of a parameter source or return sink.
 *
 */
class CallInfo final {
 private:
  using MethodCallKindPair =
      PointerIntPair<const Method*, 3, CallKind::Encoding>;

 public:
  explicit CallInfo(
      const Method* MT_NULLABLE callee,
      CallKind call_kind,
      const AccessPath* MT_NULLABLE callee_port,
      const Position* MT_NULLABLE call_position)
      : method_call_kind_(callee, call_kind.encode()),
        callee_port_(callee_port),
        call_position_(call_position) {
    if (callee != nullptr) {
      mt_assert(callee_port_ != nullptr);
      mt_assert(call_kind.is_callsite());
    }
  }

  INCLUDE_DEFAULT_COPY_CONSTRUCTORS_AND_ASSIGNMENTS(CallInfo)

  const Method* MT_NULLABLE callee() const {
    return method_call_kind_.get_pointer();
  }

  CallKind call_kind() const {
    return CallKind::decode(method_call_kind_.get_int());
  }

  const AccessPath* MT_NULLABLE callee_port() const {
    return callee_port_;
  }

  const Position* MT_NULLABLE call_position() const {
    return call_position_;
  }

  bool is_default() const {
    return callee() == nullptr && call_kind() == CallKind::declaration() &&
        callee_port() == nullptr && call_position() == nullptr;
  }

  static CallInfo make_default() {
    return CallInfo(nullptr, CallKind::declaration(), nullptr, nullptr);
  }

  bool is_leaf() const {
    return call_kind().is_origin();
  }

  CallInfo propagate(
      const Method* MT_NULLABLE callee,
      const AccessPath* MT_NULLABLE callee_port,
      const Position* call_position,
      Context& context) const;

  static CallInfo from_json(const Json::Value& value, Context& context);
  Json::Value to_json() const;

  friend bool operator==(const CallInfo& self, const CallInfo& other);

  friend bool operator<(const CallInfo& self, const CallInfo& other);

  friend struct std::hash<CallInfo>;

  friend std::ostream& operator<<(std::ostream& out, const CallInfo& call_info);

 private:
  MethodCallKindPair method_call_kind_;
  const AccessPath* MT_NULLABLE callee_port_;
  const Position* MT_NULLABLE call_position_;
};

} // namespace marianatrench

template <>
struct std::hash<marianatrench::CallInfo> {
  std::size_t operator()(const marianatrench::CallInfo& call_info) const {
    std::size_t seed = 0;
    boost::hash_combine(seed, call_info.method_call_kind_.encode());
    boost::hash_combine(seed, call_info.callee_port_);
    boost::hash_combine(seed, call_info.call_position_);
    return seed;
  }
};
