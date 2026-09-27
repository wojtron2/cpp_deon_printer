#include "novitus_protocol.h"		// wlasny naglowek - budowanie ramek protokolu Novitus
#include "money_utils.h"			// konwersje kwot grosze <-> "zlotowki.grosze" wykorzystywane w modulach protokolu

#include <stdexcept>	// dla std::runtime_error
#include <cstdio>		// dla std::snprintf
#include <cstdint>		// dla std::int64_t
#include <iostream>		// dla std::cout
#include <iomanip>		// manipulatory strumieni: std::hex, std::uppercase, std::setw, std::setfill – formatowanie HEX


// anonymous namespace - funkcje i symbole widoczne tylko w tym pliku (internal linkage)
namespace {

// oblicza bajt kontrolny zgodnie z opisem protokolu:
// start od 255, XOR po wszystkich znakach ciala ramki (body)
unsigned char computeCrcByte(const std::string& body) {
    unsigned char crc = 0xFF; // 255									// wartosc startowa CRC = 255 (0xFF)
    for (unsigned char ch : body) {										// iteracja po kazdym bajcie ciala ramki
        crc ^= ch;														// XOR aktualnej wartosci CRC z bajtem danych
    }
    return crc;															// zwracamy wynikowy bajt CRC
}

} // anonymous namespace



