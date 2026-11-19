#include "run_action.h"

#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

RunAction::RunAction()
    : fDetectedEvents(0),
      fDetectedReactions(0),
      fBornNeutrons(0)
{
}

void RunAction::BeginOfRunAction(const G4Run*)
{
    fDetectedEvents = 0;
    fDetectedReactions = 0;
    fBornNeutrons = 0;
}

void RunAction::AddDetectedEvent()
{
    ++fDetectedEvents;
}

void RunAction::AddDetectedReaction()
{
    ++fDetectedReactions;
}

void RunAction::AddBornNeutron()
{
    ++fBornNeutrons;
}

void RunAction::EndOfRunAction(const G4Run* run)
{
    const G4int nEvents = run->GetNumberOfEvent();

    G4cout << G4endl;
    G4cout << "============================" << G4endl;
    G4cout << "        RUN RESULTS" << G4endl;
    G4cout << "============================" << G4endl;

    G4cout << "Events             : " << nEvents << G4endl;
    G4cout << "Born neutrons      : " << fBornNeutrons << G4endl;
    G4cout << "Detected events    : " << fDetectedEvents << G4endl;
    G4cout << "Detected reactions : " << fDetectedReactions << G4endl;

    if (fBornNeutrons > 0)
    {
        const G4double efficiency =
            static_cast<G4double>(fDetectedReactions) /
            static_cast<G4double>(fBornNeutrons);

        G4cout << "Efficiency         : " << efficiency << G4endl;
        G4cout << "Efficiency (%)     : "
               << efficiency * 100.0
               << " %"
               << G4endl;
    }

    G4cout << "============================" << G4endl;
}
