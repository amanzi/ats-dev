/*
  ATS is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/
//! SEBThreeComponentEvaluator: evaluates the Surface Energy Balance model on subgrid units.

/*!

Calculates source terms for surface fluxes to and from the atmosphere and a
snowpack.  In the case of snow on the ground, this solves for a snow
temperature, given a skin temperature, that satisfies a energy balance
equation.  In the case of no-snow, this calculates a conductive heat flux to
the ground from the atmosphere.

This uses a 3-component subgrid model, setting up a collection of patches, for
portions of the column covered in snow, ponded water, and vegetated/bare
ground.  The surface energy balance on these area weighted patches are
individually calculated then averaged to form the total quantities.  All down-
and up-scaling of relevant quantities are done through the area weighting,
which is calculated by a minimum threshold in snow and a depression
depth/geometry-based approach for water.  All snow is assumed to first cover
water (likely ice), then cover land, as both water and snow prefer low-lying
depressions due to gravity- and wind-driven redistributions, respectively.

.. _seb_subgrid_evaluator-spec:
.. admonition:: seb_subgrid_evaluator-spec

   * `"wind speed reference height [m]`" ``[double]`` **2.0** Reference height at which
     wind speed is measured.
   * `"minimum wind speed [m s^-1]`" ``[double]`` **1.0** Sets a floor on wind speed for
     potential wierd data.  Models have trouble with no wind.
   * `"minimum relative humidity [-]`" ``[double]`` **1.0** Sets a floor on relative
     humidity for potential wierd data.  Models have trouble with no
     humidity.

   * `"save diagnostic data`" ``[bool]`` **false** Saves a suite of diagnostic variables to vis.

   * `"surface domain name`" ``[string]`` **DEFAULT** Default set by parameterlist name.
   * `"subsurface domain name`" ``[string]`` **DEFAULT** Default set relative to surface domain name.
   * `"snow domain name`" ``[string]`` **DEFAULT** Default set relative to surface domain name.

    KEYS:
    * `"surface water source`" **DOMAIN-water_source**  [m s^-1]
    * `"surface energy source`" **DOMAIN-total_energy_source** [MW m^-2]
    * `"subsurface water source`" **DOMAIN-water_source**  [mol s^-1]
    * `"subsurface energy source`" **DOMAIN-total_energy_source** [MW m^-3]
    * `"snow mass source - sink`" **DOMAIN-source_sink** [m_SWE s^-1]
    * `"new snow source`" **DOMAIN-source** [m_SWE s^-1]

    * `"albedo`"  [-]
    * `"snowmelt`" [m_SWE s^-1]
    * `"evaporation`" [m s^-1]
    * `"snow temperature`" [K]
    * `"sensible heat flux`" **DOMAIN-qE_sensible_heat** [W m^-2]
    * `"latent heat of evaporation`" **DOMAIN-qE_latent_heat** [W m^-2]
    * `"latent heat of snowmelt`" **DOMAIN-qE_snowmelt** [W m^-2]
    * `"outgoing longwave radiation`" **DOMAIN-qE_lw_out** [W m^-2]
    * `"conducted energy flux`" **DOMAIN-qE_conducted** [W m^-2]

    DEPENDENCIES:
    * `"incoming shortwave radiation`" [W m^-2]
    * `"incoming longwave radiation`" [W m^-2]
    * `"air temperature`" [K]
    * `"relative humidity`" [-]
    * `"wind speed`" [m s^-1]
    * `"precipitation rain`" [m s^-1]
    * `"precipitation snow`" [m_SWE s^-1]

    * `"volumetric snow depth`" [m] Area-averaged snow depth.
    * `"snow density`" [kg m^-3]
    * `"snow death rate`" [m s^-1] Snow "death" refers to the last bit of
       snowmelt that we want to remove discretely.
    * `"unfrozen fraction`" [-]  1 --> all surface water, 0 --> all surface ice
    * `"subgrid albedos`" [-] Dimension 3 field of (bare ground, water-covered, snow-covered) albedos.
    * `"subgrid emissivity`" [-] Dimension 3 field of (bare ground, water-covered, snow-covered) emissivities.
    * `"area fractions`" **DOMAIN-fractional_areas** Dimension 3 field of (bare
      ground, water-covered, snow-covered) area fractions (sum to 1).

    * `"temperature`" **DOMAIN-temperature**  [K] surface skin temperature.
    * `"pressure`" **DOMAIN-pressure** [Pa] surface skin pressure.
    * `"gas saturation`" **DOMAIN_SS-saturation_gas** [-] subsurface gas saturation
    * `"porosity`" [-] subsurface porosity
    * `"subsurface pressure`" **DOMAIN_SS-pressure** [Pa]
    * `"molar density liquid`" **DOMAIN-molar_density_liquid** [mol m^-3]
    * `"mass density liquid`" **DOMAIN-mass_density_liquid** [kg m^-3]

.. note:

   This also depends upon multiple parameters from the LandCover_ types:

   * `"roughness length of bare ground [m]`" ``[double]`` **0.04** Defines a fetch controlling
     latent and sensible heat fluxes.
   * `"roughness length of snow-covered ground [m]`" ``[double]`` **0.004** Defines a
     fetch controlling latent and sensible heat fluxes.
   * `"dessicated zone thickness [m]`" ``[double]`` Thickness of the immediate surface
     layer over which vapor pressure diffusion must move water to evaporate
     from dry soil.  More implies less evaporation.

*/

