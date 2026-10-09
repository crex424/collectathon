#include "functions.hpp"
#include "Level.hpp"

void level_plus(Level &level) {
    // This casts the enum first into an integer and increases it by one, 
    // then casts it back into an enum
    level = static_cast<Level>(static_cast<int>(level) + 1);
}