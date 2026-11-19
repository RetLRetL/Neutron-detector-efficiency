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

void EventAction::MarkReaction()
{
    // Every reaction is counted separately.
    fRunAction->AddDetectedReaction();

    // The event itself is counted at most once.
    if (!fReactionDetected)
    {
        fReactionDetected = true;
        fRunAction->AddDetectedEvent();
    }
}

G4bool EventAction::IsReactionDetected() const
{
    return fReactionDetected;
}

RunAction* EventAction::GetRunAction() const
{
    return fRunAction;
}
