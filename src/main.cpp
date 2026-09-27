#include <iostream>		// strumienie we/wy cout/cerr do komunikatow
#include <string>		// klasa std::string do przechowywania tekstu (nazwa towaru, argumenty)
#include <stdexcept>	// standardowe wyjatki (np. std::runtime_error) do zglaszania bledow

#include "serial_port.h"	// klasa SerialPort: otwieranie, konfiguracja i zapis na port szeregowy
#include "money_utils.h"	// funkcje parsujace i formatujace kwoty (zl.grosze <-> grosze)
#include "receipt_logic.h"	// logika wystawienia paragonu: sekwencja ramek protokolu na podstawie danych

// nadmiarowa ilosc komentarzy jest efektem cwiczenia code review i normalnie ich tyle nie stosuje oraz mam swiadomosc ze jest ich tu nadmiar

// domyslny port szeregowy dla drukarki
constexpr const char* DEFAULT_SERIAL_PORT = "/dev/ttyUSB0";
constexpr int         DEFAULT_BAUD_RATE   = 9600;  // baud dla Deon Online z dokumentacji


// prosty przelacznik: jesli true, program NIE wysyla ramek na port i nie sprawdza czy istnieje i jest dostepny,
// tylko wypisuje je na stdout (do debugowania bez drukarki)
constexpr bool DRY_RUN = true;  		// false = laczenie z faktycznym portem, true = tryb testowy bez laczenia z portem


struct ParsedArgs {
    std::string    serialPortPath;	 			// docelowa sciezka portu szeregowego (domyslna lub z parametru --port=)
    ReceiptContext ctx;   						// przechowuje sparsowane dane
};



void printUsage(const char* progName) {
    std::cerr
        << "Uzycie:\n"
        << "  " << progName << " [--port=/dev/ttyS0] \"NAZWA\" CENA PTU [RABAT]\n\n"
        << "Przyklady:\n"
        << "  " << progName << " \"TOWAR TESTOWY\" 12.34 A\n"
		<< "  " << progName << " \"TOWAR TESTOWY\" 12.34 A 2.55\n\n"
		<< "  " << progName << " --port=/dev/ttyS0 \"TOWAR TESTOWY\" 12.34 A\n\n"
        << "  " << progName << " --port=/dev/ttyS0 \"TOWAR TESTOWY\" 12.34 A 2.55\n\n"
        << "CENA i RABAT w formacie zl.grosze (z kropka), np. 12.34, 2.55\n"
        << "PTU to grupa podatkowa ustawiona w drukarce (np. A, B, C...).\n";
}



// parsowanie wejscia i jego walidacja
//  [--port=/dev/ttyS0] "NAZWA" CENA PTU [RABAT]
ParsedArgs parseArgs(int argc, char* argv[]) {							// argc = liczba argumentow, argv = tablica argumentow
    if (argc < 4) {														// jesli mniej niz 4 argumenty (prog, NAZWA, CENA, PTU)
        throw std::runtime_error("Za malo argumentow");					// to rzucamy wyjatek o zbyt malej liczbie argumentow
    }

    ParsedArgs result;													// tworzymy strukture wynikowa
    result.serialPortPath = DEFAULT_SERIAL_PORT;						// ustawiamy domyslna sciezke portu (mozliwe nadpisanie przez --port)

    int idx = 1;														// indeks aktualnie przetwarzanego argumentu (pominiecie argv[0] = nazwa programu)

    // opcjonalny port w postaci --port=/dev/ttyS0
    const std::string portPrefix = "--port=";							// stala na prefiks poprzedzajacy sciezke portu
    if (idx < argc && std::string(argv[idx]).rfind(portPrefix, 0) == 0) {		// jesli obecny argument zaczyna sie od "--port="
        std::string arg = argv[idx];									// kopiujemy ten argument do std::string
        result.serialPortPath = arg.substr(portPrefix.size());			// wycinamy czesc po "--port=" jako sciezke portu
        ++idx;															// przesuwamy indeks na nastepny argument

        if (argc - idx < 3) {											// po sparsowaniu --port musza zostac co najmniej 3 argumenty: NAZWA CENA PTU
            throw std::runtime_error("Brak wymaganych parametrow po --port=");
        }
    }

    // oczekujemy: NAZWA CENA PTU [RABAT]
    if (argc - idx < 3) {												// jesli nie bylo parametru --port i argumentow jest za malo zwroc blad
        throw std::runtime_error("Brak wymaganych parametrow: NAZWA CENA PTU");
    }

    const std::string itemName = argv[idx++];							// wczytujemy NAZWA towaru i inkrementujemy idx
    const std::string priceStr = argv[idx++];							// wczytujemy paramter CENA
    const std::string vatStr   = argv[idx++];							// wczytujemy tekstowo grupe PTU (np. "A")

    std::string discountStr;											// zmienna na opcjonalny parametr rabatu
    if (argc - idx >= 1) {												// jesli zostal jeszcze co najmniej jeden parametr po wczytaniu poprzednich
        discountStr = argv[idx++];										// to wczytujemy go jako string RABAT
    }

    if (vatStr.empty()) {												// jesli string PTU jest pusty
        throw std::runtime_error("Stawka PTU nie moze byc pusta");
    }

    ReceiptContext ctx;													// tworzy kontekst paragonu
    ctx.itemName      = itemName;										// zapisujemy nazwe towaru
    ctx.vatGroup      = vatStr[0];                 						// grupa PTU jako pojedynczy znak
    ctx.priceCents    = parseAmountToCents(priceStr);					// konwertujemy CENA z parametru "zl.grosze" na grosze (int64) dla obliczen na calkowitych bez bledow mantysy w IEEE 754
    ctx.quantity      = 1;                         						// w poleconym zadaniu zawsze 1 sztuka
    ctx.discountCents = discountStr.empty()								// jesli rabat nie zostal podany
                        ? 0												// to przyjmujemy 0 (brak rabatu)
                        : parseAmountToCents(discountStr);				// w przeciwnym wypadku parsujemy rabat do groszy

    result.ctx = ctx;													// zapisujemy wypelniony ReceiptContext do struktury wynikowej
    return result;														// zwracamy wynik parsowania
}



