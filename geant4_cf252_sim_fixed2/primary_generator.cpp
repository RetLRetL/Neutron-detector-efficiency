#include "primary_generator.h"

#include "G4Event.hh"
#include "G4IonTable.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"

#include "Randomize.hh"

#include <cmath>

PrimaryGeneratorAction::PrimaryGeneratorAction(
    const SimulationConfig& config
)
    : fConfig(config)
{
}

void PrimaryGeneratorAction::GeneratePrimaries(
    G4Event* event
)
{
    // ================================================================
    // Cf-252 nucleus
    // ================================================================

    G4ParticleDefinition* cf252 =
        G4IonTable::GetIonTable()->GetIon(
            98,     // Z
            252,    // A
            0.0     // excitation energy
        );

    // ================================================================
    // Geometrical center of the Cf-252 sample
    //
    // X = 0
    // Z = 0
    //
    // Y = PE thickness + half-size of the envelope along Y
    //
    // For the current geometry:
    //     Y = T_PE + 25 mm
    // ================================================================

    const G4double sampleCenterY =
        fConfig.peThickness +
        Settings::ENVELOPE_SIZE_Y / 2.0;

    const G4ThreeVector sampleCenter(
        0.0,
        sampleCenterY,
        0.0
    );

    // ================================================================
    // Actual sample geometry
    //
    // Radius = 20 mm
    // Height = 10 mm
    //
    // The cylinder axis is along Y.
    // ================================================================

    const G4double sampleRadius =
        Settings::SAMPLE_RADIUS;

    const G4double sampleHalfHeight =
        Settings::SAMPLE_HEIGHT / 2.0;

    // ================================================================
    // Generate N_IONS Cf-252 nuclei uniformly inside the sample
    // ================================================================

    for (G4int i = 0;
         i < Settings::N_IONS;
         ++i)
    {
        // ------------------------------------------------------------
        // Uniform distribution in the circular cross-section.
        //
        // sqrt(U) is required for uniform area density.
        // ------------------------------------------------------------

        const G4double r =
            sampleRadius *
            std::sqrt(
                G4UniformRand()
            );

        const G4double phi =
            2.0 * CLHEP::pi *
            G4UniformRand();

        // ------------------------------------------------------------
        // Uniform distribution along the cylinder axis Y.
        // ------------------------------------------------------------

        const G4double yOffset =
            (
                2.0 * G4UniformRand() - 1.0
            ) * sampleHalfHeight;

        // ------------------------------------------------------------
        // Convert cylindrical coordinates to Cartesian coordinates.
        //
        // Cylinder axis is Y, therefore the circular cross-section
        // lies in the XZ plane.
        // ------------------------------------------------------------

        const G4double x =
            sampleCenter.x() +
            r * std::cos(phi);

        const G4double y =
            sampleCenter.y() +
            yOffset;

        const G4double z =
            sampleCenter.z() +
            r * std::sin(phi);

        const G4ThreeVector position(
            x,
            y,
            z
        );

        // ============================================================
        // Primary vertex
        // ============================================================

        G4PrimaryVertex* vertex =
            new G4PrimaryVertex(
                position,
                0.0 * ns
            );

        // ============================================================
        // Cf-252 nucleus
        // ============================================================

        G4PrimaryParticle* primary =
            new G4PrimaryParticle(
                cf252
            );

        primary->SetKineticEnergy(
            0.0
        );

        vertex->SetPrimary(
            primary
        );

        event->AddPrimaryVertex(
            vertex
        );
    }
}