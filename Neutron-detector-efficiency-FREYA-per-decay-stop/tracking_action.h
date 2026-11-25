#pragma once

#include "G4UserTrackingAction.hh"

class EventAction;
class G4Track;

class TrackingAction final : public G4UserTrackingAction
{
public:
    explicit TrackingAction(EventAction* eventAction);
    ~TrackingAction() override = default;

    void PreUserTrackingAction(const G4Track* track) override;
    void PostUserTrackingAction(const G4Track* track) override;

private:
    EventAction* fEventAction;
};
