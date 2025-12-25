module;

#include <atomic>
#include <new>
#include <bit>

export module opn.System.Thread.SPSCQueue;

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

export namespace opn {
    /**
     * @brief A high-performance, lock-free, Single-Producer Single-Consumer (SPSC) Ring Buffer.
     *
     * This queue is designed for communicating between two specific threads (e.g., Main Thread -> Render Thread)
     * without using mutexes. It uses atomic operations and memory barriers to ensure thread safety.
     *
     * @tparam T The type of data to store. Must be default-constructible and movable/copyable.
     * @tparam Size The capacity of the queue. MUST be a power of two (e.g., 64, 1024).
     */
    template<typename T, size_t Size>
    class SPSCQueue {
        static_assert(std::has_single_bit(Size));
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_default_constructible_v<T>);

    public:
        /**
         * @brief Constructs the ring buffer.
         * 
         * Initializes the head and tail indices to 0.
         * This constructor is thread-safe only if called before the threads start using the queue.
         */
        constexpr SPSCQueue() noexcept
            : m_head(0), m_tail(0) {
        };


        /**
         * @brief Pushes an item into the queue (Copy version).
         *
         * Thread Safety: PRODUCER thread ONLY.
         *
         * @param _item The item to copy into the queue.
         * @return true If the item was successfully added.
         * @return false If the queue is full.
         */
        [[nodiscard]] bool push(const T &_item)
            noexcept(std::is_nothrow_copy_assignable_v<T>) {
            return pushImpl(_item);
        }

        /**
        * @brief Pushes an item into the queue (Move version).
        *
        * Thread Safety: PRODUCER thread ONLY.
        *
        * @param _item The item to move into the queue.
        * @return true If the item was successfully added.
        * @return false If the queue is full.
        */
        [[nodiscard]] bool push(T &&_item)
            noexcept(std::is_nothrow_move_assignable_v<T>) {
            return pushImpl(std::move(_item));
        }

        /**
         * @brief Pops an item from the queue.
         *
         * Thread Safety: CONSUMER thread ONLY.
         *
         * @param _out_item Reference to where the popped item will be moved/copied.
         * @return true If an item was successfully popped.
         * @return false If the queue was empty.
         */
        [[nodiscard]] bool pop(T &_out_item)
            noexcept(std::is_nothrow_move_assignable_v<T>) {
            const auto currentTail = m_tail.load(std::memory_order_relaxed);

            if (currentTail == m_head.load(std::memory_order_acquire)) {
                return false; // Empty
            }

            _out_item = std::move(data[currentTail]);

            const auto nextTail = (currentTail + 1) & (Size - 1);
            m_tail.store(nextTail, std::memory_order_release);
            return true;
        }

        /**
         * @brief Peeks at the next item without removing it.
         *
         * Thread Safety: CONSUMER thread ONLY.
         *
         * @return const T* Pointer to the next item, or nullptr if empty.
         */
        [[nodiscard]] const T *peek() const {
            const auto currentTail = m_tail.load(std::memory_order_relaxed);
            if (currentTail == m_head.load(std::memory_order_acquire)) {
                return nullptr;
            }
            return &data[currentTail];
        }

        /**
         * @brief Checks if the queue is empty.
         *
         * Thread Safety: Safe to call from either thread (but result is a snapshot).
         *
         * @return true If the queue is empty.
         */
        [[nodiscard]] bool isEmpty() const {
            return m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire);
        }

        /**
         * @brief Calculates the number of slots available for writing.
         *
         * Thread Safety: PRODUCER thread ONLY (accurate). 
         * If called by consumer, it may under-report available space.
         *
         * @return size_t The number of items that can be pushed before the queue is full.
         */
        [[nodiscard]] size_t availableWrite() const {
            const size_t head = m_head.load(std::memory_order_relaxed);
            const size_t tail = m_tail.load(std::memory_order_acquire);

            if (head >= tail) {
                return Size - 1 - (head - tail);
            }
            return tail - head - 1;
        }

        // Operator overloads

        /**
         * @brief Stream operator for pushing (Move).
         * @see push(T&&)
         */
        bool operator<<(T &&_item) {
            return push(std::move(_item));
        }

        /**
         * @brief Stream operator for pushing (Copy).
         * @see push(const T&)
         */
        bool operator<<(const T &_item) {
            return push(_item);
        }

        /**
         * @brief Stream operator for popping.
         * @see pop(T&)
         */
        bool operator>>(T &_out_item) {
            return pop(_out_item);
        }

        /**
         * @brief Logical NOT operator to check if empty.
         * @return true If empty.
         */
        bool operator!() const {
            return isEmpty();
        }

        /**
         * @brief Boolean conversion operator to check if NOT empty (has data).
         * @return true If NOT empty.
         */
        explicit operator bool() const {
            return !isEmpty();
        }

    private:
        /**
         * @brief Internal implementation for pushing items.
         * Uses universal references to handle both copy and move without code duplication.
         */
        template<typename U>
        bool pushImpl(U &&_item) noexcept(std::is_nothrow_assignable_v<T &, U &&>) {
            const auto currentHead = m_head.load(std::memory_order_relaxed);
            const auto nextHead = (currentHead + 1) & (Size - 1);

            // Check if full (nextHead would hit tail)
            if (nextHead == m_tail.load(std::memory_order_acquire)) {
                return false;
            }

            data[currentHead] = std::forward<U>(_item);

            m_head.store(nextHead, std::memory_order_release);
            return true;
        }

        // Padding ensures m_head and m_tail are on different cache lines to prevent false sharing.
        alignas(hardware_destructive_interference_size) std::atomic<size_t> m_head;
        alignas(hardware_destructive_interference_size) std::atomic<size_t> m_tail;

        T data[Size];
    };
}
