#include "TFile.h"
#include "TTree.h"
#include "TSystem.h"
#include "TString.h"
#include "TObjArray.h"
#include "TObjString.h"

#include <iostream>

// ============================================================================
// SETTINGS
// ============================================================================

const char* EXECUTABLE = "./main";
const char* OUTPUT_FILE = "simulation_results.root";

const int START_THICKNESS_CM = 1;
const int END_THICKNESS_CM   = 45;
const int STEP_CM            = 1;

const int EVENTS_PER_RUN = 1000;

// ============================================================================
// Extract value from Geant4 output
//
// Example:
//
//   Born neutrons      : 843
//   Detected events    : 17
//
// ============================================================================

bool ExtractValue(
    const TString& output,
    const char* key,
    Long64_t& value
)
{
    TObjArray* lines = output.Tokenize("\n");

    bool found = false;

    for (int i = 0; i < lines->GetEntries(); ++i)
    {
        TObjString* obj =
            dynamic_cast<TObjString*>(lines->At(i));

        if (!obj)
            continue;

        TString line = obj->GetString();

        if (!line.Contains(key))
            continue;

        const Ssiz_t colon = line.Index(":");

        if (colon == kNPOS)
            continue;

        TString valueString =
            line(colon + 1,
                 line.Length() - colon - 1);

        valueString =
            valueString.Strip(TString::kBoth);

        value = valueString.Atoll();

        found = true;
        break;
    }

    delete lines;

    return found;
}

// ============================================================================
// MAIN
// ============================================================================

void run_scan()
{
    // ------------------------------------------------------------------------
    // Create ROOT file and tree
    // ------------------------------------------------------------------------

    TFile* file =
        TFile::Open(
            OUTPUT_FILE,
            "RECREATE"
        );

    if (!file || file->IsZombie())
    {
        std::cerr
            << "ERROR: cannot create ROOT file "
            << OUTPUT_FILE
            << std::endl;

        return;
    }

    TTree* tree =
        new TTree(
            "results",
            "Cf-252 / He-3 simulation results"
        );

    // ------------------------------------------------------------------------
    // Tree variables
    // ------------------------------------------------------------------------

    Int_t peThicknessCm = 0;

    Long64_t bornNeutrons = 0;
    Long64_t detectedEvents = 0;

    // ------------------------------------------------------------------------
    // Branches
    // ------------------------------------------------------------------------

    tree->Branch(
        "PEThickness",
        &peThicknessCm,
        "PEThickness/I"
    );

    tree->Branch(
        "BornNeutrons",
        &bornNeutrons,
        "BornNeutrons/L"
    );

    tree->Branch(
        "DetectedEvents",
        &detectedEvents,
        "DetectedEvents/L"
    );

    // ------------------------------------------------------------------------
    // Run scan
    // ------------------------------------------------------------------------

    for (
        peThicknessCm = START_THICKNESS_CM;
        peThicknessCm <= END_THICKNESS_CM;
        peThicknessCm += STEP_CM
    )
    {
        std::cout
            << std::endl
            << "========================================"
            << std::endl
            << "PE thickness = "
            << peThicknessCm
            << " cm"
            << std::endl
            << "Events = "
            << EVENTS_PER_RUN
            << std::endl
            << "========================================"
            << std::endl;

        // Reset values before each run.
        bornNeutrons = 0;
        detectedEvents = 0;

        // --------------------------------------------------------------------
        // Run executable.
        //
        // The program is expected to be called as:
        //
        //     ./main <PE thickness in cm> <number of events>
        //
        // 2>&1 also redirects stderr, so that possible Geant4 messages
        // are included in the captured output.
        // --------------------------------------------------------------------

        TString command =
            Form(
                "%s %d %d 2>&1",
                EXECUTABLE,
                peThicknessCm,
                EVENTS_PER_RUN
            );

        std::cout
            << "Command: "
            << command
            << std::endl;

        TString output =
            gSystem->GetFromPipe(
                command
            );

        // --------------------------------------------------------------------
        // Parse Born neutrons
        // --------------------------------------------------------------------

        const bool gotBorn =
            ExtractValue(
                output,
                "Born neutrons",
                bornNeutrons
            );

        // --------------------------------------------------------------------
        // Parse Detected events
        // --------------------------------------------------------------------

        const bool gotDetected =
            ExtractValue(
                output,
                "Detected events",
                detectedEvents
            );

        // --------------------------------------------------------------------
        // Check parsing
        // --------------------------------------------------------------------

        if (!gotBorn || !gotDetected)
        {
            std::cerr
                << std::endl
                << "ERROR: failed to parse Geant4 output"
                << std::endl
                << "PE thickness = "
                << peThicknessCm
                << " cm"
                << std::endl
                << std::endl;

            std::cerr
                << "Program output:"
                << std::endl
                << output
                << std::endl;

            // Do not write an invalid result.
            continue;
        }

        // --------------------------------------------------------------------
        // Print parsed values
        // --------------------------------------------------------------------

        std::cout
            << "Born neutrons   = "
            << bornNeutrons
            << std::endl;

        std::cout
            << "Detected events = "
            << detectedEvents
            << std::endl;

        // --------------------------------------------------------------------
        // Add result to tree
        // --------------------------------------------------------------------

        tree->Fill();
    }

    // ------------------------------------------------------------------------
    // Save tree
    // ------------------------------------------------------------------------

    file->cd();

    tree->Write(
        "results",
        TObject::kOverwrite
    );

    file->Close();

    delete file;

    std::cout
        << std::endl
        << "========================================"
        << std::endl
        << "Scan finished."
        << std::endl
        << "ROOT file: "
        << OUTPUT_FILE
        << std::endl
        << "========================================"
        << std::endl;
}