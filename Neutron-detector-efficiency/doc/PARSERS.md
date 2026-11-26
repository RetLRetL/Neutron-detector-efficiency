# Separate ROOT parsers

There are two independent data-collection paths.

## 1. Efficiency scan

`run_scan.C` remains the efficiency parser. It runs `./main` over the configured PE thickness range and writes `simulation_results.root`. `plot_efficiency.C` reads that file and creates the `Efficiency` histogram. It does not parse or store neutron energies.

Keep this setting disabled for efficiency scans:

```cpp
inline constexpr bool OUTPUT_DETECTED_NEUTRON_ENERGIES = false;
```

Run:

```bash
./compile.sh
root -l -q 'run_scan.C()'
root -l -q 'plot_efficiency.C()'
```

## 2. Detected-neutron energies

To collect energies, set in `settings.h`:

```cpp
inline constexpr bool OUTPUT_DETECTED_NEUTRON_ENERGIES = true;
```

Rebuild the executable, then edit `PE_THICKNESS_CM` and `EVENTS_PER_RUN` in `energy_parce.C` and run:

```bash
./compile.sh
root -l -q 'energy_parce.C()'
```

`energy_parce.C` runs the simulation once, parses only the `detected: <energy in eV>` lines, and writes `neutron_spectra.root`, tree `neutrons`, branch `detected_energy` (`std::vector<double>`). One vector entry is written for the chosen PE thickness and run. `run_scan.C` is not involved.

The parser also reads `Detected decays` only as a consistency check; it does not calculate or store efficiency. With energy output enabled, the number of parsed energies should equal `Detected decays`.

To fit the detected-neutron spectrum:

```bash
root -l -q 'fit_neutrons.C()'
```

The fit macro reads only `detected_energy`; it does not expect the old `entered_energy` branch.

`simulation_results.root` and `neutron_spectra.root` are separate files and neither parser overwrites the other's output.
