/* -*-  mode: c++; indent-tabs-mode: nil -*- */

/*
  This biomass model evaluates 
*/

#ifndef AMANZI_BIOMASS_EVALUATOR
#define AMANZI_BIOMASS_EVALUATOR

#include "secondary_variables_field_evaluator.hh"
#include "Factory.hh"

namespace Amanzi {

  class BiomassEvaluator : public SecondaryVariablesFieldEvaluator {

  public:

    explicit
    BiomassEvaluator(Teuchos::ParameterList& plist);
    BiomassEvaluator(const BiomassEvaluator& other);

    virtual Teuchos::RCP<FieldEvaluator> Clone() const override;

    virtual bool HasFieldChanged(const Teuchos::Ptr<State>& S, Key request) override;

  protected:
    // Required methods from SecondaryVariableFieldEvaluator
    virtual void EvaluateField_(const Teuchos::Ptr<State>& S,
                                const std::vector<Teuchos::Ptr<CompositeVector> >& results) override;
    virtual void EvaluateFieldPartialDerivative_(const Teuchos::Ptr<State>& S,
                                                 Key wrt_key, const std::vector<Teuchos::Ptr<CompositeVector> > & results) override;

    void InitializeFromPlist_();


    int nspecies_, type_;
    std::vector<double> alpha_n, alpha_h, alpha_d, alpha_a;
    std::vector<double> beta_n, beta_h, beta_d, beta_a;
    std::vector<double> Bmax, zmax, zmin;
    //std::vector<std::string> species_names_;
    double last_update_, update_frequency_;

    Key biomass_key_, stem_density_key_, stem_height_key_,  stem_diameter_key_, plant_area_key_;
    Key domain_name_, elev_key_, msl_key_;

 private:
  
  static Utils::RegisteredFactory<FieldEvaluator,BiomassEvaluator> factory_;
  };

} // namespace

#endif
