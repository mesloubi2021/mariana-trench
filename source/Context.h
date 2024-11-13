/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <memory>
#include <vector>

#include <DexStore.h>

namespace marianatrench {

class KindFactory;
class FeatureFactory;
class Statistics;
class Options;
class Heuristics;
class ArtificialMethods;
class Methods;
class Fields;
class Positions;
class ControlFlowGraphs;
class Types;
class ClassProperties;
class ClassHierarchies;
class ClassIntervals;
class FieldCache;
class Overrides;
class CallGraph;
class Rules;
class Dependencies;
class Scheduler;
class OriginFactory;
class TransformsFactory;
class UsedKinds;
class ModelGeneratorNameFactory;
class AccessPathFactory;

/**
 * Mariana Trench global context.
 */
class Context final {
 public:
  Context();

  Context(const Context&) = delete;
  Context(Context&&) noexcept;
  Context& operator=(const Context&) = delete;
  Context& operator=(Context&&) = delete;
  ~Context();

  const KindFactory* kind_factory;
  const FeatureFactory* feature_factory;
  const TransformsFactory* transforms_factory;
  const ModelGeneratorNameFactory* model_generator_name_factory;
  const AccessPathFactory* access_path_factory;
  const OriginFactory* origin_factory;
  const Heuristics* heuristics;
  std::unique_ptr<Statistics> statistics;
  std::unique_ptr<Options> options;
  std::vector<DexStore> stores;
  std::unique_ptr<ArtificialMethods> artificial_methods;
  std::unique_ptr<Methods> methods;
  std::unique_ptr<Fields> fields;
  std::unique_ptr<Positions> positions;
  std::unique_ptr<ControlFlowGraphs> control_flow_graphs;
  std::unique_ptr<Types> types;
  std::unique_ptr<ClassProperties> class_properties;
  std::unique_ptr<ClassHierarchies> class_hierarchies;
  std::unique_ptr<ClassIntervals> class_intervals;
  std::unique_ptr<FieldCache> field_cache;
  std::unique_ptr<Overrides> overrides;
  std::unique_ptr<CallGraph> call_graph;
  std::unique_ptr<Rules> rules;
  std::unique_ptr<Dependencies> dependencies;
  std::unique_ptr<Scheduler> scheduler;
  std::unique_ptr<UsedKinds> used_kinds;
};

} // namespace marianatrench
