#include <cassert>
#include <iostream>
#include <exception>

#include "receipt_logic.h"

// uwaga: w testach przekazujemy nullptr jako port
// dzieki temu nic nie jest wysylane na port szeregowy

// pomocnicza funkcja do sprawdzania ze wywolanie rzuca wyjatek
template <typename Func>
static void expect_throw(Func f) {
    bool threw = false;
    try {
        f();
    } catch (const std::exception&) {
        threw = true;
    }
    assert(threw);
}

// pomocnicza funkcja do sprawdzania ze wywolanie nie rzuca wyjatku
template <typename Func>
static void expect_no_throw(Func f) {
    bool threw = false;
    try {
        f();
    } catch (const std::exception& e) {
        threw = true;
        std::cerr << "nieoczekiwany wyjatek: " << e.what() << "\n";
    }
    assert(!threw);
}

int main() {
    std::cout << "test receipt_logic (printSimpleReceipt)..." << std::endl;

    // scenariusz 1: kontekst bez rabatu
    {
        ReceiptContext ctx{};
        ctx.itemName      = "KAWA";
        ctx.vatGroup      = 'A';
        ctx.priceCents    = 850;
        ctx.quantity      = 1;
        ctx.discountCents = 0;

        expect_no_throw([&ctx] {
            printSimpleReceipt(ctx, nullptr, false);
        });
    }

    // scenariusz 2: kontekst z poprawnym rabatem
    {
        ReceiptContext ctx{};
        ctx.itemName      = "KAWA";
        ctx.vatGroup      = 'A';
        ctx.priceCents    = 2000;  // 20.00
        ctx.quantity      = 1;
        ctx.discountCents = 500;   // 5.00

        expect_no_throw([&ctx] {
            printSimpleReceipt(ctx, nullptr, false);
        });
    }

    // scenariusz 3: rabat wiekszy niz wartosc paragonu
    // zgodnie z aktualna logika funkcja powinna rzucic wyjatek
    {
        ReceiptContext ctx{};
        ctx.itemName      = "KAWA";
        ctx.vatGroup      = 'A';
        ctx.priceCents    = 850;
        ctx.quantity      = 1;
        ctx.discountCents = 99999;

        expect_throw([&ctx] {
            printSimpleReceipt(ctx, nullptr, false);
        });
    }

    // scenariusz 4: ujemna cena
    {
        ReceiptContext ctx{};
        ctx.itemName      = "X";
        ctx.vatGroup      = 'A';
        ctx.priceCents    = -1;
        ctx.quantity      = 1;
        ctx.discountCents = 0;

        expect_throw([&ctx] {
            printSimpleReceipt(ctx, nullptr, false);
        });
    }

    // scenariusz 5: ilosc rowna zero
    {
        ReceiptContext ctx{};
        ctx.itemName      = "X";
        ctx.vatGroup      = 'A';
        ctx.priceCents    = 100;
        ctx.quantity      = 0;
        ctx.discountCents = 0;

        expect_throw([&ctx] {
            printSimpleReceipt(ctx, nullptr, false);
        });
    }

    // scenariusz 6: ujemny rabat
    {
        ReceiptContext ctx{};
        ctx.itemName      = "X";
        ctx.vatGroup      = 'A';
        ctx.priceCents    = 100;
        ctx.quantity      = 1;
        ctx.discountCents = -1;

        expect_throw([&ctx] {
            printSimpleReceipt(ctx, nullptr, false);
        });
    }

    std::cout << "OK\n";
    return 0;
}



/* kompilacja i uruchomienie pojedynczego testu (opcjonalnie uruchomienie wszystkich testow za pomoca cmake w README.md)

cd do katalogu projektu, nastepnie

g++ -std=c++17 -Wall -Wextra \
    -Isrc \
    src/receipt_logic.cpp src/novitus_protocol.cpp src/money_utils.cpp src/serial_port.cpp \
    tests/test_receipt_logic.cpp \
    -o tests/test_receipt_logic

./tests/test_receipt_logic




*/
