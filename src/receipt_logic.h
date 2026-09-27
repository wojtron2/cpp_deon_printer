#ifndef RECEIPT_LOGIC_H
#define RECEIPT_LOGIC_H

#include <cstdint>	// dla std::int64_t
#include <string>	// dla std::string struktury ReceiptContext



// dane potrzebne do wystawienia prostego paragonu
struct ReceiptContext {
    std::string  itemName;       // nazwa towaru
    char         vatGroup;       // grupa PTU (np. 'A', 'B', 'C'...)
    std::int64_t priceCents;     // cena jednostkowa brutto w groszach
    std::int64_t quantity;       // ilosc sztuk (w zadaniu 1)
    std::int64_t discountCents;  // rabat kwotowy do paragonu w groszach (0 jesli brak)
};

// deklaracja, zeby nie wlaczac calego naglowka w tym miejscu
class SerialPort;

// glowna funkcja realizujaca scenariusz wystawienia paragonu
//  - ctx        : dane o pozycji, PTU i ewentualnym rabacie
//  - port       : wskaznik do otwartego SerialPort; jesli nullptr, ramki nie sa wysylane
//  - debugDump  : jesli true, ramki sa wypisywane na stdout (ASCII + HEX)
void printSimpleReceipt(const ReceiptContext& ctx,				// stale (const) odniesienie do kontekstu paragonu – bez kopiowania struktury
                        SerialPort* port,						// wskaznik na obiekt SerialPort (moze byc nullptr, gdy DRY_RUN)
                        bool debugDump);						// flaga wlaczajaca/wylaczajaca wypisywanie ramek na stdout

#endif // RECEIPT_LOGIC_H
