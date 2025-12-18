# Testing the Geant4MetadataAction Plugin

## Overview

This directory contains examples for testing the `Geant4MetadataAction` plugin, which allows metadata to be inserted from XML into Geant4 output files.

## Files

- **MetadataExample.xml**: Example XML configuration showing how to use the plugin
- **TestMetadataPlugin.py**: Python script demonstrating programmatic usage
- **Geant4MetadataAction.md**: Full documentation (in `DDG4/plugins/`)

## Prerequisites

To test this plugin, you need:
1. DD4hep installed with DDG4 support
2. A detector geometry (compact file)
3. Either EDM4hep or LCIO output support

## Testing with XML

### Using ddsim (command line)

```bash
ddsim \
  --compactFile /path/to/detector.xml \
  --steeringFile MetadataExample.xml \
  --outputFile test_metadata.root \
  --numberOfEvents 10
```

### Expected Behavior

When the simulation runs with the MetadataExample.xml configuration:

1. Constants are loaded from the detector description:
   - `ElectronBeamEnergy = 250.0 * GeV`
   - `PositronBeamEnergy = 250.0 * GeV`
   - `ColliderEnergy = 500.0 * GeV`
   - `BunchCrossing = 554 * ns`
   - `DetectorVersion = 'ILD_l5_v02'`

2. The plugin processes these at the start of the run

3. Metadata parameters are added to RunParameters:
   - `BeamEnergy_electron` = 250.0 (float in GeV)
   - `BeamEnergy_positron` = 250.0 (float in GeV)
   - `ColliderEnergy` = 500.0 (float in GeV)
   - `BunchCrossing` = 554.0 (float in ns)
   - `DetectorVersion` = "ILD_l5_v02" (string)

4. These parameters are written to the output file's run metadata

## Testing with Python

```bash
python TestMetadataPlugin.py batch
```

This will:
1. Create a simple geometry with test constants
2. Configure the metadata action
3. Run a short simulation
4. The metadata will be stored in RunParameters

## Verifying the Output

### For EDM4hep output

You can inspect the output file to verify the metadata was written:

```python
import podio
from podio import root_io

reader = root_io.Reader("test_metadata.root")
run_frame = reader.get("runs", 0)

# Check for our metadata parameters
print("BeamEnergy_electron:", run_frame.getParameter("BeamEnergy_electron"))
print("ColliderEnergy:", run_frame.getParameter("ColliderEnergy"))
print("DetectorVersion:", run_frame.getParameter("DetectorVersion"))
```

### For LCIO output

```python
from pyLCIO import IOIMPL, EVENT

reader = IOIMPL.LCFactory.getInstance().createLCReader()
reader.open("test_metadata.slcio")

run = reader.readNextRunHeader()
params = run.getParameters()

print("BeamEnergy_electron:", params.getFloatVal("BeamEnergy_electron"))
print("ColliderEnergy:", params.getFloatVal("ColliderEnergy"))
print("DetectorVersion:", params.getStringVal("DetectorVersion"))
```

## Customization

To use with your own detector:

1. Define constants in your detector compact file:
```xml
<define>
  <constant name="MyConstant" value="123.456*GeV"/>
</define>
```

2. Reference them in the metadata action:
```xml
<action name="Geant4MetadataAction/MetadataAction">
  <properties runParameters="MyParameter/F=MyConstant/GeV"/>
</action>
```

3. Add the action to your RunAction phase:
```xml
<phase type="RunAction/begin">
  <action name="MetadataAction"/>
</phase>
```

## Troubleshooting

### Error: "Constant not found"

- Check that the constant is defined in your detector description
- Verify the constant name spelling matches exactly
- Ensure the detector file is loaded before the metadata action runs

### Error: "Unit not found"

- Verify the unit is defined as a constant (e.g., `GeV`, `mm`, `ns`)
- The plugin will use 1.0 if the unit is not found (with a warning)
- Standard DD4hep units should be automatically available

### No metadata in output

- Verify the action is added to the `RunAction/begin` phase
- Check that your output format (EDM4hep/LCIO/ROOT) supports run metadata
- Look at the simulation log for info/error messages from the plugin

## Integration with Existing Workflows

This plugin is designed to work alongside the existing DDSim metadata system:

- Command-line metadata: `--meta.runParameters "name/type=value"`
- XML metadata: `<action name="Geant4MetadataAction/MetadataAction">`
- Python metadata: Direct configuration of output action properties

All three methods can be used together, and the metadata will be combined in the output.

## Further Information

For more details, see:
- Plugin documentation: `DDG4/plugins/Geant4MetadataAction.md`
- DDSim Meta helper: `DDG4/python/DDSim/Helper/Meta.py`
- RunParameters class: `DDG4/include/DDG4/RunParameters.h`
