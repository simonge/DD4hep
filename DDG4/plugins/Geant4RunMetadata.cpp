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

// Framework include files
#include <DD4hep/Detector.h>
#include <DD4hep/Printout.h>
#include <DDG4/Geant4RunAction.h>
#include <DDG4/Geant4Context.h>
#include <DDG4/Geant4Kernel.h>
#include <DDG4/RunParameters.h>
#include <DDG4/EventParameters.h>
#include <DDG4/FileParameters.h>
#include <DDG4/RunMetadataStore.h>
#include <DDG4/Factories.h>

// Geant4 headers
#include <G4Run.hh>

// C/C++ include files
#include <string>
#include <vector>

using namespace dd4hep::sim;
using namespace dd4hep;

namespace dd4hep {
  namespace sim {

    /// Helper class to access protected members of ExtensionParameters
    namespace {
      class RunParamsAccessor : public RunParameters {
      public:
        static void addInt(RunParameters* p, const std::string& n, int v) {
          static_cast<RunParamsAccessor*>(p)->m_intValues[n] = {v};
        }
        static void addFloat(RunParameters* p, const std::string& n, float v) {
          static_cast<RunParamsAccessor*>(p)->m_fltValues[n] = {v};
        }
        static void addString(RunParameters* p, const std::string& n, const std::string& v) {
          static_cast<RunParamsAccessor*>(p)->m_strValues[n] = {v};
        }
      };
      
      class FileParamsAccessor : public FileParameters {
      public:
        static void addInt(FileParameters* p, const std::string& n, int v) {
          static_cast<FileParamsAccessor*>(p)->m_intValues[n] = {v};
        }
        static void addFloat(FileParameters* p, const std::string& n, float v) {
          static_cast<FileParamsAccessor*>(p)->m_fltValues[n] = {v};
        }
        static void addString(FileParameters* p, const std::string& n, const std::string& v) {
          static_cast<FileParamsAccessor*>(p)->m_strValues[n] = {v};
        }
      };
      
      class EventParamsAccessor : public EventParameters {
      public:
        static void addInt(EventParameters* p, const std::string& n, int v) {
          static_cast<EventParamsAccessor*>(p)->m_intValues[n] = {v};
        }
        static void addFloat(EventParameters* p, const std::string& n, float v) {
          static_cast<EventParamsAccessor*>(p)->m_fltValues[n] = {v};
        }
        static void addString(EventParameters* p, const std::string& n, const std::string& v) {
          static_cast<EventParamsAccessor*>(p)->m_strValues[n] = {v};
        }
      };
    }

    /// Get unit value from the detector description
    static double getUnitValue(const std::string& unitName, Detector& description) {
      if (unitName.empty()) {
        return 1.0;
      }
      try {
        return description.constantAsDouble(unitName);
      } catch (...) {
        try {
          return static_cast<double>(description.constantAsLong(unitName));
        } catch (std::exception& e) {
          printout(WARNING, "Geant4RunMetadata", "Unit '%s' not found in constants, using 1.0", 
                   unitName.c_str());
          return 1.0;
        }
      }
    }

    /// Geant4 action to apply run metadata from the detector description
    /**
     * This action reads metadata configuration stored by the DD4hep_RunMetadata plugin
     * and applies it to RunParameters, FileParameters, or EventParameters extensions.
     *
     * The metadata is configured in the compact XML file using the DD4hep_RunMetadata plugin.
     * This action is automatically applied during run initialization.
     *
     * \author  M.Frank
     * \version 1.0
     * \ingroup DD4HEP_SIMULATION
     */
    class Geant4RunMetadata : public Geant4RunAction {
    public:
      /// Standard constructor
      Geant4RunMetadata(Geant4Context* context, const std::string& name)
        : Geant4RunAction(context, name) {
        Geant4Action::runAction().callAtBegin(this, &Geant4RunMetadata::begin);
      }

      /// Default destructor
      virtual ~Geant4RunMetadata() = default;

