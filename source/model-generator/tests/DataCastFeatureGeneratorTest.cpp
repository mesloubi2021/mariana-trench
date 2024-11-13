/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <filesystem>

#include <gtest/gtest.h>

#include <mariana-trench/Redex.h>
#include <mariana-trench/model-generator/JsonModelGenerator.h>
#include <mariana-trench/model-generator/ModelGeneratorNameFactory.h>
#include <mariana-trench/tests/Test.h>

using namespace marianatrench;

namespace {

std::filesystem::path models_file_path() {
  return test::find_repository_root() /
      "configuration/model-generators/propagations/DataCastFeatureGenerator.models";
}

} // namespace

class DataCastFeatureGeneratorTest : public test::Test {};

TEST_F(DataCastFeatureGeneratorTest, CastToInt) {
  Scope scope;

  auto* dex_method = redex::create_void_method(
      scope,
      /* class_name */
      "Ljava/lang/Integer;",
      /* method */ "parseInt",
      /* parameter_type */ "Ljava/lang/String;",
      /* return_type */ "I",
      /* super */ nullptr,
      /* is_static */ true);

  DexStore store("test-stores");
  store.add_classes(scope);
  auto context = test::make_context(store);
  auto* method = context.methods->get(dex_method);

  auto expected_model = Model(
      /* method */ method,
      context,
      /* modes */ {},
      /* frozen */ {},
      /* config_overrides */ {},
      /* generations */ std::vector<std::pair<AccessPath, TaintConfig>>{},
      /* parameter_sources */ {},
      /* sinks */ {},
      /* propagations */
      {PropagationConfig(
          /* input_path */ AccessPath(Root(Root::Kind::Argument, 0)),
          /* kind */ context.kind_factory->local_return(),
          /* output_paths */
          PathTreeDomain{{Path{}, CollapseDepth::zero()}},
          /* inferred_features */ FeatureMayAlwaysSet::bottom(),
          /* locally_inferred_features */ FeatureMayAlwaysSet::bottom(),
          /* user_features */
          FeatureSet{context.feature_factory->get("cast:numeric")})});
  expected_model.add_model_generator(
      context.model_generator_name_factory->create(
          "DataCastFeatureGenerator", 2));

  EXPECT_THAT(
      JsonModelGenerator::from_file(
          "DataCastFeatureGenerator", context, models_file_path())
          .emit_method_models(*context.methods),
      testing::UnorderedElementsAre(expected_model));
}

TEST_F(DataCastFeatureGeneratorTest, CastToBool) {
  Scope scope;

  auto* dex_method = redex::create_void_method(
      scope,
      /* class_name */
      "Ljava/lang/Boolean;",
      /* method */ "booleanValue",
      /* parameter_type */ "",
      /* return_type */ "Z");

  DexStore store("test-stores");
  store.add_classes(scope);
  auto context = test::make_context(store);
  auto* method = context.methods->get(dex_method);

  auto expected_model = Model(
      /* method */ method,
      context,
      /* modes */ {},
      /* frozen */ {},
      /* config_overrides */ {},
      /* generations */ std::vector<std::pair<AccessPath, TaintConfig>>{},
      /* parameter_sources */ {},
      /* sinks */ {},
      /* propagations */
      {PropagationConfig(
          /* input_path */ AccessPath(Root(Root::Kind::Argument, 0)),
          /* kind */ context.kind_factory->local_return(),
          /* output_paths */
          PathTreeDomain{{Path{}, CollapseDepth::zero()}},
          /* inferred_features */ FeatureMayAlwaysSet::bottom(),
          /* locally_inferred_features */ FeatureMayAlwaysSet::bottom(),
          /* user_features */
          FeatureSet{context.feature_factory->get("cast:boolean")})});
  expected_model.add_model_generator(
      context.model_generator_name_factory->create(
          "DataCastFeatureGenerator", 3));

  EXPECT_THAT(
      JsonModelGenerator::from_file(
          "DataCastFeatureGenerator", context, models_file_path())
          .emit_method_models(*context.methods),
      testing::UnorderedElementsAre(expected_model));
}
