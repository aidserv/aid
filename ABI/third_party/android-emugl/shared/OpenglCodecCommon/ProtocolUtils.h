#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

namespace emugl {

// A helper template to extract values form the wire protocol stream
// and convert them to appropriate host values.
//
// The wire protocol uses 32-bit exclusively when transferring
// GLintptr or GLsizei values, as well as opaque handles like GLeglImage,
// from the guest (even when the guest is 64-bit).
//
// The corresponding host definitions depend on the host bitness. For
// example, GLintptr is 64-bit on linux-x86_64. The following is a set
// of templates that can simplify the conversion of protocol values
// into host ones.
//
// The most important one is:
//
//     unpack<HOST_TYPE,SIZE_TYPE>(const void* ptr)
//
// Which reads bytes from |ptr|, using |SIZE_TYPE| as the underlying
// sized-integer specifier (e.g. 'uint32_t'), and converting the result
// into a |HOST_TYPE| value. For example:
//
//     unpack<EGLImage,uint32_t>(ptr + 12);
//
// will read a 4-byte value from |ptr + 12| and convert it into
// an EGLImage, which is a host void*. The template detects host
// pointer types to perform proper type casting.
//
// TODO(digit): Add custom unpackers to handle generic opaque void* values.
//              and map them to unique 32-bit values.

template <typename T, typename S>
struct UnpackerT {
    static T unpack(const void* ptr) {
        //static_assert(sizeof(T) == sizeof(S),
        //              "Bad input arguments, have to be of the same size");
		#if 0
        return *(const T*)ptr;
        #else
        T temp;
        memcpy(&temp,ptr,sizeof(T));
        
        return *(const T*)&temp;
    	#endif
    }
};

template <typename T, typename S>
struct UnpackerT<T*, S> {
    static T* unpack(const void* ptr) {
    	#if 0
        return (T*)(uintptr_t)(*(const S*)ptr);
        #else
    	S temp;
    	memcpy(&temp,ptr,sizeof(S));
    	
    	return (T*)(uintptr_t)(*(const S*)&temp);
    	#endif
    	
    	
    }
};

template <>
struct UnpackerT<ssize_t, uint32_t> {
    static ssize_t unpack(const void* ptr) {
    	#if 0
        return (ssize_t)*(const int32_t*)ptr;
        #else
        int32_t temp;
        
        memcpy(&temp, ptr, sizeof(int32_t));
        
        return (ssize_t)*(const int32_t*)&temp;
        #endif
    }
};

template <typename T, typename S>
inline T Unpack(const void* ptr) {
    return UnpackerT<T, S>::unpack(ptr);
}

// Helper classes GenericInputBuffer and GenericOutputBuffer used to ensure
// input and output buffers passed to EGL/GL functions are properly aligned
// (preventing crashes with some backends).
//
// Usage example:
//
//    GenericInputBuffer<> inputBuffer(ptrIn, sizeIn);
//    GenericOutputBuffer<> outputBuffer(ptrOut, sizeOut);
//    glDoGetStuff(inputBuffer.get(), outputBuffer.get());
//    outputBuffer.flush();
//
// get() will return the original value of |ptr| if it was aligned on the
// configured boundary (8 bytes by default). Otherwise, it will return the
// address of an aligned copy of the original |size| bytes starting from |ptr|.
//
// Allowed alignment values are 1, 2, 4, 8.
//
// outputBuffer.flush() copies the content of the copy back to |ptr| explictly,
// if needed. It is a no-op if |ptr| was aligned.
//
// Both classes try to minimize heap usage as much as possible - the first
// template argument defines the size of an internal array Generic*Buffer-s use
// if the |ptr|'s |size| is small enough. If it doesn't fit into the internal
// array, an aligned copy is allocated on the heap and freed in the dtor.

template <size_t StackSize = 1024, size_t Align = 8>
class GenericInputBuffer {
    //static_assert(Align == 1 || Align == 2 || Align == 4 || Align == 8,
    //              "Bad alignment parameter");

public:
    GenericInputBuffer(const void* input, size_t size) : mOrigBuff(input) {
        if (((uintptr_t)input & (Align - 1U)) == 0) {
            mPtr = const_cast<void*>(input);
        } else {
        	//printf("GenericInputBuffer\n");
            if (size <= StackSize) {
                mPtr = &mArray[0];
            } else {
                mPtr = malloc(size);
            }
            memcpy(mPtr, input, size);
        }
    }

    ~GenericInputBuffer() {
        if (mPtr != mOrigBuff && mPtr != &mArray[0]) {
            free(mPtr);
        }
    }

    const void* get() const { return mPtr; }

private:
    // A pointer to the aligned buffer, might point either to mOrgBuf, to mArray
    // start or to a heap-allocated chunk of data.
    void* mPtr;
    // Original buffer.
    const void* mOrigBuff;
    // Inplace aligned array for small enough buffers.
    char __attribute__((__aligned__(Align))) mArray[StackSize];
};

template <size_t StackSize = 1024, size_t Align = 8>
class GenericOutputBuffer {
    //static_assert(Align == 1 || Align == 2 || Align == 4 || Align == 8,
    //              "Bad alignment parameter");

public:
    GenericOutputBuffer(unsigned char* ptr, size_t size) :
            mOrigBuff(ptr), mSize(size) {
        if (((uintptr_t)ptr & (Align - 1U)) == 0) {
            mPtr = ptr;
        } else {
        	//printf("GenericOutputBuffer\n");
            if (size <= StackSize) {
                mPtr = &mArray[0];
            } else {
                mPtr = calloc(1, size);
            }
        }
    }

    ~GenericOutputBuffer() {
        if (mPtr != mOrigBuff && mPtr != &mArray[0]) {
            free(mPtr);
        }
    }

    void* get() const { return mPtr; }

    void flush() {
        if (mPtr != mOrigBuff) {
            memcpy(mOrigBuff, mPtr, mSize);
        }
    }

private:
    // A pointer to the aligned buffer, might point either to mOrgBuf, to mArray
    // start or to a heap-allocated chunk of data.
    void* mPtr;
    // Original buffer.
    unsigned char* mOrigBuff;
    // Original buffer size.
    size_t mSize;
    // Inplace array for small enough buffers.
    unsigned char __attribute__((__aligned__(Align))) mArray[StackSize];
};

// Pin the defaults for the commonly used type names
#if 0
using InputBuffer = GenericInputBuffer<>;
using OutputBuffer = GenericOutputBuffer<>;
#else
typedef GenericInputBuffer<> InputBuffer;
typedef GenericOutputBuffer<> OutputBuffer;
#endif
}  // namespace emugl
#if 0
// c++11 have nullptr
const class nullptr_t
{
public:
    template<class T>
    inline operator T*() const
        { return 0; }

    template<class C, class T>
    inline operator T C::*() const
        { return 0; }
 
private:
    void operator&() const;
} nullptr = {};
#endif
