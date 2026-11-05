#include "G4RunManager.hh"
#include "G4VUserDetectorConstruction.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"

#include "FTFP_BERT_HP.hh"

#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"

#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"

#include "G4Material.hh"
#include "G4Isotope.hh"
#include "G4Element.hh"

#include "G4PhysicalConstants.hh"

#include "palette.h"

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4Neutron.hh"
#include "G4SystemOfUnits.hh"

#include "G4ElectricField.hh"
#include "G4FieldManager.hh"
#include "G4EqMagElectricField.hh"
#include "G4ClassicalRK4.hh"
#include "G4ChordFinder.hh"

#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

#include "G4IntegrationDriver.hh"

#include "G4UserRunAction.hh"
#include "G4UserEventAction.hh"
#include "G4UserSteppingAction.hh"

#include "G4Run.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "G4Track.hh"

#include "G4Neutron.hh"
#include "G4Proton.hh"
#include "G4Triton.hh"
#include "G4VProcess.hh"

#include "G4IonTable.hh"
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"
#include "G4HadronicParameters.hh"

#include "G4RadioactiveDecayPhysics.hh"

#include <cmath>

using namespace std;

class He3ElectricField : public G4ElectricField
{
public:

    void GetFieldValue(
        const G4double point[4],
        G4double* field
    ) const override
    {
        const G4double y = point[1];
        const G4double z = point[2];

        const G4double r =
            sqrt(y * y + z * z);

        const G4double a = 0.5 * mm;
        const G4double b = 24.0 * mm;
        const G4double voltage = 1450.0 * volt;

        // За пределами области газа поле здесь не задаём
        if (r <= a || r >= b)
        {
            for (int i = 0; i < 6; ++i)
                field[i] = 0.0;

            return;
        }

        const G4double E =
            voltage /
            (r * log(b / a));

        // Electric field
        field[3] = 0.0;       // Ex
        field[4] = E * y / r; // Ey
        field[5] = E * z / r; // Ez

        // Magnetic field
        field[0] = 0.0;       // Bx
        field[1] = 0.0;       // By
        field[2] = 0.0;       // Bz

    }
};

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    G4VPhysicalVolume* Construct() override
    {
        G4NistManager* nist = G4NistManager::Instance();

        G4Material* air =
            nist->FindOrBuildMaterial("G4_AIR");

        G4Material* steel =
            new G4Material(
                "Steel",
                7.87 * g / cm3,
                2
            );

        steel->AddElement(
            nist->FindOrBuildElement("Fe"),
            0.998
        );

        steel->AddElement(
            nist->FindOrBuildElement("C"),
            0.002
        );

        G4Material* copper =
            nist->FindOrBuildMaterial("G4_Cu");

        G4Material* polyethylene =
            nist->FindOrBuildMaterial("G4_POLYETHYLENE");

        // ============================================================
        // He-3 gas
        // ============================================================

        G4Isotope* He3 =
            new G4Isotope(
                "He3",
                2,
                3,
                3.016 * g / mole
            );

        G4Element* helium3 =
            new G4Element(
                "Helium3",
                "He3",
                1
            );

        helium3->AddIsotope(He3, 100.0 * perCent);

        const G4double temperature = 293.15 * kelvin;
        const G4double pressure = 2.5 * atmosphere;

        const G4double density =
            pressure * (3.016 * g / mole) /
            (Avogadro * k_Boltzmann * temperature);

        G4Material* helium3Gas =
            new G4Material(
                "He3Gas",
                density,
                1,
                kStateGas,
                temperature,
                pressure
            );

        helium3Gas->AddElement(helium3, 1.0);

        // ============================================================
        // World
        // ============================================================

        G4Box* worldSolid =
            new G4Box(
                "World",
                25.0 * cm,
                25.0 * cm,
                25.0 * cm
            );

        G4LogicalVolume* worldLogic =
            new G4LogicalVolume(
                worldSolid,
                air,
                "World"
            );

        G4VPhysicalVolume* worldPhysical =
            new G4PVPlacement(
                nullptr,
                G4ThreeVector(),
                worldLogic,
                "World",
                nullptr,
                false,
                0,
                true
            );

        // ============================================================
        // Envelope
        //
        // Length: 417 mm
        // Cross-section: 50 x 50 mm
        // ============================================================

        G4Box* envelopeSolid =
            new G4Box(
                "Envelope",
                417.0 * mm / 2.0,
                50.0 * mm / 2.0,
                50.0 * mm / 2.0
            );

        G4LogicalVolume* envelopeLogic =
            new G4LogicalVolume(
                envelopeSolid,
                air,
                "Envelope"
            );

        new G4PVPlacement(
            nullptr,
            G4ThreeVector(),
            envelopeLogic,
            "Envelope",
            worldLogic,
            false,
            0,
            true
        );

        // ============================================================
        // Steel tube
        //
        // Length: 380 mm
        // Outer diameter: 50 mm
        // Wall thickness: 1 mm
        // Inner diameter: 48 mm
        //
        // Only side wall, no end caps.
        // ============================================================

        G4Tubs* steelSolid =
            new G4Tubs(
                "SteelTube",
                24.0 * mm,              // inner radius
                25.0 * mm,              // outer radius
                380.0 * mm / 2.0,       // half-length
                0.0,
                360.0 * deg
            );

        G4LogicalVolume* steelLogic =
            new G4LogicalVolume(
                steelSolid,
                steel,
                "SteelTube"
            );

        G4RotationMatrix* rotation =
            new G4RotationMatrix();

        rotation->rotateY(90.0 * deg);

        new G4PVPlacement(
            rotation,
            G4ThreeVector(),
            steelLogic,
            "SteelTube",
            envelopeLogic,
            false,
            0,
            true
        );

        // ============================================================
        // He-3 volume
        //
        // Diameter: 48 mm
        // Length: 378 mm
        //
        // The central 1 mm diameter is removed for the wire.
        // ============================================================

        G4Tubs* gasSolid =
            new G4Tubs(
                "He3Cylinder",
                0.0,
                24.0 * mm,
                378.0 * mm / 2.0,
                0.0,
                360.0 * deg
            );

        // ============================================================
        // Wire cut from the He-3 volume
        //
        // Diameter: 1 mm
        // Length: 380 mm
        // ============================================================

        G4Tubs* wireCutSolid =
            new G4Tubs(
                "WireCut",
                0.0,
                0.5 * mm,
                380.0 * mm / 2.0,
                0.0,
                360.0 * deg
            );

        G4SubtractionSolid* gasMinusWire =
            new G4SubtractionSolid(
                "He3MinusWire",
                gasSolid,
                wireCutSolid
            );

        G4LogicalVolume* gasLogic =
            new G4LogicalVolume(
                gasMinusWire,
                helium3Gas,
                "He3Gas"
            );

        // ============================================================
        // Polyethylene moderator
        //
        // Box centered at the origin, half-sizes:
        //   along axes perpendicular to the wire (Y and Z):
        //       100 mm + envelope half-size (25 mm) = 125 mm
        //   along the wire axis (X):
        //       unchanged, equal to envelope half-size (208.5 mm)
        //
        // The envelope volume (417 x 50 x 50 mm) is subtracted,
        // leaving polyethylene surrounding the detector.
        // ============================================================

        const G4double PE_HALF_PERP = 100.0 * mm + 25.0 * mm;  // 125 mm
        const G4double PE_HALF_X    = 417.0 * mm / 2.0;        // 208.5 mm

        G4Box* polyethyleneOuterSolid =
            new G4Box(
                "PolyethyleneOuter",
                PE_HALF_X,
                PE_HALF_PERP,
                PE_HALF_PERP
            );

        // Envelope solid used for subtraction.
        // Same dimensions as the envelope box in DetectorConstruction.
        G4Box* envelopeCutSolid =
            new G4Box(
                "EnvelopeCut",
                417.0 * mm / 2.0,
                50.0 * mm / 2.0,
                50.0 * mm / 2.0
            );

        G4SubtractionSolid* polyethyleneSolid =
            new G4SubtractionSolid(
                "Polyethylene",
                polyethyleneOuterSolid,
                envelopeCutSolid
            );

        G4LogicalVolume* polyethyleneLogic =
            new G4LogicalVolume(
                polyethyleneSolid,
                polyethylene,
                "Polyethylene"
            );

        new G4PVPlacement(
            nullptr,
            G4ThreeVector(),
            polyethyleneLogic,
            "Polyethylene",
            worldLogic,
            false,
            0,
            true
        );

        // ============================================================
        // Cf-example
        // ============================================================

        // ---- Cf-252 isotope ----
        G4Isotope* cf252iso = new G4Isotope(
            "Cf252",
            98,                    // Z
            252,                   // A
            252.08 * g / mole
        );

        G4Element* cfEl = new G4Element(
            "Californium",
            "Cf",
            1
        );
        cfEl->AddIsotope(cf252iso, 100.0 * perCent);

        G4Material* cfMaterial = new G4Material(
            "Cf252",
            15.1 * g / cm3,        // плотность металлического Cf
            1
        );
        cfMaterial->AddElement(cfEl, 1.0);

        G4Tubs* sampleSolid = new G4Tubs(
            "Cf252Sample",
            0.0,
            5.0 * mm,
            0.5 * mm,
            0.0, 360.0 * deg
        );

        G4LogicalVolume* sampleLogic = new G4LogicalVolume(
            sampleSolid, cfMaterial, "Cf252Sample"
        );

        new G4PVPlacement(
            nullptr,
            G4ThreeVector(0.0, 125.5 * mm, 0.0),
            sampleLogic, "Cf252Sample",
            worldLogic, false, 0, true
        );


        // ============================================================
        // Electric field
        // ============================================================

        He3ElectricField* field =
            new He3ElectricField();

        G4EqMagElectricField* equation =
            new G4EqMagElectricField(field);

        const G4int nvar = 8;

        G4ClassicalRK4* stepper =
            new G4ClassicalRK4(equation, nvar);

        const G4double minStep = 0.01 * mm;

        G4IntegrationDriver<G4ClassicalRK4>* driver =
            new G4IntegrationDriver<G4ClassicalRK4>(
                minStep,
                stepper,
                nvar
            );

        G4ChordFinder* chordFinder =
            new G4ChordFinder(driver);

        G4FieldManager* fieldManager =
            new G4FieldManager(
                field,
                chordFinder
            );

        gasLogic->SetFieldManager(
            fieldManager,
            true
        );

        new G4PVPlacement(
            rotation,
            G4ThreeVector(),
            gasLogic,
            "He3Gas",
            envelopeLogic,
            false,
            0,
            true
        );

        // ============================================================
        // Copper wire
        //
        // Diameter: 1 mm
        // Length: 380 mm
        // ============================================================

        G4Tubs* wireSolid =
            new G4Tubs(
                "Wire",
                0.0,
                0.5 * mm,
                380.0 * mm / 2.0,
                0.0,
                360.0 * deg
            );

        G4LogicalVolume* wireLogic =
            new G4LogicalVolume(
                wireSolid,
                copper,
                "Wire"
            );

        new G4PVPlacement(
            rotation,
            G4ThreeVector(),
            wireLogic,
            "Wire",
            envelopeLogic,
            false,
            0,
            true
        );

        // ============================================================
        // Visualization
        // ============================================================

        G4VisAttributes* envelopeVis =
            new G4VisAttributes();

        envelopeVis->SetVisibility(false);
        envelopeLogic->SetVisAttributes(envelopeVis);

        G4VisAttributes* steelVis =
            new G4VisAttributes(
                G4Colour(palette[9])
            );

        steelVis->SetVisibility(true);
        steelVis->SetForceSolid(true);
        steelLogic->SetVisAttributes(steelVis);

        G4VisAttributes* heliumVis =
            new G4VisAttributes(
                G4Colour(palette[14])
            );

        heliumVis->SetVisibility(true);
        heliumVis->SetForceSolid(true);
        gasLogic->SetVisAttributes(heliumVis);

        G4VisAttributes* wireVis =
            new G4VisAttributes(
                G4Colour(palette[10])
            );

        wireVis->SetVisibility(true);
        wireVis->SetForceSolid(true);
        wireLogic->SetVisAttributes(wireVis);

        G4VisAttributes* sampleVis =
            new G4VisAttributes(
                G4Colour(palette[2])
            );

        sampleVis->SetVisibility(true);
        sampleVis->SetForceSolid(true);
        sampleLogic->SetVisAttributes(sampleVis);

        return worldPhysical;
    }
};

