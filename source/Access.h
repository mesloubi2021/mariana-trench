/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <initializer_list>
#include <limits>
#include <optional>
#include <string_view>
#include <vector>

#include <boost/functional/hash.hpp>
#include <json/json.h>

#include <sparta/ConstantAbstractDomain.h>
#include <sparta/PatriciaTreeKeyTrait.h>

#include <DexClass.h>
#include <IRInstruction.h>

#include <mariana-trench/Assert.h>
#include <mariana-trench/Compiler.h>
#include <mariana-trench/IncludeMacros.h>
#include <mariana-trench/PointerIntPair.h>

namespace marianatrench {

class Method;

/* Integer type representing a register number. */
using Register = std::uint32_t;

// This should match with the type `reg_t` used in Redex.
static_assert(std::is_same_v<Register, reg_t>, "type mismatch");

/* Integer type representing a parameter number. */
using ParameterPosition = std::uint32_t;

std::optional<ParameterPosition> parse_parameter_position(
    const std::string& string);

/**
 * Represents the root of an access path.
 *
 * This is either the return value or an argument.
 */
class Root final {
 public:
  using IntegerEncoding = ParameterPosition;

  enum class Kind : IntegerEncoding {
    Argument = 0,
    Return = std::numeric_limits<IntegerEncoding>::max(),
    /* When used as a callee port of a `Frame`, it represents a leaf frame. */
    Leaf = std::numeric_limits<IntegerEncoding>::max() - 1,
    /*
     * When used as a callee port of a `Frame`, `Anchor` and `Producer` are used
     * as "connection points" where data flows into another codebase, e.g.:
     * GraphQL, native. Information about these will be output to CRTEX. They
     * mark connection points with sources/sinks that flow to/from another
     * codebase. `Anchor` is for those where Mariana Trench detected the flow
     * and will output to CRTEX. `Producer` is for those detected by another
     * analyzer and then read as input by Mariana Trench.
     */
    Anchor = std::numeric_limits<IntegerEncoding>::max() - 2,
    Producer = std::numeric_limits<IntegerEncoding>::max() - 3,
    /*
     * In CRTEX, "this" argument, represented by argument(0) in Mariana Trench,
     * has index -1 in other codebases. This cannot be represented by the
     * unsigned encoding, so use a special kind. In the analysis,
     * `CanonicalThis` is not considered an argument.
     */
    CanonicalThis = std::numeric_limits<IntegerEncoding>::max() - 4,
    /*
     * Used for call effects. The call chain effect means the corresponding
     * Taint is derived from a method call to the callee. All call effect
     * roots are stored in a separate call effects source/sink TaintTree in
     * the Model.
     */
    CallEffectCallChain = std::numeric_limits<IntegerEncoding>::max() - 5,
    /*
     * Similar to call chain effect but used specifically by
     * SourceSinkWithExploitabilityRule. Taint on this port is propagated the
     * same way as the call chain effect. The differences are:
     * - Sink taint on this port cannot be specified by the user but instead
     *   is inferred based on the exploitability rule match.
     * - Sources and Sinks on the exploitability call effect port are *both*
     *   read from the method being analyzed (vs caller to callee). This allows
     *   us to emit 0 hop issues in case the exploitability sink is inferred on
     *   the method with the exploitability source. This is not desired for
     *   call chain effect.
     */
    CallEffectExploitability = std::numeric_limits<IntegerEncoding>::max() - 6,
    /*
     * Used for propagation of taint via activity Intents.
     */
    CallEffectIntent = std::numeric_limits<IntegerEncoding>::max() - 7,
    MaxArgument = std::numeric_limits<IntegerEncoding>::max() - 8,
  };

 private:
  explicit Root(IntegerEncoding value) : value_(value) {}

 public:
  /* Default constructor required by sparta, do not use. */
  explicit Root() : value_(static_cast<IntegerEncoding>(Kind::Return)) {}

  explicit Root(Kind kind, ParameterPosition parameter_position = 0) {
    if (kind == Kind::Argument) {
      value_ = parameter_position;
    } else {
      value_ = static_cast<IntegerEncoding>(kind);
    }
  }

  INCLUDE_DEFAULT_COPY_CONSTRUCTORS_AND_ASSIGNMENTS(Root)

  bool operator==(const Root& other) const {
    return value_ == other.value_;
  }

  bool operator!=(const Root& other) const {
    return value_ != other.value_;
  }

  bool operator<(const Root& other) const {
    return value_ < other.value_;
  }

  bool is_argument() const {
    return value_ <= static_cast<IntegerEncoding>(Kind::MaxArgument);
  }

  bool is_return() const {
    return value_ == static_cast<IntegerEncoding>(Kind::Return);
  }

