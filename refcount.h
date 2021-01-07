#ifndef REFCOUNT_H
#define REFCOUNT_H

#include <utility>
#include <ostream>

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

        // Private constructor for refcount class
        constexpr object(storage* s) : _s { s } { }

        public:
        // Default, empty constructor
        constexpr object() = default;

        // Copy -> increate refcount
        constexpr object(const object& other) : _s { other._s } {
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
            return os << *o;
        }
    };

    // Allocator interface
    struct allocator {
        template <std::destructible T>
        using object_type = object<T>;

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