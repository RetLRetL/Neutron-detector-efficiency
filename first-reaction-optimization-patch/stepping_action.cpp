#include "stepping_action.h"
#include "event_action.h"

#include "G4LogicalVolume.hh"
#include "G4Neutron.hh"
#include "G4Proton.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Triton.hh"
#include "G4VPhysicalVolume.hh"

#include <vector>

SteppingAction::SteppingAction(EventAction* eventAction)
    : fEventAction(eventAction)
{}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    const G4Track* track = step->GetTrack();

    // Only neutrons can initiate the reaction we are looking for.
    if (track->GetDefinition() != G4Neutron::NeutronDefinition())
        return;

    // ================================================================
    // 1. The interaction must occur inside He3Gas.
    // ================================================================

    const G4VPhysicalVolume* volume =
        step->GetPreStepPoint()
            ->GetTouchableHandle()
            ->GetVolume();

    if (!volume)
        return;

    const G4LogicalVolume* logicalVolume =
        volume->GetLogicalVolume();

    if (!logicalVolume || logicalVolume->GetName() != "He3Gas")
        return;

    // ================================================================
    // 2. Search secondaries produced on this step.
    // ================================================================

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

    // ================================================================
    // 3. 3He(n,p)3H reaction detected.
    // ================================================================

    if (protonFound && tritonFound && fEventAction->MarkReaction())
    {
        // We only need to know whether this event had at least one reaction.
        // Stop tracking the current event immediately; BeamOn() will continue
        // with the next Geant4 event.
        G4RunManager::GetRunManager()->AbortEvent();
        return;
    }
}
