//==========================================================================
//  AIDA Detector description implementation 
//--------------------------------------------------------------------------
// Copyright (C) Organisation europeenne pour la Recherche nucleaire (CERN)
// All rights reserved.
//
// For the licensing terms see $DD4hepINSTALL/LICENSE.
// For the list of contributors see $DD4hepINSTALL/doc/CREDITS.
//
// Author     : Generated for DD4hep
//
//==========================================================================

// Framework include files
#include <DD4hep/Detector.h>
#include <DD4hep/Printout.h>
#include <DDG4/Geant4RunAction.h>
#include <DDG4/Geant4Context.h>
#include <DDG4/Geant4Kernel.h>
#include <DDG4/RunParameters.h>
#include <DDG4/Factories.h>

// Geant4 headers
#include <G4Run.hh>

// C/C++ include files
#include <string>
#include <vector>
#include <sstream>

using namespace dd4hep::sim;
using namespace dd4hep;

namespace dd4hep {
  namespace sim {

    /// Helper class to access protected members of RunParameters
    /// This is a standard C++ pattern for accessing protected members from
    /// non-derived classes within the same namespace
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
      
      void addIntParameterToRun(RunParameters* params, const std::string& name, int value) {
        RunParamsAccessor::addInt(params, name, value);
      }
      
      void addFloatParameterToRun(RunParameters* params, const std::string& name, float value) {
        RunParamsAccessor::addFloat(params, name, value);
      }
      
      void addStringParameterToRun(RunParameters* params, const std::string& name, const std::string& value) {
        RunParamsAccessor::addString(params, name, value);
      }
    }

    /// Plugin to add metadata from XML configuration to run parameters
    /**
     * This plugin allows metadata to be inserted from XML into the Geant4 metadata
     * collections (e.g. "runs"/"metadata"). It follows the same concept as the 
     * command line approach (e.g. --meta.runParameters "BeamEnergy_electron/F=ElectronBeamEnergy/GeV").
     * 
     * The parameter name to put into the tree is specified along with the value,
     * which is interpreted from constants defined elsewhere in the detector description.
     *
     * Usage in XML:
     * <action name="Geant4MetadataAction/MetadataAction">
     *   <properties runParameters="BeamEnergy_electron/F=ElectronBeamEnergy/GeV,ColliderEnergy/F=ColliderEnergy/GeV"/>
     * </action>
     *
     * The format is: ParameterName/Type=ConstantName[/Unit]
     * where:
     *   - ParameterName: Name to store in the output metadata
     *   - Type: Data type (I=int, F=float, C=string)
     *   - ConstantName: Name of the constant in the detector description
     *   - Unit (optional): Unit to divide the constant value by (e.g., GeV, mm)
     *
     * Multiple parameters can be specified separated by commas.
     *
     * \author  DD4hep Project
     * \version 1.0
     * \ingroup DD4HEP_SIMULATION
     */
    class Geant4MetadataAction : public Geant4RunAction {
    protected:
      /// Property: Run parameters specifications
      std::vector<std::string> m_runParameters;

      /// Helper struct to hold parsed parameter specification
      struct ParameterSpec {
        std::string outputName;     // Name to use in output
        std::string type;           // I, F, or C
        std::string constantName;   // Name of constant in detector description
        std::string unit;           // Optional unit (e.g., GeV, mm)
      };

      /// Parse a single parameter specification
      ParameterSpec parseParameterSpec(const std::string& spec) {
        ParameterSpec result;
        
        // Format: OutputName/Type=ConstantName[/Unit]
        size_t eqPos = spec.find('=');
        if (eqPos == std::string::npos) {
          throw std::runtime_error("Invalid parameter specification: " + spec + 
                                   " (missing '=')");
        }

        std::string lhs = spec.substr(0, eqPos);  // OutputName/Type
        std::string rhs = spec.substr(eqPos + 1); // ConstantName[/Unit]

        // Parse left-hand side: OutputName/Type
        size_t slashPos = lhs.find('/');
        if (slashPos == std::string::npos) {
          throw std::runtime_error("Invalid parameter specification: " + spec + 
                                   " (missing type '/' in left-hand side)");
        }
        result.outputName = lhs.substr(0, slashPos);
        result.type = lhs.substr(slashPos + 1);

        // Validate type
        if (result.type != "I" && result.type != "F" && result.type != "C") {
          throw std::runtime_error("Invalid parameter type: " + result.type + 
                                   " (must be I, F, or C)");
        }

        // Parse right-hand side: ConstantName[/Unit]
        slashPos = rhs.find('/');
        if (slashPos != std::string::npos) {
          result.constantName = rhs.substr(0, slashPos);
          result.unit = rhs.substr(slashPos + 1);
        } else {
          result.constantName = rhs;
          result.unit = "";
        }

        return result;
      }

