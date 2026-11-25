#include "tracking_action.h"

#include "event_action.h"
#include "source_track_information.h"

#include "G4DynamicParticle.hh"
#include "G4PrimaryParticle.hh"
#include "G4Track.hh"
#include "G4TrackStatus.hh"
#include "G4TrackingManager.hh"
#include "G4TrackVector.hh"

TrackingAction::TrackingAction(EventAction* eventAction)
    : fEventAction(eventAction)
{
}

void TrackingAction::PreUserTrackingAction(const G4Track* track)
{
    // A primary track obtains its source ID from the G4PrimaryParticle that
    // was created by PrimaryGeneratorAction.
    if (track->GetParentID() == 0)
    {
        const G4DynamicParticle* dynamicParticle = track->GetDynamicParticle();
        const G4PrimaryParticle* primaryParticle =
            dynamicParticle ? dynamicParticle->GetPrimaryParticle() : nullptr;

        if (primaryParticle)
        {
            const auto* primaryInfo =
                dynamic_cast<const SourcePrimaryParticleInformation*>(
                    primaryParticle->GetUserInformation()
                );

            if (primaryInfo)
            {
                fpTrackingManager->SetUserTrackInformation(
                    new SourceTrackInformation(primaryInfo->GetSourceID())
                );
            }
        }
    }

    // If a sibling particle from this source was already registered as the
    // first one to react, kill this track before it performs its first step.
    const auto* sourceInfo =
        dynamic_cast<const SourceTrackInformation*>(track->GetUserInformation());

    if (sourceInfo && fEventAction->IsSourceStopped(sourceInfo->GetSourceID()))
    {
        const_cast<G4Track*>(track)->SetTrackStatus(fStopAndKill);
    }
}

void TrackingAction::PostUserTrackingAction(const G4Track* track)
{
    // Copy the source identifier to every secondary created by this track.
    // This preserves the ancestry through decays, scattering and other
    // interactions, so all descendants can be stopped with their source.
    const auto* sourceInfo =
        dynamic_cast<const SourceTrackInformation*>(track->GetUserInformation());

    if (!sourceInfo)
        return;

    G4TrackVector* secondaries = fpTrackingManager->GimmeSecondaries();
    if (!secondaries)
        return;

    for (G4Track* secondary : *secondaries)
    {
        if (!secondary)
            continue;

        // The project does not assign other track user information. Avoid
        // overwriting it should another action add such information later.
        if (!secondary->GetUserInformation())
        {
            secondary->SetUserInformation(
                new SourceTrackInformation(sourceInfo->GetSourceID())
            );
        }
    }
}