  bool is_leaf() const {
    return value_ == static_cast<IntegerEncoding>(Kind::Leaf);
  }

  bool is_anchor() const {
    return value_ == static_cast<IntegerEncoding>(Kind::Anchor);
  }

  bool is_producer() const {
    return value_ == static_cast<IntegerEncoding>(Kind::Producer);
  }

  /* Is it used as callee port for a leaf frame? */
  bool is_leaf_port() const {
    switch (kind()) {
      case Kind::Leaf:
      case Kind::Anchor:
      case Kind::Producer:
        return true;
      default:
        return false;
    }
  }

  bool is_call_effect() const {
    switch (kind()) {
      case Kind::CallEffectCallChain:
      case Kind::CallEffectExploitability:
      case Kind::CallEffectIntent:
        return true;
      default:
        return false;
    }
  }

  bool is_call_chain_exploitability() const {
    return kind() == Kind::CallEffectExploitability;
  }

  /**
   * These non-argument/return ports can be used for propagation inputs, but are
   * only expected to work intraprocedurally to infer sinks. They are not
   * propagated into other propagations for both performance reasons and also
   * because they are not expected to be reachable in the absence of appropriate
   * shims.
   */
  bool is_call_effect_for_local_propagation_input() const {
    switch (kind()) {
      case Kind::CallEffectIntent:
        return true;
      default:
        return false;
    }
  }

  Kind kind() const {
    if (is_argument()) {
      return Kind::Argument;
    } else {
      return static_cast<Kind>(value_);
    }
  }

  ParameterPosition parameter_position() const {
    mt_assert(is_argument());
    return value_;
  }

  std::size_t hash() const {
    return std::hash<IntegerEncoding>()(value_);
  }

  std::string to_string() const;

  static Root argument(ParameterPosition value) {
    return Root(Kind::Argument, value);
  }

  static Root from_json(const Json::Value& value);
  Json::Value to_json() const;

 private:
  friend std::ostream& operator<<(std::ostream& out, const Root& root);

 private:
  // If the root is a parameter, this is the parameter position.
  // If the root is the return value, this is the biggest integer.
  // Note that `RootPatriciaTreeAbstractPartition` relies on this encoding.
  IntegerEncoding value_;
};

} // namespace marianatrench

template <>
struct std::hash<marianatrench::Root> {
  std::size_t operator()(const marianatrench::Root& root) const {
    return root.hash();
  }
};

template <>
struct sparta::PatriciaTreeKeyTrait<marianatrench::Root> {
  using IntegerType = marianatrench::Root::IntegerEncoding;
};

namespace marianatrench {

class PathElement final {
 private:
  using KindEncoding = unsigned int;
  using Element = PointerIntPair<const DexString*, 3, KindEncoding>;

 public:
  using IntegerEncoding = uintptr_t;

 public:
  enum class Kind : KindEncoding {
    Field = 1,
    Index = 2,
    AnyIndex = 3,
    IndexFromValueOf = 4,
  };

 private:
  explicit PathElement(Kind kind, const DexString* element);

 public:
  PathElement() = delete;

  INCLUDE_DEFAULT_COPY_CONSTRUCTORS_AND_ASSIGNMENTS(PathElement)

  bool operator==(const PathElement& other) const {
    return value_ == other.value_;
  }

  bool operator!=(const PathElement& other) const {
    return value_ != other.value_;
  }

 public:
  std::size_t hash() const {
    return std::hash<IntegerEncoding>()(value_.encode());
  }

  const DexString* MT_NULLABLE name() const {
    return value_.get_pointer();
  }

  Kind kind() const {
    return static_cast<Kind>(value_.get_int());
  }

  bool is_field() const {
    return kind() == Kind::Field;
  }

  bool is_index() const {
    return kind() == Kind::Index;
  }

  bool is_any_index() const {
    return kind() == Kind::AnyIndex;
  }

  bool is_index_from_value_of() const {
    return kind() == Kind::IndexFromValueOf;
  }

  ParameterPosition parameter_position() const;

  std::string to_string() const;
  static PathElement from_string(std::string_view value);

  PathElement resolve_index_from_value_of(
      const std::vector<std::optional<std::string>>& source_constant_arguments)
      const;

 public:
  static PathElement field(const DexString* name);
  static PathElement field(std::string_view name);

  static PathElement index(const DexString* name);
  static PathElement index(std::string_view name);

  static PathElement any_index();

  static PathElement index_from_value_of(Root root);

 private:
  friend std::ostream& operator<<(
      std::ostream& out,
      const PathElement& path_element);

 private:
  Element value_;
};

} // namespace marianatrench

template <>
struct std::hash<marianatrench::PathElement> {
  std::size_t operator()(const marianatrench::PathElement& path_element) const {
    return path_element.hash();
  }
};

