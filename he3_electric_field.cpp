#include "he3_electric_field.h"
#include "settings.h"

#include "G4SystemOfUnits.hh"

#include <cmath>

void He3ElectricField::GetFieldValue(
    const G4double point[4],
    G4double* field
) const
{
    const G4double y = point[1];
    const G4double z = point[2];

    const G4double r = std::sqrt(y * y + z * z);

    // Outside the gas cylinder there is no field in this model.
    if (r <= Settings::FIELD_INNER_RADIUS ||
        r >= Settings::FIELD_OUTER_RADIUS)
    {
        for (int i = 0; i < 6; ++i)
            field[i] = 0.0;

        return;
    }

    const G4double E =
        Settings::FIELD_VOLTAGE /
        (r * std::log(
            Settings::FIELD_OUTER_RADIUS /
            Settings::FIELD_INNER_RADIUS));

    // Magnetic field B = 0.
    field[0] = 0.0;
    field[1] = 0.0;
    field[2] = 0.0;

    // Electric field E.
    field[3] = 0.0;
    field[4] = E * y / r;
    field[5] = E * z / r;
}
