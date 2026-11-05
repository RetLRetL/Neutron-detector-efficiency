// fit_neutrons.C

#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TF1.h>
#include <TROOT.h>
#include <TMath.h>

#include <iostream>

// ============================================================
// SETTINGS
// ============================================================

const char* INPUT_FILE = "neutron_spectra_3.root";

const int N_BINS = 200;

const double E_MIN = 0.0;
const double E_MAX = 2500000;

// Initial guess for kT [eV]
const double KT_INITIAL = 0.025;

// ============================================================
// Truncated Maxwell distribution
//
// Only free parameter:
//     [0] = kT
//
// The normalization is fixed by the number of entries
// inside the fitting interval.
// ============================================================

TF1* MakeMaxwell(
    const char* name,
    double eMin,
    double eMax,
    double nEntries,
    double binWidth,
    double kTInitial)
{
    TString formula = Form(
        "%g * %g * "
        "(2.0/TMath::Sqrt(TMath::Pi())) * "
        "TMath::Sqrt(x) / TMath::Power([0],1.5) * "
        "TMath::Exp(-x/[0]) / "
        "("
        "TMath::Erf(TMath::Sqrt(%g/[0]))"
        " - "
        "(2.0/TMath::Sqrt(TMath::Pi())) * "
        "TMath::Sqrt(%g/[0]) * "
        "TMath::Exp(-%g/[0])"
        ")",
        nEntries,
        binWidth,
        eMax,
        eMax,
        eMax
    );

    TF1* f = new TF1(
        name,
        formula.Data(),
        eMin,
        eMax
    );

    f->SetParameter(0, kTInitial);

    f->SetParName(0, "kT");

    f->SetParLimits(
        0,
        1e-6,
        10.0
    );

    f->SetLineWidth(2);

    return f;
}

// ============================================================
// MAIN
// ============================================================

void fit_neutrons()
{
    // --------------------------------------------------------
    // Remove old canvases
    // --------------------------------------------------------

    while (gROOT->GetListOfCanvases()->GetSize() > 0)
    {
        TCanvas* c =
            (TCanvas*)gROOT->GetListOfCanvases()->At(0);

        delete c;
    }

    // --------------------------------------------------------
    // Open ROOT file
    // --------------------------------------------------------

    TFile* file =
        TFile::Open(INPUT_FILE, "READ");

    if (!file || file->IsZombie())
    {
        std::cerr
            << "ERROR: cannot open "
            << INPUT_FILE
            << std::endl;
        return;
    }

    TTree* tree = nullptr;

    file->GetObject(
        "neutrons",
        tree
    );

    if (!tree)
    {
        std::cerr
            << "ERROR: cannot find tree 'neutrons'"
            << std::endl;
        return;
    }

    // ========================================================
    // ENTERED HE3
    // ========================================================

    TCanvas* cEntered =
        new TCanvas(
            "cEntered",
            "Entered He3",
            800,
            600
        );

    TH1D* hEntered =
        new TH1D(
            "hEntered",
            "Neutrons entering He3;"
            "Kinetic energy [eV];"
            "Counts",
            N_BINS,
            E_MIN,
            E_MAX
        );

    tree->Draw(
        "entered_energy>>hEntered",
        "",
        "goff"
    );

    // Number of events INSIDE fitting range.
    const double enteredN =
        hEntered->Integral();

    const double enteredBinWidth =
        hEntered->GetBinWidth(1);

    TF1* fitEntered =
        MakeMaxwell(
            "fitEntered",
            E_MIN,
            E_MAX,
            enteredN,
            enteredBinWidth,
            KT_INITIAL
        );

    hEntered->Fit(
        fitEntered,
        "R0"
    );

    cEntered->cd();

    hEntered->Draw("E");

    fitEntered->Draw("same");

    cEntered->Modified();
    cEntered->Update();

    // ========================================================
    // DETECTED
    // ========================================================

    TCanvas* cDetected =
        new TCanvas(
            "cDetected",
            "Detected",
            800,
            600
        );

    TH1D* hDetected =
        new TH1D(
            "hDetected",
            "Detected neutrons;"
            "Kinetic energy [eV];"
            "Counts",
            N_BINS,
            E_MIN,
            E_MAX
        );

    tree->Draw(
        "detected_energy>>hDetected",
        "",
        "goff"
    );

    const double detectedN =
        hDetected->Integral();

    const double detectedBinWidth =
        hDetected->GetBinWidth(1);

    TF1* fitDetected =
        MakeMaxwell(
            "fitDetected",
            E_MIN,
            E_MAX,
            detectedN,
            detectedBinWidth,
            KT_INITIAL
        );

    hDetected->Fit(
        fitDetected,
        "R0"
    );

    cDetected->cd();

    hDetected->Draw("E");

    fitDetected->Draw("same");

    cDetected->Modified();
    cDetected->Update();

    // ========================================================
    // CHI2 / NDF
    // ========================================================

    const double enteredChi2NDF =
        fitEntered->GetNDF() > 0
        ? fitEntered->GetChisquare()
          / fitEntered->GetNDF()
        : 0.0;

    const double detectedChi2NDF =
        fitDetected->GetNDF() > 0
        ? fitDetected->GetChisquare()
          / fitDetected->GetNDF()
        : 0.0;

    // ========================================================
    // PRINT RESULTS
    // ========================================================

    std::cout << std::endl;
    std::cout
        << "===================================="
        << std::endl;
    std::cout
        << "ENTERED HE3"
        << std::endl;
    std::cout
        << "===================================="
        << std::endl;

    std::cout
        << "Entries in fit range = "
        << enteredN
        << std::endl;

    std::cout
        << "kT = "
        << fitEntered->GetParameter(0)
        << " +/- "
        << fitEntered->GetParError(0)
        << " eV"
        << std::endl;

    std::cout
        << "chi2/NDF = "
        << enteredChi2NDF
        << std::endl;

    std::cout << std::endl;

    std::cout
        << "===================================="
        << std::endl;
    std::cout
        << "DETECTED"
        << std::endl;
    std::cout
        << "===================================="
        << std::endl;

    std::cout
        << "Entries in fit range = "
        << detectedN
        << std::endl;

    std::cout
        << "kT = "
        << fitDetected->GetParameter(0)
        << " +/- "
        << fitDetected->GetParError(0)
        << " eV"
        << std::endl;

    std::cout
        << "chi2/NDF = "
        << detectedChi2NDF
        << std::endl;
}