#pragma once

#include "Factory.hh"
#include "Debugger.hh"
#include "secondary_variables_field_evaluator.hh"
#include "LandCover.hh"

namespace Amanzi {
namespace SurfaceBalance {
namespace Relations {

class SEBThreeComponentEvaluator : public SecondaryVariablesFieldEvaluator {
 public:
  explicit
  SEBThreeComponentEvaluator(Teuchos::ParameterList& plist);
  SEBThreeComponentEvaluator(const SEBThreeComponentEvaluator& other) = default;

  virtual Teuchos::RCP<FieldEvaluator> Clone() const {
    return Teuchos::rcp(new SEBThreeComponentEvaluator(*this));
  }

  virtual void EnsureCompatibility(const Teuchos::Ptr<State>& S);

 protected:
  // Required methods from SecondaryVariableFieldEvaluator
  virtual void EvaluateField_(const Teuchos::Ptr<State>& S,
          const std::vector<Teuchos::Ptr<CompositeVector> >& results);

  virtual void EvaluateFieldPartialDerivative_(const Teuchos::Ptr<State>& S,
          Key wrt_key, const std::vector<Teuchos::Ptr<CompositeVector> > & results);

  // this is non-standard practice.  Implementing UpdateFieldDerivative_ to
  // override the default chain rule behavior, instead doing a numerical
  // finite difference
  virtual void UpdateFieldDerivative_(const Teuchos::Ptr<State>& S, Key wrt_key);

 protected:
  Key water_source_key_, energy_source_key_;
  Key ss_water_source_key_, ss_energy_source_key_;
  Key snow_source_key_, new_snow_key_;
  Key met_sw_key_, met_lw_key_, met_air_temp_key_, met_rel_hum_key_;
  Key met_wind_speed_key_, met_prain_key_, met_psnow_key_;
  Key snow_depth_key_, snow_dens_key_, snow_death_rate_key_;
  Key ponded_depth_key_, unfrozen_fraction_key_;
  Key sg_albedo_key_, sg_emissivity_key_, area_frac_key_;
  Key surf_temp_key_, surf_pres_key_;
  Key sat_gas_key_, poro_key_,ss_pres_key_;
  Key mol_dens_key_, mass_dens_key_;

  Key melt_key_, evap_key_;
  Key snow_temp_key_;
  Key qE_sh_key_, qE_lh_key_, qE_sm_key_, qE_lw_out_key_, qE_cond_key_;
  Key albedo_key_;

  Key domain_;
  Key domain_ss_;
  Key domain_snow_;

  double min_rel_hum_;       // relative humidity of 0 causes problems -- large evaporation
  double min_wind_speed_;       // wind speed of 0, under this model, would have 0 latent or sensible heat?
  double wind_speed_ref_ht_;    // reference height of the met data

  LandCoverMap land_cover_;

  bool compatible_;
  bool diagnostics_;
  Teuchos::RCP<Debugger> db_;
  Teuchos::RCP<Debugger> db_ss_;
  Teuchos::ParameterList plist_;

 private:
  static Utils::RegisteredFactory<FieldEvaluator,SEBThreeComponentEvaluator> reg_;
};

}  // namespace Relations
}  // namespace SurfaceBalance
}  // namespace Amanzi