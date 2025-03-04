/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ConstantUses.h"
#include "IRInstruction.h"
#include "Lazy.h"
#include "ReachingDefinitions.h"
#include "TypeInference.h"

namespace outliner_impl {

using TypeEnvironments =
    std::unordered_map<const IRInstruction*, type_inference::TypeEnvironment>;

using ReachingDefsEnvironments =
    std::unordered_map<const IRInstruction*, reaching_defs::Environment>;

class PartialCandidateAdapter;
class ReducedCFGClosureAdapter;

class CandidateAdapter {
 public:
  virtual const type_inference::TypeEnvironment& get_type_env() const = 0;
  virtual const reaching_defs::Environment& get_rdef_env() const = 0;
  virtual void gather_type_demands(
      std::unordered_set<reg_t> regs_to_track,
      const std::function<bool(IRInstruction*, src_index_t)>& follow,
      std::unordered_set<const DexType*>* type_demands) const = 0;
  virtual bool contains(IRInstruction* insn) const = 0;
  virtual ~CandidateAdapter() {}
};

class OutlinerTypeAnalysis {
 public:
  OutlinerTypeAnalysis() = delete;
  OutlinerTypeAnalysis(const OutlinerTypeAnalysis&) = delete;
  OutlinerTypeAnalysis& operator=(const OutlinerTypeAnalysis&) = delete;
  explicit OutlinerTypeAnalysis(DexMethod* method);

  // Infer permissible result type of a set of instructions with a destination,
  // and possibly another incoming type. The return value nullptr indicates that
  // the result type could not be determined.
  const DexType* get_result_type(
      const CandidateAdapter* ca,
      const std::unordered_set<const IRInstruction*>& insns,
      const DexType* optional_extra_type);

  // Infer type demand imposed on a register anywhere in a partial candidate.
  // If there's no useful type demand, we fall back to type inference to see
  // what we can assume about the type.
  // The return value nullptr indicates that no useful type could be determined.
  const DexType* get_type_demand(const CandidateAdapter& ca, reg_t reg);

  // Infer type of a register at a particular instruction.
  // If we cannot get enough detail from type inference, go back to
  // reaching definitions.
  // The return value nullptr indicates that a type could not be inferred.
  const DexType* get_inferred_type(const CandidateAdapter& ca, reg_t reg);

 private:
  DexMethod* m_method;
  Lazy<ReachingDefsEnvironments> m_reaching_defs_environments;
  Lazy<TypeEnvironments> m_type_environments;
  Lazy<constant_uses::ConstantUses> m_constant_uses;

  const DexType* narrow_type_demands(
      std::unordered_set<const DexType*> type_demands);

  size_t get_load_param_index(const IRInstruction* load_param_insn);

  const DexType* get_result_type_helper(const IRInstruction* insn);

  const DexType* get_type_of_reaching_defs(const CandidateAdapter& ca,
                                           reg_t reg);

  const DexType* get_if_insn_type_demand(IRInstruction* insn);

  const DexType* get_type_demand(IRInstruction* insn, size_t src_index);

  boost::optional<std::vector<const IRInstruction*>> get_defs(
      const std::unordered_set<const IRInstruction*>& insns);

  void get_type_demand_helper(const CandidateAdapter& ca,
                              std::unordered_set<reg_t> regs_to_track,
                              std::unordered_set<const DexType*>* type_demands);

  const DexType* get_const_insns_type_demand(
      const CandidateAdapter* ca,
      const std::unordered_set<const IRInstruction*>& const_insns);

  const DexType* get_type_of_defs(const CandidateAdapter* ca,
                                  const std::vector<const IRInstruction*>& defs,
                                  const DexType* optional_extra_type);

  friend PartialCandidateAdapter;
  friend ReducedCFGClosureAdapter;
}; // class OutlinerTypeAnalysis

} // namespace outliner_impl
