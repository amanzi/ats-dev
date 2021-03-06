/*
  ATS is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/
//! Evaluates albedos and emissivities in a three-region subgrid model.

#include "Key.hh"
#include "albedo_threecomponent_evaluator.hh"
#include "seb_physics_defs.hh"
#include "seb_physics_funcs.hh"

namespace Amanzi {
namespace SurfaceBalance {
namespace Relations {

AlbedoThreeComponentEvaluator::AlbedoThreeComponentEvaluator(Teuchos::ParameterList& plist) :
    SecondaryVariablesFieldEvaluator(plist)
{
  // determine the domain
  domain_ = Keys::getDomain(Keys::cleanPListName(plist_.name()));
  domain_snow_ = Keys::readDomainHint(plist_, domain_, "surface", "snow");

  // my keys
  // -- sources
  albedo_key_ = Keys::readKey(plist, domain_, "albedos", "albedos");
  my_keys_.push_back(albedo_key_);
  emissivity_key_ = Keys::readKey(plist, domain_, "emissivities", "emissivities");
  my_keys_.push_back(emissivity_key_);

  // dependencies
  // -- snow properties
  snow_dens_key_ = Keys::readKey(plist, domain_snow_, "snow density", "density");
  dependencies_.insert(snow_dens_key_);

  // -- skin properties
  unfrozen_fraction_key_ = Keys::readKey(plist, domain_, "unfrozen fraction", "unfrozen_fraction");
  dependencies_.insert(unfrozen_fraction_key_);

  // parameters
  a_ice_ = plist_.get<double>("albedo ice [-]", 0.44);
  a_water_ = plist_.get<double>("albedo water [-]", 0.1168);

  e_ice_ = plist_.get<double>("emissivity ice [-]", 0.98);
  e_water_ = plist_.get<double>("emissivity water [-]", 0.995);
  e_snow_ = plist_.get<double>("emissivity ground surface [-]", 0.98);
}

// Required methods from SecondaryVariableFieldEvaluator
void
AlbedoThreeComponentEvaluator::EvaluateField_(const Teuchos::Ptr<State>& S,
        const std::vector<Teuchos::Ptr<CompositeVector> >& results)
{
  auto mesh = S->GetMesh(domain_);

  // collect dependencies
  const auto& snow_dens = *S->GetFieldData(snow_dens_key_)->ViewComponent("cell",false);
  const auto& unfrozen_fraction = *S->GetFieldData(unfrozen_fraction_key_)->ViewComponent("cell",false);

  // collect output vecs
  auto& albedo = *results[0]->ViewComponent("cell",false);
  auto& emissivity = *results[1]->ViewComponent("cell",false);

  emissivity(2)->PutScalar(e_snow_);

  for (const auto& lc : land_cover_) {
    AmanziMesh::Entity_ID_List lc_ids;
    mesh->get_set_entities(lc.first, AmanziMesh::Entity_kind::CELL,
                           AmanziMesh::Parallel_type::OWNED, &lc_ids);

    for (auto c : lc_ids) {
      // albedo of the snow
      albedo[2][c] = Relations::CalcAlbedoSnow(snow_dens[0][c]);

      // a and e of water
      albedo[1][c] = unfrozen_fraction[0][c] * a_water_ + (1-unfrozen_fraction[0][c]) * a_ice_;
      emissivity[1][c] = unfrozen_fraction[0][c] * e_water_ + (1-unfrozen_fraction[0][c]) * e_ice_;
      // a and e of soil
      albedo[0][c] = lc.second.albedo_ground;
      emissivity[0][c] = lc.second.emissivity_ground;
    }
  }
}

void
AlbedoThreeComponentEvaluator::EvaluateFieldPartialDerivative_(const Teuchos::Ptr<State>& S,
        Key wrt_key, const std::vector<Teuchos::Ptr<CompositeVector> > & results) {}


void AlbedoThreeComponentEvaluator::EnsureCompatibility(const Teuchos::Ptr<State>& S)
{
  // new state!
  if (land_cover_.size() == 0)
    land_cover_ = getLandCover(S->ICList().sublist("land cover types"),
            {"albedo_ground", "emissivity_ground"});

  CompositeVectorSpace domain_fac;
  domain_fac.SetMesh(S->GetMesh(domain_))
      ->SetGhosted()
      ->AddComponent("cell", AmanziMesh::CELL, 1);

  CompositeVectorSpace domain_fac_snow;
  domain_fac_snow.SetMesh(S->GetMesh(domain_snow_))
      ->SetGhosted()
      ->AddComponent("cell", AmanziMesh::CELL, 1);

  CompositeVectorSpace domain_fac_owned;
  domain_fac_owned.SetMesh(S->GetMesh(domain_))
      ->SetGhosted()
      ->SetComponent("cell", AmanziMesh::CELL, 3);

  // see if we can find a master fac
  for (auto my_key : my_keys_) {
    // Ensure my field exists, and claim ownership.
    auto my_fac = S->RequireField(my_key, my_key);
    my_fac->Update(domain_fac_owned);

    // Check plist for vis or checkpointing control.
    bool io_my_key = plist_.get<bool>("visualize", true);
    S->GetField(my_key, my_key)->set_io_vis(io_my_key);
    bool checkpoint_my_key = plist_.get<bool>("checkpoint", false);
    S->GetField(my_key, my_key)->set_io_checkpoint(checkpoint_my_key);
  }

  // Loop over dependencies, making sure they are the same mesh
  for (auto key : dependencies_) {
    auto fac = S->RequireField(key);
    if (Keys::starts_with(key, domain_snow_)) {
      fac->Update(domain_fac_snow);
    } else {
      fac->Update(domain_fac);
    }

    // Recurse into the tree to propagate info to leaves.
    S->RequireFieldEvaluator(key)->EnsureCompatibility(S);
  }
}

}  // namespace Relations
}  // namespace AmanziFlow
}  // namespace Amanzi
