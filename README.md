# deon_printer_app

Prosty program w C++ dla Linuxa do wystawienia pojedynczego paragonu na drukarce fiskalnej Novitus Deon.

Program:

- przyjmuje dane towaru z linii poleceń, opcjonalnie port,
- przelicza kwoty na grosze,
- buduje ramki protokołu NOVITUS (Online),
- wysyła je po kolei na port szeregowy (albo tylko wypisuje – tryb `DRY_RUN`).

---
Aktywacja trybu testowego (bez laczenia z portem i zwracania bledu w przypadku jego braku, zwracanie wiadomosci na konsole.

potrzebna jest do tego zmiana stałej w main.cpp

constexpr bool DRY_RUN = false; // false = laczenie z faktycznym portem, true = tryb testowy bez laczenia z portem

---

## 1. Budowanie

Projekt korzysta z CMake i wymaga kompilatora z obsługą C++17.

Przygotowanie środowiska Linux do kompilacji projektu:

sudo apt install -y build-essential cmake

Zbudowanie projektu:


mkdir build
cd build
cmake ..
cmake --build .

# Testy

cmake --build . --target run_tests

lub jezeli mamy juz zbudowany projekt

ctest -V


## 2. Użycie:

#Dla testow bez uzycia portu/drukarki w pliku main.cpp
DRY_RUN = true;  // domyslna wartosc pod szybkie testy
#Dla testow z użyciem faktycznych portow:
DRY_RUN = false;


# Domyślny port /dev/ttyUSB0, bez rabatu:
./deon_printer_app "TOWAR TESTOWY" 12.34 A

# Domyślny port /dev/ttyUSB0, z użyciem rabatu kwotowego:
./deon_printer_app "TOWAR TESTOWY" 12.34 A 2.55


# Własny port bez rabatu do paragonu:
./deon_printer_app --port=/dev/ttyS0 "TOWAR TESTOWY" 12.34 A

# Własny port + rabat kwotowy do paragonu:
./deon_printer_app --port=/dev/ttyS0 "TOWAR TESTOWY" 12.34 A 2.55

