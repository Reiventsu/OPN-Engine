#pragma once
#include <atomic>
#include <new>
#include <bit>
#include <cstddef> // for std::byte

// ABI safety
#ifdef __cpp_lib_hardware_interference_size
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

namespace opn {
    /**
     * @brief A raw-memory, lock-free, Single-Producer Single-Consumer (SPSC) Ring Buffer.
     *
     * Unlike the standard SPSCQueue, this version uses uninitialized memory (`std::byte` array)
     * and placement new. This avoids it default-constructing `T` for every slot at startup, making it
     * ideal for expensive-to-construct or non-default-constructible types.
     *
     * @tparam T The type of data to store.
     * @tparam Size The capacity of the queue. MUST be a power of two.
     */
    template<typename T, size_t Size>
    class RawSPSCQueue {
        static_assert(std::has_single_bit(Size), "Size must be a power of two for bitwise optimization.");

        static_assert(std::is_nothrow_move_assignable_v<T>,
                      "T must have a noexcept move assignment operator for RawSPSCQueue safety.");

    public:
        /**
         * @brief Constructs the raw buffer indices.
         * @note Does NOT construct any elements of type T.
         */
        constexpr RawSPSCQueue() noexcept
            : m_head(0), m_tail(0) {
        };

        // Disable Copy/Move to prevent double-free logic errors with raw memory
        RawSPSCQueue(const RawSPSCQueue &) = delete;

        RawSPSCQueue &operator=(const RawSPSCQueue &) = delete;

        RawSPSCQueue(RawSPSCQueue &&) = delete;

        RawSPSCQueue &operator=(RawSPSCQueue &&) = delete;

        /**
         * @brief Destructor.
         * Manually calls the destructor for any elements still resident in the queue.
         */
        ~RawSPSCQueue() noexcept {
            size_t currentTail = m_tail.load(std::memory_order_relaxed);
            const size_t head = m_head.load(std::memory_order_relaxed);

            while (currentTail != head) {
                T *itemPtr = reinterpret_cast<T *>(data + currentTail * sizeof(T));
                itemPtr->~T();
                currentTail = (currentTail + 1) & (Size - 1);
            }
        }

        /**
         * @brief Pushes a copy of an item into the queue.
         * 
         * Constructs the item in-place using the copy constructor.
         * @thread_safety PRODUCER thread ONLY.
         * @param _item The item to copy.
         * @return true If success, false if full.
         */
        [[nodiscard]] bool push(const T &_item)
            noexcept(std::is_nothrow_copy_constructible_v<T>) {
            return pushImpl(_item);
        }

        /**
         * @brief Pushes a moved item into the queue.
         * 
         * Constructs the item in-place using the move constructor.
         * @thread_safety PRODUCER thread ONLY.
         * @param _item The item to move.
         * @return true If success, false if full.
         */
        [[nodiscard]] bool push(T &&_item)
            noexcept(std::is_nothrow_move_constructible_v<T>) {
            return pushImpl(std::move(_item));
        }

        /**
         * @brief Pops an item from the queue.
         *
         * Moves the item out, then manually destroys the in-place object to free the slot.
         * @thread_safety CONSUMER thread ONLY.
         * @param _out_item Destination for the popped item.
         * @return true If success, false if empty.
         */
        [[nodiscard]] bool pop(T &_out_item)
            noexcept(std::is_nothrow_move_assignable_v<T>) {
            const auto currentTail = m_tail.load(std::memory_order_relaxed);

            if (currentTail == m_head.load(std::memory_order_acquire)) {
                return false; // Empty
            }

            // Get address of constructed object
            T *itemPtr = reinterpret_cast<T *>(data + currentTail * sizeof(T));

            // Move out
            _out_item = std::move(*itemPtr);

            // Manually destroy object in buffer
            itemPtr->~T();

            const auto nextTail = (currentTail + 1) & (Size - 1);
            m_tail.store(nextTail, std::memory_order_release);
            return true;
        }

        /**
         * @brief Peeks at the next item without removing it.
         * @thread_safety CONSUMER thread ONLY.
         * @return Pointer to the next item, or nullptr if empty.
         */
        [[nodiscard]] const T *peek() const {
            const auto currentTail = m_tail.load(std::memory_order_relaxed);
            if (currentTail == m_head.load(std::memory_order_acquire)) {
                return nullptr;
            }
            return reinterpret_cast<const T *>(data + currentTail * sizeof(T));
        }

        /**
         * @brief Checks if the queue is empty.
         * @thread_safety Safe to call from either thread.
         */
        [[nodiscard]] bool isEmpty() const {
            return m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire);
        }

        /**
         * @brief Calculates available write slots.
         * @thread_safety PRODUCER thread ONLY (accurate).
         */
        [[nodiscard]] size_t availableWrite() const {
            const size_t head = m_head.load(std::memory_order_relaxed);
            const size_t tail = m_tail.load(std::memory_order_acquire);

            if (head >= tail) {
                return Size - 1 - (head - tail);
            }
            return tail - head - 1;
        }

        // --- Operator Overloads ---

        bool operator<<(T &&_item) { return push(std::move(_item)); }
        bool operator<<(const T &_item) { return push(_item); }
        bool operator>>(T &_out_item) { return pop(_out_item); }
        bool operator!() const { return isEmpty(); }
        explicit operator bool() const { return !isEmpty(); }

    private:
        /**
         * @brief Internal push implementation using Placement New.
         * Supports both copy and move construction via forwarding references.
         */
        template<typename U>
        bool pushImpl(U &&_item) noexcept(std::is_nothrow_constructible_v<T, U &&>) {
            const auto currentHead = m_head.load(std::memory_order_relaxed);
            const auto nextHead = (currentHead + 1) & (Size - 1);

            if (nextHead == m_tail.load(std::memory_order_acquire)) {
                return false;
            }

            void *slotPtr = data + currentHead * sizeof(T);
            new(slotPtr) T(std::forward<U>(_item));

            m_head.store(nextHead, std::memory_order_release);
            return true;
        }

        alignas(hardware_destructive_interference_size) std::atomic<size_t> m_head;
        alignas(hardware_destructive_interference_size) std::atomic<size_t> m_tail;

        // Raw byte storage aligned to T
        alignas(T) std::byte data[Size * sizeof(T)];
    };
}
