#ifndef NOVITUS_PROTOCOL_H
#define NOVITUS_PROTOCOL_H

#include <cstdint>	// dla std::int64_t
#include <string>	// dla std::string



namespace novitus {

// buduje pelna ramke: ESC 'P' + body + CRC(2 hexy) + ESC '\'
std::string buildFrame(const std::string& body);						// przyjmuje cialo ramki (body), zwraca gotowa ramke z naglowkiem, CRC i znakiem konca

// 3.4.1 Poczatek transakcji, tryb ON-LINE (ilosc pozycji = 0)
std::string makeBeginTransactionOnline();								// tworzy body + ramke dla polecenia "poczatek transakcji ON-LINE"

// prosta pozycja paragonu: sprzedaz 1 sztuki, grupa PTU, cena, wartosc brutto
std::string makeSaleLineSimple(const std::string& itemName,				// nazwa towaru
                               char vatGroup,							// grupa PTU (A..G)
                               std::int64_t unitPriceCents,				// cena jednostkowa brutto w groszach
                               std::int64_t lineTotalCents);			// wartosc pozycji (brutto) w groszach

// 3.4.15 Rabat/narzut do paragonu ($Y) - rabat kwotowy do paragonu od podsumy
std::string makeReceiptDiscountY(std::int64_t subtotalCents,			// podsuma paragonu przed rabatem (w groszach)
                                 std::int64_t discountCents,			// kwota rabatu do paragonu (w groszach)
                                 const std::string& description);		// opis rabatu drukowany na paragonie

// 3.4.9 Standardowe zatwierdzenie transakcji (pierwszy wariant)
std::string makeStandardEnd(std::int64_t totalCents,					// kwota koncowa paragonu (po rabacie) w groszach
                            const std::string& cashierName);			// nazwa kasjera wysylana do drukarki



// debug: wypisuje ramke jako ASCII i HEX
void dumpFrameToStdout(const std::string& frame);						// pomocnicza funkcja diagnostyczna – loguje ramke (ASCII + HEX) na stdout

} // close namespace novitus


#endif // NOVITUS_PROTOCOL_H
