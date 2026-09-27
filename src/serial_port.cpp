#include "serial_port.h"	// wlasny naglowek z deklaracja klasy SerialPort

#include <stdexcept>	// dla std::runtime_error w przypadku bledow z baud, open, write, tcsetattr, tcdrain itd.
#include <cerrno>		// globalna zmienna errno oraz kody bledow funkcji systemowych (open, write, tcsetattr itp.)
#include <cstring>		// dla std::strerror(errno) – zamiana errno na tekst bledu
#include <fcntl.h>		// deklaracje ::open oraz flagi O_RDWR, O_NOCTTY, O_SYNC do otwierania portu
#include <unistd.h>		// funkcje POSIX do obslugi portu szeregowego ::close, ::write, typ ssize_t – obsluga zapisu i zamykania portu
#include <termios.h>	// udostepnia elementy konfiguracji portu szeregowego struct termios, tcgetattr/tcsetattr, praca w trybie "raw", 8N1, XON/XOFF

// anonymous namespace - funkcje i symbole widoczne tylko w tym pliku (internal linkage)
namespace {

// funkcja pomocnicza: mapuje wartosc baudRate (int) na odpowiadajaca stala typu speed_t (B9600, B19200 itd.)
	speed_t toSpeedT(int baudRate) {
		switch (baudRate) {
		case 9600:   return B9600;
		case 19200:  return B19200;
		case 38400:  return B38400;
		case 57600:  return B57600;
		case 115200: return B115200;
		default:														// jesli zaden z powyzszych nie pasuje czyli nie rozpoznalismy predkosci
			throw std::runtime_error("Nieobslugiwany baudRate: " + std::to_string(baudRate)); // rzucamy wyjatek z opisem nieobslugiwanej predkosci
		}
	}

} // close anonymous namespace - koniec przestrzeni anonimowej – toSpeedT jest widoczne tylko w tym pliku


// konstruktor SerialPort: otwiera i konfiguruje port szeregowy
SerialPort::SerialPort(const std::string& devicePath, int baudRate)
    : fd_{-1}															// lista inicjalizacyjna: na start fd_ ustawiony na -1 (brak poprawnego deskryptora)
{
    fd_ = ::open(devicePath.c_str(), O_RDWR | O_NOCTTY | O_SYNC);		// probujemy otworzyc urzadzenie pod sciezka devicePath
																		// O_RDWR  - otwarcie do odczytu i zapisu
																		// O_NOCTTY - nie robic z tego terminala sterujacego
																		// O_SYNC  - zapis synchroniczny (czekamy az dane wyjda z buforow)
    if (fd_ < 0) {														// jesli open zwrocil ujemny deskryptor -> blad otwarcia													
        throw std::runtime_error(
            "Nie udalo sie otworzyc portu " + devicePath +				// budujemy komunikat bledu z podana sciezka
            ": " + std::strerror(errno)									// dopisujemy opis bledu wynikajacy z errno
        );
    }

    try {
        configurePort(baudRate);										// probujemy skonfigurowac port: predkosc, 8N1, XON/XOFF itd.
    } catch (...) {														// jesli cokolwiek rzuci wyjatek (dowolny typ)
        ::close(fd_);													// zamykamy juz otwarty deskryptor, zeby nie wyciekal
        fd_ = -1;														// ustawiamy fd_ na wartosc "niepoprawna"
        throw;															// ponownie rzucamy ten sam wyjatek wyzej
    }
}


// destruktor SerialPort: pilnuje zamkniecia portu (RAII)
SerialPort::~SerialPort() {
    if (fd_ >= 0) {														// jesli deskryptor jest poprawny (port jest otwarty)
        ::close(fd_);													// zamykamy port
        fd_ = -1;														// ustawiamy wartosc niepoprawna (opcjonalne zabezpieczenie)
    }
}

// konstruktor przenoszacy: przejmuje wlasnosc deskryptora z innego obiektu SerialPort
SerialPort::SerialPort(SerialPort&& other) noexcept							// konstruktor przenoszacy przyjmuje referencje przenoszaca (&&) do obiektu SerialPort (other) jako zrodla zasobow, noexcept - jesli jednak rzuci wyjatek natychmiast zamknij program zapobiegajac wyciekom
    : fd_{other.fd_}													// kopiujemy wartosc deskryptora z obiektu zrodlowego
{
    other.fd_ = -1;														// uniewazniamy deskryptor w obiekcie zrodlowym (zeby nie zamknal go drugi raz)
}

// operator przypisania przenoszacego: zwalnia aktualny deskryptor i przejmuje nowy
SerialPort& SerialPort::operator=(SerialPort&& other) noexcept {  			// operator przypisania przenoszacego: przejmuje zasob (deskryptor fd_) od obiektu 'other', zwalniajac dotychczasowy
    if (this != &other) {												// zabezpieczenie przed przypisaniem do samego siebie
        if (fd_ >= 0) {													// jesli mamy aktualnie otwarty port
            ::close(fd_);												// zamykamy go zeby nie wyciekal
        }
        fd_ = other.fd_;												// przejmujemy deskryptor z obiektu zrodlowego
        other.fd_ = -1;													// uniewazniamy deskryptor w obiekcie zrodlowym
    }
    return *this;														// zwracamy referencje do siebie (pozwala na lancuchowe przypisania)
}


