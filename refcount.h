#ifndef REFCOUNT_H
#define REFCOUNT_H

#include <utility>

// Reference counting GC "algorithm"

namespace gc {
    template <typename T>
    class refcount_object {
        struct storage {
            std::size_t count{};
            T data;
        };

        storage* _s = nullptr;

        friend class refcount;

        // Private constructor for refcount class
        constexpr refcount_object(storage* s) : _s { s } { }

        public:
        // Copy -> increate refcount
        constexpr refcount_object(const refcount_object& other) : _s { other._s } {
            _s->count += 1;
        }

        constexpr refcount_object& operator=(const refcount_object& other) {
            if (this != &other) {
                _s = other._s;
                _s->count += 1;
            }
            
            return *this;
        }

        // Move -> keep refcount
        constexpr refcount_object(refcount_object&& other) noexcept
            : _s { std::exchange(other._s, nullptr) } { }

        constexpr refcount_object& operator=(refcount_object&& other) noexcept {
            _s = std::exchange(other._s, nullptr);

            return *this;
        }

        // Decrease refcount and possibly free memory
        ~refcount_object() {
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
            const refcount_object& lhs, const refcount_object& rhs) {

            return lhs._s <=> &rhs._s;
        }

        [[nodiscard]] constexpr friend bool operator==(
            const refcount_object& lhs, const refcount_object& rhs) {

            return lhs._s == rhs._s;
        }

        [[nodiscard]] constexpr explicit operator bool() const {
            return _s;
        }
    };

    // Use a class instead of a namespace to allow the testing function to be templated
    // TODO use allocator instead of new
    class refcount {
        public:
        // Only allow use of the static functions
        refcount() = delete;
        refcount(const refcount&) = delete;
        refcount(refcount&&) = delete;

        template <std::destructible T, typename... Args>
        [[nodiscard]] static constexpr refcount_object<T> allocate(Args&&... args) {
            return {
                new typename refcount_object<T>::storage{
                    1, { std::forward<Args>(args)... }
                }
            };
        }
    };
}

#endif /* REFCOUNT_H */