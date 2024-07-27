#pragma once
#ifndef TEXT_BUFFER_H
#define TEXT_BUFFER_H

#include "Arduino.h"

class TextBuffer {
    #define HEIGHT 6
    #define WIDTH 32

    char text_buffer[HEIGHT][WIDTH];  // buffer for ASCII text

    public:
        TextBuffer();
        void clear();
        void set_char(uint8_t row, uint8_t col);
};

#endif