// prywatna metoda konfigurujaca parametry portu szeregowego
void SerialPort::configurePort(int baudRate) {
    struct termios tio{};												// tworzymy strukture termios i zerujemy ja inicjalizacja {}
	
    if (tcgetattr(fd_, &tio) != 0) {									// pobieramy aktualne ustawienia portu do struktury tio, sprawdza czy funkcja tcgetattr nie zakonczyla sie bledem, 
        throw std::runtime_error(					
            "tcgetattr nie powiodl sie: " + std::string(std::strerror(errno))					// jesli blad, rzucamy wyjatek z opisem errno
        );
    }

    // tryb "raw" - bez przetwarzania znakow przez terminal (zadnych konwersji CR/LF, kontroli strumienia itp.)
    cfmakeraw(&tio);	 												// resetuje wiele pol termios tak, aby port dzialal w trybie "raw"

    // ustawiamy predkosc transmisji
    const speed_t speed = toSpeedT(baudRate);									// mapujemy int baudRate na speed_t (B9600, B19200 itd.)
    if (cfsetispeed(&tio, speed) != 0 || cfsetospeed(&tio, speed) != 0) {		// ustawia predkosc wejsciowa (ispeed) i wyjsciowa (ospeed) w strukturze, sprawdza, czy ktorakolwiek operacja zakonczyła sie bledem
        throw std::runtime_error(												
            "cfsetispeed/cfsetospeed nie powiodl sie: " + std::string(std::strerror(errno))			// jesli ustawienie predkosci sie nie powiedzie, rzucamy wyjatek z opisem errno
        );			
    }

    // 8N1: 8 bitow danych, brak parzystosci, 1 bit stopu
    tio.c_cflag &= ~PARENB; 											// wylaczamy bit parzystosci (zastosuj brak parzystosci)
    tio.c_cflag &= ~CSTOPB; 											// ustawiamy 1 bit stopu (czyscimy flage 2 bit stopu)
    tio.c_cflag &= ~CSIZE;												// czyscimy bity rozmiaru znaku
    tio.c_cflag |= CS8;     											// ustawiamy 8 bitow danych

    // wlaczamy odbiornik i oznaczamy, ze urzadzenie jest lokalne (bez zmian sygnalow kontroli terminala)
    tio.c_cflag |= (CLOCAL | CREAD);									// CLOCAL - ignoruj sygnaly modemu, CREAD - wlacz odbior danych

    // wlaczamy handshaking programowy XON/XOFF (kontrola przeplywu w warstwie znakowej)
    // cfmakeraw wylacza IXON/IXOFF, wiec ustawiamy je od nowa
    tio.c_iflag |= (IXON | IXOFF);										// wlaczamy XON/XOFF

#ifdef CRTSCTS
    // wylaczamy sprzetowy RTS/CTS, zeby nie przeszkadzal przy programowym XON/XOFF
    tio.c_cflag &= ~CRTSCTS;
#endif

    // parametry odczytu (chociaz tutaj glownie piszemy, to ustawiamy sensowne VMIN/VTIME)
    tio.c_cc[VMIN]  = 0;												// minimalna liczba bajtow do odczytu (0 -> tryb z timeoutem)
    tio.c_cc[VTIME] = 10; 												// timeout 1.0 s (0.1 s * 10)

    if (tcsetattr(fd_, TCSANOW, &tio) != 0) {							// zapisuje nowe ustawienia (zgodnie ze strukturą tio) na port szeregowy, 
																		// stosuje je natychmiast (TCSANOW), sprawdza czy zapis nie zwrocil bledu
        throw std::runtime_error(
            "tcsetattr nie powiodl sie: " + std::string(std::strerror(errno))			// jesli ustawianie atrybutow sie nie powiedzie, rzucamy wyjatek z opisem errno
        );											
    }

    if (tcdrain(fd_) != 0) {											// czekamy az wszystkie dane wyjsciowe zostana wypchniete (czyszczenie bufora)
        throw std::runtime_error(
            "tcdrain nie powiodl sie: " + std::string(std::strerror(errno))				// jesli tcdrain zawiedzie, rzucamy wyjatek z opisem errno
        );											 
    }
}


// metoda publiczna: zapisuje caly bufor danych do portu szeregowego (dopoki wszystkie bajty nie zostana wyslane)
void SerialPort::writeAll(const void* data, std::size_t size) {
    if (fd_ < 0) {														// jesli deskryptor jest niepoprawny (port nie jest otwarty)
        throw std::runtime_error("Port szeregowy nie jest otwarty");		// sygnalizujemy blad przez wyjatek
    }								

    const auto* ptr = static_cast<const std::uint8_t*>(data);			// rzutujemy wskaznik na bufor na const std::uint8_t*
    std::size_t totalWritten = 0;										// licznik wyslanych dotad bajtow

    while (totalWritten < size) {										// dopoki nie wyslalismy wszystkich bajtow
        ssize_t written = ::write(fd_,									// probujemy zapisac na port
								  ptr + totalWritten,					// od aktualnej pozycji w buforze
								  size - totalWritten);					// pozostala liczba bajtow do wyslania
        if (written < 0) {												// jesli write zwrocil blad
            if (errno == EINTR) {										// jesli zostal przerwany sygnalem (EINTR)
                continue;												// ponawiamy probe zapisu
            }
            throw std::runtime_error(
                "write nie powiodl sie: " + std::string(std::strerror(errno))		// dla innych bledow zwracamy wyjatek z opisem errno
            );							
        }
        if (written == 0) {												// jesli write zwrocil 0, ale jeszcze nie wyslalismy wszystkiego
            throw std::runtime_error("write zwrocil 0, polaczenie przerwane?");		// traktujemy to jako przerwanie polaczenia i zwracamy wyjatek
        }
        totalWritten += static_cast<std::size_t>(written);				// zwiekszamy licznik wyslanych bajtow
    }

    if (tcdrain(fd_) != 0) {											// na koncu czekamy, az wszystkie dane z buforow trafia na linie
        throw std::runtime_error(
            "tcdrain po writeAll nie powiodl sie: " + std::string(std::strerror(errno))
        );																// jesli tcdrain po zapisie zawiedzie, sygnalizujemy blad
    }
}
