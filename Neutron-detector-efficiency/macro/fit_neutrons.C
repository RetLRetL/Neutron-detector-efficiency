#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TF1.h>
#include <TROOT.h>
#include <TMath.h>
#include <TString.h>

#include <iostream>

// ============================================================================
// SETTINGS
// ============================================================================

const char* INPUT_FILE = "neutron_spectra.root";
const int N_BINS = 200;
const double E_MIN = 0.0;
const double E_MAX = 2500000.0; // eV
const double KT_INITIAL = 0.025; // eV

// Truncated Maxwell distribution with kT as the only free parameter.
TF1* MakeDetectedMaxwell(
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
        "(TMath::Erf(TMath::Sqrt(%g/[0])) - "
        "(2.0/TMath::Sqrt(TMath::Pi())) * "
        "TMath::Sqrt(%g/[0]) * TMath::Exp(-%g/[0]))",
        nEntries, binWidth, eMax, eMax, eMax
    );

    TF1* fit = new TF1(name, formula.Data(), eMin, eMax);
    fit->SetParameter(0, kTInitial);
    fit->SetParName(0, "kT");
    fit->SetParLimits(0, 1e-6, 10.0);
    fit->SetLineWidth(2);
    return fit;
}

void fit_neutrons()
{
    TFile* file = TFile::Open(INPUT_FILE, "READ");
    if (!file || file->IsZombie())
    {
        std::cerr << "ERROR: cannot open " << INPUT_FILE << std::endl;
        return;
    }

    TTree* tree = nullptr;
    file->GetObject("neutrons", tree);
    if (!tree || !tree->GetBranch("detected_energy"))
    {
        std::cerr << "ERROR: tree 'neutrons' or branch 'detected_energy' not found"
                  << std::endl;
        file->Close();
        delete file;
        return;
    }

    TCanvas* canvas = new TCanvas(
        "cDetectedNeutrons",
        "Detected neutron energy spectrum",
        900,
        650
    );

    TH1D* histogram = new TH1D(
        "hDetectedNeutrons",
        "Neutrons that caused ^{3}He(n,p)^{3}H reactions;"
        "Kinetic energy [eV];Counts",
        N_BINS,
        E_MIN,
        E_MAX
    );

    tree->Draw("detected_energy>>hDetectedNeutrons", "", "goff");

    const double entriesInRange = histogram->Integral();
    if (entriesInRange <= 0.0)
    {
        std::cerr << "ERROR: no detected-neutron energies in the fit range"
                  << std::endl;
        file->Close();
        delete file;
        return;
    }

    TF1* fit = MakeDetectedMaxwell(
        "fitDetectedNeutrons",
        E_MIN,
        E_MAX,
        entriesInRange,
        histogram->GetBinWidth(1),
        KT_INITIAL
    );

    histogram->Fit(fit, "R0");
    histogram->Draw("E");
    fit->Draw("same");
    canvas->Modified();
    canvas->Update();

    const double chi2NDF = fit->GetNDF() > 0
        ? fit->GetChisquare() / fit->GetNDF()
        : 0.0;

    std::cout << "Detected-neutron entries in fit range = "
              << entriesInRange << std::endl;
    std::cout << "kT = " << fit->GetParameter(0)
              << " +/- " << fit->GetParError(0) << " eV" << std::endl;
    std::cout << "chi2/NDF = " << chi2NDF << std::endl;

    file->Close();
    delete file;
}
