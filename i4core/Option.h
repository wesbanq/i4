#pragma once

enum class Option : unsigned char {
	NONE	= 0, 
	VERBOSE	= 1 << 0,
	LIMIT	= 1 << 1,
	BOX		= 1 << 2,
	NOFS	= 1 << 3,
    MEMORY	= 1 << 4,
	STEP	= 1 << 5,
	DONTRUN = LIMIT | MEMORY | STEP,
	SAFE	= LIMIT | NOFS,
	DEBUG	= MEMORY | STEP | SAFE,
};