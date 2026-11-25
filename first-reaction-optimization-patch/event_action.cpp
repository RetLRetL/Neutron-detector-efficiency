#include "event_action.h"
#include "run_action.h"

EventAction::EventAction(RunAction* runAction)
    : fRunAction(runAction),
      fReactionDetected(false)
{
}

void EventAction::BeginOfEventAction(const G4Event*)
{
    fReactionDetected = false;
}

G4bool EventAction::MarkReaction()
{
    // The requested observable is binary per Geant4 event:
    // did at least one 3He(n,p)3H reaction occur?
    if (fReactionDetected)
        return false;

    fReactionDetected = true;
    fRunAction->AddDetectedEvent();
    return true;
}
