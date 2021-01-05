#include "refcount.h"

#include <iostream>

int main() {
	auto s = gc::refcount::allocate<std::string>("foo");

	std::cout << '[' << s->size() << "] " << *s << " at " << s.get()
		<< " (valid: " << static_cast<bool>(s) << ")\n";
	
	return 0;
}