int main(int argc, char* argv[]) {										// funkcja glowna programu ktora przyjmuje argc/argv z linii polecen
    try {																// blok try: kod, ktory moze rzucac wyjatki
        const ParsedArgs args = parseArgs(argc, argv);		 			// parsujemy argumenty, jesli blad, poleci wyjatek

        std::cout << "Uzywany port: " << args.serialPortPath				// wypisujemy na stdout jaki port bedzie uzyty
                  << " (baud " << DEFAULT_BAUD_RATE << ")\n";				// jaka bedzie predkosc transmisji
        std::cout << "Towar: \"" << args.ctx.itemName << "\"\n";			// wypisujemy nazwe towaru w cudzyslowie
        std::cout << "Cena: " << formatCentsToAmount(args.ctx.priceCents)			// wypisujemy cene w formacie "zl.grosze"
                  << ", PTU: " << args.ctx.vatGroup << "\n";						// wypisujemy grupe PTU
        if (args.ctx.discountCents > 0) {											// jesli rabat jest wiekszy od zera
            std::cout << "Rabat kwotowy do paragonu: "								// wypisujemy informacje o rabacie
                      << formatCentsToAmount(args.ctx.discountCents) << "\n";			// w formacie "zl.grosze"
        } else {
            std::cout << "Rabat: brak\n";
        }

        // Transaction Script:
        //  - sparsuj argumenty,
        //  - policz kwoty w groszach,
        //  - zbuduj ramki protokolu,
        //  - wyslij je kolejno na drukarke / lub tylko wypisz

        if (DRY_RUN) {
            std::cout << "DRY_RUN = true -> ramki nie beda wysylane na port "
                         "ani port nie bedzie otwierany; ramki beda tylko wypisane.\n";

            // port == nullptr -> receipt_logic wyswietli tylko ramki
            // bez prob zapisu na port szeregowy
            printSimpleReceipt(args.ctx,								// przekazujemy kontekst paragonu
                             /*port=*/	nullptr,						// port = nullptr -> brak rzeczywistego wysylania ramek
                             /*debugDump=*/	true);						// debugDump = true -> ramki wypisywane ASCII + HEX
        } else {														// jesli DRY_RUN == false (tryb normalny gdzie laczy z drukarka/portem)
            // tylko w tym bloku faktycznie otwieramy port
            SerialPort serial(args.serialPortPath, DEFAULT_BAUD_RATE);	// tworzymy obiekt SerialPort (otwarcie i konfiguracja portu)
            printSimpleReceipt(args.ctx,								// przekazujemy kontekst paragonu
                               &serial,									// przekazujemy wskaznik do otwartego portu szeregowego
                             /*debugDump=*/	true);						// nadal wypisujemy ramki debugowo na stdout
        }

        return 0;														// zwracamy 0 -> kod wyjscia "sukces" dla systemu operacyjnego
    }
    catch (const std::exception& ex) {									// lapie wszystkie wyjatki pochodne od std::exception np. jezeli w poprzedzajacym bloku try wystapil blad
        std::cerr << "Blad: " << ex.what() << "\n\n";					// wypisuje tresc bledu na stderr
        printUsage(argv[0]);											// pokazuje uzytkownikowi jak poprawnie uzyc programu
        return 1;														// zwracamy 1 -> kod wyjscia "blad" (niepoprawne wywolanie / dane)
    }
}
