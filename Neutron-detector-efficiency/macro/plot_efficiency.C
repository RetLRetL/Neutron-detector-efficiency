#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TAxis.h"

#include <iostream>
#include <cmath>

// ============================================================================
// SETTINGS
// ============================================================================

const char* INPUT_FILE = "simulation_results_geant.root";

const char* TREE_NAME = "results";

const char* HISTOGRAM_NAME = "Efficiency";

const int START_THICKNESS_CM = 1;
const int END_THICKNESS_CM   = 45;

// ============================================================================
// MAIN
// ============================================================================

void plot_efficiency()
{
    // ------------------------------------------------------------------------
    // Open ROOT file
    // ------------------------------------------------------------------------

    TFile* file =
        TFile::Open(
            INPUT_FILE,
            "UPDATE"
        );

    if (!file || file->IsZombie())
    {
        std::cerr
            << "ERROR: cannot open ROOT file "
            << INPUT_FILE
            << std::endl;

        return;
    }

    // ------------------------------------------------------------------------
    // Get tree
    // ------------------------------------------------------------------------

    TTree* tree = nullptr;

    file->GetObject(
        TREE_NAME,
        tree
    );

    if (!tree)
    {
        std::cerr
            << "ERROR: tree '"
            << TREE_NAME
            << "' not found."
            << std::endl;

        file->Close();
        delete file;

        return;
    }

    // ------------------------------------------------------------------------
    // Tree variables
    // ------------------------------------------------------------------------

    Int_t peThicknessCm = 0;

    Long64_t totalEvents = 0;
    Long64_t totalDecays = 0;
    Long64_t detectedDecays = 0;
    Long64_t bornNeutrons = 0;
    Long64_t detectedEvents = 0;

    // ------------------------------------------------------------------------
    // Connect branches
    // ------------------------------------------------------------------------

    tree->SetBranchAddress(
        "PEThickness",
        &peThicknessCm
    );

    tree->SetBranchAddress(
        "TotalEvents",
        &totalEvents
    );

    if (!tree->GetBranch("TotalDecays") ||
        !tree->GetBranch("DetectedDecays"))
    {
        std::cerr
            << "ERROR: ROOT tree does not contain TotalDecays/DetectedDecays. "
            << "Run run_scan.C again to regenerate simulation_results.root."
            << std::endl;
        file->Close();
        delete file;
        return;
    }

    tree->SetBranchAddress(
        "TotalDecays",
        &totalDecays
    );

    tree->SetBranchAddress(
        "DetectedDecays",
        &detectedDecays
    );

    tree->SetBranchAddress(
        "BornNeutrons",
        &bornNeutrons
    );

    tree->SetBranchAddress(
        "DetectedEvents",
        &detectedEvents
    );

    // ------------------------------------------------------------------------
    // Create histogram
    //
    // 45 bins:
    //
    //   [0.5, 1.5)
    //   [1.5, 2.5)
    //   ...
    //   [44.5, 45.5)
    //
    // Therefore bin centers are:
    //
    //   1, 2, ..., 45 cm
    // ------------------------------------------------------------------------

    const int nBins =
        END_THICKNESS_CM -
        START_THICKNESS_CM +
        1;

    const double xMin =
        START_THICKNESS_CM - 0.5;

    const double xMax =
        END_THICKNESS_CM + 0.5;

    // Remove an old version of the histogram, if present.
    file->Delete(
        Form(
            "%s;*",
            HISTOGRAM_NAME
        )
    );

    TH1D* histogram =
        new TH1D(
            HISTOGRAM_NAME,
            "Detection efficiency;"
            "PE thickness [cm];"
            "Efficiency",
            nBins,
            xMin,
            xMax
        );

    histogram->SetStats(false);

    // ------------------------------------------------------------------------
    // Read tree
    // ------------------------------------------------------------------------

    const Long64_t nEntries =
        tree->GetEntries();

    for (Long64_t i = 0;
         i < nEntries;
         ++i)
    {
        tree->GetEntry(i);

        // ------------------------------------------------------------
        // Basic validity check
        // ------------------------------------------------------------

        if (totalDecays <= 0)
        {
            std::cerr
                << "WARNING: TotalDecays <= 0 for "
                << peThicknessCm
                << " cm."
                << std::endl;

            continue;
        }

        // ------------------------------------------------------------
        // Efficiency
        //
        // p = DetectedDecays / TotalDecays
        // ------------------------------------------------------------

        const double p =
            static_cast<double>(detectedDecays) /
            static_cast<double>(totalDecays);

        if (p < 0.0 || p > 1.0)
        {
            std::cerr
                << "ERROR: DetectedDecays/TotalDecays is outside [0, 1] for "
                << peThicknessCm << " cm: " << p << std::endl;
            continue;
        }

        // ------------------------------------------------------------
        // Binomial statistical uncertainty
        //
        // sigma = sqrt(p(1-p)/N)
        // ------------------------------------------------------------

        const double error =
            std::sqrt(
                p * (1.0 - p) /
                static_cast<double>(totalDecays)
            );

        // ------------------------------------------------------------
        // Find histogram bin
        // ------------------------------------------------------------

        const int bin =
            histogram->FindBin(
                peThicknessCm
            );

        histogram->SetBinContent(
            bin,
            p
        );

        histogram->SetBinError(
            bin,
            error
        );
    }

    // ------------------------------------------------------------------------
    // Save histogram in the same ROOT file
    // ------------------------------------------------------------------------

    file->cd();

    histogram->Write(
        HISTOGRAM_NAME,
        TObject::kOverwrite
    );

    //file->Write();

    file->Close();

    delete file;

    std::cout
        << std::endl
        << "Efficiency histogram saved to "
        << INPUT_FILE
        << std::endl;
}