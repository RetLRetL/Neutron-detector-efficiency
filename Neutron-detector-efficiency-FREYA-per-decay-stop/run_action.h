#pragma once

#include "G4UserRunAction.hh"
#include "globals.hh"

class G4Run;

class RunAction final : public G4UserRunAction
{
public:
    RunAction();
    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run* run) override;
    void EndOfRunAction(const G4Run* run) override;

    void AddTotalDecay();
    void AddDetectedDecay();
    void AddEventWithDetectedDecay();
    void AddBornNeutron();

    void AddDirectCf252Neutrons(G4int count);
private:
    G4long fTotalDecays;
    G4long fDetectedDecays;
    G4long fEventsWithDetectedDecay;
    G4long fBornNeutrons;
    G4long fDirectCf252Neutrons;
};
