#pragma once

enum class Option : unsigned char {
	NONE	= 0, 
	VERBOSE	= 1 << 0,
	LIMIT	= 1 << 1,
	BOX		= 1 << 2,
	NOFS	= 1 << 3,
	NOWEB	= 1 << 4,
	HELP	= 1 << 5,
	VERSION	= 1 << 6,
    DEBUG	= 1 << 7,
	SAFE	= LIMIT | NOFS | NOWEB,
	DONTRUN = LIMIT | DEBUG,
};