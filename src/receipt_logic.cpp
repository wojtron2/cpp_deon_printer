#include "receipt_logic.h"		// wlasny naglowek: ReceiptContext i deklaracja printSimpleReceipt

#include "novitus_protocol.h"	// budowanie ramek protokolu Novitus
#include "serial_port.h"		// pelna definicja klasy SerialPort
#include "money_utils.h"		// konwersje kwot grosze <-> "zlotowki.grosze" wykorzystywane w modulach protokolu, tutaj opcjonalny


#include <stdexcept>	// dla std::runtime_error


// anonymous namespace - funkcje i symbole widoczne tylko w tym pliku (internal linkage)
// pomocnicza struktura na wyniki obliczen kwot
namespace {

struct ReceiptTotals {
    std::int64_t subtotalCents;  // suma brutto przed rabatem (cena * ilosc)
    std::int64_t discountCents;  // rabat kwotowy do paragonu
    std::int64_t totalCents;     // suma po rabacie
};


// liczy podsume, sprawdza poprawnosci i przygotowuje wartosci subtotal/discount/total
ReceiptTotals calculateTotalsFromContext(const ReceiptContext& ctx) {
    if (ctx.priceCents < 0) {
        throw std::runtime_error("Cena nie moze byc ujemna");
    }
    if (ctx.quantity <= 0) {
        throw std::runtime_error("Ilosc musi byc dodatnia");
    }
    if (ctx.discountCents < 0) {
        throw std::runtime_error("Rabat nie moze byc ujemny");
    }

    const std::int64_t subtotal = ctx.priceCents * ctx.quantity;

    if (ctx.discountCents > subtotal) {
        throw std::runtime_error("Rabat nie moze byc wiekszy niz wartosc paragonu");
    }

    ReceiptTotals totals{};										// inicjalizacja struktury wynikowej zerami
    totals.subtotalCents  = subtotal;							// suma przed rabatem
    totals.discountCents  = ctx.discountCents;					// przenosimy rabat z kontekstu
    totals.totalCents     = subtotal - ctx.discountCents;		// suma po rabacie
    return totals;												// zwracamy policzone wartosci
}

} // close anonymous namespace


// realizuje caly scenariusz wystawienia paragonu na podstawie ctx
void printSimpleReceipt(const ReceiptContext& ctx,						// ctx – dane paragonu: nazwa, PTU, cena, ilosc, rabat (stale odniesienie, bez kopiowania)
                        SerialPort* port,								// port – wskaznik na otwarty SerialPort; jesli nullptr, nic nie wysylamy do drukarki
                        bool debugDump)									// debugDump – jesli true, ramki sa dodatkowo wypisywane na stdout (ASCII + HEX)
{
    const ReceiptTotals totals = calculateTotalsFromContext(ctx);		// najpierw liczymy kwoty: podsuma, rabat, suma koncowa

    // 1) Poczatek transakcji (paragon ON-LINE)
    const std::string beginFrame = novitus::makeBeginTransactionOnline();		// budujemy ramke "poczatek transakcji"
    if (debugDump) {															// jesli wlaczony podglad
        novitus::dumpFrameToStdout(beginFrame);									// wypisujemy ramke ASCII + HEX
    }
    if (port) {																	// jesli port nie jest nullptr
        port->writeAll(beginFrame);												// wysylamy ramke na port szeregowy
    }

    // 2) pozycja paragonu - sprzedaz 1 sztuki bez rabatu
    const std::string lineFrame =
        novitus::makeSaleLineSimple(ctx.itemName,								// nazwa towaru
                                    ctx.vatGroup,								// grupa PTU
                                    ctx.priceCents,								// cena jednostkowa
                                    totals.subtotalCents);						// wartosc linii (tutaj = podsuma, bo ilosc = 1)
    if (debugDump) {
        novitus::dumpFrameToStdout(lineFrame);									// podglad ramki linii paragonu
    }
    if (port) {
        port->writeAll(lineFrame);												// wyslanie ramki z pozycja
    }

    // 3) ewentualny rabat kwotowy do paragonu ($Y)
    // dzieki temu rabat jest wspomniany na paragonie
    if (totals.discountCents > 0) {												// jesli rabat > 0, to trzeba wyslac rame rabatu
        const std::string discountFrame =
            novitus::makeReceiptDiscountY(totals.subtotalCents,					// podsuma przed rabatem
                                          totals.discountCents,					// kwota rabatu
                                          "RABAT KWOTOWY");						// opis pojawiajacy sie na paragonie
        if (debugDump) {
            novitus::dumpFrameToStdout(discountFrame);							// podglad ramki rabatu
        }
        if (port) {
            port->writeAll(discountFrame);										// wyslanie ramki rabatu $Y
        }
    }

    // 4) zatwierdzenie transakcji z kwota po rabacie
    const std::string endFrame =
        novitus::makeStandardEnd(totals.totalCents, "KASJER");					// budujemy ramke zamkniecia paragonu z kwota po rabacie
    if (debugDump) {
        novitus::dumpFrameToStdout(endFrame);									// podglad ramki zakonczenia
    }
    if (port) {
        port->writeAll(endFrame);												// wyslanie ramki zatwierdzajacej transakcje
    }
}