      /// Get unit value from the detector description
      /// Handles both integer and floating-point constants
      double getUnitValue(const std::string& unitName, Detector& description) {
        if (unitName.empty()) {
          return 1.0;
        }
        try {
          // Try to get as double first (most common case)
          return description.constantAsDouble(unitName);
        } catch (...) {
          // If that fails, might be defined as integer
          try {
            return static_cast<double>(description.constantAsLong(unitName));
          } catch (std::exception& e) {
            warning("Unit '%s' not found in constants, using 1.0", unitName.c_str());
            return 1.0;
          }
        }
      }

    public:
      /// Standard constructor
      Geant4MetadataAction(Geant4Context* context, const std::string& name)
        : Geant4RunAction(context, name) {
        declareProperty("runParameters", m_runParameters);
        Geant4Action::runAction().callAtBegin(this, &Geant4MetadataAction::begin);
      }

      /// Default destructor
      virtual ~Geant4MetadataAction() = default;

      /// Begin-of-run callback
      void begin(const G4Run* run) {
        if (m_runParameters.empty()) {
          info("No run parameters configured");
          return;
        }

        // Get the detector description
        Detector& description = context()->kernel().detectorDescription();
        
        // Get or create RunParameters extension
        Geant4Run* g4run = context()->run();
        RunParameters* runParams = g4run->extension<RunParameters>(false);
        if (!runParams) {
          runParams = new RunParameters();
          g4run->addExtension<RunParameters>(runParams);
        }

        // Process each parameter specification
        for (const auto& paramSpec : m_runParameters) {
          try {
            ParameterSpec spec = parseParameterSpec(paramSpec);
            
            info("Processing metadata parameter: %s", paramSpec.c_str());
            
            // Get the constant value
            double unitValue = getUnitValue(spec.unit, description);
            
            if (spec.type == "I") {
              // Integer parameter - try double first, then long
              int value;
              try {
                value = static_cast<int>(description.constantAsDouble(spec.constantName) / unitValue);
              } catch (...) {
                value = static_cast<int>(description.constantAsLong(spec.constantName) / static_cast<long>(unitValue));
              }
              addIntParameterToRun(runParams, spec.outputName, value);
              info("  Added int parameter: %s = %d (from %s/%s)", 
                   spec.outputName.c_str(), value, spec.constantName.c_str(),
                   spec.unit.empty() ? "1" : spec.unit.c_str());
            }
            else if (spec.type == "F") {
              // Float parameter - try double first, then long
              float value;
              try {
                value = static_cast<float>(description.constantAsDouble(spec.constantName) / unitValue);
              } catch (...) {
                value = static_cast<float>(description.constantAsLong(spec.constantName) / unitValue);
              }
              addFloatParameterToRun(runParams, spec.outputName, value);
              info("  Added float parameter: %s = %f (from %s/%s)", 
                   spec.outputName.c_str(), value, spec.constantName.c_str(),
                   spec.unit.empty() ? "1" : spec.unit.c_str());
            }
            else if (spec.type == "C") {
              // String parameter
              std::string value = description.constantAsString(spec.constantName);
              addStringParameterToRun(runParams, spec.outputName, value);
              info("  Added string parameter: %s = %s (from %s)", 
                   spec.outputName.c_str(), value.c_str(), spec.constantName.c_str());
            }
          }
          catch (std::exception& e) {
            error("Failed to process parameter specification '%s': %s", 
                  paramSpec.c_str(), e.what());
          }
        }
      }
    };

  }  // End namespace sim
}    // End namespace dd4hep

// Factory declaration
DECLARE_GEANT4ACTION(Geant4MetadataAction)
