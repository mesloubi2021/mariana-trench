/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <mariana-trench/Rule.h>
#include <mariana-trench/Rules.h>

namespace marianatrench {

struct CoveredRule {
  int code;
  Rule::KindSet used_sources;
  Rule::KindSet used_sinks;
  Rule::TransformSet used_transforms;

  bool operator==(const CoveredRule& other) const;

  Json::Value to_json() const;
};

struct RulesCoverage {
  std::unordered_map<int, CoveredRule> covered_rules;
  std::unordered_set<int> non_covered_rule_codes;

  bool operator==(const RulesCoverage& other) const;

  Json::Value to_json() const;

  /**
   * Computes rule(/category) coverage based on the set of known
   * sources/sinks/transforms that are used. A rule is "covered" if its
   * sources/sinks are "used" in some model. Since a rule comprises of multiple
   * source/sink/transform kinds, additional information is included to indicate
   * which ones in the rule were used.
   */
  static RulesCoverage create(
      const Rules& rules,
      const Rule::KindSet& used_sources,
      const Rule::KindSet& used_sinks,
      const Rule::TransformSet& used_transforms);
};

} // namespace marianatrench
