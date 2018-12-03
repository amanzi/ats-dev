/* -*-  mode: c++; indent-tabs-mode: nil -*- */

/*
  Evaluates the Kr associated with the unfrozen fraction of water.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#include "boost/math/constants/constants.hpp"
#include <cmath>

#include "dbc.hh"
#include "errors.hh"
#include "one_uf_relperm_model.hh"

namespace Amanzi {
namespace Flow {

OneUFRelPermModel::OneUFRelPermModel(Teuchos::ParameterList& plist) :
    plist_(plist),
    pi_(boost::math::constants::pi<double>()) {

  alpha_ = plist_.get<int>("unfrozen rel perm alpha", 4);
  if (alpha_ % 2 != 0) {
    Errors::Message message("Unfrozen Fraction Rel Perm: alpha must be an even integer");
    Exceptions::amanzi_throw(message);
  }

  h_cutoff_ = plist_.get<double>("unfrozen rel perm cutoff pressure", 100.);
}

double
OneUFRelPermModel::SurfaceRelPerm(double uf, double h) {
  double kr;
  // if (h <= 101325. - h_cutoff_) {
  //   kr = 1.;
  // } else if (h >= 101325.) {
  //   kr = std::pow(std::sin(pi_ * uf / 2.), alpha_);
  // } else {
  //   double fac = (101325. - h)/ h_cutoff_;
  //   kr = fac + (1-fac) * std::pow(std::sin(pi_ * uf / 2.), alpha_);
  // }

  if (h <= 101325.) {
    kr = 1.;
  } else if (h >= 101325. + h_cutoff_) {
    kr = std::pow(std::sin(pi_ * uf / 2.), alpha_);
  } else {
    double fac = (101325. + h_cutoff_ - h)/ h_cutoff_;
    kr = fac + (1-fac) * std::pow(std::sin(pi_ * uf / 2.), alpha_);
  }
  return kr;
}



} // namespace
} // namespace

