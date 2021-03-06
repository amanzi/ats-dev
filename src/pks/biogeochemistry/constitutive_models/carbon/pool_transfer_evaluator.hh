/* -*-  mode: c++; indent-tabs-mode: nil -*- */

/*
  Evaluates carbon pool turnover.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#ifndef AMANZI_BGCRELATIONS_POOL_DECOMP_HH_
#define AMANZI_BGCRELATIONS_POOL_DECOMP_HH_

#include "Factory.hh"
#include "secondary_variables_field_evaluator.hh"

class Epetra_SerialDenseVector;
class Epetra_SerialDenseMatrix;

namespace Amanzi {
namespace BGC {
namespace BGCRelations {

class PoolTransferEvaluator : public SecondaryVariablesFieldEvaluator {
 public:
  explicit
  PoolTransferEvaluator(Teuchos::ParameterList& plist);
  PoolTransferEvaluator(const PoolTransferEvaluator& other);
  Teuchos::RCP<FieldEvaluator> Clone() const;

  // Required methods from SecondaryVariablesFieldEvaluator
  virtual void EvaluateField_(const Teuchos::Ptr<State>& S,
          const std::vector<Teuchos::Ptr<CompositeVector> >& results);
  virtual void EvaluateFieldPartialDerivative_(const Teuchos::Ptr<State>& S,
          Key wrt_key, const std::vector<Teuchos::Ptr<CompositeVector> >& results);


 protected:
  void InitModel_(const Teuchos::Ptr<State>& S, int npools);
  void InitCenturyModel_(double percent_sand);
  
 protected:
  Key carbon_key_;
  Key decay_key_;

  Key partition_key_;
  std::vector<Epetra_SerialDenseVector> resp_frac_;
  std::vector<Epetra_SerialDenseMatrix> transfer_frac_;
  bool init_model_;
  
 private:
  static Utils::RegisteredFactory<FieldEvaluator,PoolTransferEvaluator> fac_;



};

} // namespace
} // namespace
} // namespace

#endif
