#include "run_action.h"

#include "G4Run.hh"
#include "G4ios.hh"

RunAction::RunAction()
    : fDetectedEvents(0)
{
}

void RunAction::BeginOfRunAction(const G4Run*)
{
    fDetectedEvents = 0;
}

void RunAction::AddDetectedEvent()
{
    ++fDetectedEvents;
}

void RunAction::EndOfRunAction(const G4Run* run)
{
    // Use the requested event count as the denominator. AbortEvent() aborts
    // transport inside an event, not the BeamOn() event loop itself.
    const G4int totalEvents = run->GetNumberOfEventToBeProcessed();
    G4cout << G4endl;
    G4cout << "============================" << G4endl;
    G4cout << "        RUN RESULTS" << G4endl;
    G4cout << "============================" << G4endl;
    G4cout << "Total events     : " << totalEvents << G4endl;
    G4cout << "Detected events  : " << fDetectedEvents << G4endl;

    if (totalEvents > 0)
    {
        const G4double efficiency =
            static_cast<G4double>(fDetectedEvents) /
            static_cast<G4double>(totalEvents);

        G4cout << "Event efficiency : " << efficiency << G4endl;
        G4cout << "Event efficiency : "
               << efficiency * 100.0
               << " %"
               << G4endl;
    }

    G4cout << "============================" << G4endl;
}
