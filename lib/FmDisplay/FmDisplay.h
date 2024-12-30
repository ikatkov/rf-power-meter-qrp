#ifndef Fm_Display_h
#define Fm_Display_h
#include <Arduino.h>

class FmDisplay
{
public:
    FmDisplay(uint8_t clk, uint8_t dio, uint8_t cs);
    void start();
    void clear();
    void display_int(const int value, const bool leading_spaces = false);
    void display_symbols(const int data[]);
    void display_show_dot(bool value);
    void sendRawData(uint8_t bitOrder, int *data, uint8_t word_length, uint8_t length);
    void sendInitData(uint8_t bitOrder, int val, uint8_t word_length);

private:
    uint16_t lastData[4];
    uint16_t point_data;

    uint8_t clkPin;
    uint8_t dataPin;
    uint8_t csPin;

    void display(uint8_t DispData[]);

    void wordShiftOut(uint8_t bitOrder, int val, uint8_t word_length);
};

const int segment_b = 0b010000000000;
const int segment_c = 0b001000000000;
const int segment_dp = 0b000100000000;
const int segment_g = 0b000001000000;
const int segment_d = 0b000000100000;
const int segment_a = 0b000000010000;
const int segment_f = 0b000000000100;
const int segment_e = 0b000000000010;

enum display_symbols
{
    _0 = segment_a | segment_b | segment_c | segment_d | segment_e | segment_f,
    _1 = segment_b | segment_c,
    _2 = segment_a | segment_b | segment_d | segment_e | segment_g,
    _3 = segment_a | segment_b | segment_c | segment_d | segment_g,
    _4 = segment_b | segment_c | segment_f | segment_g,
    _5 = segment_a | segment_c | segment_d | segment_f | segment_g,
    _6 = segment_a | segment_c | segment_d | segment_e | segment_f | segment_g,
    _7 = segment_a | segment_b | segment_c,
    _8 = segment_a | segment_b | segment_c | segment_d | segment_e | segment_f | segment_g,
    _9 = segment_a | segment_b | segment_c | segment_d | segment_f | segment_g,
    _space = 0,
    _dash = segment_g,
    _A = segment_a | segment_b | segment_c | segment_e | segment_f | segment_g,
    _a = segment_a | segment_b | segment_c | segment_d | segment_e | segment_g,
    _b = segment_c | segment_d | segment_e | segment_f | segment_g,
    _C = segment_a | segment_d | segment_e | segment_e | segment_f,
    _c = segment_d | segment_e | segment_g,
    _d = segment_b | segment_c | segment_d | segment_e | segment_g,
    _E = segment_a | segment_d | segment_e | segment_f | segment_g,
    _F = segment_a | segment_e | segment_f | segment_g,
    _G = segment_a | segment_c | segment_d | segment_e | segment_f,
    _H = segment_b | segment_c | segment_e | segment_f | segment_g,
    _h = segment_c | segment_e | segment_f | segment_g,
    _I = segment_e | segment_f,
    _J = segment_b | segment_c | segment_d | segment_e,
    _L = segment_d | segment_e | segment_f,
    _n = segment_c | segment_e | segment_g,
    _O = segment_a | segment_b | segment_c | segment_d | segment_e | segment_f,
    _o = segment_c | segment_d | segment_e | segment_g,
    _P = segment_a | segment_b | segment_e | segment_f | segment_g,
    _q = segment_a | segment_b | segment_c | segment_f | segment_g,
    _r = segment_e | segment_g,
    _S = segment_a | segment_c | segment_d | segment_f | segment_g,
    _t = segment_d | segment_e | segment_f | segment_g,
    _U = segment_b | segment_c | segment_d | segment_e | segment_f,
    _u = segment_c | segment_d | segment_e,
    _y = segment_b | segment_c | segment_d | segment_f | segment_g,
    _degree = segment_a | segment_b | segment_f | segment_g

};

const int symbols[] = {_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _space, _dash};

#endif