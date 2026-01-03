#include "money_utils.h"	// wlasny naglowek - deklaracje parseAmountToCents i formatCentsToAmount

#include <stdexcept>	// dla std::runtime_error – zglaszanie bledow formatu/zakresu kwoty
#include <string>		// dla std::string, std::size_t, std::to_string – operacje na napisach i budowa tekstu wyniku
#include <cctype>		// dla std::isdigit – sprawdzanie, czy znaki w kwocie sa cyframi


// funkcja parsuje tekstowa kwote "zlotowki.grosze" do liczby calkowitej w groszach
std::int64_t parseAmountToCents(const std::string& text) {
    if (text.empty()) {													// jesli napis jest pusty (brak jakiejkolwiek kwoty)
        throw std::runtime_error("Pusta kwota");	
    }																	// zglaszamy blad – pusta kwota jest niedozwolona

    // szuka kropki jako separatora groszy
    std::size_t dotPos = text.find('.');								// szukamy pozycji znaku '.' w tekscie; npos jesli brak
    std::string intPart;												// zmienna na czesc zlotowkowa (przed kropka)
    std::string fracPart;												// zmienna na czesc groszowa (po kropce)

    if (dotPos == std::string::npos) {									// jezeli nie znaleziono kropki w tekscie
        // brak czesci groszowej - traktujemy jako ".00"
        intPart = text;													// caly napis traktujemy jako czesc zlotowkowa
        fracPart = "00";												// czesc groszowa ustawiamy na "00"
    } else {															// jezeli kropka zostala znaleziona
        intPart = text.substr(0, dotPos);								// wycinamy czesc od poczatku napisu do kropki (bez kropki)
        fracPart = text.substr(dotPos + 1);			 					// wycinamy czesc po kropce (od znaku po '.' do konca)
    }

    if (intPart.empty()) {												// jesli czesc zlotowkowa jest pusta (np. ".50")
        throw std::runtime_error("Brak czesci zlotowej w kwocie: " + text);
    }																	// zglaszamy blad z podaniem oryginalnego tekstu

    // wszystkie znaki w czesci zlotowkowej musza byc cyframi
    for (char c : intPart) {											// iterujemy po kazdym znaku w czesci zlotowej
        if (!std::isdigit(static_cast<unsigned char>(c))) {				// jezeli znak nie jest cyfra (po rzutowaniu na unsigned char)
            throw std::runtime_error("Niepoprawny znak w czesci zlotowej: " + text);
        }																// zglaszamy blad – np. litera, minus, spacja itp.
    }

    // czesc groszowa: do 2 znakow (00..99)
    if (fracPart.empty()) {												// jesli nie ma zadnej czesci groszowej (np. "12.")
        fracPart = "00";												// traktujemy to jak "12.00"
    } else if (fracPart.size() == 1) {									// jesli jest tylko jedna cyfra groszy (np. "12.3")
        fracPart.push_back('0'); // "1" -> "10" (czyli 0.10)			// dopisujemy '0' na koncu: "3" -> "30" czyli 0.30
    } else if (fracPart.size() > 2) {									// jesli jest wiecej niz 2 cyfry (np. "12.345")
        throw std::runtime_error("Za duzo cyfr groszy w kwocie: " + text);
    }																	// zglaszamy blad – nie wspieramy wiecej niz 2 miejsca po kropce

    for (char c : fracPart) {											// iterujemy po kazdym znaku czesci groszowej
        if (!std::isdigit(static_cast<unsigned char>(c))) {				// sprawdzamy czy jest cyfra
            throw std::runtime_error("Niepoprawny znak w czesci groszowej: " + text);
        }																// jesli nie, zglaszamy blad z oryginalnym tekstem
    }

    // konwersja na liczby calkowite
    std::int64_t zl = 0;												// zmienna na kwote w zlotych (liczba calkowita)
    for (char c : intPart) {											// przechodzimy po kazdej cyfrze czesci zlotowej
        zl = zl * 10 + (c - '0');										// przesuwamy dotychczasowa wartosc w lewo (razy 10) i dodajemy nowa cyfre
    }

    std::int64_t gr = 0;												// zmienna na kwote w groszach (0..99)
    for (char c : fracPart) {											// przechodzimy po kazdej cyfrze czesci groszowej
        gr = gr * 10 + (c - '0');										// analogicznie: budujemy liczbe groszy z cyfr
    }

    if (zl < 0 || gr < 0) {												// dodatkowe zabezpieczenie – nie dopuszczamy wartosci ujemnych
        throw std::runtime_error("Ujemna kwota jest niedozwolona: " + text);
    }																	// komunikat bledu z oryginalnym napisem kwoty

    return zl * 100 + gr;												// zwracamy calkowita liczbe groszy: zl * 100 + grosze
}


// funkcja formatuje liczbe groszy do postaci tekstowej "zlotowki.grosze"
std::string formatCentsToAmount(std::int64_t cents) {	
    if (cents < 0) {													// jesli kwota w groszach jest mniejsza od zera
        throw std::runtime_error("Nie mozna sformatowac ujemnej kwoty w groszach");
    }																	// zglaszamy blad – formatowanie ujemnych kwot jest zabronione

    std::int64_t zl = cents / 100;										// obliczamy czesc zlotowa przez dzielenie calkowite przez 100
    std::int64_t gr = cents % 100;										// obliczamy czesc groszowa jako reszte z dzielenia przez 100

    std::string result = std::to_string(zl);							// zamieniamy czesc zlotowa na string (np. 12 -> "12")
    result.push_back('.');												// dodajemy kropke jako separator zlotowki/grosze

    if (gr < 10) {														// jesli grosze sa jednocyfrowe (0..9)
        result.push_back('0');											// dopisujemy '0' z przodu, zeby miec dwie cyfry (np. "05")
    }
    result += std::to_string(gr);										// dopisujemy liczbe groszy jako tekst (np. "34" -> "12.34")

    return result;														// zwracamy gotowy napis w formacie "zlotowki.grosze"
}
