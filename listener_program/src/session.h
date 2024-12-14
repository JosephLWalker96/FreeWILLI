
#pragma once
#include "pch.h"

using namespace std::chrono_literals;

template <typename T, std::size_t Alignment = 16>
struct AlignedAllocator
{
    using value_type = T;

    AlignedAllocator() noexcept = default;

    template <class U>
    AlignedAllocator(const AlignedAllocator<U, Alignment> &) noexcept {}

    T *allocate(std::size_t n)
    {
        // Allocate memory with alignment
        void *ptr = nullptr;
        std::size_t size = n * sizeof(T);
        if (posix_memalign(&ptr, Alignment, size) != 0)
        {
            throw std::bad_alloc();
        }
        return reinterpret_cast<T *>(ptr);
    }

    void deallocate(T *p, std::size_t) noexcept
    {
        free(p);
    }

    // Define rebind to make it standard-compliant for C++11
    template <class U>
    struct rebind
    {
        using other = AlignedAllocator<U, Alignment>;
    };

    // Optional: define equality operators
    template <class U, std::size_t A>
    bool operator==(const AlignedAllocator<U, A> &) const noexcept { return Alignment == A; }

    template <class U, std::size_t A>
    bool operator!=(const AlignedAllocator<U, A> &) const noexcept { return Alignment != A; }
};
//(focuses on managing shared resources and thread safety):
class Session
{
private:
    std::mutex dataBufferLock;

public:
    std::atomic<bool> errorOccurred = false;
    std::queue<std::vector<uint8_t>> dataBuffer;
    std::vector<std::vector<uint8_t>> dataBytesSaved;
    std::vector<float> dataSegment;
    // std::vector<float, AlignedAllocator<float, 16>> dataSegment;
    std::vector<std::chrono::system_clock::time_point> dataTimes; // timestamps of UDP packet
    int detectionCounter = 0;

    // Add methods for buffer management
    int pushDataToBuffer(const std::vector<uint8_t> &data)
    {
        std::lock_guard<std::mutex> lock(dataBufferLock);
        dataBuffer.push(data);
        return dataBuffer.size();
    }

    bool popDataFromBuffer(std::vector<uint8_t> &data)
    {
        std::lock_guard<std::mutex> lock(dataBufferLock);
        if (!dataBuffer.empty())
        {
            data = dataBuffer.front();
            dataBuffer.pop();
            return true;
        }
        return false;
    }
};
