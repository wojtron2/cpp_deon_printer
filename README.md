# deon_printer_app

A simple C++ program for Linux designed to print a single receipt on a Novitus Deon fiscal printer.
App designed and written with highest standards as a showcase of design patterns, SOLID and proper architecture.

The program:

- accepts product data from the command line, with an optional serial port parameter,
- converts monetary values to grosz units,
- builds NOVITUS (Online) protocol frames,
- sends them sequentially to the serial port (or only prints them to the console in `DRY_RUN` mode).

---

Test mode activation (without connecting to the serial port and without returning an error if the port is unavailable; messages are printed to the console instead).

To enable this mode, change the constant in `main.cpp`:

```cpp
constexpr bool DRY_RUN = false; // false = connect to the actual serial port, true = test mode without connecting to the port
```

---

## 1. Building

The project uses CMake and requires a compiler with C++17 support.

Preparing the Linux environment for compiling the project:

```bash
sudo apt install -y build-essential cmake
```

Building the project:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

# Tests

```bash
cmake --build . --target run_tests
```

or, if the project has already been built:

```bash
ctest -V
```

## 2. Usage

For testing without using a serial port or printer, set the following in `main.cpp`:

```cpp
DRY_RUN = true;  // default value for quick testing
```

For testing with actual serial ports:

```cpp
DRY_RUN = false;
```

Default port `/dev/ttyUSB0`, without a discount:

```bash
./deon_printer_app "TEST PRODUCT" 12.34 A
```

Default port `/dev/ttyUSB0`, with a fixed-amount discount:

```bash
./deon_printer_app "TEST PRODUCT" 12.34 A 2.55
```

Custom port, without a receipt discount:

```bash
./deon_printer_app --port=/dev/ttyS0 "TEST PRODUCT" 12.34 A
```

Custom port with a fixed-amount receipt discount:

```bash
./deon_printer_app --port=/dev/ttyS0 "TEST PRODUCT" 12.34 A 2.55
```







---------------------------------------



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

