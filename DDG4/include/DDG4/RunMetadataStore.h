//==========================================================================
//  AIDA Detector description implementation 
//--------------------------------------------------------------------------
// Copyright (C) Organisation europeenne pour la Recherche nucleaire (CERN)
// All rights reserved.
//
// For the licensing terms see $DD4hepINSTALL/LICENSE.
// For the list of contributors see $DD4hepINSTALL/doc/CREDITS.
//
// Author     : M.Frank
//
//==========================================================================
#ifndef DDG4_RUNMETADATASTORE_H
#define DDG4_RUNMETADATASTORE_H

// Framework include files
#include <DD4hep/Detector.h>

// C/C++ include files
#include <string>
#include <vector>
#include <memory>

/// Namespace for the AIDA detector description toolkit
namespace dd4hep {
  namespace sim {

    /// Helper class to store metadata configuration in the Detector instance
    /**
     * This class stores metadata parameter specifications that are configured
     * via the DD4hep_RunMetadata plugin and later applied by the Geant4RunMetadata action.
     * 
     * \author  M.Frank
     * \version 1.0
     * \ingroup DD4HEP_SIMULATION
     */
    class RunMetadataStore {
    public:
      /// Structure holding a single metadata parameter specification
      struct Parameter {
        std::string name;          ///< Parameter name for output
        std::string type;          ///< Parameter type: "int", "float", or "string"
        std::string constantName;  ///< Name of constant in detector description
        std::string unit;          ///< Optional unit for conversion
        std::string branch;        ///< Metadata branch: "runs", "metadata", or "events"
      };
      
      /// Collection of configured parameters
      std::vector<Parameter> parameters;
      
      /// Get the singleton instance for a given detector
      static RunMetadataStore& instance(Detector& detector);
      
      /// Destructor
      ~RunMetadataStore() = default;
      
    private:
      /// Private constructor for singleton
      RunMetadataStore() = default;
      
      /// No copy constructor
      RunMetadataStore(const RunMetadataStore&) = delete;
      
      /// No assignment operator
      RunMetadataStore& operator=(const RunMetadataStore&) = delete;
    };

  }  // End namespace sim
}    // End namespace dd4hep

#endif // DDG4_RUNMETADATASTORE_H
