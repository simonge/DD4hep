#!/usr/bin/env python
"""
Example Python script demonstrating the Geant4MetadataAction plugin

This script shows how to programmatically configure the metadata action
to insert constants from the detector description into the run parameters.

The plugin allows metadata to be inserted from XML (or Python) into the Geant4
metadata collections. It follows the same concept as the command line approach
(e.g. --meta.runParameters "BeamEnergy_electron/F=ElectronBeamEnergy/GeV").

Author: GitHub Copilot
"""
from __future__ import absolute_import, unicode_literals
import logging
import sys

# Configure logging
logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.INFO)
logger = logging.getLogger(__name__)

try:
  import DDG4
  from DDG4 import OutputLevel as Output
  from g4units import GeV, mm, ns
except ImportError as e:
  logger.error("Failed to import DDG4 modules: %s", str(e))
  logger.error("This script requires DD4hep with DDG4 to be installed and available")
  sys.exit(1)

def run():
  """Configure and run a simple simulation with metadata action"""
  
  import DD4hep
  
  # Load geometry (use a simple example geometry)
  # In a real scenario, you would load your actual detector geometry
  geometry_file = "file:../../ClientTests/compact/SiliconBlock.xml"
  
  logger.info("Loading geometry from: %s", geometry_file)
  
  kernel = DDG4.Kernel()
  description = kernel.detectorDescription()
  
  # Try to load the geometry
  try:
    kernel.loadGeometry(str(geometry_file))
  except Exception as e:
    logger.warning("Could not load geometry: %s", str(e))
    logger.info("Continuing with empty geometry for demonstration")
  
  # Add some constants to the detector description that we'll use for metadata
  # In a real scenario, these would be defined in your XML detector description
  description.addConstant(DD4hep.Constant("ElectronBeamEnergy", "250.0*GeV", "double"))
  description.addConstant(DD4hep.Constant("PositronBeamEnergy", "250.0*GeV", "double"))
  description.addConstant(DD4hep.Constant("ColliderEnergy", "500.0*GeV", "double"))
  description.addConstant(DD4hep.Constant("BunchCrossing", "554*ns", "double"))
  description.addConstant(DD4hep.Constant("DetectorVersion", "ILD_l5_v02", "string"))
  
  # Import Geant4
  DDG4.importConstants(kernel.detectorDescription())
  
  # Setup Geant4
  geant4 = DDG4.Geant4(kernel)
  geant4.printDetectors()
  
  # Configure UI and run parameters
  if len(sys.argv) >= 2 and sys.argv[1] == "batch":
    kernel.UI = ""
    kernel.NumEvents = 5
  else:
    # Interactive mode
    kernel.UI = "UI"
  
  # Setup tracking field
  geant4.setupTrackingField(prt=True)
  
  # Setup random generator
  rndm = DDG4.Action(kernel, 'Geant4Random/Random')
  rndm.Seed = 987654321
  rndm.initialize()
  
  # Create the metadata action
  logger.info("Setting up Geant4MetadataAction")
  metadata_action = DDG4.Action(kernel, 'Geant4MetadataAction/MetadataAction')
  
  # Configure the metadata parameters
  # Format: ParameterName/Type=ConstantName[/Unit]
  # Type: I=int, F=float, C=string
  # Multiple parameters separated by commas
  metadata_action.runParameters = [
    "BeamEnergy_electron/F=ElectronBeamEnergy/GeV",
    "BeamEnergy_positron/F=PositronBeamEnergy/GeV",
    "ColliderEnergy/F=ColliderEnergy/GeV",
    "BunchCrossing/F=BunchCrossing/ns",
    "DetectorVersion/C=DetectorVersion"
  ]
  metadata_action.OutputLevel = Output.INFO
  
  # Add the metadata action to the run action sequence
  geant4.runAction().adopt(metadata_action)
  
  # Setup particle gun
  logger.info("Setting up particle gun")
  geant4.setupGun("Gun", particle='e-', energy=10*GeV, multiplicity=1)
  
  # Setup tracker - if geometry loaded
  # geant4.setupTracker('SiliconBlockUpper')
  
  # Setup EDM4hep output (optional, requires EDM4hep)
  # output = geant4.setupEDM4hepOutput('EDM4hepOutput', 'metadata_test.root')
  # output.RunParametersString = {}
  # output.RunParametersInt = {}
  # output.RunParametersFloat = {}
  
  # Or use ROOT output
  # output = geant4.setupROOTOutput('RootOutput', 'metadata_test.root')
  
  # Build physics list
  phys = geant4.setupPhysics('QGSP_BERT')
  
  # Configure, initialize and run
  logger.info("Configuring Geant4")
  geant4.execute()
  
  logger.info("Simulation complete")
  logger.info("Metadata was added to RunParameters and will be available in output files")
  
  return 0


if __name__ == "__main__":
  try:
    sys.exit(run())
  except Exception as e:
    logger.exception("Failed to run simulation: %s", str(e))
    sys.exit(1)
