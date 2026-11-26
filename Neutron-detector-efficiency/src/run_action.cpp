#include "run_action.h"

#include "settings.h"

#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

#include <iomanip>

RunAction::RunAction()
    : fTotalDecays(0),
      fDetectedDecays(0),
      fEventsWithDetectedDecay(0),
      fBornNeutrons(0),
      fDirectCf252Neutrons(0)
{
}

void RunAction::BeginOfRunAction(const G4Run*)
{
    fTotalDecays = 0;
    fDetectedDecays = 0;
    fEventsWithDetectedDecay = 0;
    fBornNeutrons = 0;
    fDirectCf252Neutrons = 0;
}

void RunAction::AddTotalDecay()
{
    ++fTotalDecays;
}

void RunAction::AddDetectedDecay()
{
    ++fDetectedDecays;
}

void RunAction::AddEventWithDetectedDecay()
{
    ++fEventsWithDetectedDecay;
}

void RunAction::AddBornNeutron()
{
    ++fBornNeutrons;
}

void RunAction::AddDirectCf252Neutrons(G4int count)
{
    fDirectCf252Neutrons += count;
}

void RunAction::EndOfRunAction(const G4Run* run)
{
    const G4long nEvents = run->GetNumberOfEvent();
    const G4long expectedDecays =
        nEvents * static_cast<G4long>(Settings::N_IONS);

    G4cout << G4endl;
    G4cout << "============================" << G4endl;
    G4cout << "        RUN RESULTS" << G4endl;
    G4cout << "============================" << G4endl;

    G4cout << "Total events processed       : " << nEvents << G4endl;
    G4cout << "Expected source decays       : " << expectedDecays << G4endl;
    G4cout << "Total decays                 : " << fTotalDecays << G4endl;
    G4cout << "Detected decays              : " << fDetectedDecays << G4endl;
    G4cout << "Events with detected decays  : "
           << fEventsWithDetectedDecay << G4endl;
    G4cout << "Born neutrons (diagnostic)   : " << fBornNeutrons << G4endl;
    G4cout << "Direct Cf-252 neutrons      : "
        << fDirectCf252Neutrons << G4endl;

    if (fTotalDecays > 0)
    {
        G4cout << "Mean direct neutrons/decay : "
            << static_cast<G4double>(fDirectCf252Neutrons) /
                    fTotalDecays
            << G4endl;
    }

    if (fTotalDecays != expectedDecays)
    {
        G4cerr << "WARNING: counted decay total (" << fTotalDecays
               << ") differs from N_IONS * processed events ("
               << expectedDecays << "). Check that every GEANT Cf-252 nucleus "
               << "undergoes the forced spontaneous-fission channel." << G4endl;
    }

    if (fTotalDecays > 0)
    {
        const G4double efficiency =
            static_cast<G4double>(fDetectedDecays) /
            static_cast<G4double>(fTotalDecays);

        G4cout << std::setprecision(8);
        G4cout << "Decay efficiency             : " << efficiency << G4endl;
        G4cout << "Decay efficiency (%)         : "
               << efficiency * 100.0 << " %" << G4endl;
    }
    else
    {
        G4cout << "Decay efficiency             : undefined (no decays counted)"
               << G4endl;
    }

    G4cout << "============================" << G4endl;
}
