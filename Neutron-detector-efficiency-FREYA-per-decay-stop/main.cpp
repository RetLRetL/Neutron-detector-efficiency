#include "settings.h"
#include "detector.h"
#include "primary_generator.h"
#include "run_action.h"
#include "event_action.h"
#include "stepping_action.h"
#include "tracking_action.h"
#include "stacking_action.h"

#include "G4RunManager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4Exception.hh"
#include "G4HadronicParameters.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

#include "FTFP_BERT_HP.hh"
#include "G4RadioactiveDecayPhysics.hh"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
    void LoadCf252SpontaneousFissionData()
    {
        // This user-provided decay table must contain the Cf-252 scheme with
        // only the spontaneous-fission branch (100% branching ratio).
        // The default file is data/z98.a252_modify.txt; an alternate absolute
        // or relative path can be supplied through the environment variable.
        std::filesystem::path decayFile = Settings::CF252_DECAY_DATA_FILE;

        if (const char* configured = std::getenv("G4_CF252_DECAY_DATA_FILE"))
        {
            if (*configured != '\0')
                decayFile = configured;
        }

        if (!std::filesystem::exists(decayFile))
        {
            const std::string message =
                "Cf-252 user decay data file does not exist: " +
                std::filesystem::absolute(decayFile).string() +
                ". Put z98.a252_modify.txt into data/ or set "
                "G4_CF252_DECAY_DATA_FILE.";

            G4Exception(
                "main::LoadCf252SpontaneousFissionData",
                "CF252DATA001",
                FatalException,
                message.c_str()
            );
            return;
        }

        const std::filesystem::path absolutePath =
            std::filesystem::absolute(decayFile);

        const G4String command =
            "/process/had/rdm/setRadioactiveDecayFile 98 252 " +
            absolutePath.string();

        const G4int status = G4UImanager::GetUIpointer()->ApplyCommand(command);
        if (status != 0)
        {
            const std::string message =
                "Geant4 could not register the Cf-252 user decay data file: " +
                absolutePath.string();

            G4Exception(
                "main::LoadCf252SpontaneousFissionData",
                "CF252DATA002",
                FatalException,
                message.c_str()
            );
            return;
        }

        G4cout << "Cf-252 decay data : " << absolutePath.string() << G4endl;
        G4cout << "Cf-252 decay mode : user-defined spontaneous fission only"
               << G4endl;
    }

    void PrintUsage(const char* programName)
    {
        std::cout
            << "Usage:\n"
            << "  " << programName << "\n"
            << "  " << programName
            << " <PE_thickness_cm> <number_of_events>\n\n"
            << "Defaults:\n"
            << "  PE thickness = "
            << Settings::DEFAULT_PE_THICKNESS / cm
            << " cm\n"
            << "  Events       = "
            << Settings::DEFAULT_N_EVENTS
            << "\n";
    }

    SimulationConfig ParseArguments(int argc, char** argv)
    {
        SimulationConfig config;

        if (argc > 3)
        {
            PrintUsage(argv[0]);
            throw std::runtime_error("Too many command-line arguments.");
        }

        try
        {
            if (argc >= 2)
            {
                const double peCm = std::stod(argv[1]);
                if (!(peCm > 0.0))
                    throw std::runtime_error(
                        "PE thickness must be positive."
                    );

                config.peThickness = peCm * cm;
            }

            if (argc >= 3)
            {
                std::size_t parsedChars = 0;
                const long long events =
                    std::stoll(argv[2], &parsedChars);

                if (parsedChars != std::string(argv[2]).size())
                    throw std::runtime_error(
                        "Number of events must be an integer."
                    );

                if (events <= 0 ||
                    events > std::numeric_limits<G4int>::max())
                {
                    throw std::runtime_error(
                        "Number of events must be a positive G4int."
                    );
                }

                config.nEvents = static_cast<G4int>(events);
            }
        }
        catch (const std::exception& ex)
        {
            PrintUsage(argv[0]);
            throw std::runtime_error(
                std::string("Invalid command-line arguments: ") + ex.what()
            );
        }

        return config;
    }
}

