#pragma once

#include "G4VUserEventInformation.hh"
#include "G4VUserPrimaryParticleInformation.hh"
#include "G4VUserTrackInformation.hh"
#include "globals.hh"

#include <unordered_set>

// Metadata attached to a primary particle. All primary particles produced by
// one Cf-252 source (GEANT parent ion or FREYA fission products) carry the same
// source ID. The ID is local to one G4Event: 0 ... N_IONS-1.
class SourcePrimaryParticleInformation final
    : public G4VUserPrimaryParticleInformation
{
public:
    explicit SourcePrimaryParticleInformation(G4int sourceID)
        : fSourceID(sourceID)
    {
    }

    G4int GetSourceID() const
    {
        return fSourceID;
    }

    void Print() const override {}

private:
    G4int fSourceID;
};

// Track-level copy of the source ID. TrackingAction propagates it from every
// track to all of its secondaries, preserving the original source ancestry.
class SourceTrackInformation final
    : public G4VUserTrackInformation
{
public:
    explicit SourceTrackInformation(G4int sourceID)
        : fSourceID(sourceID)
    {
    }

    G4int GetSourceID() const
    {
        return fSourceID;
    }

    void Print() const override {}

private:
    G4int fSourceID;
};

// FREYA performs each fission while GeneratePrimaries() is building the event.
// Store the IDs of all generated fissions on G4Event so EventAction can count
// every generated decay at EndOfEventAction(), including an event whose
// fission happened to produce no tracked primary particle.
class SourceEventInformation final
    : public G4VUserEventInformation
{
public:
    void AddFreyaFissionSource(G4int sourceID)
    {
        fFreyaFissionSourceIDs.insert(sourceID);
    }

    const std::unordered_set<G4int>& GetFreyaFissionSourceIDs() const
    {
        return fFreyaFissionSourceIDs;
    }

    void Print() const override {}

private:
    std::unordered_set<G4int> fFreyaFissionSourceIDs;
};