/*
class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
    PrimaryGeneratorAction()
    {
        
        
        particleGun = new G4ParticleGun(1);

        G4ParticleDefinition* neutron =
            G4Neutron::NeutronDefinition();

        particleGun->SetParticleDefinition(neutron);

        // Кинетическая энергия 1 GeV
        particleGun->SetParticleEnergy(2.13 * eV);

        // Направление вдоль +X
        particleGun->SetParticleMomentumDirection(
            G4ThreeVector(0.0, -1.0, 0.0)
        );

        // Точка старта перед детектором
        particleGun->SetParticlePosition(
            G4ThreeVector(0.0, 10.0 * cm, 0.0)
        );
        

        particleGun->SetParticlePosition(
            G4ThreeVector(
                0.0,
                225.2 * mm,
                0.0
            )
        );

    }

    ~PrimaryGeneratorAction() override
    {
        delete particleGun;
    }

    void GeneratePrimaries(G4Event* event) override
    {
        particleGun->GeneratePrimaryVertex(event);
    }

private:
    G4ParticleGun* particleGun;
};
*/

class PrimaryGeneratorAction
    : public G4VUserPrimaryGeneratorAction
{
public:

    PrimaryGeneratorAction() = default;

    ~PrimaryGeneratorAction() override = default;
    /*
    void GeneratePrimaries(G4Event* event) override
    {
        // ------------------------------------------------------------
        // Cf-252 nucleus
        // Z = 98
        // A = 252
        // excitation energy = 0
        // ------------------------------------------------------------

        G4ParticleDefinition* cf252 =
            G4IonTable::GetIonTable()->GetIon(
                98,      // Z
                252,     // A
                0.0      // excitation energy
            );

        if (!cf252)
        {
            G4Exception(
                "PrimaryGeneratorAction::GeneratePrimaries",
                "Cf252NotFound",
                FatalException,
                "Cannot create Cf-252 ion."
            );
        }

        // ------------------------------------------------------------
        // Cf-252 source position
        // PE occupies approximately:
        // y = 25.1 ... 225.1 mm
        //
        // Source is placed just outside the outer PE surface.
        // ------------------------------------------------------------

        const G4ThreeVector sourcePosition(
            0.0,
            225.2 * mm,
            0.0
        );

        // ------------------------------------------------------------
        // Create primary vertex
        // ------------------------------------------------------------

        G4PrimaryVertex* vertex =
            new G4PrimaryVertex(
                sourcePosition,
                0.0 * ns
            );

        // ------------------------------------------------------------
        // Put one Cf-252 nucleus at rest into the vertex
        // ------------------------------------------------------------

        G4PrimaryParticle* primary =
            new G4PrimaryParticle(cf252);

        primary->SetKineticEnergy(0.0);

        vertex->SetPrimary(primary);

        event->AddPrimaryVertex(vertex);
    }
    */

    void GeneratePrimaries(G4Event* event) override
    {
        G4ParticleDefinition* cf252 =
            G4IonTable::GetIonTable()->GetIon(98, 252, 0.0);

        const G4ThreeVector center(0.0, 125.5 * mm, 0.0);
        const G4double R  = 5.0 * mm;
        const G4double HZ = 0.5 * mm;

        const G4int nIons = 100;   // подберите под статистику

        for (G4int i = 0; i < nIons; ++i)
        {
            const G4double r   = R * sqrt(G4UniformRand());
            const G4double phi = 2.0 * CLHEP::pi * G4UniformRand();
            const G4double z   = (2.0 * G4UniformRand() - 1.0) * HZ;

            const G4ThreeVector pos =
                center + G4ThreeVector(
                    r * cos(phi),
                    r * sin(phi),
                    z
                );
            G4PrimaryVertex* vertex =
                new G4PrimaryVertex(pos, 0.0 * ns);

            G4PrimaryParticle* primary =
                new G4PrimaryParticle(cf252);

            primary->SetKineticEnergy(0.0);

            vertex->SetPrimary(primary);

            event->AddPrimaryVertex(vertex);
        }
    }
};

