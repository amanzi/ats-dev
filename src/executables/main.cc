#include <iostream>

#include <Epetra_Comm.h>
#include <Epetra_MpiComm.h>
#include "Epetra_SerialComm.h"

#include "Teuchos_ParameterList.hpp"
#include "Teuchos_ParameterXMLFileReader.hpp"
#include "Teuchos_XMLParameterListHelpers.hpp"
#include "Teuchos_CommandLineProcessor.hpp"
#include "Teuchos_StandardParameterEntryValidators.hpp"
#include "Teuchos_VerboseObjectParameterListHelpers.hpp"

#include "VerboseObject_objs.hh"

#include "ats_version.hh"
#include "amanzi_version.hh"
#include "tpl_versions.h"

#include "dbc.hh"
#include "errors.hh"
#include "simulation_driver.hh"

// registration files
#include "state_evaluators_registration.hh"

#include "ats_relations_registration.hh"
#include "ats_transport_registration.hh"
#include "ats_energy_pks_registration.hh"
#include "ats_energy_relations_registration.hh"
#include "ats_flow_pks_registration.hh"
#include "ats_flow_relations_registration.hh"
#include "ats_deformation_registration.hh"
#include "ats_bgc_registration.hh"
#include "ats_surface_balance_registration.hh"
#include "ats_mpc_registration.hh"
#include "ats_sediment_transport_registration.hh"
#include "mdm_transport_registration.hh"
#include "multiscale_transport_registration.hh"
#ifdef ALQUIMIA_ENABLED
#include "pks_chemistry_registration.hh"
#endif


// include fenv if it exists
#include "boost/version.hpp"
#if (BOOST_VERSION / 100 % 1000 >= 46)
#include "boost/config.hpp"
#ifndef BOOST_NO_FENV_H
#ifdef _GNU_SOURCE
#define AMANZI_USE_FENV
#include "boost/detail/fenv.hpp"
#endif
#endif
#endif

#include <boost/filesystem.hpp>
//using namespace boost::filesystem;


Teuchos::EVerbosityLevel Amanzi::VerbosityLevel::level_ = Teuchos::VERB_MEDIUM;

int main(int argc, char *argv[])
{

#ifdef AMANZI_USE_FENV
  //  feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
  feraiseexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

  Teuchos::GlobalMPISession mpiSession(&argc,&argv,0);
  int rank = mpiSession.getRank();

  try{
    Teuchos::CommandLineProcessor CLP;
    CLP.setDocString("\nATS: simulations for ecosystem hydrology\n");

    std::string xmlInFileName = "";
    CLP.setOption("xml_file", &xmlInFileName, "XML options file");
    
    bool print_version(false);
    CLP.setOption("print_version", "no_print_version", &print_version, "Print version number and exit.");

    CLP.throwExceptions(false);
    CLP.recogniseAllOptions(true);

    Teuchos::CommandLineProcessor::EParseCommandLineReturn
      parseReturn = CLP.parse(argc, argv);

    if (parseReturn == Teuchos::CommandLineProcessor::PARSE_HELP_PRINTED) {
      return 0;
    }
    if (parseReturn != Teuchos::CommandLineProcessor::PARSE_SUCCESSFUL) {
      return 1;
    }

    // strinigy magic
#define XSTR(s) STR(s)
#define STR(s) #s

    // check for verbose option
    if (print_version) {
      if (rank == 0) {
        std::cout << std::endl;
        std::cout << "ATS Version     " << XSTR(ATS_VERSION) << std::endl;
        std::cout << "GIT branch      " << XSTR(ATS_GIT_BRANCH) << std::endl;
        std::cout << "GIT global hash " << XSTR(ATS_GIT_GLOBAL_HASH) << std::endl;
        std::cout << std::endl;
        std::cout << "Amanzi Version  " << XSTR(AMANZI_VERSION) << std::endl;
        std::cout << "GIT branch      " << XSTR(AMANZI_GIT_BRANCH) << std::endl;
        std::cout << "GIT global hash " << XSTR(AMANZI_GIT_GLOBAL_HASH) << std::endl;
        std::cout << std::endl;
      }

      if (!boost::filesystem::exists(xmlInFileName)) {
        throw std::string("ATS not run");
      }
   
    }

    // this only make sense if the default is blank
    if (xmlInFileName.size() == 0) {
      if (rank == 0) {
        std::cout << "ERROR: No xml input file was specified. Use the command line option --xml_file to specify one." << std::endl;
      }      
      throw std::string("ATS not run");      
    }
    
    MPI_Comm mpi_comm(MPI_COMM_WORLD);

    // read the main parameter list
    Teuchos::RCP<Teuchos::ParameterList> plist = Teuchos::getParametersFromXmlFile(xmlInFileName); 
    
    Teuchos::RCP<Teuchos::FancyOStream> fos;
    Teuchos::readVerboseObjectSublist(&*plist, &fos, &Amanzi::VerbosityLevel::level_);
    
    SimulationDriver simulator;
    int ret = simulator.Run(mpi_comm, *plist);
  }
  
  catch (std::string& s) {
    if (rank == 0) {
      if (s == "ATS not run") {
        std::cout << "ATS::SIMULATION_DID_NOT_RUN\n";
      } 
    }
    return 1;
  }
  
}


