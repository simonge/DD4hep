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
#include <DD4hep/DetFactoryHelper.h>
#include <DD4hep/Printout.h>
#include <DD4hep/Plugins.h>
#include <DDG4/Geant4RunAction.h>
#include <DDG4/Geant4Context.h>
#include <DDG4/Geant4Kernel.h>
#include <DDG4/RunParameters.h>
#include <DDG4/FileParameters.h>
#include <DDG4/Factories.h>
#include <XML/DocumentHandler.h>
#include <XML/Utilities.h>

// Geant4 headers
#include <G4Run.hh>

// C/C++ include files
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <cmath>

using namespace dd4hep::sim;
using namespace dd4hep;

namespace dd4hep {
  namespace sim {

    /// Helper class to store metadata configuration
    /**
     * This class stores metadata parameter specifications that are configured
     * via the DD4hep_RunMetadata plugin and later applied by the Geant4RunMetadata action.
     */
    class RunMetadataStore {
    public:
      /// Structure holding a single metadata parameter specification
      struct Parameter {
        std::string name;    ///< Parameter name for output
        std::string type;    ///< Parameter type: "int", "float", or "string"
        double dblValue;     ///< Evaluated double value
        int intValue;        ///< Evaluated int value
        std::string strValue; ///< Evaluated string value
        std::string branch;  ///< Metadata branch: "runs" or "metadata"
      };
      
      /// Collection of configured parameters
      std::vector<Parameter> parameters;
      
      /// Flag indicating whether action should auto-register
      bool autoRegister = true;
      
      /// Get the singleton instance for a given detector
      static RunMetadataStore& instance(Detector& detector) {
        static std::mutex s_mutex;
        static std::map<Detector*, std::shared_ptr<RunMetadataStore>> stores;
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = stores.find(&detector);
        if (it == stores.end()) {
          auto store = std::make_shared<RunMetadataStore>();
          stores[&detector] = store;
          return *store;
        }
        return *it->second;
      }
      
      /// Try to auto-register the action if Geant4Kernel exists
      static void tryAutoRegister(Detector& detector) {
        try {
          RunMetadataStore& store = instance(detector);
          if (!store.autoRegister || store.parameters.empty()) {
            return;
          }
          
          Geant4Kernel* kernel = &Geant4Kernel::instance(detector);
          if (kernel) {
            Geant4RunMetadata* action = new Geant4RunMetadata(kernel, "Geant4RunMetadata");
            kernel->registerGlobalAction(action);
            kernel->runAction().adopt(action);
            store.autoRegister = false;  // Only register once
            
            printout(INFO, "RunMetadata", 
                     "++ Automatically registered Geant4RunMetadata action");
          }
        } catch (...) {
          // Geant4Kernel doesn't exist yet or other error - will try again later
        }
      }
      
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

    /// Helper class to access protected members of ExtensionParameters
    /// This is a standard C++ pattern for accessing protected members from derived classes
    /// within the same namespace, used to avoid modifying the base ExtensionParameters class
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
    }

