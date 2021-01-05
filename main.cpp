#include "refcount.h"

#include <iostream>

#ifdef _MSC_VER
// MSVC typeid(T).name() is unmangled
template <typename T>
std::string type_name() {
	return typeid(T).name();
}
#else
// Use utility function on gcc
#include <cxxabi.h>
template <typename T>
std::string type_name() {
	int dummy;
	char* name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &dummy);
	std::string actual_name = name; // Newly allocated copy

	free(name);

	return actual_name;
}
#endif

namespace test {
	struct helper {
		std::size_t value{};

		helper(std::size_t v) : value{ v } {
			std::cout << "    [helper object] Constructor called with v = " << v << '\n';
		}

		~helper() {
			std::cout << "    [helper object] Destructor called, v = " << value << '\n';
		}
	};
}

template <typename T>
bool test_allocator() {
	const auto allocator_name = type_name<T>();

	auto ident = [](std::size_t i = 0) -> std::ostream& {
		return std::cout << std::string(i * 4, ' ');
	};
	
	auto log = [&](std::size_t i = 0) -> std::ostream& {
		return ident(i) << '[' << allocator_name << "] ";
	};

	try {
		T alloc;
		
		log() << "Starting test...\n";
		
		{
			ident(1) << "Basic object allocation and deallocation\n";
			auto x = alloc.template allocate<test::helper>(42);
			ident(1) << "Allocated at " << x.get() << ", value: " << x->value << '\n';
		}

		ident() << '\n';
		
		{
			ident(1) << "Basic object, with refcount increase\n";
			// 1 instance and 4 copies
			auto instance = alloc.template allocate<test::helper>(43);

			{
				std::remove_cvref_t<
					decltype(alloc.template allocate<test::helper>(42))> elements[4];
				
				for (auto& c : elements) {
					c = instance;
				}

				ident(1) << "Values:\n";
				for (const auto& c : elements) {
					ident(2) << "value = " << c->value << '\n';
				}

				ident(1) << "Deleting copies\n";
			}
			
			ident(1) << "Deleting final reference\n";
		}

		ident() << '\n';
		
		{
			ident(1) << "Moving from 1 instance to another\n";
			auto x = alloc.template allocate<test::helper>(44);

			ident(1) << "Address and value held by old object: "
				<< x->value << " at " << x.get() << '\n';
			
			ident(1) << "Moving...\n";
			auto y = std::move(x);

			ident(1) << "Address and value held by new object: "
				<< y->value << " at " << y.get() << '\n';

			ident(1) << "Deleting\n";
		}

	} catch (std::exception& e) {
		log() << "Exception caught: " << e.what() << '\n';
		return false;
	}

	return true;
}

int main() {
	bool success = true;

	// Try all allocators
	if (!test_allocator<gc::refcount::allocator>()) {
		success = false;
	}
	
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