namespace novitus {

// buduje pelna ramke: ESC 'P' + body + CRC(2 hexy) + ESC '\'
std::string buildFrame(const std::string& body) {
    std::string frame;													// tu bedziemy skladac gotowa ramke znak po znaku
    frame.reserve(2 + body.size() + 2 + 2);								// rezerwujemy pamiec: ESC P + body + 2 znaki CRC + ESC \

    // naglowek ramki: ESC 'P'
    frame.push_back(static_cast<char>(0x1B));							// dodajemy znak ESC (0x1B) jako poczatek ramki
    frame.push_back('P');												// dodajemy litere 'P' – zgodnie z protokolem

    // cialo ramki
    frame += body;														// dopisujemy cialo ramki (polecenie + parametry)

    // CRC: dwa znaki hex ASCII
    unsigned char crc = computeCrcByte(body);							// liczymy bajt CRC z ciala ramki
    char crcText[3];													// bufor na 2 znaki HEX + znak konca '\0'
    std::snprintf(crcText, sizeof(crcText), "%02X", static_cast<unsigned int>(crc));		// formatuje bajt CRC jako dwa znaki HEX (np. "3A") i zapisuje je do bufora crcText
    frame += crcText;													// dopisujemy te dwa znaki do ramki

    // zakonczenie ramki: ESC '\'
    frame.push_back(static_cast<char>(0x1B));							// ESC jako poczatek znacznika konca
    frame.push_back('\\');												// znak '\' jako oznaczenie konca ramki

    return frame;														// zwracamy zbudowana ramke jako std::string
}


// 3.4.1 Poczatek transakcji – paragon ON-LINE (ilosc pozycji = 0)
std::string makeBeginTransactionOnline() {
    // 3.4.1 Poczatkek transakcji (prosty wariant):
    // body: "0$h"
    // 0  - ilosc pozycji = 0 -> paragon ON-LINE
    // $h - rozkaz poczatek transakcji
    std::string body;													// bufor na cialo ramki
    body += '0';														// wpisujemy '0' – ilosc pozycji = 0 (tryb ON-LINE)
    body += "$h";														// wpisujemy rozkaz "$h" – poczatek transakcji
    return buildFrame(body);											// owijamy body w pelna ramke ESC 'P' ... CRC ... ESC '\'
}


// 3.4.2 Linia paragonu  – jedna sztuka, cena brutto, grupa PTU
std::string makeSaleLineSimple(const std::string& itemName,				// nazwa towaru drukowana na paragonie
                               char vatGroup,							// grupa PTU
                               std::int64_t unitPriceCents,				// cena jednostkowa brutto w groszach
                               std::int64_t lineTotalCents)				// wartosc pozycji (brutto) w groszach
{
    if (unitPriceCents < 0 || lineTotalCents < 0) {						// nie dopuszczamy ujemnych wartosci dla ceny ani brutto
        throw std::runtime_error("Kwoty dla pozycji nie moga byc ujemne");
    }

    // uproszczony format linii paragonu dla jednej sztuki,
    // zgodnie z przykladami z dokumentacji (3.4.2/3.4.3):
    //
    // body:
    //   "1$l" + Nazwa + <CR> +
    //   "1" + <CR> +
    //   PTU + '/' + Cena + '/' + Brutto + '/'
    //
    // gdzie:
    //  - "1" to ilosc sztuk,
    //  - PTU to grupa podatkowa (np. 'A'),
    //  - Cena i Brutto w formacie zlotowki.grosze

    std::string body;													// bufor na cialo ramki
    body.reserve(4 + itemName.size() + 1 + 1 + 1 + 1 + 32);				// rezerwujemy orientacyjna ilosc bajtow (mniej alokacji)

    body += '1';   														// typ pozycji: '1' – standardowa sprzedaz
    body += "$l";  														// "$l" – rozkaz linii paragonu

    body += itemName;													// dopisujemy nazwe towaru
    body.push_back('\r'); 												// <CR> po nazwie – zgodnie z formatem protokolu

    body += '1';          												// ilosc = 1 sztuka
    body.push_back('\r'); 												// <CR> po ilosci

    body.push_back(vatGroup);											// litera grupy PTU (A..G)
    body.push_back('/');												// separator przed cena

    const std::string priceStr = formatCentsToAmount(unitPriceCents);	// konwertujemy cene jednostkowa (grosze) -> "zl.grosze"
    const std::string totalStr = formatCentsToAmount(lineTotalCents);	// konwertujemy wartosc brutto pozycji -> "zl.grosze"

    body += priceStr;													// dopisujemy tekst ceny
    body.push_back('/');												// separator po cenie

    body += totalStr;													// dopisujemy tekst brutto
    body.push_back('/');												// separator zamykajacy sekcje wartosci

    return buildFrame(body);											// budujemy z tego pelna ramke z CRC i znacznikami ESC
}


// 3.4.15 Rabat/narzut do paragonu ($Y) – rabat kwotowy od podsumy
std::string makeReceiptDiscountY(std::int64_t subtotalCents,			// podsuma paragonu przed rabatem (w groszach)
                                 std::int64_t discountCents,			// kwota rabatu do paragonu (w groszach)
                                 const std::string& description)		// opis rabatu drukowany na paragonie
{
    if (discountCents <= 0) {											// rabat musi byc dodatni (0 lub mniej jest niedozwolone)
        throw std::runtime_error("Rabat musi byc dodatni");
    }
    if (subtotalCents < 0) {
        throw std::runtime_error("Podsuma nie moze byc ujemna");
    }
    if (discountCents > subtotalCents) {
        throw std::runtime_error("Rabat nie moze przekraczac podsumy");
    }

    // 3.4.15 Rabat/narzut do paragonu ($Y) - rabat kwotowy do paragonu od podsumy
    //
    // uproszczony format:
    //   body:
    //     '3' ';' '0' "$Y" + Podsuma + '/' + Rabat + '/' + Opis + <CR>
    //
    // Rodzaj rabatu = 3  -> rabat kwotowy do paragonu,
    // Numer opisu  = 0  -> nie korzystamy z wczesniej zdefiniowanych opisow
    // Podsuma i Rabat w formacie zlotowki.grosze
    // Opis - tekst drukowany na paragonie

    const std::string subtotalStr = formatCentsToAmount(subtotalCents);		// konwersja podsumy (grosze) na tekst "zl.grosze"
    const std::string discountStr = formatCentsToAmount(discountCents);		// konwersja rabatu (grosze) na tekst "zl.grosze"

    std::string body;														// bufor na cialo ramki
    body.reserve(3 + 2 + subtotalStr.size() + discountStr.size() + description.size() + 4);		// rezerwujemy miejsce na wszystkie pola i separatorki

    body += '3';  								// rodzaj rabatu: 3 = rabat kwotowy do paragonu
    body += ';';  								// separator po rodzaju rabatu

    body += '0';  								// numer opisu = 0
    body += "$Y"; 								// rozkaz Rabat/narzut do paragonu

    body += subtotalStr;						// dopisujemy tekst podsumy
    body.push_back('/');						// separator po podsumie

    body += discountStr;						// dopisujemy tekst rabatu
    body.push_back('/');						// separator po rabacie

    if (!description.empty()) {					// jezeli opis nie jest pusty
        body += description;					// dopisujemy opis rabatu (bedzie wydrukowany)
    }
    body.push_back('\r');						// <CR> na koncu ciala

    return buildFrame(body);					// zwracamy pelna ramke zbudowana z body
}


// 3.4.9 Standardowe zatwierdzenie transakcji (pierwszy wariant)
std::string makeStandardEnd(std::int64_t totalCents,
                            const std::string& cashierName)
{
    if (totalCents < 0) {
        throw std::runtime_error("Kwota koncowa nie moze byc ujemna");
    }

    // 3.4.9 Standardowe zatwierdzenie transakcji (pierwszy wariant)
    //
    // uproszczony format:
    //   body:
    //     '1' ';' '0' "$e" + NazwaKasjera + <CR> + Wplata + '/' + Razem + '/'
    //
    // Akcja = 1 (zatwierdzenie),
    // Rabat = 0 (rabat realizujemy osobnym $Y)
    // Wplata = Razem = totalCents (brak reszty)

    const std::string totalStr = formatCentsToAmount(totalCents);				// konwersja sumy koncowej na "zl.grosze"

    std::string body;															// bufor na cialo ramki
    body.reserve(4 + cashierName.size() + 1 + totalStr.size() * 2 + 2);			// rezerwujemy przyblizona liczbe znakow

    body += '1';  // Akcja = 1 (zatwierdzenie)
    body += ';';  // separator
    body += '0';  // Rabat = 0
    body += "$e"; // rozkaz zatwierdzenia

    body += cashierName;			// dopisujemy nazwe kasjera
    body.push_back('\r'); 			// <CR> po nazwie kasjera

    // wplata
    body += totalStr;				// wplata = kwota koncowa
    body.push_back('/');			// separator po wplaconej kwocie

    // razem
    body += totalStr;				// razem = kwota koncowa
    body.push_back('/');			// separator po polu "razem"

    return buildFrame(body);		// budujemy i zwracamy pelna ramke
}


// funkcja diagnostyczna – wypisuje ramke w ASCII i HEX na stdout
void dumpFrameToStdout(const std::string& frame) {
    std::cout << "Ramka (" << frame.size() << " bajtow) ASCII: ";				// naglowek z liczba bajtow ramki i trybem ASCII

    for (unsigned char c : frame) {												// iterujemy po kazdym bajcie ramki
        if (c >= 32 && c < 127) {												// jesli bajt to "drukowalny" ASCII (bez sterujacych)
            std::cout << static_cast<char>(c);									// wypisujemy znak bezposrednio
        } else {
            std::cout << '.';													// niedrukowalne znaki pokazujemy jako '.'
        }
    }
    std::cout << "\n";															// koniec linii ASCII

    std::cout << "Ramka HEX: ";													// naglowek dla widoku heksadecymalnego
    std::ios::fmtflags f = std::cout.flags();									// zapamietujemy aktualne flagi formatowania strumienia
    char oldFill = std::cout.fill();											// zapamietujemy aktualny znak wypelnienia

    for (unsigned char c : frame) {												// iterujemy po bajtach ramki
        std::cout << std::hex << std::uppercase									// przechodzimy w tryb HEX, duze litery A..F
                  << std::setw(2)												// zawsze dwa znaki na jeden bajt
				  << std::setfill('0')											// uzupelniamy '0' z przodu jesli potrzeba
                  << static_cast<int>(c) << ' ';								// wypisujemy wartosc bajtu jako liczbe i spacje po niej
    }

    std::cout.flags(f);															// przywracamy poprzednie flagi formatowania strumienia
    std::cout.fill(oldFill);													// przywracamy poprzedni znak wypelnienia
    std::cout << "\n\n";														// dwie nowe linie na odstep w logu
}

} // close namespace novitus
