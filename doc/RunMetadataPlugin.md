# Run Metadata Plugin for DD4hep

## Overview

The Run Metadata plugin system allows metadata to be defined in compact XML detector description files and automatically inserted into Geant4 simulation output files (EDM4hep, LCIO, ROOT).

## Components

The system consists of two parts:

1. **DD4hep_RunMetadata** - A DD4hep plugin that reads metadata configuration from compact XML files during detector construction
2. **Geant4RunMetadata** - A Geant4 action that applies the configured metadata to output files during simulation

Both components are implemented in a single file: `DDG4/plugins/Geant4RunMetadataPlugin.cpp`

## Usage

### 1. Define Metadata in Compact XML

In your detector description XML file, configure metadata using the `DD4hep_RunMetadata` plugin:

```xml
<lccdd>
  <!-- Define constants (optional) -->
  <define>
    <constant name="ElectronBeamEnergy" value="250.0*GeV"/>
    <constant name="ColliderEnergy" value="500.0*GeV"/>
  </define>

  <!-- Configure metadata -->
  <plugins>
    <plugin name="DD4hep_RunMetadata" type="runs">
      <!-- Values are evaluated as DD4hep expressions -->
      <parameter name="BeamEnergy_electron" type="float" 
                 value="ElectronBeamEnergy/GeV"/>
      <parameter name="ColliderEnergy" type="float" 
                 value="ColliderEnergy/GeV"/>
      <parameter name="DetectorVersion" type="string" 
                 value="ILD_l5_v02"/>
    </plugin>
  </plugins>
</lccdd>
```

### 2. Enable Metadata Action in Geant4

In your Geant4 simulation XML configuration, add the `Geant4RunMetadata` action:

```xml
<geant4_setup>
  <actions>
    <action name="Geant4RunMetadata/RunMetadata"/>
  </actions>
  
  <phases>
    <phase type="RunAction/begin">
      <action name="RunMetadata"/>
    </phase>
  </phases>
</geant4_setup>
```

### 3. Python Configuration (Alternative)

You can also configure this in Python:

```python
import DDG4

# Load detector with metadata configuration
kernel = DDG4.Kernel()
kernel.loadGeometry("detector.xml")

# Setup Geant4
geant4 = DDG4.Geant4(kernel)

# Add metadata action
metadata = DDG4.Action(kernel, 'Geant4RunMetadata/RunMetadata')
geant4.runAction().adopt(metadata)
```

## Parameter Format

Each `<parameter>` tag has the following attributes:

- **name** (required): Name of the parameter in the output metadata
- **type** (required): Data type - `"int"`, `"float"`, or `"string"`
- **value** (required): Value as a DD4hep expression that will be evaluated

The `value` attribute is evaluated using DD4hep's expression evaluator, which supports:
- Constants defined in the detector description
- Mathematical expressions (e.g., `"250.0*GeV"`, `"10+5"`)
- Unit divisions (e.g., `"ElectronBeamEnergy/GeV"`)
- For string types, the value is used directly without evaluation

## Metadata Branches

The plugin tag's `type` attribute specifies where the metadata is stored:

- **`"runs"`** (default) - Run-level metadata (stored once per run)
- **`"metadata"`** - File-level metadata (stored once per file)

You can use multiple plugin blocks to configure different branches:

```xml
<plugins>
  <!-- Run-level metadata -->
  <plugin name="DD4hep_RunMetadata" type="runs">
    <parameter name="BeamEnergy" type="float" value="250.0*GeV"/>
  </plugin>
  
  <!-- File-level metadata -->
  <plugin name="DD4hep_RunMetadata" type="metadata">
    <parameter name="GeometryVersion" type="string" value="ILD_l5_v02"/>
  </plugin>
</plugins>
```

## Supported Data Types

### Integer Parameters

```xml
<parameter name="NumberOfLayers" type="int" value="12"/>
<!-- or with expression -->
<parameter name="LayerCount" type="int" value="NumberOfLayers*2"/>
```

The value is evaluated as a mathematical expression and converted to an integer.

### Float Parameters

```xml
<parameter name="BeamEnergy" type="float" value="250.0*GeV"/>
<!-- or with unit division -->
<parameter name="Energy_GeV" type="float" value="ElectronBeamEnergy/GeV"/>
```

The value is evaluated as a mathematical expression and stored as a float.

### String Parameters

```xml
<parameter name="DetectorVersion" type="string" value="ILD_l5_v02"/>
```

For string parameters, the value is used directly without expression evaluation.

## Expression Evaluation