class RunAction : public G4UserRunAction
{
public:

    RunAction()
        : fDetectedEvents(0),
          fDetectedReactions(0),
          fBornNeutrons(0)
    {}

    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run*) override
    {
        fDetectedEvents = 0;
        fDetectedReactions = 0;
        fBornNeutrons = 0;
    }

    void AddDetectedEvent()
    {
        ++fDetectedEvents;
    }

    void AddDetectedReaction()
    {
        ++fDetectedReactions;
    }

    void AddBornNeutron()
    {
        ++fBornNeutrons;
    }

    void EndOfRunAction(const G4Run* run) override
    {
        const G4int nEvents =
            run->GetNumberOfEvent();

        G4cout << G4endl;
        G4cout << "============================" << G4endl;
        G4cout << "        RUN RESULTS" << G4endl;
        G4cout << "============================" << G4endl;

        G4cout << "Events             : "
               << nEvents
               << G4endl;

        G4cout << "Born neutrons      : "
               << fBornNeutrons
               << G4endl;

        G4cout << "Detected events    : "
               << fDetectedEvents
               << G4endl;

        G4cout << "Detected reactions : "
               << fDetectedReactions
               << G4endl;

        if (fBornNeutrons > 0)
        {
            const G4double efficiency =
                static_cast<G4double>(fDetectedReactions)
                / fBornNeutrons;

            G4cout << "Efficiency         : "
                   << efficiency
                   << G4endl;

            G4cout << "Efficiency (%)     : "
                   << efficiency * 100.0
                   << " %"
                   << G4endl;
        }

        G4cout << "============================"
               << G4endl;
    }

