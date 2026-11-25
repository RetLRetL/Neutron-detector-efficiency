#include "stepping_action.h"

#include "event_action.h"
#include "source_track_information.h"

#include "G4Exception.hh"
#include "G4Ions.hh"
#include "G4LogicalVolume.hh"
#include "G4Neutron.hh"
#include "G4Proton.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4Track.hh"
#include "G4TrackStatus.hh"
#include "G4Triton.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VProcess.hh"
#include "G4ios.hh"
#include "run_action.h" 

#include <vector>

namespace
{
    const SourceTrackInformation* GetSourceInformation(const G4Track* track)
    {
        return dynamic_cast<const SourceTrackInformation*>(
            track->GetUserInformation()
        );
    }

    G4bool IsCf252RadioactiveDecay(const G4Step* step)
    {
        const G4Track* track = step->GetTrack();
        const auto* ion = dynamic_cast<const G4Ions*>(track->GetDefinition());

        if (!ion)
            return false;

        if (ion->GetAtomicNumber() != 98 || ion->GetAtomicMass() != 252)
            return false;

        const G4StepPoint* postStep = step->GetPostStepPoint();
        const G4VProcess* definingProcess =
            postStep ? postStep->GetProcessDefinedStep() : nullptr;

        return definingProcess &&
               definingProcess->GetProcessName() == "RadioactiveDecay";
    }
}

SteppingAction::SteppingAction(EventAction* eventAction)
    : fEventAction(eventAction)
{
}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    G4Track* track = step->GetTrack();

    // ------------------------------------------------------------
    // 1. Count an actual Cf-252 decay in GEANT mode.
    //
    // The denominator is the number of Cf-252 spontaneous-fission
    // sources (one fission per initial source nucleus), not the number
    // of G4Events and not the number of emitted neutrons.
    // The custom decay file loaded in main.cpp must force the Cf-252
    // branching ratio to the spontaneous-fission channel.
    // ------------------------------------------------------------

    if (IsCf252RadioactiveDecay(step))
    {
        const SourceTrackInformation* sourceInfo =
            GetSourceInformation(track);

        if (!sourceInfo)
        {
            G4Exception(
                "SteppingAction::UserSteppingAction",
                "SOURCE_DECAY001",
                FatalException,
                "Cf-252 RadioactiveDecay track has no source ID."
            );
            return;
        }

        // Get all particles created directly by this Cf-252 decay.
        const auto* secondaries = step->GetSecondaryInCurrentStep();

        G4int directNeutrons = 0;

        if (secondaries)
        {
            for (const G4Track* secondary : *secondaries)
            {
                if (secondary &&
                    secondary->GetDefinition() ==
                        G4Neutron::NeutronDefinition())
                {
                    ++directNeutrons;
                }
            }
        }

        // Count this decay only once.
        if (fEventAction->RegisterDecay(sourceInfo->GetSourceID()))
        {
            fEventAction->GetRunAction()
                ->AddDirectCf252Neutrons(directNeutrons);
        }

        return;
    }

    // ------------------------------------------------------------
    // 2. Only neutrons can initiate the detector reaction.
    // ------------------------------------------------------------

    if (track->GetDefinition() != G4Neutron::NeutronDefinition())
        return;

    // The interaction must occur inside He3Gas.
    const G4VPhysicalVolume* volume =
        step->GetPreStepPoint()
            ->GetTouchableHandle()
            ->GetVolume();

    if (!volume)
        return;

    const G4LogicalVolume* logicalVolume = volume->GetLogicalVolume();
    if (!logicalVolume || logicalVolume->GetName() != "He3Gas")
        return;

    // ------------------------------------------------------------
    // 3. Check whether the current neutron step produced both reaction
    //    products of 3He(n,p)3H.
    // ------------------------------------------------------------

    const std::vector<const G4Track*>* secondaries =
        step->GetSecondaryInCurrentStep();

    if (!secondaries)
        return;

    G4bool protonFound = false;
    G4bool tritonFound = false;

    for (const G4Track* secondary : *secondaries)
    {
        if (secondary->GetDefinition() == G4Proton::ProtonDefinition())
            protonFound = true;

        if (secondary->GetDefinition() == G4Triton::TritonDefinition())
            tritonFound = true;
    }

    if (!protonFound || !tritonFound)
        return;

    // ------------------------------------------------------------
    // 4. This neutron's source has produced a detectable reaction.
    //    Count the source once and mark all its remaining tracks for killing.
    // ------------------------------------------------------------

    const SourceTrackInformation* sourceInfo = GetSourceInformation(track);

    if (!sourceInfo)
    {
        G4Exception(
            "SteppingAction::UserSteppingAction",
            "SOURCE_REACTION001",
            FatalException,
            "A neutron caused 3He(n,p)3H but has no source ID. "
            "Check primary tagging and TrackingAction propagation."
        );
        return;
    }

    const G4int sourceID = sourceInfo->GetSourceID();
    const G4bool firstReactionFromSource =
        fEventAction->MarkDetectedDecay(sourceID);

    // Kill the neutron that caused the reaction immediately. The products
    // generated on this step and all previously queued tracks of this source
    // are removed by StackingAction / TrackingAction. Other source IDs in the
    // same G4Event are unaffected.
    track->SetTrackStatus(fStopAndKill);

    if (firstReactionFromSource)
    {
        G4cout << "3He(n,p)3H reaction! Detected source ID: "
               << sourceID << G4endl;
    }
}
