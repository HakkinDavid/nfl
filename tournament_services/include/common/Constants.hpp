#ifndef TOURNAMENTS_CONSTANTS_HPP
#define TOURNAMENTS_CONSTANTS_HPP

#include <regex>

// La palabra clave 'inline' resuelve el problema de la redefinición.
inline const std::regex ID_VALUE("[A-Za-z0-9\\-]+");

#endif //TOURNAMENTS_CONSTANTS_HPP