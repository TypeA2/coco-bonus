#include "refcount.h"

#include <iostream>

template <typename T>
concept gc_allocator = requires {
	T::template allocate<std::string>();
};

template <gc_allocator T>
bool test_allocator() {
	try {
		auto s = T::template allocate<std::string>("foo");

		std::cout << '[' << s->size() << "] " << *s << " at " << s.get()
			<< " (valid: " << static_cast<bool>(s) << ")\n";

	} catch (std::exception& e) {
		std::cerr << '[' << typeid(T).name() << "] Exception caught: " << e.what() << '\n';
		return false;
	}

	return true;
}

int main() {
	bool success = true;

	if (!test_allocator<gc::refcount>()) {
		success = false;
	}
	
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
