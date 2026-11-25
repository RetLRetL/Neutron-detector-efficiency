#include "geant4_decay_generator.h"

#include "G4Event.hh"
#include "G4Exception.hh"
#include "G4IonTable.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"

namespace Geant4DecayGenerator
{
    void GenerateCf252(
        const G4ThreeVector& sourcePosition,
        G4Event* event
    )
    {
        // This is the original Geant4 branch from PrimaryGeneratorAction:
        // create a stationary Cf-252 ion and let RadioactiveDecayPhysics
        // perform the decay later during tracking.
        G4ParticleDefinition* cf252 =
            G4IonTable::GetIonTable()->GetIon(
                98,     // Atomic number Z
                252,    // Mass number A
                0.0     // Excitation energy
            );

        if (!cf252)
        {
            G4Exception(
                "Geant4DecayGenerator::GenerateCf252",
                "GEANTDECAY001",
                FatalException,
                "Could not obtain the Cf-252 ion definition."
            );
            return;
        }

        auto* vertex = new G4PrimaryVertex(
            sourcePosition,
            0.0 * ns
        );

        auto* primary = new G4PrimaryParticle(cf252);
        primary->SetKineticEnergy(0.0);
        vertex->SetPrimary(primary);
        event->AddPrimaryVertex(vertex);
    }
}
