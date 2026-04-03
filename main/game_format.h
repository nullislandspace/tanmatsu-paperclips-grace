#pragma once

#include <stddef.h>

// Format a large number with magnitude suffixes (thousand, million, ... sexdecillion)
// decimals: number of decimal places (0 for values < 1000, typically 2)
void number_cruncher(double value, int decimals, char* buf, size_t size);

// Format tick count as "X hours Y minutes Z seconds"
void time_cruncher(double ticks, char* buf, size_t size);

// Format a dollar amount as "$X.XX" with magnitude suffix
void format_currency(double value, char* buf, size_t size);
