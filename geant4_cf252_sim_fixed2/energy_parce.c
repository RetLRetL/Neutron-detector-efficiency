#include <TFile.h>
#include <TTree.h>
#include <TSystem.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ============================================================
// SETTINGS
// ============================================================

const int lenght   = 1;
const int N_EVENTS = 2000;

const char* OUTPUT_FILE = "neutron_spectra_3.root"; /// _1 if for lenth = 5 cm, _2 fo 15 cm

// ============================================================
// PARSER
// ============================================================

void energy_parce()
{
    // --------------------------------------------------------
    // Run ./main and get its stdout directly.
    // No temporary files.
    // --------------------------------------------------------

    TString command = Form(
        "./main %d %d 2>&1",
        lenght,
        N_EVENTS
    );

    std::cout << "Running: "
              << command
              << std::endl;

    TString output = gSystem->GetFromPipe(command);

    // --------------------------------------------------------
    // Parse output
    // --------------------------------------------------------

    std::vector<double> enteredEnergy;
    std::vector<double> detectedEnergy;

    std::istringstream stream(output.Data());
    std::string line;

    while (std::getline(stream, line))
    {
        double energy;

        if (sscanf(
                line.c_str(),
                "entered He: %lf",
                &energy
            ) == 1)
        {
            enteredEnergy.push_back(energy);
            continue;
        }

        if (sscanf(
                line.c_str(),
                "detected: %lf",
                &energy
            ) == 1)
        {
            detectedEnergy.push_back(energy);
            continue;
        }
    }

    // --------------------------------------------------------
    // Write ROOT tree
    // --------------------------------------------------------

    TFile file(
        OUTPUT_FILE,
        "RECREATE"
    );

    TTree tree(
        "neutrons",
        "Neutron energy spectra"
    );

    tree.Branch(
        "entered_energy",
        &enteredEnergy
    );

    tree.Branch(
        "detected_energy",
        &detectedEnergy
    );

    tree.Fill();

    tree.Write();

    file.Close();

    // --------------------------------------------------------
    // Output
    // --------------------------------------------------------

    std::cout << "\n";
    std::cout << "============================\n";
    std::cout << "lenght           : " << lenght << "\n";
    std::cout << "Events per run   : " << N_EVENTS << "\n";
    std::cout << "Entered He3      : "
              << enteredEnergy.size() << "\n";
    std::cout << "Detected         : "
              << detectedEnergy.size() << "\n";
    std::cout << "Output           : "
              << OUTPUT_FILE << "\n";
    std::cout << "============================\n";
}