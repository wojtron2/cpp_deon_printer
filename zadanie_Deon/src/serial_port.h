#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <string>	// dolacza definicje std::string, uzywanej do przekazywania sciezki urzadzenia (devicePath)
#include <cstddef>	// dolacza definicje std::size_t, uzywanej jako typ rozmiaru bufora w writeAll
#include <cstdint>	// dolacza std::uint8_t, uzywane w implementacji writeAll (serial_port.cpp) do operacji na bajtach



// prosta klasa RAII do obslugi portu szeregowego (9600 8N1, XON/XOFF) na Linux (POSIX) zapobiegajaca wyciekom pamieci
class SerialPort {
public:					// sekcja publiczna – interfejs klasy widoczny na zewnatrz scope
    // otwiera i konfiguruje port
    // devicePath - np. "/dev/ttyUSB0"
    // baudRate   - np. standardowe dla Deona 9600
    SerialPort(const std::string& devicePath, int baudRate);			// konstruktor: otwiera port i wywoluje konfiguracje

    // zamyka port jesli jest otwarty
    ~SerialPort();														// destruktor: zamyka deskryptor pliku (RAII)

    // brak kopiowania (deskryptor pliku nie powinien byc bezmyslnie kopiowany)
    SerialPort(const SerialPort&) = delete;								// usuniety konstruktor kopiujacy – nie mozna kopiowac obiektu
    SerialPort& operator=(const SerialPort&) = delete;					// usuniety operator przypisania kopiujacego – zakaz kopiowania

    // mozna przenosic (przejecie wlasnosci deskryptora z innego obiektu)
    SerialPort(SerialPort&& other) noexcept;   							// konstruktor przenoszacy: przejmuje deskryptor z innego SerialPort
    SerialPort& operator=(SerialPort&& other) noexcept;  				// move assignment operator - operator przypisania przenoszacego: zwalnia stary deskryptor i przejmuje nowy, umozliwia przeniesienie jezeli obiekt juz istnieje

    // zapis calego bufora do portu (operacja blokujaca do czasu wyslania wszystkich bajtow)
    void writeAll(const void* data, std::size_t size);					// przyjmuje wskaznik na dane i ich rozmiar w bajtach

    // wygodna wersja dla std::string – pozwala wyslac caly napis bez recznego podawania rozmiaru - przeciazona (inna ilosc parametrow niz inne wystapienie pod ta sama nazwa) 
    void writeAll(const std::string& data) {							// stala referencja do obiektu umozliwiajaca bezpieczne (tylko do odczytu) przekazywanie obiektu
        writeAll(data.data(), data.size());								// deleguje do wersji przyjmujacej surowy bufor i rozmiar
    }

private:																// sekcja prywatna – szczegoly implementacyjne niewidoczne na zewnatrz scope
    int fd_; 															// deskryptor pliku reprezentujacy otwarty port szeregowy (/dev/ttyUSB0 itd.)

    void configurePort(int baudRate); 									// ustawia parametry portu (9600 8N1, XON/XOFF itp.)
};

#endif // SERIAL_PORT_H