private:

    G4int fDetectedEvents;
    G4int fDetectedReactions;
    G4int fBornNeutrons;
};


class EventAction : public G4UserEventAction
{
public:

    explicit EventAction(RunAction* runAction)
        : fRunAction(runAction),
          fReactionDetected(false)
    {}

    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event*) override
    {
        fReactionDetected = false;
    }

    void MarkReaction()
    {
        // Считаем каждую реакцию отдельно.
        fRunAction->AddDetectedReaction();

        // А событие по-прежнему считаем только один раз.
        if (!fReactionDetected)
        {
            fReactionDetected = true;
            fRunAction->AddDetectedEvent();
        }
    }

    G4bool IsReactionDetected() const
    {
        return fReactionDetected;
    }

    RunAction* GetRunAction() const
    {
        return fRunAction;
    }

private:

    RunAction* fRunAction;
    G4bool fReactionDetected;
};


class SteppingAction : public G4UserSteppingAction
{
public:

    explicit SteppingAction(EventAction* eventAction)
        : fEventAction(eventAction)
    {}

    ~SteppingAction() override = default;

    void UserSteppingAction(const G4Step* step) override
    {
        
        
        // ========================================================
        // 1. Нас интересуют только первичные нейтроны
        // ========================================================

        const G4Track* track =
            step->GetTrack();

        // ========================================================
        // Считаем нейтроны, рождённые источником Cf-252.
        //
        // Нейтрон учитывается один раз — на первом шаге.
        // Дополнительно требуем, чтобы он был вторичной
        // частицей, созданной процессом RadioactiveDecay.
        // ========================================================

        if (track->GetDefinition() ==
                G4Neutron::NeutronDefinition() &&
            track->GetCurrentStepNumber() == 1 &&
            track->GetParentID() != 0)
        {
            const G4VProcess* creator =
                track->GetCreatorProcess();

            if (creator &&
                creator->GetProcessName() == "RadioactiveDecay")
            {
                fEventAction->GetRunAction()->AddBornNeutron();
                /*
                G4cout
                    << "N born from Cf: E="
                    << track->GetKineticEnergy() / MeV
                    << " MeV"
                    << G4endl;
                */
            }
        }

        if (track->GetDefinition() !=
            G4Neutron::NeutronDefinition())
        {
            return;
        }
/*
        if (track->GetParentID() != 0)
        {
            return;
        }
*/
        // ========================================================
        // 2. Проверяем, что нейтрон находится в He3Gas
        // ========================================================

        const G4VPhysicalVolume* volume =
            step->GetPreStepPoint()
                ->GetTouchableHandle()
                ->GetVolume();

        if (!volume)
        {
            return;
        }

        const G4LogicalVolume* logicalVolume =
            volume->GetLogicalVolume();

        if (!logicalVolume)
        {
            return;
        }

        if (logicalVolume->GetName() != "He3Gas")
        {
            return;
        }

        // ========================================================
        // 3. Получаем вторичные частицы этого шага
        // ========================================================

        const vector<const G4Track*>* secondaries =
            step->GetSecondaryInCurrentStep();

        if (!secondaries)
        {
            return;
        }

        G4bool protonFound = false;
        G4bool tritonFound = false;

        // ========================================================
        // 4. Ищем p и 3H
        // ========================================================

        for (const G4Track* secondary : *secondaries)
        {
            if (secondary->GetDefinition() ==
                G4Proton::ProtonDefinition())
            {
                protonFound = true;
            }

            if (secondary->GetDefinition() ==
                G4Triton::TritonDefinition())
            {
                tritonFound = true;
            }
        }

        // ========================================================
        // 5. Нужная реакция найдена
        // ========================================================

        if (protonFound && tritonFound)
        {
            fEventAction->MarkReaction();

            G4cout
                << "3He(n,p)3H reaction!"
                << G4endl;
        }
    }

private:

