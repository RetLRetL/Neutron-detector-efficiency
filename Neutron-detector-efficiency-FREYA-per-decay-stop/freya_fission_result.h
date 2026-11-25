#pragma once

#include <array>
#include <vector>

// Type of a primary particle produced by one FREYA fission event.
enum class FreyaParticleType
{
    Neutron,
    Gamma
};

// Plain data for one particle returned by FREYA.
// Energies are stored in MeV, emission times in seconds, and directions
// are the direction cosines returned by the FREYA library.
struct FreyaParticleData
{
    FreyaParticleType type = FreyaParticleType::Neutron;
    double kineticEnergyMeV = 0.0;
    std::array<double, 3> direction{{0.0, 0.0, 1.0}};
    double emissionTimeSeconds = 0.0;
};

// Result of exactly one spontaneous fission of Cf-252.
// It intentionally contains data only: no Geant4 or ROOT objects.
struct FreyaFissionResult
{
    int neutronMultiplicity = 0;
    int photonMultiplicity = 0;
    std::vector<FreyaParticleData> particles;
};
