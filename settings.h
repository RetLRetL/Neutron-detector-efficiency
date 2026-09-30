#pragma once

#include "G4SystemOfUnits.hh"
#include "globals.hh"

namespace Settings
{
    // ================================================================
    // Run configuration
    // ================================================================

    // Default polyethylene thickness along +Y.
    inline const G4double DEFAULT_PE_THICKNESS = 20.0 * cm;

    // Number of Geant4 events if not supplied on the command line.
    inline constexpr G4int DEFAULT_N_EVENTS = 1;

    // Number of Cf-252 nuclei generated in every Geant4 event.
    inline constexpr G4int N_IONS = 100;

    // Enable / disable Geant4 visualization.
    inline constexpr bool ENABLE_VISUALIZATION = true;

    // Enable / disable measurement of BeamOn execution time.
    inline constexpr bool ENABLE_TIMING = false;

    // ================================================================
    // World
    // ================================================================

    inline constexpr G4double WORLD_HALF_X = 50.0 * cm;
    inline constexpr G4double WORLD_HALF_Y = 50.0 * cm;
    inline constexpr G4double WORLD_HALF_Z = 50.0 * cm;

    // ================================================================
    // Detector envelope
    // ================================================================

    inline const G4double ENVELOPE_LENGTH_X = 417.0 * mm;
    inline const G4double ENVELOPE_SIZE_Y   = 50.0 * mm;
    inline const G4double ENVELOPE_SIZE_Z   = 50.0 * mm;

    // ================================================================
    // Steel body
    // ================================================================

    inline const G4double STEEL_OUTER_RADIUS = 25.0 * mm;
    inline const G4double STEEL_INNER_RADIUS = 24.0 * mm;
    inline const G4double STEEL_WALL_THICKNESS = 1.0 * mm;
    inline const G4double STEEL_LENGTH = 380.0 * mm;

    // ================================================================
    // He-3 gas and wire
    // ================================================================

    inline const G4double GAS_RADIUS = 24.0 * mm;
    inline const G4double GAS_CURRENT_LENGTH = 378.0 * mm;

    // The gas half-length is reduced by one wall thickness.
    // Therefore its full length becomes 378 - 2*1 = 376 mm.
    inline const G4double GAS_HALF_LENGTH =
        GAS_CURRENT_LENGTH / 2.0 - STEEL_WALL_THICKNESS;

    // The wire has exactly the same final length as the gas.
    inline const G4double WIRE_RADIUS = 0.5 * mm;
    inline const G4double WIRE_HALF_LENGTH = GAS_HALF_LENGTH;

    // ================================================================
    // Cf-252 sample
    // ================================================================

    inline const G4double SAMPLE_RADIUS = 20.0 * mm;
    inline const G4double SAMPLE_HEIGHT = 10.0 * mm;

    // The sample center is placed at y = PE thickness.
    // With the PE occupying y = 0 ... PE thickness, half of the
    // 10 mm-high sample is embedded into the polyethylene.
    inline const G4double SAMPLE_X = 0.0 * mm;
    inline const G4double SAMPLE_Z = 0.0 * mm;

    // ================================================================
    // Polyethylene geometry
    // ================================================================

    // PE is shifted by +10 mm along the detector axis (+X).
    // Its half-width along X is increased by 10 mm.
    inline const G4double PE_SHIFT_X = 10.0 * mm;
    inline const G4double PE_HALF_X =
        ENVELOPE_LENGTH_X / 2.0 + 10.0 * mm;
    inline const G4double PE_HALF_Z = ENVELOPE_SIZE_Z / 2.0;

    // PE extends from y = 0 to y = PE thickness.

    // ================================================================
    // Electric field
    // ================================================================

    inline const G4double FIELD_INNER_RADIUS = WIRE_RADIUS;
    inline const G4double FIELD_OUTER_RADIUS = GAS_RADIUS;
    inline const G4double FIELD_VOLTAGE = 1450.0 * volt;
    inline const G4double FIELD_MIN_STEP = 0.01 * mm;

    // ================================================================
    // He-3 gas state
    // ================================================================

    inline const G4double GAS_TEMPERATURE = 293.15 * kelvin;
    inline const G4double GAS_PRESSURE = 2.5 * atmosphere;
}

struct SimulationConfig
{
    G4double peThickness = Settings::DEFAULT_PE_THICKNESS;
    G4int nEvents = Settings::DEFAULT_N_EVENTS;
};
