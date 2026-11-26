#pragma once

#include "G4UserEventAction.hh"
#include "globals.hh"

#include <unordered_set>

class G4Event;
class RunAction;

class EventAction final : public G4UserEventAction
{
public:
    explicit EventAction(RunAction* runAction);
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    // Register one actual/generated Cf-252 fission. A source is counted once.
    G4bool RegisterDecay(G4int sourceID);

    // Register a successful source, stop its remaining track tree, and count
    // this source once in the efficiency numerator.
    G4bool MarkDetectedDecay(G4int sourceID);

    G4bool IsSourceStopped(G4int sourceID) const;
    RunAction* GetRunAction() const;

private:
    RunAction* fRunAction;
    G4bool fEventHasDetectedDecay;

    // These sets are reset for every G4Event. sourceID is unique within an event.
    std::unordered_set<G4int> fCountedDecaySources;
    std::unordered_set<G4int> fDetectedDecaySources;
    std::unordered_set<G4int> fStoppedSources;
};
