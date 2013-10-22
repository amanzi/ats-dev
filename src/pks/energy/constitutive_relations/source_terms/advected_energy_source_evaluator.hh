/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

/*
  Source term evaluator for enthalpy of mass source.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#ifndef AMANZI_ENERGY_RELATIONS_ADVECTED_ENERGY_SOURCE_EVALUATOR_
#define AMANZI_ENERGY_RELATIONS_ADVECTED_ENERGY_SOURCE_EVALUATOR_

#include "factory.hh"
#include "secondary_variable_field_evaluator.hh"

namespace Amanzi {
namespace Energy {
namespace EnergyRelations {

class AdvectedEnergySourceEvaluator : public SecondaryVariableFieldEvaluator {

 public:
  // constructor format for all derived classes
  explicit
  AdvectedEnergySourceEvaluator(Teuchos::ParameterList& plist);
  AdvectedEnergySourceEvaluator(const AdvectedEnergySourceEvaluator& other);

  virtual Teuchos::RCP<FieldEvaluator> Clone() const;

  // Required methods from SecondaryVariableFieldEvaluator
  virtual void EvaluateField_(const Teuchos::Ptr<State>& S,
          const Teuchos::Ptr<CompositeVector>& results);
  virtual void EvaluateFieldPartialDerivative_(const Teuchos::Ptr<State>& S,
          Key wrt_key, const Teuchos::Ptr<CompositeVector>& results);

 protected:
  void InitializeFromPlist_();

  std::string domain_;
  Key internal_enthalpy_key_;
  Key external_enthalpy_key_;
  Key mass_source_key_;
  Key internal_density_key_;
  Key external_density_key_;

  bool include_conduction_;
  Key conducted_source_key_;

  Key cell_vol_key_;

 private:
  static Utils::RegisteredFactory<FieldEvaluator,AdvectedEnergySourceEvaluator> factory_;

};

} //namespace
} //namespace
} //namespace

#endif