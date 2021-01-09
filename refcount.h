#ifndef REFCOUNT_H
#define REFCOUNT_H

#include <utility>
#include <ostream>
#include <iterator>

// Reference counting GC "algorithm"
namespace gc::refcount {
    template <typename T>
    class object {
        struct storage {
            std::size_t count{};
            T data;
        };

        storage* _s = nullptr;

        friend struct allocator;

        // Private constructor for refcount and weak classes
        constexpr object(storage* s) : _s { s } { }

        public:
        class weak_object {
            storage* _s = nullptr;

            public:
            constexpr weak_object() = default;

            // Convert from strong object
            constexpr weak_object(const object& o) : _s { o._s } { }

            constexpr weak_object(const weak_object& other) : _s{ other._s } { }
            constexpr weak_object& operator=(const weak_object& other) {
                if (this != &other) {
                    _s = other._s;
                }

                return *this;
            }

            constexpr weak_object(weak_object&& other) noexcept
                : _s { std::exchange(other._s, nullptr) } { }
            constexpr weak_object& operator=(weak_object&& other) noexcept {
                _s = std::exchange(other._s, nullptr);
                return *this;
            }

            [[nodiscard]] constexpr object lock() const {
                if (!_s) {
                    return { nullptr };
                }

                _s->count += 1;

                return { _s };
            }
        };

        // Default, empty constructor
        constexpr object() = default;

        // Copy -> increate refcount
        constexpr object(const object& other) : _s{ other._s } {
            _s->count += 1;
        }

        constexpr object& operator=(const object& other) {
            if (this != &other) {
                _s = other._s;
                _s->count += 1;
            }
            
            return *this;
        }

        // Move -> keep refcount
        constexpr object(object&& other) noexcept
            : _s { std::exchange(other._s, nullptr) } { }

        constexpr object& operator=(object&& other) noexcept {
            _s = std::exchange(other._s, nullptr);

            return *this;
        }

        // Decrease refcount and possibly free memory
        ~object() {
            if (!_s) {
                return;
            }

            _s->count -= 1;

            if (_s->count == 0) {
                delete _s;
            }
        }


        // Dereference to the object
        [[nodiscard]] constexpr T& operator*() const {
            return _s->data;
        }

        [[nodiscard]] constexpr T* operator->() const {
            return &_s->data;
        }

        [[nodiscard]] constexpr T* get() const {
            return &_s->data;
        }


        // Compare
        [[nodiscard]] constexpr friend std::strong_ordering operator<=>(
            const object& lhs, const object& rhs) {

            return lhs._s <=> &rhs._s;
        }

        [[nodiscard]] constexpr friend bool operator==(
            const object& lhs, const object& rhs) {

            return lhs._s == rhs._s;
        }

        [[nodiscard]] constexpr explicit operator bool() const {
            return _s;
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
            return {
                new typename object<T>::storage{
                    1, T(std::forward<Args>(args)...)
                }
            };
        }
    };
}


#endif /* REFCOUNT_H */