#ifndef REFCOUNT_MANAGED_H
#define REFCOUNT_MANAGED_H

#include <utility>
#include <ostream>
#include <unordered_map>

// Reference counting GC with a global registry

namespace gc::refcount_managed {
    class registry {
        using reg_type = std::unordered_map<void*, std::size_t>;

        // Holds the registry itself
        static reg_type& _registry() {
            static reg_type reg;
            return reg;
        }

        public:

        // Register a new pointer with a refcount of 1
        static void register_instance(void* ptr) {
            _registry().emplace(ptr, 1);
        }

        // Increase a pointer's refcount
        static void increase(void* ptr) {
            _registry().at(ptr) += 1;
        }

        // Decrease a pointer's refcount. If it reaches zero, it is removed and true is returned,
        // to signal that the object should be destructed.
        [[nodiscard]] static bool decrease(void* ptr) {
            auto v = _registry().find(ptr);

            v->second -= 1;

            if (v->second == 0) {
                _registry().erase(v);
                return true;
            }

            return false;
        }
    };

    template <typename T>
    class object {
        T* _ptr = nullptr;

        friend struct allocator;

        // Private constructor for refcount and weak classes
        constexpr object(T* ptr) : _ptr{ ptr } {}

        public:
        class weak_object {
            T* _ptr = nullptr;

            public:
            constexpr weak_object() = default;

            // Convert from strong object
            constexpr weak_object(const object& o) : _ptr{ o._ptr } {}

            constexpr weak_object(const weak_object& other) : _ptr{ other._ptr } {}
            constexpr weak_object& operator=(const weak_object& other) {
                if (this != &other) {
                    _ptr = other._ptr;
                }

                return *this;
            }

            constexpr weak_object(weak_object&& other) noexcept
                : _ptr{ std::exchange(other._ptr, nullptr) } {
            }
            constexpr weak_object& operator=(weak_object&& other) noexcept {
                _ptr = std::exchange(other._ptr, nullptr);
                return *this;
            }

            [[nodiscard]] constexpr object lock() const {
                if (!_ptr) {
                    return { nullptr };
                }

                registry::increase(_ptr);

                return { _ptr };
            }
        };

        // Default, empty constructor
        constexpr object() = default;

        // Copy -> increate refcount
        constexpr object(const object& other) : _ptr{ other._ptr } {
            registry::increase(_ptr);
        }

        constexpr object& operator=(const object& other) {
            if (this != &other) {
                _ptr = other._ptr;

                registry::increase(_ptr);
            }

            return *this;
        }

        // Move -> keep refcount
        constexpr object(object&& other) noexcept
            : _ptr{ std::exchange(other._ptr, nullptr) } {
        }

        constexpr object& operator=(object&& other) noexcept {
            _ptr = std::exchange(other._ptr, nullptr);

            return *this;
        }

        // Decrease refcount and possibly free memory
        ~object() {
            if (!_ptr) {
                return;
            }

            if (registry::decrease(_ptr)) {
                delete _ptr;
            }
        }


        // Dereference to the object
        [[nodiscard]] constexpr T& operator*() const {
            return *_ptr;
        }

        [[nodiscard]] constexpr T* operator->() const {
            return _ptr;
        }

        [[nodiscard]] constexpr T* get() const {
            return _ptr;
        }


        // Compare
        [[nodiscard]] constexpr friend std::strong_ordering operator<=>(
            const object& lhs, const object& rhs) {

            return lhs._ptr <=> &rhs._ptr;
        }

        [[nodiscard]] constexpr friend bool operator==(
            const object& lhs, const object& rhs) {

            return lhs._ptr == rhs._ptr;
        }

        [[nodiscard]] constexpr explicit operator bool() const {
            return _ptr;
        }

        friend std::ostream& operator<<(std::ostream& os, const object& o) {
            return os << o.get();
        }
    };

    // Allocator interface
    struct allocator {
        template <std::destructible T>
        using object_type = object<T>;

        template <std::destructible T>
        using weak_type = typename object<T>::weak_object;

        template <std::destructible T, typename... Args>
        [[nodiscard]] constexpr object<T> allocate(Args&&... args) {

            T* ptr = new T(std::forward<Args>(args)...);

            registry::register_instance(ptr);

            return { ptr };
        }
    };
}

#endif /* REFCOUNT_MANAGED_H */
