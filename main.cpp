#include "refcount.h"

#include <iostream>
#include <array>
#include <string>

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

	template <typename T1, typename T2, typename T3>
	struct helper2 {
		T1 v1;
		T2 v2;
		T3 v3;

		helper2(T1 t1, T2 t2, T3 t3) : v1{ std::move(t1) }, v2{ std::move(t2) }, v3 { std::move(t3) } {
			std::cout << "    [helper2 object] Constructor called with:\n"
				"        " << *v1 << "\n"
				"        " << *v2 << "\n"
				"        " << *v3 << '\n';
		}

		~helper2() {
			std::cout << "    [helper2 object] Destructor called with: \n"
				"        " << *v1 << "\n"
				"        " << *v2 << "\n"
				"        " << *v3 << '\n';
		}
	};

	template <typename T1, typename T2>
	struct helper3 {
		T1 v1;
		T2 v2;

		helper3(T1 t1, T2 t2) : v1{ std::move(t1) }, v2{ std::move(t2) } {
			std::cout << "    [helper3 object] Constructor called with:\n"
				"        " << *v1 << "\n"
				"        " << *v2.lock() << '\n';
		}

		~helper3() {
			std::cout << "    [helper3 object] Destructor called with:\n"
				"        " << *v1 << "\n"
				"        " << *v2.lock() << '\n';
		}
	};
}

template <typename T, size_t N> requires std::integral<T> || std::floating_point<T>
std::ostream& operator<<(std::ostream& os, const std::array<T, N>& arr) {
	std::ostream_iterator<T> out(os, ", ");
	std::copy_n(arr.begin(), arr.size(), out);

	return os;
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
		
		log() << "Starting tests...\n";
		
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

		ident() << '\n';

		{
			ident(1) << "Creating a struct with multiple, nested members\n";

			using int_field = typename T::template object_type<int>;
			using string_field = typename T::template object_type<std::string>;
			using float_field = typename T::template object_type<std::array<float, 8>>;
			using struct_type = test::helper2<int_field, string_field, float_field>;

			auto s = alloc.template allocate<struct_type>(
				alloc.template allocate<int>(45),
				alloc.template allocate<std::string>("foo"),

				// Not a very clean interface...
				alloc.template allocate<std::array<float, 8>>(
					std::array<float, 8> { 1.f, 1.f, 2.f, 3.f, 5.f, 8.f, 13.f, 21.f })
				);

			ident(1) << "Deallocating said struct.\n";
		}

		ident() << '\n';

		{
			ident(1) << "Testing weak references.\n";

			typename T::template weak_type<test::helper> weak1;
			{
				ident(1) << "Creating strong reference and assigning weak reference.\n";

				auto strong = alloc.template allocate<test::helper>(46);
				weak1 = strong;

				{
					ident(1) << "Creating second weak reference.\n";
					decltype(weak1) weak2 = strong;
					ident(1) << "Second weak reference going out of scope.\n";
				}

				ident(1) << "Strong reference going out of scope.\n";
			}

			ident(1) << "Last weak reference going out of scope.\n";
		}
		
		ident() << '\n';

		{
			ident(1) << "Creating a struct with 1 strong and 1 weak pointer.\n";

			using strong_field = typename T::template object_type<std::string>;
			using weak_field = typename T::template weak_type<std::string>;
			using struct_type = test::helper3<strong_field, weak_field>;

			typename T::template object_type<struct_type> obj;

			{
				auto ptr = alloc.template allocate<std::string>("bar");

				// Make strong and weak pointer
				obj = alloc.template allocate<struct_type>(ptr, ptr);
				ident(1) << "Deallocating temporary object\n";
			}

			ident(1) << "Deallocating struct\n";
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
