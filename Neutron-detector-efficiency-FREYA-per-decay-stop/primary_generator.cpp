#include "primary_generator.h"

#include "freya_fission_generator.h"
#include "geant4_decay_generator.h"
#include "source_track_information.h"

#include "G4Event.hh"
#include "G4Exception.hh"
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
        G4int sourceID,
        G4Event* event
    )
    {
        // Each FREYA product is tagged with the source ID of the fission
        // that emitted it. All neutrons, photons and their later descendants
        // therefore remain associated with this one source.
        for (const FreyaParticleData& particleData : fission.particles)
        {
            G4ParticleDefinition* definition = nullptr;

            if (particleData.type == FreyaParticleType::Neutron)
                definition = G4Neutron::NeutronDefinition();
            else
                definition = G4Gamma::GammaDefinition();

            const G4ThreeVector direction(
                particleData.direction[0],
                particleData.direction[1],
                particleData.direction[2]
            );

            const G4double kineticEnergy =
                particleData.kineticEnergyMeV * MeV;

            const G4double emissionTime =
                particleData.emissionTimeSeconds * second;

            auto* vertex = new G4PrimaryVertex(
                sourcePosition,
                emissionTime
            );

            auto* primary = new G4PrimaryParticle(definition);
            primary->SetUserInformation(
                new SourcePrimaryParticleInformation(sourceID)
            );
            primary->SetKineticEnergy(kineticEnergy);
            primary->SetMomentumDirection(direction);

            vertex->SetPrimary(primary);
            event->AddPrimaryVertex(vertex);
        }
    }

    SourceEventInformation* GetOrCreateSourceEventInformation(G4Event* event)
    {
        auto* info = dynamic_cast<SourceEventInformation*>(
            event->GetUserInformation()
        );

        if (info)
            return info;

        if (event->GetUserInformation())
        {
            G4Exception(
                "PrimaryGeneratorAction::GeneratePrimaries",
                "SOURCE_EVENT001",
                FatalException,
                "G4Event already has user information of another type; "
                "cannot attach FREYA source metadata."
            );
            return nullptr;
        }

        info = new SourceEventInformation();
        event->SetUserInformation(info);
        return info;
    }
}

PrimaryGeneratorAction::PrimaryGeneratorAction(
    const SimulationConfig& config
)
    : fConfig(config)
{
    if (Settings::GENERATION_MODE == Settings::GenerationMode::FREYA)
    {
        // Initialize once before the first event. This validates the FREYA
        // data directory and configures its correlation mode and RNG.
        FreyaFissionGenerator::Initialize();
    }
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    // Geometrical center of the Cf-252 sample (unchanged).
    const G4double sampleCenterY =
        fConfig.peThickness + Settings::ENVELOPE_SIZE_Y / 2.0;

    const G4ThreeVector sampleCenter(
        0.0,
        sampleCenterY,
        0.0
    );

    const G4double sampleRadius = Settings::SAMPLE_RADIUS;
    const G4double sampleHalfHeight = Settings::SAMPLE_HEIGHT / 2.0;

    // In FREYA mode all generated fission source IDs are stored on the event.
    // EventAction counts them at EndOfEventAction(), including a fission with
    // no emitted primary tracks. This gives an exact total-fission denominator.
    SourceEventInformation* freyaEventInfo = nullptr;
    if (Settings::GENERATION_MODE == Settings::GenerationMode::FREYA)
    {
        freyaEventInfo = GetOrCreateSourceEventInformation(event);
        if (!freyaEventInfo)
            return;
    }

    // Keep the original random-position algorithm unchanged. Each iteration
    // creates one independent Cf-252 source (one primary nucleus in GEANT mode
    // or one FREYA fission in FREYA mode).
    for (G4int i = 0; i < Settings::N_IONS; ++i)
    {
        const G4double r =
            sampleRadius * std::sqrt(G4UniformRand());

        const G4double phi =
            2.0 * CLHEP::pi * G4UniformRand();

        const G4double yOffset =
            (2.0 * G4UniformRand() - 1.0) * sampleHalfHeight;

        const G4double x = sampleCenter.x() + r * std::cos(phi);
        const G4double y = sampleCenter.y() + yOffset;
        const G4double z = sampleCenter.z() + r * std::sin(phi);

        const G4ThreeVector sourcePosition(x, y, z);
        const G4int sourceID = i;

        if (Settings::GENERATION_MODE == Settings::GenerationMode::GEANT)
        {
            Geant4DecayGenerator::GenerateCf252(
                sourcePosition,
                sourceID,
                event
            );
        }
        else
        {
            const FreyaFissionResult fission =
                FreyaFissionGenerator::GenerateCf252();

            // Register this fission independently of its number of primary
            // particles, then add all its particles with the same source ID.
            freyaEventInfo->AddFreyaFissionSource(sourceID);

            AddFreyaProductsToEvent(
                fission,
                sourcePosition,
                sourceID,
                event
            );
        }
    }
}
