#include "Interpreter.h"

Interpreter::Interpreter(std::istream& inputStream, std::ostream& outputStream, unsigned char options) 
	: InputStream(inputStream), OutputStream(outputStream), Options(options) { }

int Interpreter::Run(int _argc, std::vector<std::string> _argv) {
	

	return 0;
}

bool Interpreter::HasOption(Args opt) const {
	return (Options & opt) != 0;
}
