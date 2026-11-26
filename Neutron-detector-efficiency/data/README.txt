Place the custom Geant4 radioactive-decay data file here:

    z98.a252_modify.txt

This file is not bundled because it is a user-supplied Geant4 decay table
obtained from the Geant4 Forum example and must be verified for the local
Geant4 installation. By default, main.cpp loads:

    data/z98.a252_modify.txt

You may override the path without editing code:

    G4_CF252_DECAY_DATA_FILE=/absolute/path/z98.a252_modify.txt ./main 1 10

The table must force Cf-252 to the spontaneous-fission branch only (100%).
Without that condition, the GEANT-mode denominator is not a count of only
Cf-252 spontaneous fissions.
