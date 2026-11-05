#pragma once

#include "G4UserRunAction.hh"
#include "globals.hh"

class G4Run;

class RunAction : public G4UserRunAction
{
public:
    RunAction();
    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run* run) override;
    void EndOfRunAction(const G4Run* run) override;

    void AddDetectedEvent();
    void AddDetectedReaction();
    void AddBornNeutron();

private:
    G4int fDetectedEvents;
    G4int fDetectedReactions;
    G4int fBornNeutrons;
};