    /// Geant4 action to apply run metadata from the detector description
    /**
     * This action reads metadata configuration stored by the DD4hep_RunMetadata plugin
     * and applies it to RunParameters or FileParameters extensions.
     *
     * The metadata is configured in the compact XML file using the DD4hep_RunMetadata plugin.
     * No explicit action registration is required - the plugin handles everything.
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
          return;  // No metadata configured
        }

        Geant4Run* g4run = context()->run();
        
        // Process each configured parameter
        for (const auto& param : store.parameters) {
          try {
            if (param.branch == "runs") {
              // Apply to run parameters
              RunParameters* runParams = g4run->extension<RunParameters>(false);
              if (!runParams) {
                runParams = new RunParameters();
                g4run->addExtension<RunParameters>(runParams);
              }
              applyToRunParameters(runParams, param);
            }
            else if (param.branch == "metadata") {
              // Apply to file parameters
              FileParameters* fileParams = g4run->extension<FileParameters>(false);
              if (!fileParams) {
                fileParams = new FileParameters();
                g4run->addExtension<FileParameters>(fileParams);
              }
              applyToFileParameters(fileParams, param);
            }
          } catch (std::exception& e) {
            error("Failed to apply metadata parameter '%s': %s", 
                  param.name.c_str(), e.what());
          }
        }
      }

    private:
      void applyToRunParameters(RunParameters* params, const RunMetadataStore::Parameter& param) {
        if (param.type == "int") {
          RunParamsAccessor::addInt(params, param.name, param.intValue);
          info("  Added run parameter (int): %s = %d", param.name.c_str(), param.intValue);
        }
        else if (param.type == "float") {
          float value = static_cast<float>(param.dblValue);
          RunParamsAccessor::addFloat(params, param.name, value);
          info("  Added run parameter (float): %s = %f", param.name.c_str(), value);
        }
        else if (param.type == "string") {
          RunParamsAccessor::addString(params, param.name, param.strValue);
          info("  Added run parameter (string): %s = %s", param.name.c_str(), param.strValue.c_str());
        }
      }

      void applyToFileParameters(FileParameters* params, const RunMetadataStore::Parameter& param) {
        if (param.type == "int") {
          FileParamsAccessor::addInt(params, param.name, param.intValue);
          info("  Added file parameter (int): %s = %d", param.name.c_str(), param.intValue);
        }
        else if (param.type == "float") {
          float value = static_cast<float>(param.dblValue);
          FileParamsAccessor::addFloat(params, param.name, value);
          info("  Added file parameter (float): %s = %f", param.name.c_str(), value);
        }
        else if (param.type == "string") {
          FileParamsAccessor::addString(params, param.name, param.strValue);
          info("  Added file parameter (string): %s = %s", param.name.c_str(), param.strValue.c_str());
        }
      }
    };

  }  // End namespace sim
}    // End namespace dd4hep

/// Do not clutter global namespace
namespace  {

  using namespace dd4hep;
  using namespace dd4hep::sim;

  /// Plugin to configure metadata from XML
  /** Plugin to store metadata configuration from compact XML files.
   *  The metadata is stored in the detector instance and can be
   *  accessed later by Geant4 output actions.
   *  
   *  Usage in XML (compact file):
   *  <plugins>
   *    <plugin name="DD4hep_RunMetadata" type="runs">
   *      <parameter name="BeamEnergy_electron" type="float" 
   *                 value="250.0*GeV"/>
   *      <parameter name="ColliderEnergy" type="float" 
   *                 value="500.0*GeV"/>
   *      <parameter name="DetectorVersion" type="string" 
   *                 value="ILD_l5_v02"/>
   *    </plugin>
   *  </plugins>
   *  
   *  The type attribute on the plugin tag specifies the metadata branch:
   *  - "runs" - Run-level metadata (default)
   *  - "metadata" - File-level metadata
   *  
   *  The value attribute is evaluated as a DD4hep expression, allowing
   *  references to constants and units (e.g., "250.0*GeV", "DetectorVersion").
   *  
   *  The Geant4RunMetadata action will be automatically created and registered
   *  when Geant4 is initialized. No manual action registration is required.
   *  
   *  \author M.Frank
   *  \date   2024
   */
  long configure_run_metadata(Detector& detector, xml_h e)   {
    xml_comp_t c = e;
    std::string branch = c.attr<std::string>(_U(type), "runs");
    
    if (branch != "runs" && branch != "metadata") {
      printout(ERROR, "RunMetadata", 
               "++ Invalid metadata branch '%s'. Must be 'runs' or 'metadata'",
               branch.c_str());
      return 0;
    }
    
    RunMetadataStore& store = RunMetadataStore::instance(detector);
    std::size_t count = 0;
    
    for (xml_coll_t coll(e, _U(parameter)); coll; ++coll) {
      xml_comp_t param = coll;
      
      RunMetadataStore::Parameter p;
      p.name = param.attr<std::string>(_U(name));
      p.type = param.attr<std::string>(_U(type));
      p.branch = branch;
      
      // Validate type
      if (p.type != "int" && p.type != "float" && p.type != "string") {
        printout(ERROR, "RunMetadata", 
                 "++ Invalid parameter type '%s' for parameter '%s'. Must be 'int', 'float', or 'string'",
                 p.type.c_str(), p.name.c_str());
        continue;
      }
      
      // Get the value attribute
      std::string valueStr = param.attr<std::string>(_U(value));
      
      // Evaluate the value based on type
      try {
        if (p.type == "string") {
          // For string type, use the value directly (no evaluation)
          p.strValue = valueStr;
        } else if (p.type == "int") {
          // For int type, evaluate as expression and convert to int with rounding
          p.dblValue = xml::_toDouble(valueStr.c_str());
          p.intValue = static_cast<int>(std::round(p.dblValue));
        } else if (p.type == "float") {
          // For float type, evaluate as expression
          p.dblValue = xml::_toDouble(valueStr.c_str());
        }
      } catch (std::exception& ex) {
        printout(ERROR, "RunMetadata", 
                 "++ Failed to evaluate value '%s' for parameter '%s': %s",
                 valueStr.c_str(), p.name.c_str(), ex.what());
        continue;
      }
      
      store.parameters.push_back(p);
      ++count;
      
      if (p.type == "string") {
        printout(INFO, "RunMetadata", 
                 "++ Registered metadata parameter '%s' (type=%s, value=%s, branch=%s)",
                 p.name.c_str(), p.type.c_str(), p.strValue.c_str(), p.branch.c_str());
      } else if (p.type == "int") {
        printout(INFO, "RunMetadata", 
                 "++ Registered metadata parameter '%s' (type=%s, value=%d, branch=%s)",
                 p.name.c_str(), p.type.c_str(), p.intValue, p.branch.c_str());
      } else {
        printout(INFO, "RunMetadata", 
                 "++ Registered metadata parameter '%s' (type=%s, value=%g, branch=%s)",
                 p.name.c_str(), p.type.c_str(), p.dblValue, p.branch.c_str());
      }
    }
    
    printout(INFO, "RunMetadata", "++ Registered %ld metadata parameters for branch '%s'", 
             count, branch.c_str());
    
    // Try to auto-register the Geant4 action if kernel exists
    RunMetadataStore::tryAutoRegister(detector);
    
    return 1;
  }
  
}  // End anonymous namespace

  /// Apply plugin to manually trigger action registration
  /** This plugin can be called from Geant4 setup to ensure the action is registered.
   *  Normally this happens automatically, but this can be called explicitly if needed.
   *  
   *  Usage in Geant4 XML (optional):
   *  <plugins>
   *    <plugin name="DD4hep_RunMetadata_apply"/>
   *  </plugins>
   */
  long apply_run_metadata(Detector& detector, xml_h /* e */)   {
    RunMetadataStore::tryAutoRegister(detector);
    return 1;
  }

}  // End anonymous namespace

// Factory declarations
DECLARE_XML_PLUGIN(DD4hep_RunMetadata, configure_run_metadata)
DECLARE_XML_PLUGIN(DD4hep_RunMetadata_apply, apply_run_metadata)
DECLARE_GEANT4ACTION(Geant4RunMetadata)
