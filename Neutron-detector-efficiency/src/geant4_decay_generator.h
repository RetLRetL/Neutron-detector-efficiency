#pragma once

#include "G4ThreeVector.hh"

class G4Event;

// The existing Geant4 radioactive-decay primary creation, extracted from
// PrimaryGeneratorAction so that it can be selected independently of FREYA.
namespace Geant4DecayGenerator
{
    void GenerateCf252(
        const G4ThreeVector& sourcePosition,
        G4int sourceID,
        G4Event* event
    );
}
