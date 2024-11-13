/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <mariana-trench/JsonValidation.h>
#include <mariana-trench/KindFactory.h>
#include <mariana-trench/MultiSourceMultiSinkRule.h>
#include <mariana-trench/Rule.h>
#include <mariana-trench/SourceSinkRule.h>
#include <mariana-trench/SourceSinkWithExploitabilityRule.h>

namespace marianatrench {

std::unique_ptr<Rule> Rule::from_json(
    const Json::Value& value,
    Context& context) {
  JsonValidation::validate_object(value);

  auto name = JsonValidation::string(value, /* field */ "name");
  int code = JsonValidation::integer(value, /* field */ "code");
  auto description = JsonValidation::string(value, /* field */ "description");

  // This uses the presence of specific keys to determine the rule kind.
  if (value.isMember("effect_sources")) {
    return SourceSinkWithExploitabilityRule::from_json(
        name, code, description, value, context);
  } else if (value.isMember("sources") && value.isMember("sinks")) {
    return SourceSinkRule::from_json(name, code, description, value, context);
  } else if (
      value.isMember("multi_sources") && value.isMember("partial_sinks")) {
    return MultiSourceMultiSinkRule::from_json(
        name, code, description, value, context);
  } else {
    throw JsonValidationError(
        value,
        std::nullopt,
        "keys: sources+[transforms+]sinks or multi_sources+partial_sinks or effect_sources+sources+sinks");
  }
}

Json::Value Rule::to_json() const {
  auto value = Json::Value(Json::objectValue);
  value["name"] = name_;
  value["code"] = Json::Value(code_);
  value["description"] = description_;
  return value;
}

} // namespace marianatrench
