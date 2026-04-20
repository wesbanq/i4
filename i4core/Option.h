#pragma once

enum class Option : unsigned char {
	NONE	= 0, 
	VERBOSE	= 1 << 0,
	LIMIT	= 1 << 1,
	BOX		= 1 << 2,
	NOFS	= 1 << 3,
	NOWEB	= 1 << 4,
    MEMORY	= 1 << 5,
	STEP	= 1 << 6,
	DONTRUN = LIMIT | MEMORY | STEP,
	NOEXT	= NOFS | NOWEB,
	SAFE	= LIMIT | NOEXT,
	DEBUG	= MEMORY | STEP | SAFE,
};