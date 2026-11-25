#include "primary_generator.h"

#include "freya_fission_generator.h"
#include "geant4_decay_generator.h"

#include "G4Event.hh"
#include "G4Gamma.hh"
#include "G4Neutron.hh"
#include "G4ParticleDefinition.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"

#include "Randomize.hh"

#include <cmath>

namespace
{
    void AddFreyaProductsToEvent(
        const FreyaFissionResult& fission,
        const G4ThreeVector& sourcePosition,
        G4Event* event
    )
    {
        // FREYA returns one record per emitted particle. Each primary vertex
        // is placed at the already sampled Cf-252 source position. A separate
        // vertex is used for each emission time, preserving the time ordering
        // supplied by FREYA within the fission event.
        for (const FreyaParticleData& particleData : fission.particles)
        {
            G4ParticleDefinition* definition = nullptr;

            if (particleData.type == FreyaParticleType::Neutron)
            {
                definition = G4Neutron::NeutronDefinition();
            }
            else
            {
                definition = G4Gamma::GammaDefinition();
            }

            const G4ThreeVector direction(
                particleData.direction[0],
                particleData.direction[1],
                particleData.direction[2]
            );

            // FREYA energies are in MeV. The age-to-time conversion currently
            // assumes that FREYA returns ages in seconds (see the note in
            // freya_fission_generator.cpp).
            const G4double kineticEnergy =
                particleData.kineticEnergyMeV * MeV;

            const G4double emissionTime =
                particleData.emissionTimeSeconds * second;

            auto* vertex = new G4PrimaryVertex(
                sourcePosition,
                emissionTime
            );

            auto* primary = new G4PrimaryParticle(definition);
            primary->SetKineticEnergy(kineticEnergy);
            primary->SetMomentumDirection(direction);
            vertex->SetPrimary(primary);
            event->AddPrimaryVertex(vertex);
        }
    }
}

PrimaryGeneratorAction::PrimaryGeneratorAction(
    const SimulationConfig& config
)
    : fConfig(config)
{
    if (Settings::GENERATION_MODE == Settings::GenerationMode::FREYA)
    {
        // Initialize once before the first event. This also validates the
        // FREYA data directory and configures FREYA's correlation mode/RNG.
        FreyaFissionGenerator::Initialize();
    }
}

void PrimaryGeneratorAction::GeneratePrimaries(
    G4Event* event
)
{
    // ================================================================
    // Geometrical center of the Cf-252 sample
    //
    // Keep this placement exactly as in the original project:
    //     X = 0, Z = 0
    //     Y = PE thickness + half-size of the detector envelope along Y
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
    // Actual sample geometry -- unchanged from the original project
    // ================================================================

    const G4double sampleRadius = Settings::SAMPLE_RADIUS;
    const G4double sampleHalfHeight = Settings::SAMPLE_HEIGHT / 2.0;

    // ================================================================
    // Select N_IONS independent source points inside the sample.
    // The random-position algorithm below is intentionally unchanged.
    // Only the decay/fission mechanism selected after each position differs.
    // ================================================================

    for (G4int i = 0; i < Settings::N_IONS; ++i)
    {
        // Uniform distribution in the circular cross-section.
        // sqrt(U) is required for uniform area density.
        const G4double r =
            sampleRadius * std::sqrt(G4UniformRand());

        const G4double phi =
            2.0 * CLHEP::pi * G4UniformRand();

        // Uniform distribution along the cylinder axis Y.
        const G4double yOffset =
            (2.0 * G4UniformRand() - 1.0) * sampleHalfHeight;

        // Cylinder axis is Y, so its circular cross-section lies in XZ.
        const G4double x =
            sampleCenter.x() + r * std::cos(phi);
        const G4double y =
            sampleCenter.y() + yOffset;
        const G4double z =
            sampleCenter.z() + r * std::sin(phi);

        const G4ThreeVector sourcePosition(x, y, z);

        // ============================================================
        // Only this call is switched between the two implementations.
        // GEANT: create Cf-252 and let RadioactiveDecay perform the decay.
        // FREYA: directly generate the products of one Cf-252 fission.
        // ============================================================

        if (Settings::GENERATION_MODE == Settings::GenerationMode::GEANT)
        {
            Geant4DecayGenerator::GenerateCf252(
                sourcePosition,
                event
            );
        }
        else
        {
            const FreyaFissionResult fission =
                FreyaFissionGenerator::GenerateCf252();

            AddFreyaProductsToEvent(
                fission,
                sourcePosition,
                event
            );
        }
    }
}
