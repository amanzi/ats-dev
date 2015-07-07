/*
  Plant Functionality Type class

  -- copy constructor of array?
  -- Epetra_Vector copy constructor copy values?

*/

#include "utils.hh"

#include "PFT.hh"

namespace Amanzi {
namespace BGC {

PFT::PFT(std::string pft_type_, int ncells) :
    pft_type(pft_type_),
    BRootSoil(ncells) {}

PFT::PFT(std::string pft_type_, int ncells, double* brootcells_) :
    pft_type(pft_type_),
    BRootSoil(View, brootcells_, ncells) {}

void PFT::Init(double col_area)
{
  
  SLA = 20;
  leaf2rootratio = 1.0;
  leaf2stemratio = 5;
  Vcmax25 = 100;
  Jmax25 = 200;
  GDDleafon = 100;
  GDDbase = 0.0;
  GDD = 0.0;
  Bleaf = 1.0/SLA * col_area;//es note that all the following B vals are perm^2, so that elsewhere they should be *gridarea to account for varying grid areas.
  Bstore = 2* Bleaf;
  Bleafmemory = Bleaf;
  Bstem = Bleaf/leaf2stemratio;
  Broot = Bleaf/leaf2rootratio; 
  leaflitterfrc[0] = 0.1;
  leaflitterfrc[1] = 0.5;
  leaflitterfrc[1] = 0.4;
  rootlitterfrc[0] = 0.1;
  rootlitterfrc[1] = 0.5;
  rootlitterfrc[1] = 0.4;
  stemlitterfrc[0] = 0.05;
  stemlitterfrc[1] = 0.15;
  stemlitterfrc[1] = 0.8;
  LUE = 0.05;
  LER = 0.8;
  mp = 6.0;
  GPP = 0.0;
  annNPP = 0.0;
  NPP = 0.0;
  root2leafrespratio = 0.8;
  stem2leafrespratio = 0.05;
  maxRootD = 0.5;
  minLeafWP = -2.0;
  storagecleaf2sw = 1.5;
  storagecroot2sw = 15;
  rootlongevity = 4.0;
  leaflongevity = 4.0;
  stemlongevity = 10.0;
  tar_leafstorageccon = 0.2;
  Emax25 = 0.1*Vcmax25;
  gRespF = 0.25;
  storagecRspFrc = 0.8;
  leafstatus = 1;
  leafondays = 5;
  leafoffdays = 5;
  leafoffdaysi = 365.25;
  leafondaysi = 0;
  CSinkLimit = 0.0;
  seedrainlai = 0.01;
  totalBiomass = Bleaf + Bstem + Broot;
  maxLAI = 1;

  for (int i = 0; i < 10; i++){
    annCBalance[i] = 0;
  }
}

void PFT::Init(Teuchos::ParameterList& plist,double col_area)
{
  Init(col_area);
  // ex
  // rootlongevity = plist.get<double>("root longevity", 4.0);
  //note default vals below are those of sedge
       maxRootD = plist.get<double>("max root depth", 0.5);
       Vcmax25 = plist.get<double>("Vcmax25", 100.);
       Emax25 = plist.get<double>("Emax25", 10.);
       SLA = plist.get<double>("SLA", 16);

}


// Initialize the root distribution
void PFT::InitRoots(const Epetra_SerialDenseVector& SoilTArr,
                    const Epetra_SerialDenseVector& SoilDArr,
                    const Epetra_SerialDenseVector& SoilThicknessArr) {

  // locate the root depth
  int nSoilLayers = SoilTArr.Length();
  double initPermD = PermafrostDepth(SoilTArr,SoilThicknessArr,273.15);
  //rootD = std::min(maxRootD, initPermD);
  rootD = std::min(maxRootD, 0.3);//es following Xu's advice - 0.3m accounts for the previous year root depth that will contribute to this yr Broot. Note that if spinning data exists feed in last year's root depth instead of 0.3.
 
  double totalweights = 0.0;
  for (int c=0; c!=nSoilLayers; ++c) {
    if (SoilDArr[c] < rootD){
      BRootSoil[c] = SoilThicknessArr[c]*std::exp((rootD - SoilDArr[c]) / rootD);
      totalweights += BRootSoil[c];
    } else {
      BRootSoil[c] = 0.0;
    }
  }

  if (BRootSoil[0] <= 0.0) { //avoid the numerical errors
    BRootSoil[0] = 1.0;
    totalweights = 1.0;
  }

  for (int c=0; c!=nSoilLayers; ++c) {
    BRootSoil[c] = BRootSoil[c] / totalweights * Broot;
  }

  AssertRootBalance_or_die();
  return;
}

} // namespace
} // namespace
