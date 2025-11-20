#pragma once
#include <atomic>
#include <iterator>
#include <new>
#include <bit>

// ABI safety
#ifdef __cpp_lib_hardware_interference_size
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

namespace opn {
    template<typename T, size_t Size>
    class ccRingBuffer {
        static_assert( std::has_single_bit(Size), "Size must be a power of two." );
    public:
        constexpr ccRingBuffer()
            : m_head( 0 ), m_tail( 0 )
        { };

        bool push( T &&_item ) {
            const auto currentHead = m_head.load( std::memory_order_relaxed );
            const auto nextHead = ( currentHead + 1 ) & ( Size - 1 );

            if (nextHead == m_tail.load(std::memory_order_acquire)) {
                return false;
            }

            data[ currentHead ] = std::move( _item );

            m_head.store(nextHead, std::memory_order_release);
            return true;
        };

        bool pop( T &_out_item ) {
            const auto currentTail = m_tail.load( std::memory_order_relaxed );

            if (currentTail == m_head.load( std::memory_order_acquire ) ) {
                return false;
            }

            _out_item = std::move( data[ currentTail ] );

            const auto nextTail = ( currentTail + 1 ) & ( Size - 1 );

            m_tail.store(nextTail, std::memory_order_release );
            return true;
        }

        bool isEmpty() const {
            return m_head.load( std::memory_order_acquire ) == m_tail.load( std::memory_order_acquire );
        }


        size_t availableWrite() const {
            const size_t head = m_head.load( std::memory_order_relaxed );
            const size_t tail = m_tail.load( std::memory_order_relaxed );

            if ( head >= tail ) {
                return Size - 1 - ( head - tail );
            }

            return tail - head;
        }

    private:
        alignas( hardware_destructive_interference_size ) std::atomic<size_t> m_head;
        alignas( hardware_destructive_interference_size ) std::atomic<size_t> m_tail;

        T data[Size];
    };
}
