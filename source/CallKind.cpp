/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <algorithm>

#include <boost/algorithm/string/predicate.hpp>
#include <mariana-trench/Assert.h>
#include <mariana-trench/CallKind.h>
#include <mariana-trench/JsonValidation.h>

namespace marianatrench {

CallKind::CallKind(Encoding encoding) : encoding_(encoding) {}

CallKind CallKind::from_trace_string(std::string_view trace_string) {
  Encoding encoding = 0;

  size_t current_position = 0;
  if (boost::starts_with(trace_string, "PropagationWithTrace:")) {
    encoding |= CallKind::PropagationWithTrace;
    current_position += sizeof("PropagationWithTrace:") - 1;
  }

  auto call_kind = trace_string.substr(current_position);
  if (call_kind == "Declaration") {
    encoding |= CallKind::Declaration;
  } else if (call_kind == "Origin") {
    encoding |= CallKind::Origin;
  } else if (call_kind == "CallSite") {
    encoding |= CallKind::CallSite;
  } else if (call_kind == "Propagation") {
    encoding |= CallKind::Propagation;
    mt_assert(!(encoding & CallKind::PropagationWithTrace));
  } else {
    throw JsonValidationError(
        std::string(trace_string),
        /* field */ std::nullopt,
        "CallKind should be a [PropagationWithTrace:][Declaration|Origin|CallSite], or just Propagation");
  }

  return CallKind(encoding);
}

std::string CallKind::to_trace_string() const {
  std::string call_kind = "";

  if (is_propagation_with_trace()) {
    call_kind = "PropagationWithTrace:";
  }

  if (is_declaration()) {
    call_kind += "Declaration";
  } else if (is_origin()) {
    call_kind += "Origin";
  } else if (is_callsite()) {
    call_kind += "CallSite";
  } else {
    mt_assert(is_propagation_without_trace());
    call_kind += "Propagation";
  }

  return call_kind;
}

std::ostream& operator<<(std::ostream& out, const CallKind& call_kind) {
  return out << call_kind.to_trace_string();
}

CallKind CallKind::propagation_with_trace(CallKind::Encoding kind) {
  mt_assert(
      kind == CallKind::Declaration || kind == CallKind::Origin ||
      kind == CallKind::CallSite);

  return CallKind{CallKind::PropagationWithTrace | kind};
}

CallKind CallKind::decode(Encoding encoding) {
  mt_assert(
      ((encoding & (CallKind::PropagationWithTrace)) !=
       CallKind::PropagationWithTrace) ||
      ((encoding & (CallKind::Propagation)) != CallKind::Propagation));

  return CallKind(encoding);
}

bool CallKind::is_declaration() const {
  return (encoding_ & (~CallKind::PropagationWithTrace)) == Declaration;
}

bool CallKind::is_origin() const {
  return (encoding_ & (~CallKind::PropagationWithTrace)) == Origin;
}

bool CallKind::is_callsite() const {
  return (encoding_ & (~CallKind::PropagationWithTrace)) == CallSite;
}

bool CallKind::is_propagation() const {
  return encoding_ == Propagation ||
      (encoding_ & CallKind::PropagationWithTrace) == PropagationWithTrace;
}

bool CallKind::is_propagation_with_trace() const {
  return (encoding_ & CallKind::PropagationWithTrace) == PropagationWithTrace;
}

bool CallKind::is_propagation_without_trace() const {
  return encoding_ == Propagation;
}

CallKind CallKind::propagate() const {
  if (is_propagation_without_trace()) {
    return *this;
  }

  Encoding encoding{};

  if (is_propagation_with_trace()) {
    encoding |= CallKind::PropagationWithTrace;
  }

  // Propagate the callinfo states.
  if (is_declaration()) {
    encoding |= CallKind::Origin;
  } else if (is_origin()) {
    encoding |= CallKind::CallSite;
  } else if (is_callsite()) {
    encoding |= CallKind::CallSite;
  }

  return CallKind{encoding};
}

} // namespace marianatrench
