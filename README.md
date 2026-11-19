# Cf-252 / He-3 Geant4 simulation

## Files

- `main.cpp` — command-line arguments, Geant4 initialization, run and timing.
- `settings.h` — simulation and geometry settings.
- `detector.h/.cpp` — materials, geometry, PE subtraction, source geometry, electric field setup and visualization attributes.
- `he3_electric_field.h/.cpp` — cylindrical He-3 electric field.
- `primary_generator.h/.cpp` — unchanged uniform sampling of `N_IONS` source points and routing to the selected decay/fission implementation.
- `geant4_decay_generator.h/.cpp` — original Geant4 branch: create a stationary Cf-252 ion and let `RadioactiveDecay` perform the decay.
- `freya_fission_generator.h/.cpp` — FREYA 2.0.5 integration: generate one spontaneous Cf-252 fission and copy its neutron/photon data.
- `freya_fission_result.h` — plain data structure returned by FREYA (particle type, energy, direction and emission age).
- `run_action.h/.cpp` — run counters and efficiency.
- `event_action.h/.cpp` — event-level reaction counting.
- `stepping_action.h/.cpp` — neutron birth and `3He(n,p)3H` detection.

## Command line

Build the executable first with `./compile.sh` (see the Compile section below). The archive does not include the old pre-integration executable.

```bash
./main
```

uses:

- PE thickness = 20 cm
- number of Geant4 events = 1
- sampled source points per event = 100

To override the first two values:

```bash
./main <PE_thickness_cm> <number_of_events>
```

Example:

```bash
./main 20 10000
./main 15 50000
```

The first argument is in centimeters; the second is the number of Geant4 events. Each event processes `Settings::N_IONS` uniformly sampled source points. In `GEANT` mode each point receives one stationary Cf-252 ion; in `FREYA` mode each point produces one spontaneous Cf-252 fission and its primary neutrons and photons.

## Generator selection

In `settings.h`, change only `Settings::GENERATION_MODE`:

```cpp
inline constexpr GenerationMode GENERATION_MODE = GenerationMode::FREYA;
// or: GenerationMode::GEANT
```

- `GEANT` preserves the original behavior: Geant4 creates a Cf-252 ion at each sampled point and `G4RadioactiveDecay` performs its decay.
- `FREYA` replaces that decay step: one FREYA spontaneous-fission event is generated at each of the same sampled points. The resulting neutrons and photons are inserted into the same `G4Event` as Geant4 primary particles.

The source-point sampling code (cylindrical radius, azimuth and coordinate along the cylinder axis) is shared and unchanged. No ROOT output writer has been added: `FreyaFissionResult` is passed directly from the FREYA generator to `PrimaryGeneratorAction`, which creates the Geant4 primary vertices and particles.

**Important physical distinction:** `FREYA` models a spontaneous-fission event, not the competition between Cf-252 alpha decay and spontaneous fission. Therefore, in the current `FREYA` mode each of the `N_IONS` sampled source points corresponds to one fission. No additional 3.086% branching-probability draw is applied. The `GEANT` mode continues to use the decay channels configured by Geant4.

FREYA's neutron and photon emission ages are copied to the primary-vertex times. Energies are converted from MeV to Geant4 units. The current implementation interprets the returned age values as seconds; this assumption is isolated in `kFreyaAgeToSeconds` in `freya_fission_generator.cpp`, because the FREYA API manual names the age getters but does not explicitly state their units. The manual also notes that delayed fission neutrons and photons are not implemented, so this time handling should be checked against the installed 2.0.5 source before interpreting non-zero ages physically.

## FREYA data directory

At runtime the program uses `FREYADATAPATH` if it is set. Otherwise it expects the data directory at:

```text
$HOME/llnl/fission_v2.0.5/data_freya
```

The directory must contain `react.dat`. To use a different location, set either `FREYADATAPATH` directly or `FREYA_ROOT` (the latter should contain a `data_freya` subdirectory):

```bash
export FREYADATAPATH=/path/to/data_freya
# alternatively:
export FREYA_ROOT=/path/to/fission_v2.0.5
```

The FREYA library is initialized with correlation option `3`, which selects event-by-event FREYA mode and correlated neutron/photon multiplicities and energies. FREYA's random-number callback is connected to `G4UniformRand()`, so Geant4 run seeds control both samplers.

## Compile

```bash
./compile.sh
```

The script also links the FREYA library. Its default installation root is `$HOME/llnl/fission_v2.0.5`; override it if necessary:

```bash
FREYA_ROOT=/path/to/fission_v2.0.5 ./compile.sh
```

## Important settings

In `settings.h`:

```cpp
inline constexpr G4int N_IONS = 100;
inline constexpr Settings::GenerationMode GENERATION_MODE = Settings::GenerationMode::FREYA;
inline constexpr bool ENABLE_VISUALIZATION = true;
inline constexpr bool ENABLE_TIMING = false;
```

Set `ENABLE_VISUALIZATION = false` for batch runs without a GUI, for example when a ROOT macro will launch many simulations.

Set `ENABLE_TIMING = false` to disable the timing output.

The measured interval surrounds only `runManager->BeamOn(config.nEvents)`, so Geant4 construction and initialization are outside the measured analysis time.
