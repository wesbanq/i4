#pragma once

#include <iostream>
#include <string>
#include <vector>

class Interpreter {
private:
	std::istream& InputStream;
	std::ostream& OutputStream;
	unsigned char Options;
public:
	enum Args : unsigned char { 
		NONE	= 0, 
		VERBOSE	= 1 << 0,
		SAFE	= 1 << 1,
		BOX		= 1 << 2,
		NOFS	= 1 << 3,
		NOWEB	= 1 << 4,
		HELP	= 1 << 5,
		VERSION	= 1 << 6,
	};

	Interpreter(std::istream& inputStream, std::ostream& outputStream, unsigned char options);

	int Run(int _argc, std::vector<std::string> _argv);
	bool HasOption(Args opt) const;
};
