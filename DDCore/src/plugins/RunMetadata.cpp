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
//
// Framework includes
#include <DD4hep/DetFactoryHelper.h>
#include <DD4hep/Printout.h>
#include <DD4hep/Plugins.h>
#include <DDG4/RunMetadataStore.h>
#include <XML/DocumentHandler.h>
#include <XML/Utilities.h>

// C/C++ include files
#include <string>

/// Do not clutter global namespace
namespace  {

  using namespace dd4hep;
  using namespace dd4hep::sim;

  /// Plugin to configure metadata from XML
  /** Plugin to store metadata configuration from compact XML files.
   *  The metadata is stored in the detector instance and can be
   *  accessed later by Geant4 output actions.
   *  
   *  Usage in XML:
   *  <plugins>
   *    <plugin name="DD4hep_RunMetadata" type="runs">
   *      <parameter name="BeamEnergy_electron" type="float" 
   *                 constant="ElectronBeamEnergy" unit="GeV"/>
   *      <parameter name="ColliderEnergy" type="float" 
   *                 constant="ColliderEnergy" unit="GeV"/>
   *      <parameter name="DetectorVersion" type="string" 
   *                 constant="DetectorVersion"/>
   *    </plugin>
   *  </plugins>
   *  
   *  The type attribute on the plugin tag specifies the metadata branch:
   *  - "runs" - Run-level metadata (default)
   *  - "metadata" - File-level metadata
   *  - "events" - Event-level metadata
   *  
   *  \author M.Frank
   *  \date   18.12.2024
   */
  long configure_run_metadata(Detector& detector, xml_h e)   {
    xml_comp_t c = e;
    std::string branch = c.attr<std::string>(_U(type), "runs");
    
    if (branch != "runs" && branch != "metadata" && branch != "events") {
      printout(ERROR, "RunMetadata", "++ Invalid metadata branch '%s'. Must be 'runs', 'metadata', or 'events'",
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
      p.constantName = param.attr<std::string>(_U(constant));
      p.unit = param.attr<std::string>(_U(unit), "");
      p.branch = branch;
      
      // Validate type
      if (p.type != "int" && p.type != "float" && p.type != "string") {
        printout(ERROR, "RunMetadata", 
                 "++ Invalid parameter type '%s' for parameter '%s'. Must be 'int', 'float', or 'string'",
                 p.type.c_str(), p.name.c_str());
        continue;
      }
      
      // Validate that constant exists
      try {
        if (p.type == "string") {
          detector.constantAsString(p.constantName);
        } else {
          // Try as double first
          try {
            detector.constantAsDouble(p.constantName);
          } catch (...) {
            // Try as long
            detector.constantAsLong(p.constantName);
          }
        }
      } catch (std::exception& ex) {
        printout(WARNING, "RunMetadata", 
                 "++ Constant '%s' not found for parameter '%s': %s",
                 p.constantName.c_str(), p.name.c_str(), ex.what());
      }
      
      store.parameters.push_back(p);
      ++count;
      
      printout(INFO, "RunMetadata", 
               "++ Registered metadata parameter '%s' (type=%s, constant=%s, unit=%s, branch=%s)",
               p.name.c_str(), p.type.c_str(), p.constantName.c_str(), 
               p.unit.empty() ? "none" : p.unit.c_str(), p.branch.c_str());
    }
    
    printout(INFO, "RunMetadata", "++ Registered %ld metadata parameters for branch '%s'", 
             count, branch.c_str());
    return 1;
  }
  
}  // End anonymous namespace

/// Instantiate factory
DECLARE_XML_PLUGIN(DD4hep_RunMetadata, configure_run_metadata)
