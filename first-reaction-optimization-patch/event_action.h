#pragma once

#include "G4UserEventAction.hh"
#include "globals.hh"

class RunAction;

class EventAction : public G4UserEventAction
{
public:
    explicit EventAction(RunAction* runAction);
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;

    // Returns true only for the first reaction in this Geant4 event.
    G4bool MarkReaction();

private:
    RunAction* fRunAction;
    G4bool fReactionDetected;
};
