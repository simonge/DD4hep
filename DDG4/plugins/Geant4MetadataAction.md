# Geant4MetadataAction Plugin

## Overview

The `Geant4MetadataAction` plugin allows metadata to be inserted from XML configuration (or Python) into the Geant4 metadata collections (e.g., "runs"/"metadata"). It follows the same concept as the command-line approach used in DDSim (e.g., `--meta.runParameters "BeamEnergy_electron/F=ElectronBeamEnergy/GeV"`).

## Purpose

This plugin enables users to:
- Define metadata parameters in XML configuration files
- Reference constants defined in the detector description
- Automatically resolve and convert constant values with appropriate units
- Store the metadata in run parameters for output to EDM4hep, LCIO, or ROOT files

## Usage

### XML Configuration

Add the action to your Geant4 XML setup file:

```xml
<geant4_setup>
  <!-- Define constants in your detector description -->
  <define>
    <constant name="ElectronBeamEnergy" value="250.0*GeV"/>
    <constant name="PositronBeamEnergy" value="250.0*GeV"/>
    <constant name="ColliderEnergy" value="500.0*GeV"/>
    <constant name="DetectorVersion" value="'ILD_l5_v02'"/>
  </define>
  
  <!-- Register the metadata action -->
  <actions>
    <action name="Geant4MetadataAction/MetadataAction">
      <properties runParameters="BeamEnergy_electron/F=ElectronBeamEnergy/GeV,BeamEnergy_positron/F=PositronBeamEnergy/GeV,ColliderEnergy/F=ColliderEnergy/GeV,DetectorVersion/C=DetectorVersion"/>
    </action>
  </actions>
  
  <!-- Add to the run phase -->
  <phases>
    <phase type="RunAction/begin">
      <action name="MetadataAction"/>
    </phase>
  </phases>
</geant4_setup>
```

### Python Configuration

```python
import DDG4

# Create kernel and load geometry
kernel = DDG4.Kernel()
kernel.loadGeometry("detector.xml")

# Setup Geant4
geant4 = DDG4.Geant4(kernel)

# Create metadata action
metadata = DDG4.Action(kernel, 'Geant4MetadataAction/MetadataAction')

# Configure parameters
metadata.runParameters = [
    "BeamEnergy_electron/F=ElectronBeamEnergy/GeV",
    "BeamEnergy_positron/F=PositronBeamEnergy/GeV",
    "ColliderEnergy/F=ColliderEnergy/GeV",
    "DetectorVersion/C=DetectorVersion"
]

# Add to run action sequence
geant4.runAction().adopt(metadata)

# Continue with simulation setup...
```

## Parameter Format

Each parameter specification follows the format:

```
ParameterName/Type=ConstantName[/Unit]
```

Where:
- **ParameterName**: The name to use when storing the parameter in the output metadata
- **Type**: The data type for the parameter
  - `I` - Integer
  - `F` - Float
  - `C` - String
- **ConstantName**: The name of the constant defined in the detector description
- **Unit** (optional): The unit to divide the constant value by (e.g., `GeV`, `mm`, `ns`)

Multiple parameters can be specified by separating them with commas in XML, or as a list in Python.

## Examples

### Integer Parameter
```
NumberOfLayers/I=NumberOfLayers
```
Stores an integer parameter named "NumberOfLayers" from the constant with the same name.

### Float Parameter with Unit
```
BeamEnergy/F=ElectronBeamEnergy/GeV
```
Stores a float parameter named "BeamEnergy" from the constant "ElectronBeamEnergy", divided by the GeV unit.

### String Parameter
```
DetectorVersion/C=DetectorVersion
```
Stores a string parameter named "DetectorVersion" from the constant with the same name.

## Integration with Output Formats

The metadata parameters are added to `RunParameters` and will be automatically included in the output when using:

- **EDM4hep output**: Parameters are written to the run frame metadata
- **LCIO output**: Parameters are written to the run header
- **ROOT output**: Parameters may be accessible through the run information

## Error Handling

The plugin will:
- Log an error and skip parameters with invalid format
- Warn if a specified constant is not found in the detector description
- Warn if a specified unit is not found (defaults to 1.0)
- Continue processing other parameters even if one fails

## Implementation Details

- The plugin is implemented as a `Geant4RunAction`
- It is called at the beginning of each run
- It accesses the `Detector` description to read constant values
- It stores values in the `RunParameters` extension of the run context
- Multiple parameters can be configured via the `runParameters` property

## See Also

- DDSim Meta helper: `DDG4/python/DDSim/Helper/Meta.py`
- Command-line metadata: Use `--meta.runParameters` with DDSim
- Example XML: `examples/DDG4/compact/MetadataExample.xml`
- Example Python script: `examples/DDG4/scripts/TestMetadataPlugin.py`
