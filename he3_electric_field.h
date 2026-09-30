#pragma once

#include "G4ElectricField.hh"

class He3ElectricField : public G4ElectricField
{
public:
    He3ElectricField() = default;
    ~He3ElectricField() override = default;

    void GetFieldValue(
        const G4double point[4],
        G4double* field
    ) const override;
};
