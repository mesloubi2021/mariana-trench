/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <ostream>

#include <mariana-trench/CallClassIntervalContext.h>
#include <mariana-trench/Frame.h>
#include <mariana-trench/TaintConfig.h>

namespace marianatrench {

CallClassIntervalContext::CallClassIntervalContext(
    ClassIntervals::Interval interval,
    bool preserves_type_context)
    : callee_interval_(std::move(interval)),
      preserves_type_context_(preserves_type_context) {}

CallClassIntervalContext::CallClassIntervalContext(const TaintConfig& config)
    : CallClassIntervalContext(config.class_interval_context()) {}

CallClassIntervalContext::CallClassIntervalContext(const Frame& frame)
    : CallClassIntervalContext(frame.class_interval_context()) {}

Json::Value CallClassIntervalContext::to_json() const {
  auto value = Json::Value(Json::objectValue);
  if (is_default()) {
    // Default intervals are not emitted in JSON to reduce verbosity.
    return value;
  }

  value["callee_interval"] = ClassIntervals::interval_to_json(callee_interval_);
  value["preserves_type_context"] = Json::Value(preserves_type_context_);
  return value;
}

CallClassIntervalContext CallClassIntervalContext::from_json(
    const ::Json::Value& value) {
  JsonValidation::validate_object(value);
  if (!value.isMember("callee_interval")) {
    // If the interval is absent, return the default interval.
    return CallClassIntervalContext();
  }

  return CallClassIntervalContext(
      ClassIntervals::interval_from_json(
          JsonValidation::nonempty_array(value, "callee_interval")),
      JsonValidation::boolean(value, "preserves_type_context"));
}

std::ostream& operator<<(
    std::ostream& out,
    const CallClassIntervalContext& class_interval_context) {
  return out << "{" << class_interval_context.callee_interval()
             << ", preserves_type_context="
             << class_interval_context.preserves_type_context() << "}";
}

} // namespace marianatrench
