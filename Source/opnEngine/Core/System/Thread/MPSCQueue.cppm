module;

#include <atomic>
#include <bit>

export module opn.System.Thread.MPSCQueue;

export namespace opn {
    template< typename T, size_t Size >
        requires ( std::has_single_bit( Size ) ) && ( std::is_nothrow_move_assignable_v< T > )
    class MPSCQueue {
        struct sSlot {
            T data;
            std::atomic_bool isReady{ false };
        };

        alignas( 64 ) std::atomic< size_t > m_head;
        alignas( 64 ) std::atomic< size_t > m_tail;
        sSlot m_data[Size];

    public:
        constexpr MPSCQueue() noexcept : m_head( 0 ), m_tail( 0 ) {}

        [[nodiscard]] constexpr bool push( T &&_item ) noexcept {
            size_t head = m_head.load( std::memory_order_relaxed );
            size_t next;

            do {
                next = ( head + 1 ) & ( Size - 1 );
                if( next == m_tail.load( std::memory_order_acquire ) ) {
                    return false;
                }
            } while ( !m_head.compare_exchange_weak( head, next, std::memory_order_relaxed ) );

            m_data[head] = std::move( _item );
            m_data[head].isReady.store( true, std::memory_order_release );
            return true;
        }

        [[nodiscard]] constexpr bool pop( T &_outItem ) noexcept {
            const size_t tail = m_tail.load( std::memory_order_relaxed );

            if( tail == m_head.load( std::memory_order_acquire ) ) {
                return false;
            }

            if( !m_data[ tail ].isReady.load( std::memory_order_acquire ) ) {
                return false;
            }

            _outItem = std::move( m_data[ tail ].data );
            m_data[ tail ].isReady.store( false, std::memory_order_release );
            return true;
        }

        [[nodiscard]] constexpr bool isEmpty() noexcept {
            return m_head.load( std::memory_order_acquire ) ==
                   m_tail.load( std::memory_order_acquire );
        }

        [[nodiscard]] constexpr size_t size() noexcept {
            const size_t head = m_head.load( std::memory_order_relaxed );
            const size_t tail = m_tail.load( std::memory_order_acquire );
            return (head - tail) & ( Size - 1 );
        }

        bool operator<<(T&& _item) noexcept { return push(std::move(_item)); }
        bool operator>>(T&& _item) noexcept { return pop(std::move(_item)); }
        explicit operator bool() const noexcept { return !isEmpty(); }
    };
}
