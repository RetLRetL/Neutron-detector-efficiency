#pragma once

#include "G4VUserDetectorConstruction.hh"
#include "settings.h"

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    explicit DetectorConstruction(const SimulationConfig& config);
    ~DetectorConstruction() override = default;

    G4VPhysicalVolume* Construct() override;

private:
    SimulationConfig fConfig;
};
