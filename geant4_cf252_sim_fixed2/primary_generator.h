#pragma once

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"
#include "settings.h"

class PrimaryGeneratorAction
    : public G4VUserPrimaryGeneratorAction
{
public:
    explicit PrimaryGeneratorAction(
        const SimulationConfig& config
    );

    ~PrimaryGeneratorAction() override = default;

    void GeneratePrimaries(
        G4Event* event
    ) override;

private:
    SimulationConfig fConfig;
};