template <>
struct sparta::PatriciaTreeKeyTrait<marianatrench::PathElement> {
  using IntegerType = marianatrench::PathElement::IntegerEncoding;
};

namespace marianatrench {

/**
 * Represents the path of an access path, without the root, e.g. `x.y.z`
 */
class Path final {
 public:
  using Element = PathElement;
  using ConstIterator = std::vector<Element>::const_iterator;

 public:
  // C++ container concept member types
  using iterator = ConstIterator;
  using const_iterator = ConstIterator;
  using value_type = Element;
  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;
  using const_reference = const Element&;
  using const_pointer = const Element*;

 public:
  Path() = default;

  explicit Path(std::initializer_list<Element> elements)
      : elements_(elements) {}

  INCLUDE_DEFAULT_COPY_CONSTRUCTORS_AND_ASSIGNMENTS(Path)

  bool operator==(const Path& other) const;

  void append(Element element);

  void extend(const Path& path);

  void pop_back();

  void truncate(std::size_t max_size);

  bool empty() const {
    return elements_.empty();
  }

  std::size_t size() const {
    return elements_.size();
  }

  ConstIterator begin() const {
    return elements_.cbegin();
  }

  ConstIterator end() const {
    return elements_.cend();
  }

  bool is_prefix_of(const Path& other) const;

  void reduce_to_common_prefix(const Path& other);

  Path resolve(const std::vector<std::optional<std::string>>&
                   source_constant_arguments) const;

  /**
   * Split a string into path elements.
   *
   * For instance:
   * ```
   * >>> split_path(Json::Value(".x.y"));
   * <<< ["x", "y"]
   * ```
   *
   * Throws a `JsonValidationError` if the format is invalid.
   */
  static std::vector<std::string> split_path(std::string_view value);

  static Path from_string(std::string_view value);
  std::string to_string() const;

  static Path from_json(const Json::Value& value);
  Json::Value to_json() const;

 private:
  friend std::ostream& operator<<(std::ostream& out, const Path& path);

 private:
  std::vector<Element> elements_;
};

} // namespace marianatrench

template <>
struct std::hash<marianatrench::Path> {
  std::size_t operator()(const marianatrench::Path& path) const {
    std::size_t seed = 0;
    for (const auto& path_element : path) {
      boost::hash_combine(
          seed, std::hash<marianatrench::Path::Element>()(path_element));
    }
    return seed;
  }
};

namespace marianatrench {

/**
 * Represents an access path, with a root and a path.
 */
class AccessPath final {
 public:
  /* Default constructor required by sparta, do not use. */
  explicit AccessPath() = default;

  explicit AccessPath(Root root, Path path = {})
      : root_(root), path_(std::move(path)) {}

  INCLUDE_DEFAULT_COPY_CONSTRUCTORS_AND_ASSIGNMENTS(AccessPath)

  bool operator==(const AccessPath& other) const;
  bool operator!=(const AccessPath& other) const;
  bool leq(const AccessPath& other) const;
  void join_with(const AccessPath& other);

  Root root() const {
    return root_;
  }

  const Path& path() const {
    return path_;
  }

  void append(Path::Element element) {
    path_.append(element);
  }

  void extend(const Path& path) {
    path_.extend(path);
  }

  void pop_back() {
    path_.pop_back();
  }

  void truncate(std::size_t max_size) {
    path_.truncate(max_size);
  }

  /**
   * Used to produce canonical ports (alongside canonical_names) for CRTEX.
   *
   * Returns the canonical port for `method` that is compatible with other
   * analyses, in the form "Anchor:Argument(x)" with two roots. `Anchor` is
   * stored as the root while "Argument(x)" is stored in the Path.
   */
  AccessPath canonicalize_for_method(const Method* method) const;

  std::string to_string() const;

  /**
   * Parse a json string into an access path.
   *
   * See `split_path` for examples of the syntax.
   */
  static AccessPath from_json(const Json::Value& value);
  Json::Value to_json() const;

 private:
  friend std::ostream& operator<<(
      std::ostream& out,
      const AccessPath& access_path);

 private:
  Root root_;
  Path path_;
};

} // namespace marianatrench

template <>
struct std::hash<marianatrench::AccessPath> {
  std::size_t operator()(const marianatrench::AccessPath& access_path) const {
    std::size_t seed = 0;
    boost::hash_combine(
        seed, std::hash<marianatrench::Root>()(access_path.root()));
    boost::hash_combine(
        seed, std::hash<marianatrench::Path>()(access_path.path()));
    return seed;
  }
};

namespace marianatrench {

/**
 * Represents the access path constant abstract domain.
 *
 * This is either bottom, top or an access path.
 */
using AccessPathConstantDomain = sparta::ConstantAbstractDomain<AccessPath>;

} // namespace marianatrench
