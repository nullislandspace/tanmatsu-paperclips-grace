#include "game_format.h"

#include <math.h>
#include <stdio.h>

static const char* suffixes[] = {
    "",                   // < 1e3
    " thousand",          // 1e3
    " million",           // 1e6
    " billion",           // 1e9
    " trillion",          // 1e12
    " quadrillion",       // 1e15
    " quintillion",       // 1e18
    " sextillion",        // 1e21
    " septillion",        // 1e24
    " octillion",         // 1e27
    " nonillion",         // 1e30
    " decillion",         // 1e33
    " undecillion",       // 1e36
    " duodecillion",      // 1e39
    " tredecillion",      // 1e42
    " quattuordecillion", // 1e45
    " quindecillion",     // 1e48
    " sexdecillion",      // 1e51
};

#define NUM_SUFFIXES (sizeof(suffixes) / sizeof(suffixes[0]))

void number_cruncher(double value, int decimals, char* buf, size_t size) {
    if (size == 0) return;

    if (isnan(value)) { snprintf(buf, size, "NaN"); return; }
    if (isinf(value)) { snprintf(buf, size, value > 0 ? "Inf" : "-Inf"); return; }

    const char* sign = "";
    if (value < 0) {
        sign = "-";
        value = -value;
    }

    if (value < 1000.0) {
        if (decimals > 0) {
            snprintf(buf, size, "%s%.*f", sign, decimals, value);
        } else {
            snprintf(buf, size, "%s%.0f", sign, value);
        }
        return;
    }

    int idx = 0;
    double scaled = value;
    while (scaled >= 1000.0 && idx < (int)NUM_SUFFIXES - 1) {
        scaled /= 1000.0;
        idx++;
    }

    snprintf(buf, size, "%s%.2f%s", sign, scaled, suffixes[idx]);
}

void time_cruncher(double ticks, char* buf, size_t size) {
    if (size == 0) return;

    double seconds = ticks / 100.0;
    int    hours   = (int)(seconds / 3600.0);
    int    minutes = (int)(fmod(seconds, 3600.0) / 60.0);
    int    secs    = (int)(fmod(seconds, 60.0));

    if (hours > 0) {
        snprintf(buf, size, "%d hours %d minutes %d seconds", hours, minutes, secs);
    } else if (minutes > 0) {
        snprintf(buf, size, "%d minutes %d seconds", minutes, secs);
    } else {
        snprintf(buf, size, "%d seconds", secs);
    }
}

void format_currency(double value, char* buf, size_t size) {
    if (size == 0) return;

    if (value < 1000.0) {
        snprintf(buf, size, "$%.2f", value);
    } else {
        char numbuf[64];
        number_cruncher(value, 2, numbuf, sizeof(numbuf));
        snprintf(buf, size, "$%s", numbuf);
    }
}