int main(int argc, char** argv)
{
    SimulationConfig config;

    try
    {
        config = ParseArguments(argc, argv);
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    G4cout << "============================" << G4endl;
    G4cout << "     Cf-252 / He-3 RUN" << G4endl;
    G4cout << "============================" << G4endl;
    G4cout << "PE thickness : "
           << config.peThickness / cm
           << " cm"
           << G4endl;
    G4cout << "Events       : "
           << config.nEvents
           << G4endl;
    G4cout << "Sample center Y: "
           << config.peThickness / cm
           << " cm"
           << G4endl;
    G4cout << "Generation mode: "
           << (Settings::GENERATION_MODE == Settings::GenerationMode::GEANT
                   ? "GEANT (RadioactiveDecay)"
                   : "FREYA (spontaneous fission)")
           << G4endl;

    G4cout << (Settings::GENERATION_MODE == Settings::GenerationMode::GEANT
                   ? "Cf-252 nuclei/event: "
                   : "Cf-252 fissions/event: ")
           << Settings::N_IONS
           << G4endl;
    G4cout << G4endl;

    // ================================================================
    // Run manager and physics
    // ================================================================

    G4RunManager* runManager = new G4RunManager();

    runManager->SetUserInitialization(
        new DetectorConstruction(config)
    );

    FTFP_BERT_HP* physicsList = new FTFP_BERT_HP();
    physicsList->RegisterPhysics(new G4RadioactiveDecayPhysics());
    runManager->SetUserInitialization(physicsList);

    // ================================================================
    // User actions
    // ================================================================

    runManager->SetUserAction(new PrimaryGeneratorAction(config));

    RunAction* runAction = new RunAction();
    EventAction* eventAction = new  EventAction(runAction);

    runManager->SetUserAction(runAction);
    runManager->SetUserAction(eventAction);
    runManager->SetUserAction(new SteppingAction(eventAction));
    runManager->SetUserAction(new TrackingAction(eventAction));
    runManager->SetUserAction(new StackingAction(eventAction));

    // ================================================================
    // Optional visualization
    // ================================================================

    G4VisExecutive* visManager = nullptr;
    G4UIExecutive* ui = nullptr;

    if (Settings::ENABLE_VISUALIZATION)
    {
        visManager = new G4VisExecutive();
        visManager->Initialize();
    }

    // The following parameter is part of the original model: allow
    // radioactive decays on the time scale relevant to this run.
    G4HadronicParameters::Instance()
        ->SetTimeThresholdForRadioactiveDecay(1.0e+60 * year);

    // ================================================================
    // Initialization -- deliberately outside timing region
    // ================================================================

    runManager->Initialize();

    // GEANT mode must use the custom Cf-252 decay table so the denominator
    // counts spontaneous fissions, not the natural mixture of alpha and SF
    // decays. FREYA already generates one spontaneous fission per source.
    if (Settings::GENERATION_MODE == Settings::GenerationMode::GEANT)
        LoadCf252SpontaneousFissionData();

    if (Settings::ENABLE_VISUALIZATION)
    {
        ui = new G4UIExecutive(argc, argv);
        G4UImanager* uiManager = G4UImanager::GetUIpointer();

        uiManager->ApplyCommand("/vis/open OGL");
        uiManager->ApplyCommand("/vis/drawVolume");
        uiManager->ApplyCommand("/vis/viewer/set/style surface");
        uiManager->ApplyCommand("/vis/viewer/set/autoRefresh true");
        uiManager->ApplyCommand("/vis/scene/add/trajectories");
        uiManager->ApplyCommand("/vis/geometry/set/visibility World 0 false");
        uiManager->ApplyCommand("/vis/geometry/set/forceWireframe Polyethylene 0 true"); 
    }

    // ================================================================
    // Timed analysis region: only BeamOn is measured
    // ================================================================

    if (Settings::ENABLE_TIMING)
    {
        const auto tStart = std::chrono::steady_clock::now();

        runManager->BeamOn(config.nEvents);

        const auto tEnd = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = tEnd - tStart;

        const double totalSeconds = elapsed.count();
        const double averageMilliseconds =
            totalSeconds * 1000.0 /
            static_cast<double>(config.nEvents);

        G4cout << G4endl;
        G4cout << "============================" << G4endl;
        G4cout << "          TIMING" << G4endl;
        G4cout << "============================" << G4endl;
        G4cout << std::fixed << std::setprecision(6);
        G4cout << "Total analysis time : "
               << totalSeconds
               << " s"
               << G4endl;
        G4cout << "Average event time  : "
               << averageMilliseconds
               << " ms/event"
               << G4endl;
        G4cout << "============================" << G4endl;
    }
    else
    {
        runManager->BeamOn(config.nEvents);
    }

    // ================================================================
    // GUI stays open only when visualization is enabled
    // ================================================================

    if (Settings::ENABLE_VISUALIZATION)
    {
        G4UImanager* uiManager = G4UImanager::GetUIpointer();
        uiManager->ApplyCommand("/vis/viewer/refresh");
        ui->SessionStart();
    }

    delete ui;
    delete visManager;
    delete runManager;

    return 0;
}
