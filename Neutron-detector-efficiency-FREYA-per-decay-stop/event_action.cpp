#include "event_action.h"

#include "run_action.h"
#include "source_track_information.h"

#include "G4Event.hh"

EventAction::EventAction(RunAction* runAction)
    : fRunAction(runAction),
      fEventHasDetectedDecay(false)
{
}

void EventAction::BeginOfEventAction(const G4Event*)
{
    fEventHasDetectedDecay = false;
    fCountedDecaySources.clear();
    fDetectedDecaySources.clear();
    fStoppedSources.clear();
}

void EventAction::EndOfEventAction(const G4Event* event)
{
    // FREYA fissions are generated before Geant4 begins tracking the event.
    // Count the source IDs from event metadata here, so the denominator does
    // not depend on how many neutron/gamma primary tracks FREYA produced.
    const auto* sourceEventInfo =
        dynamic_cast<const SourceEventInformation*>(event->GetUserInformation());

    if (!sourceEventInfo)
        return;

    for (const G4int sourceID : sourceEventInfo->GetFreyaFissionSourceIDs())
        RegisterDecay(sourceID);
}

G4bool EventAction::RegisterDecay(G4int sourceID)
{
    const auto inserted = fCountedDecaySources.insert(sourceID);
    if (!inserted.second)
        return false;

    fRunAction->AddTotalDecay();
    return true;
}

G4bool EventAction::MarkDetectedDecay(G4int sourceID)
{
    // A source must be counted in the denominator even if, due to an unusual
    // process ordering, its decay-registration callback has not run yet.
    RegisterDecay(sourceID);

    // This insertion both makes the numerator unique and marks this source
    // for early termination of all remaining tracks in its ancestry tree.
    const auto inserted = fDetectedDecaySources.insert(sourceID);
    if (!inserted.second)
        return false;

    fStoppedSources.insert(sourceID);
    fRunAction->AddDetectedDecay();

    // This event-level value is only a diagnostic. It is NOT used for the
    // per-decay detection efficiency.
    if (!fEventHasDetectedDecay)
    {
        fEventHasDetectedDecay = true;
        fRunAction->AddEventWithDetectedDecay();
    }

    return true;
}

G4bool EventAction::IsSourceStopped(G4int sourceID) const
{
    return fStoppedSources.find(sourceID) != fStoppedSources.end();
}

RunAction* EventAction::GetRunAction() const
{
    return fRunAction;
}
