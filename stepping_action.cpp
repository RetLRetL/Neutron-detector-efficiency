#include "stepping_action.h"
#include "event_action.h"
#include "run_action.h"

#include "G4LogicalVolume.hh"
#include "G4Neutron.hh"
#include "G4Proton.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Triton.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VProcess.hh"
#include "G4ios.hh"

#include <vector>

SteppingAction::SteppingAction(EventAction* eventAction)
    : fEventAction(eventAction)
{}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    const G4Track* track = step->GetTrack();

    // ================================================================
    // 1. Count neutrons produced by radioactive decay.
    // ================================================================

    if (track->GetDefinition() == G4Neutron::NeutronDefinition() &&
        track->GetCurrentStepNumber() == 1 &&
        track->GetParentID() != 0)
    {
        const G4VProcess* creator = track->GetCreatorProcess();

        if (creator && creator->GetProcessName() == "RadioactiveDecay")
            fEventAction->GetRunAction()->AddBornNeutron();
    }

    // Only neutrons can initiate the reaction we are looking for.
    if (track->GetDefinition() != G4Neutron::NeutronDefinition())
        return;

    // ================================================================
    // 2. The interaction must occur inside He3Gas.
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
    // 3. Search secondaries produced on this step.
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
    // 4. 3He(n,p)3H reaction detected.
    // ================================================================

    if (protonFound && tritonFound)
    {
        fEventAction->MarkReaction();

        G4cout << "3He(n,p)3H reaction!" << G4endl;
    }
}
