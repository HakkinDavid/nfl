#ifndef TOURNAMENTS_CONSTANTS_HPP
#define TOURNAMENTS_CONSTANTS_HPP

#include <regex>

// La palabra clave 'inline' resuelve el problema de la redefinición.
inline const std::regex ID_VALUE("[A-Za-z0-9\\-]+");
inline const std::regex UUID_REGEX(
    "[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}"
);

#endif //TOURNAMENTS_CONSTANTS_HPP