      /// Begin-of-run callback
      void begin(const G4Run* /* run */) {
        Detector& description = context()->kernel().detectorDescription();
        RunMetadataStore& store = RunMetadataStore::instance(description);
        
        if (store.parameters.empty()) {
          info("No metadata parameters configured");
          return;
        }

        Geant4Run* g4run = context()->run();
        
        // Process each configured parameter
        for (const auto& param : store.parameters) {
          try {
            double unitValue = getUnitValue(param.unit, description);
            
            if (param.branch == "runs") {
              // Apply to run parameters
              RunParameters* runParams = g4run->extension<RunParameters>(false);
              if (!runParams) {
                runParams = new RunParameters();
                g4run->addExtension<RunParameters>(runParams);
              }
              applyToRunParameters(runParams, param, description, unitValue);
            }
            else if (param.branch == "metadata") {
              // Apply to file parameters
              FileParameters* fileParams = g4run->extension<FileParameters>(false);
              if (!fileParams) {
                fileParams = new FileParameters();
                g4run->addExtension<FileParameters>(fileParams);
              }
              applyToFileParameters(fileParams, param, description, unitValue);
            }
            else if (param.branch == "events") {
              // Event parameters require an event action, which is not implemented yet
              // This would require a separate Geant4EventAction to apply event-level metadata
              warning("Event-level metadata (branch='events') is not yet supported. "
                      "Parameter '%s' will be ignored.", param.name.c_str());
            }
            
          } catch (std::exception& e) {
            error("Failed to apply metadata parameter '%s': %s", 
                  param.name.c_str(), e.what());
          }
        }
      }

    private:
      void applyToRunParameters(RunParameters* params, const RunMetadataStore::Parameter& param,
                                Detector& description, double unitValue) {
        if (param.type == "int") {
          int value;
          try {
            value = static_cast<int>(description.constantAsDouble(param.constantName) / unitValue);
          } catch (...) {
            value = static_cast<int>(description.constantAsLong(param.constantName) / static_cast<long>(unitValue));
          }
          RunParamsAccessor::addInt(params, param.name, value);
          info("  Added run parameter (int): %s = %d", param.name.c_str(), value);
        }
        else if (param.type == "float") {
          float value;
          try {
            value = static_cast<float>(description.constantAsDouble(param.constantName) / unitValue);
          } catch (...) {
            value = static_cast<float>(description.constantAsLong(param.constantName) / unitValue);
          }
          RunParamsAccessor::addFloat(params, param.name, value);
          info("  Added run parameter (float): %s = %f", param.name.c_str(), value);
        }
        else if (param.type == "string") {
          std::string value = description.constantAsString(param.constantName);
          RunParamsAccessor::addString(params, param.name, value);
          info("  Added run parameter (string): %s = %s", param.name.c_str(), value.c_str());
        }
      }

      void applyToFileParameters(FileParameters* params, const RunMetadataStore::Parameter& param,
                                  Detector& description, double unitValue) {
        if (param.type == "int") {
          int value;
          try {
            value = static_cast<int>(description.constantAsDouble(param.constantName) / unitValue);
          } catch (...) {
            value = static_cast<int>(description.constantAsLong(param.constantName) / static_cast<long>(unitValue));
          }
          FileParamsAccessor::addInt(params, param.name, value);
          info("  Added file parameter (int): %s = %d", param.name.c_str(), value);
        }
        else if (param.type == "float") {
          float value;
          try {
            value = static_cast<float>(description.constantAsDouble(param.constantName) / unitValue);
          } catch (...) {
            value = static_cast<float>(description.constantAsLong(param.constantName) / unitValue);
          }
          FileParamsAccessor::addFloat(params, param.name, value);
          info("  Added file parameter (float): %s = %f", param.name.c_str(), value);
        }
        else if (param.type == "string") {
          std::string value = description.constantAsString(param.constantName);
          FileParamsAccessor::addString(params, param.name, value);
          info("  Added file parameter (string): %s = %s", param.name.c_str(), value.c_str());
        }
      }
    };

  }  // End namespace sim
}    // End namespace dd4hep

// Factory declaration
DECLARE_GEANT4ACTION(Geant4RunMetadata)
