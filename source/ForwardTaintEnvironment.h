/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <sparta/AbstractDomain.h>

#include <mariana-trench/IncludeMacros.h>
#include <mariana-trench/MemoryLocation.h>
#include <mariana-trench/MemoryLocationEnvironment.h>
#include <mariana-trench/PointsToEnvironment.h>
#include <mariana-trench/Taint.h>
#include <mariana-trench/TaintEnvironment.h>
#include <mariana-trench/TaintTree.h>

namespace marianatrench {

class ForwardTaintEnvironment final
    : public sparta::AbstractDomain<ForwardTaintEnvironment> {
 public:
  /* Create the bottom environment. */
  ForwardTaintEnvironment() : environment_(TaintEnvironment::bottom()) {}

  explicit ForwardTaintEnvironment(TaintEnvironment taint)
      : environment_(std::move(taint)) {}

  static ForwardTaintEnvironment initial();

  INCLUDE_ABSTRACT_DOMAIN_METHODS(
      ForwardTaintEnvironment,
      TaintEnvironment,
      environment_)

  TaintTree read(MemoryLocation* memory_location) const;

  TaintTree read(MemoryLocation* memory_location, const Path& path) const;

  TaintTree read(const MemoryLocationsDomain& memory_locations) const;

  TaintTree read(
      const MemoryLocationsDomain& memory_locations,
      const Path& path) const;

  void write(MemoryLocation* memory_location, TaintTree taint, UpdateKind kind);

  void write(
      MemoryLocation* memory_location,
      const Path& path,
      TaintTree taint,
      UpdateKind kind);

  void write(
      MemoryLocation* memory_location,
      const Path& path,
      Taint taint,
      UpdateKind kind);

  void write(
      const MemoryLocationsDomain& memory_locations,
      TaintTree taint,
      UpdateKind kind);

  void write(
      const MemoryLocationsDomain& memory_locations,
      Taint taint,
      UpdateKind kind);

  void write(
      const MemoryLocationsDomain& memory_locations,
      const Path& path,
      TaintTree taint,
      UpdateKind kind);

  void write(
      const MemoryLocationsDomain& memory_locations,
      const Path& path,
      Taint taint,
      UpdateKind kind);

  TaintTree deep_read(
      const ResolvedAliasesMap& resolved_aliases,
      MemoryLocation* memory_location) const;

  TaintTree deep_read(
      const ResolvedAliasesMap& resolved_aliases,
      const MemoryLocationsDomain& memory_locations) const;

  TaintTree deep_read(
      const ResolvedAliasesMap& resolved_aliases,
      const MemoryLocationsDomain& memory_locations,
      const Path& path) const;

  void deep_write(
      const ResolvedAliasesMap& resolved_aliases,
      MemoryLocation* memory_location,
      TaintTree taint,
      UpdateKind kind);

  void deep_write(
      const ResolvedAliasesMap& resolved_aliases,
      MemoryLocation* memory_location,
      const Path& path,
      TaintTree taint,
      UpdateKind kind);

  void deep_write(
      const ResolvedAliasesMap& resolved_aliases,
      const MemoryLocationsDomain& memory_locations,
      TaintTree taint,
      UpdateKind kind);

  void deep_write(
      const ResolvedAliasesMap& resolved_aliases,
      const MemoryLocationsDomain& memory_locations,
      Taint taint,
      UpdateKind kind);

  void deep_write(
      const ResolvedAliasesMap& resolved_aliases,
      const MemoryLocationsDomain& memory_locations,
      const Path& path,
      TaintTree taint,
      UpdateKind kind);

  friend std::ostream& operator<<(
      std::ostream& out,
      const ForwardTaintEnvironment& environment);

 private:
  TaintEnvironment environment_;
};

} // namespace marianatrench
