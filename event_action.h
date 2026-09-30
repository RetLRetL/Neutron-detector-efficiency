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

    void MarkReaction();
    G4bool IsReactionDetected() const;
    RunAction* GetRunAction() const;

private:
    RunAction* fRunAction;
    G4bool fReactionDetected;
};
