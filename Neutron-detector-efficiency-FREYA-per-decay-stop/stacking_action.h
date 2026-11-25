#pragma once

#include "G4UserStackingAction.hh"
#include "G4ClassificationOfNewTrack.hh"

class EventAction;
class G4Track;


class StackingAction final : public G4UserStackingAction
{
public:
    explicit StackingAction(EventAction* eventAction);
    ~StackingAction() override = default;

    G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track* track) override;

private:
    EventAction* fEventAction;
};
