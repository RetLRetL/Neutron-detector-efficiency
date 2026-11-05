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
#include "G4SystemOfUnits.hh"
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
        {
            fEventAction->GetRunAction()->AddBornNeutron();
        }
    }

    // Only neutrons can initiate the reaction we are looking for.
    if (track->GetDefinition() != G4Neutron::NeutronDefinition())
        return;

    // ================================================================
    // 2. Check whether the neutron entered He3Gas.
    // ================================================================

    const G4VPhysicalVolume* preVolume =
        step->GetPreStepPoint()
            ->GetTouchableHandle()
            ->GetVolume();

    const G4VPhysicalVolume* postVolume =
        step->GetPostStepPoint()
            ->GetTouchableHandle()
            ->GetVolume();

    if (preVolume && postVolume)
    {
        const G4LogicalVolume* preLogicalVolume =
            preVolume->GetLogicalVolume();

        const G4LogicalVolume* postLogicalVolume =
            postVolume->GetLogicalVolume();

        if (preLogicalVolume &&
            postLogicalVolume &&
            preLogicalVolume->GetName() != "He3Gas" &&
            postLogicalVolume->GetName() == "He3Gas")
        {
            const G4double neutronEnergyAtEntry =
                step->GetPostStepPoint()->GetKineticEnergy();

            G4cout
                << "entered He: "
                << neutronEnergyAtEntry / eV
                << G4endl;
        }
    }

    // ================================================================
    // 3. The interaction must occur inside He3Gas.
    // ================================================================

    if (!postVolume)
        return;

    const G4LogicalVolume* logicalVolume =
        postVolume->GetLogicalVolume();

    if (!logicalVolume ||
        logicalVolume->GetName() != "He3Gas")
    {
        return;
    }

    // ================================================================
    // 4. Search secondaries produced on this step.
    // ================================================================

    const std::vector<const G4Track*>* secondaries =
        step->GetSecondaryInCurrentStep();

    if (!secondaries)
        return;

    G4bool protonFound = false;
    G4bool tritonFound = false;

    for (const G4Track* secondary : *secondaries)
    {
        if (secondary->GetDefinition() ==
            G4Proton::ProtonDefinition())
        {
            protonFound = true;
        }

        if (secondary->GetDefinition() ==
            G4Triton::TritonDefinition())
        {
            tritonFound = true;
        }
    }

    // ================================================================
    // 5. 3He(n,p)3H reaction detected.
    // ================================================================

    if (protonFound && tritonFound)
    {
        if (!fEventAction->IsReactionDetected())
        {
            const G4double neutronEnergy =
                step->GetPreStepPoint()->GetKineticEnergy();

            fEventAction->MarkReaction();

            G4cout
                << "detected: "
                << neutronEnergy / eV
                << G4endl;
        }
    }
}