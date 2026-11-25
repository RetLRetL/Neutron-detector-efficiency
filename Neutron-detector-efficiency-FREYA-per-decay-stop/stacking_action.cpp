#include "stacking_action.h"

#include "event_action.h"
#include "run_action.h"
#include "settings.h"
#include "source_track_information.h"

#include "G4DynamicParticle.hh"
#include "G4Neutron.hh"
#include "G4PrimaryParticle.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"

StackingAction::StackingAction(EventAction* eventAction)
    : fEventAction(eventAction)
{
}

G4ClassificationOfNewTrack StackingAction::ClassifyNewTrack(
    const G4Track* track
)
{
    // Count a neutron when the track is created/stacked, not at its first
    // transport step. Thus neutrons from a source that is stopped before
    // transport are still included in BornNeutrons.
    if (track->GetDefinition() == G4Neutron::NeutronDefinition())
    {
        const G4VProcess* creator = track->GetCreatorProcess();

        const G4bool fromGeant4RadioactiveDecay =
            track->GetParentID() != 0 &&
            creator &&
            creator->GetProcessName() == "RadioactiveDecay";

        const G4bool fromFreyaPrimary =
            Settings::GENERATION_MODE == Settings::GenerationMode::FREYA &&
            track->GetParentID() == 0;

        if (fromGeant4RadioactiveDecay || fromFreyaPrimary)
            fEventAction->GetRunAction()->AddBornNeutron();
    }

    // A secondary already carries SourceTrackInformation from its parent.
    const auto* trackInfo =
        dynamic_cast<const SourceTrackInformation*>(track->GetUserInformation());

    if (trackInfo && fEventAction->IsSourceStopped(trackInfo->GetSourceID()))
        return fKill;

    // Primary particles (including FREYA's separate neutron/gamma primaries)
    // carry the identifier in their G4PrimaryParticle user information.
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

            if (primaryInfo &&
                fEventAction->IsSourceStopped(primaryInfo->GetSourceID()))
            {
                return fKill;
            }
        }
    }

    return fUrgent;
}
