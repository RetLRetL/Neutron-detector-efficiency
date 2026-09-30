# Cf-252 / He-3 Geant4 simulation

## Files

- `main.cpp` — command-line arguments, Geant4 initialization, run and timing.
- `settings.h` — simulation and geometry settings.
- `detector.h/.cpp` — materials, geometry, PE subtraction, source geometry, electric field setup and visualization attributes.
- `he3_electric_field.h/.cpp` — cylindrical He-3 electric field.
- `primary_generator.h/.cpp` — generation of `N_IONS` Cf-252 nuclei uniformly inside the sample.
- `run_action.h/.cpp` — run counters and efficiency.
- `event_action.h/.cpp` — event-level reaction counting.
- `stepping_action.h/.cpp` — neutron birth and `3He(n,p)3H` detection.

## Command line

```bash
./main
```

uses:

- PE thickness = 20 cm
- number of Geant4 events = 1
- Cf-252 nuclei per event = 100

To override the first two values:

```bash
./main <PE_thickness_cm> <number_of_events>
```

Example:

```bash
./main 20 10000
./main 15 50000
```

The first argument is in centimeters; the second is the number of Geant4 events. Each event generates `Settings::N_IONS` Cf-252 nuclei.

## Compile

```bash
./compile.sh
```

or equivalently:

```bash
g++ -std=c++17 \
    main.cpp detector.cpp he3_electric_field.cpp primary_generator.cpp \
    run_action.cpp event_action.cpp stepping_action.cpp \
    $(geant4-config --cflags) \
    $(root-config --cflags) \
    $(geant4-config --libs) \
    $(root-config --libs) \
    -o main
```

## Important settings

In `settings.h`:

```cpp
inline constexpr G4int N_IONS = 100;
inline constexpr bool ENABLE_VISUALIZATION = true;
inline constexpr bool ENABLE_TIMING = true;
```

Set `ENABLE_VISUALIZATION = false` for batch runs without a GUI, for example when a ROOT macro will launch many simulations.

Set `ENABLE_TIMING = false` to disable the timing output.

The measured interval surrounds only `runManager->BeamOn(config.nEvents)`, so Geant4 construction and initialization are outside the measured analysis time.
