#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#include "money_utils.h"

// testuje poprawne przypadki parsowania kwot
static void test_parse_valid() {
    assert(parseAmountToCents("0") == 0);
    assert(parseAmountToCents("0.00") == 0);
    assert(parseAmountToCents("1") == 100);
    assert(parseAmountToCents("1.23") == 123);
    assert(parseAmountToCents("123.45") == 12345);

    // brak czesci groszowej lub jedna cyfra po kropce
    assert(parseAmountToCents("1.") == 100);    // 1.00
    assert(parseAmountToCents("1.2") == 120);   // 1.20

    // zera wiodace
    assert(parseAmountToCents("000.05") == 5);
}

// pomocnicza funkcja do sprawdzania ze parsowanie rzuca wyjatek
static void expect_parse_throw(const std::string& text) {
    bool threw = false;
    try {
        (void)parseAmountToCents(text);
    } catch (const std::exception&) {
        threw = true;
    }
    assert(threw);
}

// testuje przypadki parsowania ktore powinny rzucic wyjatek
static void test_parse_invalid() {
    // pusty napis
    expect_parse_throw("");

    // brak czesci zlotowej przed kropka
    expect_parse_throw(".12");

    // za duzo cyfr groszy
    expect_parse_throw("1.234");

    // przecinek zamiast kropki
    expect_parse_throw("1,23");

    // litery w kwocie
    expect_parse_throw("1.a");
    expect_parse_throw("a12");

    // ujemna kwota
    expect_parse_throw("-1.00");
}

// testuje poprawne formatowanie kwot w groszach na napis
static void test_format_valid() {
    assert(formatCentsToAmount(0) == "0.00");
    assert(formatCentsToAmount(5) == "0.05");
    assert(formatCentsToAmount(123) == "1.23");
    assert(formatCentsToAmount(12345) == "123.45");
}

// pomocnicza funkcja do sprawdzania ze formatowanie rzuca wyjatek
static void expect_format_throw(long long cents) {
    bool threw = false;
    try {
        (void)formatCentsToAmount(cents);
    } catch (const std::exception&) {
        threw = true;
    }
    assert(threw);
}

// testuje przypadki formatowania ktore powinny rzucic wyjatek
static void test_format_invalid() {
    expect_format_throw(-1);
}

// testuje czy parsowanie i formatowanie sa ze soba spojne
static void test_roundtrip() {
    const long long values[] = {
        0, 1, 5, 10, 99, 100, 123, 9999, 12345, 999999
    };

    for (long long v : values) {
        std::string s = formatCentsToAmount(v);
        long long back = parseAmountToCents(s);
        assert(back == v);
    }
}

int main() {
    std::cout << "test money_utils..." << std::endl;

    test_parse_valid();
    test_parse_invalid();
    test_format_valid();
    test_format_invalid();
    test_roundtrip();

    std::cout << "OK\n";
    return 0;
}



/* kompilacja i uruchomienie pojedynczego testu (opcjonalnie uruchomienie wszystkich testow za pomoca cmake w README.md)

cd do katalogu projektu, nastepnie

g++ -std=c++17 -Wall -Wextra \
    -Isrc \
    src/money_utils.cpp tests/test_money_utils.cpp \
    -o tests/test_money_utils

./tests/test_money_utils



*/