#include <cassert>
#include <iostream>
#include <string>

#include "novitus_protocol.h"
#include "money_utils.h"

// pomocnicza funkcja do wyciecia body z ramki
// przyjmujemy format: ESC 'P' + body + crc(2 znaki) + ESC '\'
static std::string extract_body(const std::string& frame) {
    assert(frame.size() >= 6);
    const std::size_t body_len = frame.size() - 2 /*ESC P*/ - 2 /*CRC*/ - 2 /*ESC \*/;
    return frame.substr(2, body_len);
}

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

int main() {
    using namespace novitus;

    std::cout << "test novitus_protocol..." << std::endl;

    // scenariusz 1: poczatek transakcji 0$h
    {
        std::string frame = makeBeginTransactionOnline();
        assert(frame.size() >= 6);
        assert(static_cast<unsigned char>(frame[0]) == 0x1B);
        assert(frame[1] == 'P');

        std::string body = extract_body(frame);
        assert(body == "0$h");
    }

    // scenariusz 2: linia paragonu prosty przypadek
    {
        std::string frame = makeSaleLineSimple("KAWA", 'A', 850, 850);
        std::string body = extract_body(frame);

        std::string expected = "1$lKAWA";
        expected.push_back('\r');
        expected += "1";
        expected.push_back('\r');
        expected += "A/8.50/8.50/";

        assert(body == expected);
    }

    // scenariusz 3: rabat kwotowy Y
    {
        std::string frame = makeReceiptDiscountY(1234, 200, "RABAT");
        std::string body = extract_body(frame);

        std::string expected = "3;0$Y12.34/2.00/RABAT";
        expected.push_back('\r');

        assert(body == expected);
    }

    // scenariusz 4: standardowe zakonczenie paragonu
    {
        std::string frame = makeStandardEnd(1034, "KASJER");
        std::string body = extract_body(frame);

        std::string expected = "1;0$eKASJER";
        expected.push_back('\r');
        expected += "10.34/10.34/";

        assert(body == expected);
    }

    // scenariusz 5: niepoprawne dane dla linii paragonu
    {
        expect_throw([] {
            makeSaleLineSimple("X", 'A', -1, 100);
        });

        expect_throw([] {
            makeSaleLineSimple("X", 'A', 100, -1);
        });
    }

    // scenariusz 6: niepoprawne dane dla rabatu Y
    {
        // rabat zero lub ujemny
        expect_throw([] {
            makeReceiptDiscountY(1000, 0, "R");
        });

        // ujemna podsuma
        expect_throw([] {
            makeReceiptDiscountY(-1, 100, "R");
        });

        // rabat wiekszy niz podsuma
        expect_throw([] {
            makeReceiptDiscountY(1000, 2000, "R");
        });
    }

    // scenariusz 7: niepoprawna kwota koncowa w makeStandardEnd
    {
        expect_throw([] {
            makeStandardEnd(-1, "KASJER");
        });
    }

    std::cout << "OK\n";
    return 0;
}



/* kompilacja i uruchomienie pojedynczego testu (opcjonalnie uruchomienie wszystkich testow za pomoca cmake w README.md)

cd do katalogu projektu, nastepnie

g++ -std=c++17 -Wall -Wextra \
    -Isrc \
    src/novitus_protocol.cpp src/money_utils.cpp tests/test_novitus_protocol.cpp \
    -o tests/test_novitus_protocol

./tests/test_novitus_protocol




*/