/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

/*
  A linear sat-pc curve, plus a constant rel perm, makes the system linear, so
  nonlinear solver should always converge in one step.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#ifndef _FLOWRELATIONS_WRM_
#define _FLOWRELATIONS_WRM_

#include "Teuchos_ParameterList.hpp"

namespace Amanzi {
namespace Flow {
namespace FlowRelations {

class WRM {

public:
  // required methods from the base class
  virtual double k_relative(double pc) = 0;
  virtual double saturation(double pc) = 0;
  virtual double d_saturation(double pc) = 0;
  virtual double capillaryPressure(double saturation) = 0;
  virtual double residualSaturation() = 0;

};

} //namespace
} //namespace
} //namespace

#endif
