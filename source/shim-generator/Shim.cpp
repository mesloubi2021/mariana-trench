/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <algorithm>

#include <Show.h>
#include <TypeUtil.h>

#include <mariana-trench/Assert.h>
#include <mariana-trench/Log.h>
#include <mariana-trench/Methods.h>
#include <mariana-trench/shim-generator/Shim.h>

namespace marianatrench {
namespace {

static const InstantiatedShim::FlatSet<ShimTarget> empty_shim_targets;

static const InstantiatedShim::FlatSet<ShimLifecycleTarget>
    empty_lifecycle_targets;

static const InstantiatedShim::FlatSet<ShimReflectionTarget>
    empty_reflection_targets;

bool verify_has_parameter_type(
    std::string_view method_name,
    const DexType* dex_class,
    const DexProto* dex_proto,
    bool is_static,
    ParameterPosition position) {
  auto number_of_parameters =
      dex_proto->get_args()->size() + (is_static ? 0u : 1u);
  if (position >= number_of_parameters) {
    ERROR(
        1,
        "Parameter mapping for shim_target `{}.{}{}` contains a port on parameter {} but the method only has {} parameters.",
        show(dex_class),
        method_name,
        show(dex_proto),
        position,
        number_of_parameters);
    return false;
  }

  return (
      dex_proto->get_args()->at(position - (is_static ? 0u : 1u)) != nullptr);
}

} // namespace

ShimMethod::ShimMethod(const Method* method) : method_(method) {
  ShimParameterPosition index = 0;

  if (!method_->is_static()) {
    // Include `this` as argument 0
    types_to_position_.emplace(method->get_class(), index++);
  }

  const auto* dex_arguments = method_->get_proto()->get_args();
  if (!dex_arguments) {
    return;
  }

  for (auto* dex_argument : *dex_arguments) {
    types_to_position_.emplace(dex_argument, index++);
  }
}

const Method* ShimMethod::method() const {
  return method_;
}

DexType* MT_NULLABLE
ShimMethod::parameter_type(ShimParameterPosition argument) const {
  return method_->parameter_type(argument);
}

std::optional<ShimParameterPosition> ShimMethod::type_position(
    const DexType* dex_type) const {
  auto found = types_to_position_.find(dex_type);
  if (found == types_to_position_.end()) {
    return std::nullopt;
  }

  LOG(5,
      "Found dex type {} in shim parameter position: {}",
      found->first->str(),
      found->second);

  return found->second;
}

ShimParameterMapping::ShimParameterMapping(
    std::initializer_list<MapType::value_type> init)
    : map_(init), infer_from_types_(false) {}

bool ShimParameterMapping::operator==(const ShimParameterMapping& other) const {
  return infer_from_types_ == other.infer_from_types_ && map_ == other.map_;
}

bool ShimParameterMapping::operator<(const ShimParameterMapping& other) const {
  // Lexicographic comparison
  return std::tie(infer_from_types_, map_) <
      std::tie(other.infer_from_types_, other.map_);
}

bool ShimParameterMapping::empty() const {
  return map_.empty();
}

bool ShimParameterMapping::contains(const Root& position) const {
  return map_.count(position) > 0;
}

std::optional<ShimParameterPosition> ShimParameterMapping::at(
    const Root& parameter_position) const {
  auto found = map_.find(parameter_position);
  if (found == map_.end()) {
    return std::nullopt;
  }

  return found->second;
}

void ShimParameterMapping::insert(
    const Root& parameter_position,
    ShimParameterPosition shim_parameter_position) {
  map_.insert({parameter_position, shim_parameter_position});
}

void ShimParameterMapping::set_infer_from_types(bool value) {
  infer_from_types_ = value;
}

bool ShimParameterMapping::infer_from_types() const {
  return infer_from_types_;
}

void ShimParameterMapping::add_receiver(
    ShimParameterPosition shim_parameter_position) {
  // Include `this` as argument 0
  insert(Root::argument(0), shim_parameter_position);
}

void ShimParameterMapping::infer_parameters_from_types(
    const DexProto* shim_target_proto,
    bool shim_target_is_static,
    const ShimMethod& shim_method) {
  const auto* dex_arguments = shim_target_proto->get_args();
  if (!dex_arguments) {
    return;
  }

  ParameterPosition first_parameter_position = shim_target_is_static ? 0 : 1;

  for (std::size_t position = 0; position < dex_arguments->size(); position++) {
    if (auto shim_position =
            shim_method.type_position(dex_arguments->at(position))) {
      insert(
          Root::argument(static_cast<ParameterPosition>(
              position + first_parameter_position)),
          *shim_position);
    }
  }
}

ShimParameterMapping ShimParameterMapping::from_json(
    const Json::Value& value,
    bool infer_from_types) {
  ShimParameterMapping parameter_mapping;
  parameter_mapping.set_infer_from_types(infer_from_types);

  if (value.isNull()) {
    return parameter_mapping;
  }

  JsonValidation::validate_object(value);

  for (auto item = value.begin(); item != value.end(); ++item) {
    auto parameter_argument = JsonValidation::string(item.key());
    auto shim_argument = JsonValidation::string(*item);
    parameter_mapping.insert(
        Root::from_json(parameter_argument),
        Root::from_json(shim_argument).parameter_position());
  }

  return parameter_mapping;
}

ShimParameterMapping ShimParameterMapping::instantiate_parameters(
    std::string_view shim_target_method,
    const DexType* shim_target_class,
    const DexProto* shim_target_proto,
    bool shim_target_is_static,
    const ShimMethod& shim_method) const {
  ShimParameterMapping parameter_mapping;
  parameter_mapping.set_infer_from_types(infer_from_types());

  for (const auto& [shim_target_position, shim_position] : map_) {
    if (shim_target_position.is_argument() &&
        !verify_has_parameter_type(
            shim_target_method,
            shim_target_class,
            shim_target_proto,
            shim_target_is_static,
            shim_target_position.parameter_position())) {
      continue;
    }

    parameter_mapping.insert(shim_target_position, shim_position);
  }

  if (infer_from_types()) {
    parameter_mapping.infer_parameters_from_types(
        shim_target_proto, shim_target_is_static, shim_method);
  }

  return parameter_mapping;
}

ShimTarget::ShimTarget(
    DexMethodSpec method_spec,
    ShimParameterMapping parameter_mapping,
    bool is_static)
    : method_spec_(std::move(method_spec)),
      parameter_mapping_(std::move(parameter_mapping)),
      is_static_(is_static) {
  mt_assert(
      method_spec_.cls != nullptr && method_spec_.name != nullptr &&
      method_spec_.proto != nullptr);
}

ShimTarget::ShimTarget(
    const Method* method,
    ShimParameterMapping parameter_mapping)
    : ShimTarget(
          DexMethodSpec{
              method->get_class(),
              DexString::get_string(method->get_name()),
              method->get_proto()},
          std::move(parameter_mapping),
          method->is_static()) {}

bool ShimTarget::operator==(const ShimTarget& other) const {
  return is_static_ == other.is_static_ && method_spec_ == other.method_spec_ &&
      parameter_mapping_ == other.parameter_mapping_;
}

bool ShimTarget::operator<(const ShimTarget& other) const {
  // Lexicographic comparison
  return std::tie(
             is_static_,
             method_spec_.cls,
             method_spec_.name,
             method_spec_.proto,
             parameter_mapping_) <
      std::tie(
             other.is_static_,
             other.method_spec_.cls,
             other.method_spec_.name,
             other.method_spec_.proto,
             other.parameter_mapping_);
}

std::optional<Register> ShimTarget::receiver_register(
    const IRInstruction* instruction) const {
  if (is_static_) {
    return std::nullopt;
  }

  auto receiver_position = parameter_mapping_.at(Root::argument(0));
  if (!receiver_position) {
    return std::nullopt;
  }

  mt_assert(*receiver_position < instruction->srcs_size());

  return instruction->src(*receiver_position);
}

std::unordered_map<Root, Register> ShimTarget::root_registers(
    const IRInstruction* instruction) const {
  std::unordered_map<Root, Register> root_registers;

  for (const auto& [root, shimmed_method_position] : parameter_mapping_) {
    mt_assert(shimmed_method_position < instruction->srcs_size());
    root_registers.emplace(root, instruction->src(shimmed_method_position));
  }

  return root_registers;
}

ShimReflectionTarget::ShimReflectionTarget(
    DexMethodSpec method_spec,
    ShimParameterMapping parameter_mapping)
    : method_spec_(method_spec),
      parameter_mapping_(std::move(parameter_mapping)) {
  mt_assert(
      method_spec_.cls == type::java_lang_Class() &&
      method_spec_.name != nullptr && method_spec_.proto != nullptr);
  mt_assert_log(
      parameter_mapping_.contains(Root::argument(0)),
      "Missing parameter mapping for receiver for reflection shim target");
}

bool ShimReflectionTarget::operator==(const ShimReflectionTarget& other) const {
  return method_spec_ == other.method_spec_ &&
      parameter_mapping_ == other.parameter_mapping_;
}

bool ShimReflectionTarget::operator<(const ShimReflectionTarget& other) const {
  // Lexicographic comparison
  return std::tie(
             method_spec_.cls,
             method_spec_.name,
             method_spec_.proto,
             parameter_mapping_) <
      std::tie(
             other.method_spec_.cls,
             other.method_spec_.name,
             other.method_spec_.proto,
             other.parameter_mapping_);
}

Register ShimReflectionTarget::receiver_register(
    const IRInstruction* instruction) const {
  auto receiver_position = parameter_mapping_.at(Root::argument(0));
  mt_assert(*receiver_position < instruction->srcs_size());

  return instruction->src(*receiver_position);
}

std::unordered_map<Root, Register> ShimReflectionTarget::root_registers(
    const Method* resolved_reflection,
    const IRInstruction* instruction) const {
  std::unordered_map<Root, Register> root_registers;

  // For reflection receivers, do not propagate the `this` argument, as it
  // is always a new instance.
  for (ParameterPosition position = 1;
       position < resolved_reflection->number_of_parameters();
       ++position) {
    if (auto shim_position = parameter_mapping_.at(Root::argument(position))) {
      mt_assert(*shim_position < instruction->srcs_size());
      root_registers.emplace(
          Root(Root::Kind::Argument, position),
          instruction->src(*shim_position));
    }
  }

  return root_registers;
}

ShimLifecycleTarget::ShimLifecycleTarget(
    std::string method_name,
    ShimParameterPosition receiver_position,
    bool is_reflection,
    bool infer_from_types)
    : method_name_(std::move(method_name)),
      receiver_position_(std::move(receiver_position)),
      is_reflection_(is_reflection),
      infer_from_types_(infer_from_types) {}

bool ShimLifecycleTarget::operator==(const ShimLifecycleTarget& other) const {
  return infer_from_types_ == other.infer_from_types_ &&
      is_reflection_ == other.is_reflection_ &&
      receiver_position_ == other.receiver_position_ &&
      method_name_ == other.method_name_;
}

bool ShimLifecycleTarget::operator<(const ShimLifecycleTarget& other) const {
  // Lexicographic comparison
  return std::tie(
             infer_from_types_,
             is_reflection_,
             receiver_position_,
             method_name_) <
      std::tie(
             other.infer_from_types_,
             other.is_reflection_,
             other.receiver_position_,
             other.method_name_);
}

Register ShimLifecycleTarget::receiver_register(
    const IRInstruction* instruction) const {
  mt_assert(receiver_position_ < instruction->srcs_size());

  return instruction->src(receiver_position_);
}

std::unordered_map<Root, Register> ShimLifecycleTarget::root_registers(
    const Method* callee,
    const Method* lifecycle_method,
    const IRInstruction* instruction) const {
  std::unordered_map<Root, Register> root_registers;
  ShimMethod shim_method{callee};

  ShimParameterMapping parameter_mapping;

  // For reflection receivers, do not propagate the `this` argument, as it
  // is always a new instance.
  if (!is_reflection()) {
    parameter_mapping.add_receiver(receiver_position_);
  }

  if (infer_from_types()) {
    parameter_mapping.infer_parameters_from_types(
        lifecycle_method->get_proto(),
        lifecycle_method->is_static(),
        shim_method);
  }

  for (ParameterPosition position = 0;
       position < lifecycle_method->number_of_parameters();
       ++position) {
    if (auto shim_position = parameter_mapping.at(Root::argument(position))) {
      mt_assert(*shim_position < instruction->srcs_size());
      root_registers.emplace(
          Root(Root::Kind::Argument, position),
          instruction->src(*shim_position));
    }
  }

  return root_registers;
}

InstantiatedShim::InstantiatedShim(const Method* method) : method_(method) {}

void InstantiatedShim::add_target(ShimTargetVariant target) {
  if (std::holds_alternative<ShimTarget>(target)) {
    targets_.emplace(std::get<ShimTarget>(target));
  } else if (std::holds_alternative<ShimReflectionTarget>(target)) {
    reflections_.emplace(std::get<ShimReflectionTarget>(target));
  } else if (std::holds_alternative<ShimLifecycleTarget>(target)) {
    lifecycles_.emplace(std::get<ShimLifecycleTarget>(target));
  } else {
    mt_unreachable();
  }
}

void InstantiatedShim::merge_with(InstantiatedShim other) {
  targets_.insert(
      std::make_move_iterator(other.targets_.begin()),
      std::make_move_iterator(other.targets_.end()));
  reflections_.insert(
      std::make_move_iterator(other.reflections_.begin()),
      std::make_move_iterator(other.reflections_.end()));
  lifecycles_.insert(
      std::make_move_iterator(other.lifecycles_.begin()),
      std::make_move_iterator(other.lifecycles_.end()));
}

Shim::Shim(
    const InstantiatedShim* MT_NULLABLE instantiated_shim,
    InstantiatedShim::FlatSet<ShimTarget> intent_routing_targets)
    : instantiated_shim_(instantiated_shim),
      intent_routing_targets_(std::move(intent_routing_targets)) {}

const InstantiatedShim::FlatSet<ShimTarget>& Shim::targets() const {
  if (instantiated_shim_ == nullptr) {
    return empty_shim_targets;
  }
  return instantiated_shim_->targets();
}

const InstantiatedShim::FlatSet<ShimReflectionTarget>& Shim::reflections()
    const {
  if (instantiated_shim_ == nullptr) {
    return empty_reflection_targets;
  }
  return instantiated_shim_->reflections();
}

const InstantiatedShim::FlatSet<ShimLifecycleTarget>& Shim::lifecycles() const {
  if (instantiated_shim_ == nullptr) {
    return empty_lifecycle_targets;
  }
  return instantiated_shim_->lifecycles();
}

std::ostream& operator<<(std::ostream& out, const ShimMethod& shim_method) {
  out << "ShimMethod(method=`";
  if (shim_method.method_ != nullptr) {
    out << shim_method.method_->show();
  }
  out << "`)";
  return out;
}

std::ostream& operator<<(std::ostream& out, const ShimParameterMapping& map) {
  out << "infer_from_types=`";
  out << (map.infer_from_types() ? "true" : "false") << "`, ";
  out << "parameters_map={";
  for (const auto& [parameter, shim_parameter] : map.map_) {
    out << " " << parameter << ":";
    out << " Argument(" << shim_parameter << "),";
  }
  return out << " }";
}

std::ostream& operator<<(std::ostream& out, const ShimTarget& shim_target) {
  out << "ShimTarget(type=`";
  out << show(shim_target.method_spec_.cls);
  out << "`, method_name=`";
  out << show(shim_target.method_spec_.name);
  out << "`, proto=`";
  out << show(shim_target.method_spec_.proto);
  out << "`";
  out << ", " << shim_target.parameter_mapping_;
  return out << ")";
}

std::ostream& operator<<(
    std::ostream& out,
    const ShimReflectionTarget& shim_reflection_target) {
  out << "ShimReflectionTarget(method_name=`";
  out << show(shim_reflection_target.method_spec_.name);
  out << "`, proto=`";
  out << show(shim_reflection_target.method_spec_.proto);
  out << "`, " << shim_reflection_target.parameter_mapping_;
  return out << ")";
}

std::ostream& operator<<(
    std::ostream& out,
    const ShimLifecycleTarget& shim_lifecycle_target) {
  out << "ShimLifecycleTarget(method_name=`";
  out << shim_lifecycle_target.method_name_;
  out << "`, receiver_position=Argument("
      << shim_lifecycle_target.receiver_position_ << ")";
  out << "`, is_reflection=`";
  out << (shim_lifecycle_target.is_reflection_ ? "true" : "false") << "`, ";
  out << "`, infer_from_types=`";
  out << (shim_lifecycle_target.infer_from_types_ ? "true" : "false") << "`, ";
  return out << ")";
}

std::ostream& operator<<(std::ostream& out, const InstantiatedShim& shim) {
  out << "InstantiatedShim(method=`";
  if (auto* method = shim.method()) {
    out << method->show();
  }
  out << "`";

  if (!shim.targets().empty()) {
    out << ",\n  targets=[\n";
    for (const auto& target : shim.targets()) {
      out << "    " << target << ",\n";
    }
    out << "  ]";
  }

  if (!shim.reflections().empty()) {
    out << ",\n  reflections=[\n";
    for (const auto& target : shim.reflections()) {
      out << "    " << target << ",\n";
    }
    out << "  ]";
  }

  if (!shim.lifecycles().empty()) {
    out << ",\n  lifecycles=[\n";
    for (const auto& target : shim.lifecycles()) {
      out << "    " << target << ",\n";
    }
    out << "  ]";
  }

  return out << ")";
}

std::ostream& operator<<(std::ostream& out, const Shim& shim) {
  out << "Shim(shim=`";
  if (shim.instantiated_shim_) {
    out << *(shim.instantiated_shim_);
  }
  out << "`";

  if (!shim.intent_routing_targets().empty()) {
    out << ",\n  intent_routing_targets=[\n";
    for (const auto& target : shim.intent_routing_targets()) {
      out << "    " << target << ",\n";
    }
    out << "  ]";
  }
  return out << ")";
}

} // namespace marianatrench
