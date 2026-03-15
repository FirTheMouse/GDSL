#pragma once

#include <iostream>
#include <atomic>


class q_object {
    protected:
        mutable std::atomic<int> refCount{0};
        std::atomic<bool> tombstone{true};
    
    public:
        q_object() {}
        virtual ~q_object() {}
        // Explicitly delete copy operations
        q_object(const q_object&) = delete;
        q_object& operator=(const q_object&) = delete;

        // Properly implement move operations
        q_object(q_object&& other) noexcept 
            : refCount(other.refCount.load()), tombstone(other.tombstone.load()) {
            // Reset the moved-from object
            other.refCount.store(0);
            other.tombstone.store(false);
        }

        q_object& operator=(q_object&& other) noexcept {
            if (this != &other) {
                refCount.store(other.refCount.load());
                tombstone.store(other.tombstone.load());
                // Reset the moved-from object
                other.refCount.store(0);
                other.tombstone.store(false);
            }
            return *this;
        }

        void stop() {tombstone.store(false);}
        void resurrect() {tombstone.store(true);}
        bool isActive() {return tombstone.load();}
    
        void retain() { ++refCount; }
        virtual void release() {
            if (refCount.fetch_sub(1) == 1) {
                delete this;
            }
        }

        int getRefCount() const {
            return refCount.load();
        }
    };
    
    template<typename T>
    class g_ptr {
        //static_assert(std::is_base_of<Object, T>::value, "T must inherit from Object");
    
        T* ptr = nullptr;
    
    public:
        g_ptr() = default;
    
        g_ptr(T* raw) : ptr(raw) {
            if (ptr) ptr->retain();
        }
    
        g_ptr(const g_ptr<T>& other) : ptr(other.ptr) {
            if (ptr) ptr->retain();
        }
    
        g_ptr(g_ptr<T>&& other) noexcept : ptr(other.ptr) {
            other.ptr = nullptr;
        }
    
        ~g_ptr() {
            if (ptr) ptr->release();
        }
    
        g_ptr<T>& operator=(const g_ptr<T>& other) {
            if (this != &other) {
                if (ptr) ptr->release();
                ptr = other.ptr;
                if (ptr) ptr->retain();
            }
            return *this;
        }
    
        g_ptr<T>& operator=(g_ptr<T>&& other) noexcept {
            if (this != &other) {
                if (ptr) ptr->release();
                ptr = other.ptr;
                other.ptr = nullptr;
            }
            return *this;
        }
    
        T* operator->() const { return ptr; }
        T& operator*() const { return *ptr; }
        T* getPtr() const { return ptr; }

        friend bool operator==(const g_ptr<T>& lhs, const g_ptr<T>& rhs) {
            return lhs.ptr == rhs.ptr;
        }
        
        friend bool operator!=(const g_ptr<T>& lhs, const g_ptr<T>& rhs) {
            return lhs.ptr != rhs.ptr;
        }

        explicit operator bool() const { return ptr != nullptr; }

        template<typename U>
        operator g_ptr<U>() const {
            static_assert(std::is_base_of<U, T>::value, "Can only convert to base types");
            return g_ptr<U>(ptr);
        }

    };
    
    template<typename T, typename... Args>
    g_ptr<T> make(Args&&... args) {
        static_assert(std::is_base_of<q_object, T>::value, "make<T>: T must derive from Object");
        T* obj = new T(std::forward<Args>(args)...);
        return g_ptr(obj);
    }

    template<typename T, typename U>
    g_ptr<T> g_dynamic_pointer_cast(const g_ptr<U>& from) {
        static_assert(std::is_base_of<q_object, U>::value, "U must inherit from Object");
        static_assert(std::is_base_of<q_object, T>::value, "T must inherit from Object");
        
        if (!from) return g_ptr<T>(nullptr);
        
        T* casted = dynamic_cast<T*>(from.getPtr());
        if (casted) {
            return g_ptr<T>(casted);
        } else {
            return g_ptr<T>(nullptr);
        }
   }

   template<typename T, typename U>
   g_ptr<T> as(const g_ptr<U>& from) {
        return g_dynamic_pointer_cast<T>(from);
   }