Values are evaluated using DD4hep's built-in expression evaluator, which supports:

- **Constants**: References to constants defined in `<define>` sections
- **Mathematical operations**: `+`, `-`, `*`, `/`, `(`, `)`
- **Units**: Standard DD4hep units (GeV, mm, ns, tesla, degree, etc.)
- **Unit conversions**: Use division for unit conversion (e.g., `"250.0*GeV/GeV"` = `250.0`)

Examples:
```xml
<parameter name="Energy" type="float" value="250.0*GeV"/>
<parameter name="Distance" type="float" value="(10.0*mm + 5.0*cm)/mm"/>
<parameter name="Angle" type="float" value="90.0*degree"/>
```

## Output Format Integration

The metadata is automatically written to output files when using:

### EDM4hep Output

```python
output = geant4.setupEDM4hepOutput('EDM4hepOutput', 'output.root')
```

Metadata appears in the run frame and can be accessed:

```python
import podio
reader = podio.root_io.Reader("output.root")
run_frame = reader.get("runs", 0)
print("BeamEnergy:", run_frame.getParameter("BeamEnergy"))
```

### LCIO Output

```python
output = geant4.setupLCIOOutput('LCIOOutput', 'output.slcio')
```

Metadata appears in the run header:

```python
from pyLCIO import IOIMPL
reader = IOIMPL.LCFactory.getInstance().createLCReader()
reader.open("output.slcio")
run = reader.readNextRunHeader()
print("BeamEnergy:", run.getParameters().getFloatVal("BeamEnergy"))
```

### ROOT Output

```python
output = geant4.setupROOTOutput('RootOutput', 'output.root')
```

## Examples

### Complete Example

See the following files for complete examples:

- **Compact XML**: `examples/ClientTests/compact/RunMetadataExample.xml`
- **Geant4 Setup**: `examples/DDG4/compact/RunMetadataExample.xml`
- **Plugin Implementation**: `DDG4/plugins/Geant4RunMetadataPlugin.cpp`

### Basic Setup

```xml
<!-- detector.xml -->
<lccdd>
  <define>
    <constant name="BeamEnergy" value="10.0*GeV"/>
  </define>
  
  <plugins>
    <plugin name="DD4hep_RunMetadata" type="runs">
      <!-- Value is evaluated: BeamEnergy/GeV = 10.0*GeV/GeV = 10.0 -->
      <parameter name="Energy" type="float" value="BeamEnergy/GeV"/>
    </plugin>
  </plugins>
</lccdd>
```

```xml
<!-- simulation.xml -->
<geant4_setup>
  <actions>
    <action name="Geant4RunMetadata/RunMetadata"/>
  </actions>
  
  <phases>
    <phase type="RunAction/begin">
      <action name="RunMetadata"/>
    </phase>
  </phases>
</geant4_setup>
```

## Error Handling

The plugin performs validation and provides informative messages:

- **Invalid branch type**: Error if branch is not "runs", "metadata", or "events"
- **Invalid parameter type**: Error if type is not "int", "float", or "string"
- **Missing constant**: Warning if the specified constant is not defined
- **Missing unit**: Warning if unit is not found (defaults to 1.0)

All validation occurs during detector construction, before simulation starts.

## Implementation Details

### Storage Mechanism

The metadata configuration is stored in a singleton `RunMetadataStore` associated with each `Detector` instance. This allows the configuration to persist from detector construction (DD4hep) to simulation runtime (Geant4).

### Thread Safety

The system is designed to work with both single-threaded and multi-threaded Geant4 simulations. The metadata store is read-only during simulation, so no locking is required.

## Migration from Command-Line Metadata

If you're currently using DDSim command-line metadata:

**Before:**
```bash
ddsim --meta.runParameters "BeamEnergy/F=250.0" ...
```

**After:**
Add to your detector XML:
```xml
<plugins>
  <plugin name="DD4hep_RunMetadata" type="runs">
    <parameter name="BeamEnergy" type="float" value="250.0*GeV"/>
  </plugin>
</plugins>
```

Or reference a constant:
```xml
<define>
  <constant name="BeamEnergyConstant" value="250.0*GeV"/>
</define>

<plugins>
  <plugin name="DD4hep_RunMetadata" type="runs">
    <parameter name="BeamEnergy" type="float" 
               value="BeamEnergyConstant/GeV"/>
  </plugin>
</plugins>
```

Both methods can be used simultaneously - command-line metadata and XML metadata will be combined in the output.

## See Also

- DD4hep Detector Description Manual
- Geant4 Actions Documentation
- EDM4hep/LCIO Output Formats
