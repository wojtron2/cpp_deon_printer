#ifndef MONEY_UTILS_H
#define MONEY_UTILS_H

#include <cstdint>	// dolacza definicje std::int64_t – uzywamy go jako typ liczbowy dla kwot w groszach (64 bity, bezpieczenstwo przed przepelnieniem)
#include <string>	// dolacza std::string – potrzebny do przekazywania kwoty w postaci tekstu (np. "12.34")

// parsuje kwote w formacie "zlotowki.grosze" (zawsze z kropka) do groszy
// np. "12.34" -> 1234. Rzuca std::runtime_error przy blednym formacie lub ujemnej kwocie
std::int64_t parseAmountToCents(const std::string& text);		// przyjmuje kwote jako tekst (text), zwraca liczbe groszy jako std::int64_t


// formatuje grosze do "zlotowki.grosze" z dwoma cyframi groszy
// np. 1234 -> "12.34"
// rzuca std::runtime_error, jesli cents < 0
std::string formatCentsToAmount(std::int64_t cents);			// przyjmuje kwote w groszach (cents), zwraca tekst "zl.grosze"

#endif // MONEY_UTILS_H
