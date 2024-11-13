/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <mariana-trench/ClassHierarchies.h>
#include <mariana-trench/IncludeMacros.h>
#include <mariana-trench/JsonValidation.h>
#include <mariana-trench/LifecycleMethod.h>
#include <mariana-trench/Methods.h>
#include <mariana-trench/Options.h>

namespace marianatrench {

class LifecycleMethodsJsonError : public JsonValidationError {
 public:
  LifecycleMethodsJsonError(
      const Json::Value& value,
      const std::optional<std::string>& field,
      const std::string& expected);
};

/**
 * This class adds artificial methods to simulate common framework behaviors
 * that the analysis may not otherwise be able to see or handle.
 */
class LifecycleMethods final {
 public:
  static LifecycleMethods run(
      const Options& options,
      const ClassHierarchies& class_hierarchies,
      Methods& methods);

  LifecycleMethods() = default;

  MOVE_CONSTRUCTOR_ONLY(LifecycleMethods)

  void add_methods_from_json(const Json::Value& lifecycle_definitions);

  const auto& methods() const {
    return lifecycle_methods_;
  }

 private:
  std::unordered_map<std::string, LifecycleMethod> lifecycle_methods_;
};

} // namespace marianatrench