    EventAction* fEventAction;
};

int main(int argc, char** argv)
{
    // =============================================================
    // RUN MANAGER
    // =============================================================

    G4RunManager* runManager = new G4RunManager();

    runManager->SetUserInitialization(
        new DetectorConstruction()
    );

    FTFP_BERT_HP* physicsList = new FTFP_BERT_HP();

    physicsList->RegisterPhysics(
        new G4RadioactiveDecayPhysics()
    );

    runManager->SetUserInitialization(physicsList);

    runManager->SetUserAction(
        new PrimaryGeneratorAction()
    );

    RunAction* runAction =
    new RunAction();

    EventAction* eventAction =
        new EventAction(runAction);


    runManager->SetUserAction(
        runAction
    );

    runManager->SetUserAction(
        eventAction
    );

    runManager->SetUserAction(
        new SteppingAction(eventAction)
    );

    

    // =============================================================
    // VISUALIZATION
    // =============================================================

    G4VisExecutive* visManager = new G4VisExecutive();

    visManager->Initialize();


    // =============================================================
    // INITIALIZE GEANT4
    // =============================================================

    G4HadronicParameters::Instance()
        ->SetTimeThresholdForRadioactiveDecay(
            1.0e+60 * year
        );

    runManager->Initialize();


    // =============================================================
    // USER INTERFACE
    // =============================================================

    G4UIExecutive* ui = new G4UIExecutive(argc, argv);

    G4UImanager* UImanager = G4UImanager::GetUIpointer();


    // =============================================================
    // OPEN OPENGL WINDOW
    // =============================================================

    UImanager->ApplyCommand("/vis/open OGL");

    UImanager->ApplyCommand("/vis/drawVolume");

    UImanager->ApplyCommand("/vis/viewer/set/style surface");

    UImanager->ApplyCommand("/vis/viewer/set/autoRefresh true");

    UImanager->ApplyCommand("/vis/scene/add/trajectories");

    //UImanager->ApplyCommand("/vis/scene/add/electricField");

    UImanager->ApplyCommand("/run/beamOn 10000");

    UImanager->ApplyCommand("/vis/viewer/refresh");

    
    // =============================================================
    // START GUI
    // =============================================================

    ui->SessionStart();


    // =============================================================
    // CLEANUP
    // =============================================================

    delete ui;
    delete visManager;
    delete runManager;

    return 0;
}
