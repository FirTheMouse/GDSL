#pragma once

#include <atomic>
#include <chrono>
#include <random>
#include <functional>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <fstream>
#include <initializer_list>
#include <stdlib.h>
#include <any>
#include <sys/mman.h>
template<typename... Args>
void print(Args&&... args) {
  (std::cout << ... << args) << std::endl;
}

template<typename... Args>
void printnl(Args&&... args) {
  (std::cout << ... << args);
}


static inline std::string lime(const std::string& text) {
  return "\x1b[92m"+text+"\x1b[0m";
}
static inline std::string green(const std::string& text) {
  return "\x1b[32m"+text+"\x1b[0m";
}
static inline std::string pine(const std::string& text) {
  return "\x1b[2;32m"+text+"\x1b[0m";
}
static inline std::string yellow(const std::string& text) {
  return "\x1b[33m"+text+"\x1b[0m";
}
static inline std::string red(const std::string& text) {
  return "\x1b[31m"+text+"\x1b[0m";
}
static inline std::string blue(const std::string& text) {
  return "\x1b[34m"+text+"\x1b[0m";
}
static inline std::string magenta(const std::string& text) {
  return "\x1b[35m"+text+"\x1b[0m";
}
static inline std::string cyan(const std::string& text) {
  return "\x1b[36m"+text+"\x1b[0m";
}
static inline std::string white(const std::string& text) {
  return "\x1b[37m"+text+"\x1b[0m";
}
static inline std::string gray(const std::string& text) {
  return "\x1b[90m"+text+"\x1b[0m";
}

static inline std::string bold_str(const std::string& text) {
  return "\x1b[1m"+text+"\x1b[0m";
}
static inline std::string dim_str(const std::string& text) {
  return "\x1b[2m"+text+"\x1b[0m";
}
static inline std::string underline_str(const std::string& text) {
  return "\x1b[4m"+text+"\x1b[0m";
}

static inline std::string italic_str(const std::string& s) {
  return "\033[3m" + s + "\033[23m";
}


static inline std::string rgb(const std::string& text, int r, int g, int b) {
  return "\x1b[38;2;"+std::to_string(r)+";"+std::to_string(g)+";"+std::to_string(b)+"m"+text+"\x1b[0m";
}
static inline std::string bg(const std::string& text, int r, int g, int b) {
  return "\x1b[48;2;"+std::to_string(r)+";"+std::to_string(g)+";"+std::to_string(b)+"m"+text+"\x1b[0m";
}


std::string to_bin(uint32_t n) {
  std::string s = "";
  for(int i = 31; i >= 0; i--) {
      s += ((n >> i) & 1) ? '1' : '0';
  }
  return s;
}

std::string to_hex(uint32_t n) {
  const char digits[] = "0123456789ABCDEF";
  std::string s = "0x";
  for(int i = 28; i >= 0; i -= 4) {
      s += digits[(n >> i) & 0b1111];
  }
  return s;
}

static std::string ftime(double t) 
{
  if(t >= 100000000) {
      return red(std::to_string(t/1000000000.0)+"s");
  } else if(t >= 100000) {
      return yellow(std::to_string(t/1000000.0)+"ms");
  } else { 
      return  green(std::to_string(t/1000.0)+"ns");
  } 
}

namespace Log {
  class Line {
      public:
      Line() {}
  
      ~Line() {}

      std::string label_;
      std::chrono::steady_clock::time_point start_;
      double total_time_ = 0.0;

      void start() {
          start_ = std::chrono::steady_clock::now();
      }

    double time_ns() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
        return (double)duration;
    }

    double time_s() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<double>(end - start_).count();
        return duration;
    }

      double end() {
          auto end = std::chrono::steady_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
          total_time_ += duration;
          return (double)duration;
      }
      
  };
}

inline float randf(float min, float max)
{
    // One engine per thread; seeded once from real entropy.
    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);                  // [min, max) – max exclusive by default
}

inline int randi(int min, int max)
{
    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(min, max); // inclusive both ends
    return dist(rng);
}

inline void clamp(float& value, float min, float max) {
  value = std::max(min,std::min(max,value));
}



#define DISABLE_BOUNDS_CHECK 1

struct d_object {};


template <typename T>
class list : public d_object {
protected:
    T* ptr;
    size_t size_;
    size_t capacity_;
public:
    list() {
        size_ = 0;
        capacity_ = 0;
        ptr = nullptr;
    }

    list(size_t cap) {
        size_ = 0;
        capacity_ = cap;
        ptr = (capacity_ > 0) ? new T[capacity_] : nullptr;
    }

    list(size_t cap,T def) {
        size_ = cap;
        capacity_ = cap;
        ptr = (capacity_ > 0) ? new T[capacity_] : nullptr;
        for(int i = 0;i<cap;i++) {
            ptr[i] = def;
        }
    }

    list(std::initializer_list<T> values) {
    size_ = 0;
    capacity_ = values.size();
    ptr = (capacity_ > 0) ? new T[capacity_] : nullptr;
    for (const T& value : values) {
        push(value);
    }
    }

    list(list&& other) noexcept {
        ptr = other.ptr;
        size_ = other.size_;
        capacity_ = other.capacity_;

        other.ptr = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    list(const list& other) noexcept {
        size_ = other.size_;
        capacity_ = other.capacity_;

        if (capacity_ > 0) {
        ptr = new T[capacity_];
        for (size_t i = 0; i < size_; ++i) {
            ptr[i] = other.ptr[i];
        }
        } else {
            ptr = nullptr;
        }
    }

    list<T>& operator=(const list<T>& other) {
    if (this != &other) {
        destroy();
        size_ = other.size_;
        capacity_ = other.capacity_;
        if (capacity_ > 0) {
            ptr = new T[capacity_];
            for (size_t i = 0; i < size_; ++i) {
                ptr[i] = other.ptr[i];
            }
        } else {
            ptr = nullptr;
        }
    }
    return *this;
    }

    list<T>& operator=(list<T>&& other) noexcept {
        if (this == &other) return *this; // self-assignment safety
    
        // Steal data
        T* oldPtr = ptr;
        ptr = other.ptr;
        size_ = other.size_;
        capacity_ = other.capacity_;
    
        // Reset the other
        other.ptr = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    
        // Only delete after stealing to avoid alias corruption
        if (oldPtr) delete[] oldPtr;
    
        return *this;
    }
    

    ~list() {
        destroy();
    }

    inline size_t length() const {return size_;}
    inline size_t size() const {return size_;}
    inline size_t space() const { return capacity_; }
    inline size_t capacity() const { return capacity_; }
    inline bool empty() const {return length()==0;}
    inline T& last() {return ptr[size_-1];}
    inline T& first() {return ptr[0];}

    //This implmentation is flawed, or at least, OpenGL doesn't play nice with it
    T* data() noexcept { return ptr; }
    const T* data() const noexcept { return ptr; }

    inline T* begin() {return ptr;}
    inline T* end() {return ptr+size_;}

    inline T* begin() const {return ptr;}
    inline T* end() const {return ptr+size_;}

    void pushAll(const list<T>& input) {
        for(int i = 0;i<input.size_;i++)
        {
            push(input[i]);
        }
    }

    void insertAll(const list<T>& input, size_t index) {
        for(int i = input.size_-1;i>=0;i--) {
            insert(input[i],index);
        }
    }

    // void printAll() const {
    //     for(size_t i = 0;i<size;i++)
    //     {
    //         std::cout << ptr[i] << std::endl;
    //     }
    // }
   
    /// @brief Pushes all values of the input to this list
    list<T>&  operator<<(const list<T>& input) {
        for(size_t i = 0;i<input.size_;i++)
        {
            push(input.get(i));
        }
        return *this;
    }

    /// @brief Pushes all values of this list to the output
    void operator>>(list<T>& output) {
        for(size_t i = 0;i<size_;i++)
        {
            output.push(get(i));
        }
    }
    
    template <typename... Args>
    void pushAll(Args... args) {
        (push(args),...);
    }

    void destroy() {
        if (ptr) {
        delete[] ptr;
        ptr = nullptr;
        }
        size_ = 0;
        capacity_ = 0;
    }

    void clear() {
        if constexpr (std::is_trivially_destructible_v<T>) {
            size_ = 0;
        } else {
            for (size_t i = 0; i < size_; ++i) {
                ptr[i].~T();
            }
            size_ = 0;
        }
    }

    void merge(list<T>& input) {
        for(size_t i = 0;i<input.size_;i++)
        {
            push(input.get(i));
        }
        input.destroy();
    }

    /// @brief Merges the input to this list, erasing the input
    void operator<=(list<T>& input) {
        for(size_t i = 0;i<input.size_;i++)
        {
            push(input.get(i));
        }
        input.destroy();
    }

 /// @brief Merges this list to the output, erasing it
    void operator>=(list<T>& output) {
        for(size_t i = 0;i<size_;i++)
        {
            output.push(get(i));
        }
        destroy();
    }

    void operator>=(list<T>&& output) {
        for(size_t i = 0;i<size_;i++)
        {
            output.push(get(i));
        }
        destroy();
    }
    
    template <typename Func>
    void forEach(Func&& function)
    {
        for(size_t i=0;i<size_;i++)
        {
            function(ptr[i]);
        }
    }

    template <typename Func>
     /// @brief Executes a function on each of the items in the list
    void operator()(Func&& function)
    {
        for(size_t i=0;i<size_;i++)
        {
            function(ptr[i]);
        }
    }

    /// @brief Returns the index of a value in the list, similar to keylist, returns -1 if not found
    int find(const T& v) {
        for(int i=0;i<size_;i++) {
            if(ptr[i] == v) return i;
       }
       return -1;
    }

    /// @brief Returns a single index matching the function! -1 if nothing is found
    int find_if(std::function<bool(const T&)> pred) {
        for (int i = 0; i < size_; ++i) {
            if (pred(ptr[i])) return i;
        }
        return -1;
    }

    /// @brief Removes a single value from the list based on the function
    void erase_if (std::function<bool(const T&)> pred) {
        int f = find_if(pred);
        if(f!=-1) {removeAt(f);}
    }

    /// @brief Removes a value from the list based on type rather than just index, uses find
    void erase(const T& v) {
        int f = find(v);
        if(f!=-1) {removeAt(f);}
    }


    template<typename TT>
    void insert(TT&& value,size_t index)
    {
        if (index > size_) {
            if(index==0&&size_==0) {
                push(value);
                return;
            } else {
                throw std::out_of_range("list::insert::212 Insert index, "+std::to_string(index)+" out of size: "+std::to_string(size_));
            }
        }
        push(value);
        for (std::size_t i = size_ - 1; i > index; --i) {
            ptr[i] = std::move(ptr[i - 1]);
        }
        ptr[index] = std::forward<TT>(value);
    }

    template<typename TT>
    inline void push(TT&& value) {
        if (size_ >= capacity_) 
        {
        capacity_ = capacity_ == 0 ? 4 : capacity_ * 2; 
        T* newPtr = new T[capacity_];
        for (size_t i = 0; i < size_; ++i) {
             newPtr[i] = std::move(ptr[i]);
        }
        delete[] ptr;
        ptr = newPtr;
        }
        new (&ptr[size_]) T(std::forward<TT>(value));
        ++size_;
    }

    /// @brief a conditonal push that ensures the list does not already have an entry first
    template<typename TT>
    void push_if_absent(TT&& value) {
        if(!has(value)) push(value);
    }


    void reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) return;
        
        T* newPtr = new T[new_capacity];
        for (size_t i = 0; i < size_; ++i) {
            newPtr[i] = std::move(ptr[i]);
        }
        
        delete[] ptr;
        ptr = newPtr;
        capacity_ = new_capacity;
    }

    void resize(size_t new_size) {
        if (new_size > capacity_) {
            reserve(new_size);
        }
        size_ = new_size;
    }

    void shrink_to_fit() {
        if (size_ == capacity_ || capacity_ == 0) return;
        
        if (size_ == 0) {
            delete[] ptr;
            ptr = nullptr;
            capacity_ = 0;
            return;
        }
        
        T* newPtr = new T[size_];
        for (size_t i = 0; i < size_; ++i) {
            newPtr[i] = std::move(ptr[i]);
        }
        
        delete[] ptr;
        ptr = newPtr;
        capacity_ = size_;
    }

    /// @brief Pushes a value to the list
    /// @param value the value to be pushed
    list<T>& operator<<(T value) {
        push(value);
        return *this;
    }

    T pop() {
        if (size_ == 0) throw std::out_of_range("ERROR: List is empty");

        T val = ptr[size_ - 1];
        --size_;
        return val;
    }

    void removeAt(size_t index) {
        if (index >= size_) throw std::out_of_range("ERROR: Remove index out of bounds!");
        for (size_t i = index; i + 1 < size_; ++i) {
            ptr[i] = ptr[i + 1];
        }
        --size_;
    }

    T take(size_t index) {
        T val = ptr[index];
        removeAt(index);
        return val;
    }

    // T& rand() {return }
    
    inline T& get(size_t index) {
        #if !DISABLE_BOUNDS_CHECK
        if(index >= size_) {
            throw std::out_of_range("Util 265: List out of Bounds");
        }
        #endif
        return ptr[index];
    }

    inline const T& get(size_t index) const {
        if(index >= size_) {
            throw std::out_of_range("Util 268: List out of Bounds");
        }
        return ptr[index];
    }

    inline T& get(size_t index,const std::string& from) {
        if(index >= size_) {
            throw std::out_of_range("Util 275: List out of Bounds from \n  "+from);
        }
        return ptr[index];
    }

    inline T& operator[](size_t index) {
    #if !DISABLE_BOUNDS_CHECK
    if(index >= size_) {
        throw std::out_of_range("Util 265: List out of Bounds");
    }
    #endif
      return ptr[index];
    }

    inline const T& operator[](size_t index) const {
    #if !DISABLE_BOUNDS_CHECK
        if(index >= size_) {
            throw std::out_of_range("Util 268: List out of Bounds");
        }
    #endif
        return ptr[index];
    }

    inline T& rand() {
        return ptr[randi(0,size_-1)];
    }

    inline T& rand() const {
        return ptr[randi(0,size_-1)];
    }


    inline void shuffle() {
        for(int i = size_ - 1; i > 0; i--) {
            int j = randi(0, i);
            T& temp = ptr[i];
            ptr[i] = ptr[j];
            ptr[j] = temp;
        }
    }

    //This is crude and temporary
    inline void sort(std::function<bool(T,T)> func) {
        std::vector<T> temp_vec;
        for(int i=0;i<size_;i++) {
            temp_vec.push_back(ptr[i]);
        }
        std::sort(temp_vec.begin(), temp_vec.end(), func);
        clear();
        for(int i=0;i<temp_vec.size();i++) {
            push(temp_vec[i]);
        }
    }
    
    inline void reverse() {
        T* left = ptr;
        T* right = ptr + size_;
        
        if (left >= right) return;
        --right;
        
        while (left < right) {
            std::swap(*left++, *right--);
        }
    }

    inline void swap(size_t from, size_t to) {
        std::swap(ptr[from], ptr[to]);
    }

    template<typename TT>
    /// @brief Compares two lists and returns true if they are equivalent.
    bool operator==(list<TT>& other) {
        if(other.size_!=size_) return false;
        for(size_t i=0;i<size_;i++)
        {
           if(other[i]!=ptr[i]) return false;
        }
        return true;
    }

    template<typename TT>
    /// @brief Compares two lists and returns true if they are equivalent.
    bool operator==(const list<TT>& other) const {
        if(other.size_!=size_) return false;
        for(size_t i=0;i<size_;i++)
        {
           if(other[i]!=ptr[i]) return false;
        }
        return true;
    }

    template<typename TT>
    /// @brief Compares two lists and returns false if they are equivalent.
    bool operator!=(list<TT>& other) {
        return !(*this == other);
    }

    /// @brief Returns if the list contains an instance of a given value
    bool has(T search) const {
        for(size_t i=0;i<size_;i++)
        {
            if(ptr[i]==search) return true;
        }
        return false;
    }

};
template <typename T>
list(std::initializer_list<T>) -> list<T>;



    // template<typename... Args>
    // void emplace(Args&&... args) {
    //     if (size >= capacity) {
    //         capacity = capacity == 0 ? 4 : capacity * 2;
    //         T* newPtr = new T[capacity];
    //         for (size_t i = 0; i < size; ++i) {
    //             newPtr[i] = std::move(ptr[i]);
    //         }
    //         delete[] ptr;
    //         ptr = newPtr;
    //     }
    //     ptr[size++] = T(std::forward<Args>(args)...); // Constructs in-place
    // }


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




template<typename K,typename V>
struct entry
{
    K key;
    V value;
    entry() = default;
    entry(const K& k, const V& v) : key(k), value(v) {}
    entry(K&& k, V&& v) : key(std::move(k)), value(std::move(v)) {}
    entry(const entry& other) = default;
    entry(entry&& other) noexcept = default;
    entry& operator=(const entry& other) = default;
    entry& operator=(entry&& other) noexcept = default;
};


template<typename K,typename V>
class keylist : public list<entry<K,V>>
{
private:

public:
    using base = list<entry<K, V>>;

    //Add more control here in the future, with conventions for const and such and r/l
    template<typename KK, typename VV>
    void put(KK&& key, VV&& value) {
        *this << entry<K,V>(std::forward<KK>(key), std::forward<VV>(value));
    }

    template<typename EE>
    void put(EE&& e) {
        *this << std::forward<EE>(e);
    }

    V& get(const K& key){
       for(entry<K,V>& e : *this){
            if(e.key == key) return e.value;
       }
       //This is so that ASAN can trace the error origin 
    //    std::vector<int> ints;
    //    ints.push_back(1);
    //    volatile int b = ints[3];
       throw std::runtime_error("map::43 key not found ");
    }

    list<V> getAll(const K& key){
        list<V> l;
        for(entry<K,V>& e : *this){
             if(e.key == key) l << e.value;
        }
        return l;
     }

    list<V> allValues() {
        list<V> l;
        for(entry<K,V>& e : *this){
             l << e.value;
        }
        return l;
     }

    template<typename VV>
    V& getOrDefault(const K& key,VV&& fallback){
       for(entry<K,V>& e : *this){
            if(e.key == key) return e.value;
       }
       return fallback;
    }

    bool hasKey(const K& key){
       for(entry<K,V>& e : *this){
            if(e.key == key) return true;
       }
       return false;
    }

    list<K> keySet() {  
        list<K> result;
        for(entry<K,V>& e : *this) result << e.key;
        return result;
    }

    list<entry<K,V>> entrySet() {
        list<entry<K,V>> result;
        for(entry<K,V>& e : *this) {
            result << e;
        }
        return result;
    }
    
    template<typename KK, typename VV>
    bool set(KK& key, VV&& value){
       for(entry<K,V>& e : *this){
            if(e.key == key) {
             e = entry<K,V>(std::forward<KK>(key), std::forward<VV>(value));
             return true;
            }
       }
       return false;
    }

    // V& operator[](const K& key) {
    // return get(key);
    // }

     bool has(const K& key){
       for(entry<K,V>& e : *this){
            if(e.key == key) return true;
       }
       return false;
    }

    bool remove(const K& key) {
    for (size_t i = 0; i < this->length(); ++i) {
        if (this->base::operator[](i).key == key) {
            this->removeAt(i);
            return true;
        }
    }
    return false;
    }
};

static inline uint32_t hashString(const std::string& str) {
    uint32_t hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

static inline uint32_t mix32(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return (uint32_t)x;
}

template<typename K,typename V>
class map
{
private:
   list<keylist<K,V>> buckets;
   size_t size_;
   size_t capacity;
public:
    map()
    {
        size_ = 0;
        capacity = 8;
        for(int i=0;i<capacity;i++)
        {
            buckets.push(keylist<K,V>());
        }
    }

    size_t size() {return size_;}


    uint32_t mix96(uint32_t a, uint32_t b, uint32_t c) {
        uint64_t combined = ((uint64_t)a << 32) | b;
        return mix32(combined) ^ mix32((uint64_t)c);
    }

    template<typename T>
    uint32_t hashT(const T& val)
    {
        if constexpr (std::is_same_v<T,std::string>) {
            return hashString(val);
        }
        else if constexpr (std::is_same_v<T,const char*>) {
            return hashString(string(val));
        }
        else if constexpr (std::is_array_v<T> && std::is_same_v<std::remove_extent_t<T>, char>) {
            return hashString(string(val));
        }
        else if constexpr (std::is_same_v<T,int>) {
            return val;
        } else if constexpr (std::is_pointer_v<T>) {
            auto addr = (std::uintptr_t)val;
            addr >>= 3; 
            return mix32((uint64_t)addr);
        } else if constexpr (sizeof(T)==12) {
            const uint32_t* parts = reinterpret_cast<const uint32_t*>(&val);
            return mix96(parts[0], parts[1], parts[2]);
        } else if constexpr (sizeof(T)==8) {
            return mix32((uint64_t)val);
        }
        else {
            return val;
        }
    };

    template<class U>
    uint32_t hashT(const g_ptr<U>& p)
    {
        return hashT(p.getPtr());
    }

    void put(const K& key,V value)
    {
        if(size_>=(capacity*2))
        {
            capacity = capacity*2;
            list<keylist<K,V>> newBuckets(capacity);
            for(int i=0;i<capacity;i++)
            {   
                newBuckets.push(keylist<K,V>());
            }
            for(keylist<K,V>& old : buckets)
            {
                // for(const auto& e : old.entrySet()) {
                //     newBuckets[(hashT(e.key)%capacity)].put(e.key,e.value);
                // }
                //For some reason the lambda is faster in testing
                old([&](const entry<K,V>& e){newBuckets[(hashT(e.key)%capacity)].put(e.key,e.value);});
            }
            buckets = std::move(newBuckets);

        }
        buckets[hashT(key)%capacity].put(key,value);
        size_++;
    }
    

    bool set(const K& key,V value) {
        return buckets.get(hashT(key)%capacity).set(key,value);
    }

    // keylist<K, V> b = buckets[hashT(key)%capacity];
    // if(b.hasKey(key)) return b.get(key);
    // else {throw}
    V& get(const K& key)
    {
       return buckets.get(hashT(key)%capacity).get(key);
    }

    keylist<K, V>& getBucket(const K& key) {
        return buckets.get(hashT(key)%capacity);
    }
    

    //Returns all values in the map
    list<V> getAll(){
        list<V> l;
        for(auto b : buckets) {
            for(auto v : b.allValues()) {
                l << v; }
        }
        return l;
     }

     //Reuturns all values associated with a key
     list<V> getAll(const K& key){
       return buckets.get(hashT(key)%capacity,"map::getAll::210").getAll(key);
     }



    template<typename VV>
    V& getOrDefault(const K& key,VV&& fallback)
    {
       return buckets[hashT(key)%capacity].getOrDefault(key,fallback);
    }

    bool hasKey(const K& key)
    {
       return buckets.get(hashT(key)%capacity).hasKey(key);
    }

    V& getOrPut(const K& key,const V& fallback) {
        if(!hasKey(key)) {
            put(key,fallback);
        }
        return get(key);
    }

    V& getOr(const K& key,std::function<V()> func) {
        if(!hasKey(key)) {
            put(key,func());
        }
        return get(key);
    }

    V& operator[](const K& key) {
        return getOrPut(key,V());
    }

    list<K> keySet() {  
        list<K> result;
        for(keylist<K,V>& e : buckets) e.keySet()>=result;
        return result;
    }

    list<entry<K,V>> entrySet() {
        list<entry<K,V>> result;
        for(const keylist<K,V>& e : buckets){
            for(int i=0;i<e.length();i++){
                result << e[i];
            }
        } 
        return result;
    }

    // list<entry<K,V>> entrySet() {
    //     list<entry<K,V>> result;
    //     for(int bucket_idx = 0; bucket_idx < buckets.length(); bucket_idx++) {
    //         keylist<K,V>& bucket = buckets[bucket_idx];
    //         for(int i = 0; i < bucket.length(); i++){
    //             result << bucket[i];
    //         }
    //     } 
    //     return result;
    // }

    void clear()
    {
        buckets([](keylist<K,V> keyl){keyl.base::destroy();});
        buckets.destroy();
        size_=0;
        capacity = 8;
        for(int i=0;i<capacity;i++)
        {
            buckets.push(keylist<K,V>());
        }
    }

    bool remove(const K& key) {
        int hash = (hashT(key)%capacity);
        if(hash<buckets.length())
            return buckets.get(hash).remove(key);
        else
            return false;
    }

    void debugMap() {
        buckets([](keylist<K,V> keyl){keyl([](entry<K,V> e){std::cout << e.key << std::endl;});});
    }
};
inline list<std::string> split_str(const std::string& s,char delimiter)
{
    list<std::string> toReturn;
    int last = 0;
    for(int i=0;i<s.length();i++)
    {
        if(s.at(i)==delimiter) {
            toReturn << s.substr(last,i-last);
            last = i+1;
        }
    }
    if(last<s.length())
    {
        toReturn << s.substr(last,s.length()-last);
    }
    return toReturn;
}

class Data{
public:
    Data() {}

    map<std::string,std::any> notes;

    template<typename T = std::string>
    void add(const std::string& label,T info)
    {
        notes.put(label,std::any(info));
    }

    template<typename T = std::string>
    T get(const std::string& label)
    {
        #if !DISABLE_BOUNDS_CHECK
            if(!has(label)) std::cerr << "Data does not have label " << label <<"\n";
        #endif
        return std::any_cast<T>(notes.get(label));
    }

    bool has(const std::string& label)
    {
        return notes.hasKey(label);
    }

    bool check(const std::string& label)
    {
        if(!has(label)) return false;
        try {
            return get<bool>(label);
        }
        catch(std::exception e)
        {
            print("data::check::59 Attempted to check a non-bool in data");
            //Or just return false?
            return false;
        }
    }

    bool toggle(const std::string& label) {
        if(!has(label)) set<bool>(label,true);
        bool toReturn = !get<bool>(label);
        set<bool>(label,toReturn);
        return toReturn;
    }

    void flagOn(const std::string& label) {set<bool>(label,true);}
    void flagOff(const std::string& label) {set<bool>(label,false);}

    template<typename T>
    void set(const std::string& label,T info) {
        if(!notes.set(label,info))
            add<T>(label,info);
    }

    template<typename T = int>
    T inc(const std::string& label,T by)
    {
        if(has(label)) {set<T>(label,get<T>(label)+by);}
        else {add<T>(label,by);}
        return get<T>(label);
    }

    void debugData() {
        notes.debugMap();
    }
    
    /// @brief Scans through based on provided list, returns all missing labels
    list<std::string> validate(list<std::string> toCheck)
    {
        list<std::string> toReturn;
        for(auto s : toCheck) if(!has(s)) toReturn << s;
        return toReturn;
    }
};

inline std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Could not open file: " + filename);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
  }
  
  inline void writeFile(const std::string& filename, const std::string& contents) {
    std::ofstream file(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file) throw std::runtime_error("Could not open file for writing: " + filename);
  
    file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!file) throw std::runtime_error("Failed while writing file: " + filename);
  }
  
  inline void editTextFile(  const std::string& filename, const std::function<void(std::string&)>& editor) {
    std::string text = readFile(filename);
    editor(text);
    writeFile(filename, text);
  }

inline std::ifstream openReadStream(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if(!in) throw std::runtime_error("Can't read from file: " + path);
    return std::move(in);
}

inline std::ofstream openWriteStream(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if(!out) throw std::runtime_error("Can't write to file: " + path);
    return std::move(out);
}


  template<typename T>
  inline void write_raw(std::ostream& out, const T& val) {
      out.write(reinterpret_cast<const char*>(&val), sizeof(T));
  }

  inline void write_string(std::ostream& out, const std::string& s) {
      uint32_t len = s.length();
      write_raw(out, len);
      out.write(s.data(), len);
  }

  template<typename T>
  inline T read_raw(std::istream& in) {
      T val;
      in.read(reinterpret_cast<char*>(&val), sizeof(T));
      return val;
  }

  inline std::string read_string(std::istream& in) {
      uint32_t len = read_raw<uint32_t>(in);
      std::string s(len, '\0');
      in.read(s.data(), len);
      return s;
  }





class Type;

class Object : virtual public q_object {    
    public:
        uint32_t sidx = 0;
        std::atomic<bool> recycled{false};
        Type* type_ = nullptr;

        Object() {

        }
        virtual ~Object() {}

        Object(Object&& other) noexcept 
        : q_object(std::move(other)) {}

        Object& operator=(Object&& other) noexcept {
            if (this != &other) {
                q_object::operator=(std::move(other));
            }
            return *this;
        }
    };




inline std::string add_commas(int num) {
    std::string str = std::to_string(num);
    int insert_position = str.length() - 3;
    
    while(insert_position > 0) {
        str.insert(insert_position, ",");
        insert_position -= 3;
    }
    
    return str;
  }
  
  inline void indent_multiline(std::string& str, const std::string& pad) {
    size_t pos = 0;
    while((pos = str.find('\n', pos)) != std::string::npos) {
        str.replace(pos, 1, "\n" + pad);
        pos += pad.length() + 1;
    }
  }

    inline void strip_whitespace(std::string& s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](char c) {
            return c == ' ' || c == '\n' || c == '\r' || c == '\t';
        }), s.end());
    }
  
  inline std::string wrap_str(const std::string& s,const std::string& c) {
    return c+s+c;
  }
  
  inline std::string trim_str(const std::string& s,const char c) {
    if (s.size() >= 2 && s.front() == c && s.back() == c)
        return s.substr(1, s.size() - 2);
    return s; 
  }


    std::string pad_str(const std::string& s, uint32_t width) {
        std::string to_return = s;
        while(width>to_return.length()) to_return+=" ";
        return to_return;
    }

    std::string center_pad(const std::string& s, uint32_t width) {
        if(s.length() >= width) return s;
        uint32_t total_pad = width - s.length();
        uint32_t left_pad = total_pad / 2;
        uint32_t right_pad = total_pad - left_pad;
        return std::string(left_pad, ' ') + s + std::string(right_pad, ' ');
    }
    std::string center_pad_known(const std::string& s, uint32_t s_visible_len, uint32_t width) {
        if(s_visible_len >= width) return s;
        uint32_t total_pad = width - s_visible_len;
        uint32_t left_pad = total_pad / 2;
        uint32_t right_pad = total_pad - left_pad;
        return std::string(left_pad, ' ') + s + std::string(right_pad, ' ');
    }

    uint32_t digit_count(uint32_t n) {
        if(n == 0) return 1;
        uint32_t digits = 0;
        while(n > 0) { n /= 10; digits++; }
        return digits;
    }

    bool is_str_num(const std::string& tocheck) {for(auto c : tocheck) {if(!std::isdigit(c)) return false;} return true;}

    std::string escape_string(const std::string& content, bool compact_spaces = true) {
        std::string escaped;
        int space_count = 0;
        for(char c : content) {
            if(compact_spaces) {
                if(c == ' ') {
                    if(space_count==1) {escaped.pop_back(); escaped += "..."; space_count++; continue;}
                    else if(space_count>1) {continue;}
                    else {space_count++;}
                } else {
                    space_count = 0;
                }
            }
            switch(c) {
                case '\n': escaped += "\\n"; break;
                case '\t': escaped += "\\t"; break;
                case '\r': escaped += "\\r"; break;
                //case '"':  escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                default:   escaped += c; break;
            }
        }
        return escaped;
    }
  
namespace sgen {
    struct namebase {
        namebase() {}
        explicit namebase(const list<list<std::string>>& _opts) : opts(_opts) {}
        explicit namebase(const std::string& seed) {
            list<std::string> lines = split_str(seed,',');
            for(const auto& l : lines) {
                opts << split_str(l,'|');
            }
        }
        list<list<std::string>> opts;
    };

    const namebase STANDARD("Ja|Be|Ma|Cer|Le,ck|de|ly|th|ch|un|el");
    const namebase TRUE_RANDOM("a|A|b|B|c|C|d|D|e|E|f|F|g|G|h|H|i|I|j|J|k|K|l|L|m|M|n|N|o|O|p|P|q|Q|r|R|s|S|t|T|u|U|v|V|w|W|x|X|y|Y|z|Z|0|1|2|3|4|5|6|7|8|9|_|+|-|*|/|=|<|>|!|&|^|.|,|:|;|(|)|[|]|{|}|\"|#|@|~|`|\\");
    const namebase RANDOM(
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z,"
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z,"
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z,"
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z");
    const namebase AVAL_WEST_TAMOR_FIRST(
        "Bu|Ahm|He|Ol|Mo|In|Bir|Ba|Tu," 
        "|||||||||||||||||||ha|ck|a|ch," 
        "el|ba|ak|ael|he|med");

    const namebase AVAL_CENTRAL_FIRST_MALE(
        "Al|Ed|Da|Ro|Wil|Tho|Hen|Mar|Reg|Cla|Luc|Aug,"  
        "||||||||||||an|ar|er|or|ald|ric|vid|lan|den|bert|tor|mon,"
        "us|d|n|rt|mer|son|ard|ton|las|ius|mond|iel");

    const namebase AVAL_WESTERN_FIRST_MALE(
        "Jo|Al|Con|Se|Sok|Va|Wel|Eg," 
        "|||||||||rgo|ra|ell|ber,"
        "der|us|ard|rk|on|th|n|l|vid");

    const namebase AVAL_CENTRAL_FIRST_FEMALE(
        "My|Al|Se|Ma|Eg|Cha|Sha|Tha," 
        "|||||ri|ex|il,"
        "|||na|der|ra|us|da|na|et");
    const namebase AVAL_CENTRAL_LAST(
        "Copper|Silver|Iron|Wood|High|Low|Swift|Old|New|Red|White|Black|Green|Blue|Yellow," 
        "paw|tail|fang|talon|wing|feather|river|hill|heart|claw|hall");

    inline std::string randsgen(const namebase& g) {
        std::string result;
        for(const auto& s : g.opts) 
            result.append(s.rand());
        return result;
    }
    
    inline std::string randsgen(const std::string& line) {
        list<std::string> lines = split_str(line,',');
        std::string result;
        for(const auto& l : lines) {
            list<std::string> sub = split_str(l,'|');
            std::string app = sub.rand();
            result.append(app);
        }
        return result;
    }
}

inline std::string rands() {
    return sgen::randsgen(sgen::TRUE_RANDOM);
}


namespace Log {

// Provides the time it takes for a function to run, not avereged over iterations
inline double time_function(int ITERATIONS,std::function<void(int)> process) {
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0;i<ITERATIONS;i++) {
        process(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return (double)time.count();
}

struct Comparison_ {
    Comparison_() {}
    Comparison_(int a, int b, int c, int d) : a_table(a), a_row(b), b_table(c), b_row(d) {}
    int a_table, a_row;
    int b_table, b_row;
};


inline void run_rig(list<list<std::function<void(int)>>> f_table,list<list<std::string>> s_table,list<Comparison_> comps,bool warm_up,int PROCESS_ITERATIONS,int C_ITS) {
    list<list<double>> t_table;

    for(int c=0;c<f_table.length();c++) {
        t_table.push(list<double>{});
        for(int r=0;r<f_table[c].length();r++) {
            t_table[c].push(0.0);
        }
    }

    for(int m = 0;m<(warm_up?2:1);m++) {
        int C_ITERATIONS = m==0?warm_up?1:C_ITS:C_ITS;

        for(int c=0;c<t_table.length();c++) {
            for(int r=0;r<t_table[c].length();r++) {
                t_table[c][r]=0.0;
            }
        }

        for(int i = 0;i<C_ITERATIONS;i++)
        {
            for(int c=0;c<f_table.length();c++) {
                for(int r=0;r<f_table[c].length();r++) {
                    // if(r==0) print("Running: ",s_table[c][r]);
                    double time = time_function(PROCESS_ITERATIONS,f_table[c][r]);
                    t_table[c][r]+=time;
                }
            }
        }
        if(warm_up) {
        print("-------------------------");
        print(m==0 ? "      ==COLD==" : "       ==WARM==");
        }
        print("-------------------------");
        for(int c=0;c<t_table.length();c++) {
            for(int r=0;r<t_table[c].length();r++) {
                t_table[c][r]/=C_ITERATIONS;
                print(s_table[c][r],": ",t_table[c][r]," ns (",t_table[c][r] / PROCESS_ITERATIONS," ns per operation)");
            }
            print("-------------------------");
        }
        for(auto v : comps) {
            double factor = t_table[v.a_table][v.a_row]/t_table[v.b_table][v.b_row];
            std::string sfs;
            double tolerance = 5.0;
            if (std::abs(factor - 1.0) < tolerance/100.0) {
                sfs = "around the same as ";
            } else if (factor > 1.0) {
                double percentage = (factor - 1.0) * 100.0;
                sfs = std::to_string(percentage) + "% slower than ";
            } else {
                double percentage = (1.0/factor - 1.0) * 100.0;
                sfs = std::to_string(percentage) + "% faster than ";
            }
            print("Factor [",s_table[v.a_table][v.a_row],"/",s_table[v.b_table][v.b_row],
            "]: ",factor," (",s_table[v.a_table][v.a_row]," is ",sfs,s_table[v.b_table][v.b_row],")");
        }
        print("-------------------------");

    }
}

// A helper for using the benchmarking tools to reduce boilerplate
struct rig {
private:

    list<list<std::function<void(int)>>> f_table;
    list<list<std::string>> s_table;
    list<Comparison_> comps;
    map<std::string,std::pair<int,int>> processes;
public:
    //Adds another table, tables are isolated test blocks
    void add_table() {
        f_table << list<std::function<void(int)>>{};
        s_table << list<std::string>{};
    }

    /// @brief Add a process to run
    /// @param process_name Name of the process, used for lookup and display when run
    /// @param process The function to time and run, the int argument is the process iteration, assuming PROCESS_ITERATIONS is not 1
    /// @param table Default value is 0, this can be used to split processes into distinct blocks when run
    void add_process(const std::string& process_name,std::function<void(int)> process,int table = 0) {
        while(f_table.length() <= table) add_table();
        processes.put(process_name,std::make_pair(table,f_table.get(table).length()));
        s_table.get(table) << process_name;
        f_table.get(table) << process;
    }

    /// @brief Add a comparison to be printed
    /// @param a Process to compare against
    /// @param b Process to compare to a
    void add_comparison(const std::string& a,const std::string& b) {
        try {
            std::pair<int,int> ap = processes.get(a);
            std::pair<int,int> bp = processes.get(b);
            comps << Comparison_(ap.first,ap.second,bp.first,bp.second);
        } catch(std::exception e) {
            if(!processes.hasKey(a)) {
                print("rig::110 Unable to add comparison to rig: ",a," was never added as a process");
            }
            if(!processes.hasKey(b)) {
                print("rig::110 Unable to add comparison to rig: ",b," was never added as a process");
            }
        }
    }

    /// @brief Run the rig and print out the results of the benchmark
    /// @param C_ITS How many iterations of the processes there should be, this contributes to the averege
    /// @param warm_up Whether or not to do a cold run to warm up the cache
    /// @param PROCESS_ITERATIONS How many times each process should run, not part of the averege
    void run(int C_ITS,bool warm_up = false,int PROCESS_ITERATIONS = 1) {
        run_rig(f_table,s_table,comps,warm_up,PROCESS_ITERATIONS,C_ITS);
    }
};


struct SeqLine : public q_object
{
    SeqLine() {};
    SeqLine(const std::string _label, bool _is_log) {
        label = _label;
        is_log = _is_log;
        Log::Line new_timer; new_timer.start();
        timer = new_timer;
    }

    Log::Line timer;
    std::string label = "";
    SeqLine* parent = nullptr;
    list<g_ptr<SeqLine>> children;
    bool is_log = true;

    std::string get_indent() {
        if(!parent) return "";
        int depth = 0;
        SeqLine* cursor = this;
        while(cursor->parent) { depth++; cursor = cursor->parent; }
        std::string indent(depth * 3, ' ');
        return indent;
    }

    std::string to_string() {
        std::string indent = get_indent();
        std::string to_return = "";
        if(is_log) {
            to_return.append(indent+label+"\n");
        } else {
            to_return.append(indent+label+" [time: " + ftime(timer.total_time_)+"]\n");
            for(auto& child : children) {
                to_return.append(child->to_string());
            }
        }
        return to_return;
    }
};

class Span : public Object
{
public:
    Span() {line_root = make<SeqLine>("Root",false);};

    map<std::string, Log::Line> timers;
    map<std::string, int> counters;
    bool print_on_line_end = false;
    bool log_everything = false;

    void start_timer(const std::string &label) {
        if (timers.hasKey(label)) {
            Log::Line &timer = timers.get(label);
            timer.start();
        } else {
            Log::Line timer;
            timer.start();
            timers.put(label, timer);
        }
    }

    double end_timer(const std::string &label) {
        if (timers.hasKey(label)) {
            Log::Line &timer = timers.get(label);
            return timer.end();
        }
        return 0.0;
    }
    double get_time(const std::string &label) {
        if (timers.hasKey(label)) {return timers.get(label).total_time_;}
        else {return -1.0;}
    }
    inline std::string timer_string(const std::string &label) {return label + ": " + ftime(get_time(label));}
    void print_timers() {for (auto label : timers.keySet()) {print(timer_string(label));}}

    inline void increment(const std::string &label, int by = 1) {counters.getOrPut(label, 0) += by;}
    inline int get_count(const std::string &label) {return counters.getOrDefault(label, 0);}
    void print_counters() {for (auto label : counters.keySet())  { print(label, ": ", get_count(label)); }}

    g_ptr<SeqLine> line_root = nullptr;
    g_ptr<SeqLine> on_line = nullptr;

    g_ptr<SeqLine> get_last_line() {
        if(on_line) return on_line;
        else return line_root;
    }

    void add_line(const std::string& label) {
        g_ptr<SeqLine> parent = get_last_line();
        parent->children << make<SeqLine>(label,false);
        parent->children.last()->parent = parent.getPtr();
        on_line = parent->children.last();

        if(log_everything)
            print(label);
    }

    double end_line() 
    {
        double time = 0.0;
        if(!on_line) return time;
        time = on_line->timer.end();
        if(print_on_line_end)
            std::cout << on_line->to_string() << std::flush;
        if(on_line->parent) {
            on_line = on_line->parent;
        }
        return time;
    }

    template<typename... Args>
    void log(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        if(log_everything)
            std::cout << oss.str() << std::endl;
        std::string indent = get_last_line()->get_indent();
        indent += "  > "; //Extra space to distinquish from header
        std::string msg = indent+oss.str();
        indent_multiline(msg,indent);
        g_ptr<SeqLine> new_log = make<SeqLine>(msg,true);
        get_last_line()->children << new_log;
    }

    void print_all() {
        line_root->timer.end();
        print(line_root->to_string());
    }

    void newline(const std::string& label) {
        add_line(label);
    }

    double endline() {
        return end_line();
    }
};

}



    
//Controls for the compiler printing, for debugging
#define PRINT_ALL 1

g_ptr<Log::Span> span = nullptr;
static inline void newline(const std::string& label) {
    #if PRINT_ALL
    if(!span) span = make<Log::Span>();
    span->add_line(label);
    #endif
}
static inline double endline() {
    #if PRINT_ALL
    return span->end_line();
    #else
    return 0;
    #endif
}

template<typename... Args>
static inline void log(Args&&... args) {
    #if PRINT_ALL
    if(!span) span = make<Log::Span>();
    span->log(std::forward<Args>(args)...);
    #endif
}

std::string ptr_to_string(uint64_t addr) {
    uint64_t varied = addr >> 4;
    
    const char* profiles[] = {"CGOQD", "IHLTFE", "AVWXZK", "BPRSM"};
    int prof = varied & 0x3;
    
    const char* group = profiles[prof];
    int letter_idx = (varied >> 2) % strlen(group);
    char letter = group[letter_idx];
    
    uint64_t tiebreaker = (varied >> 6) & 0xFFF;
    std::string tbstr = std::to_string(tiebreaker);
    std::reverse(tbstr.begin(), tbstr.end());
    
    return std::string({letter}) + "-" + tbstr + "-" + letter;
}

std::string ptr_to_string(void* ptr) {
    return ptr_to_string((uint64_t)ptr);
}

struct IdPool {
    list<int> ids;
    int top = 0;
    
    void init(list<int> available) {
        ids = available;
        top = available.length();
    }
    
    int alloc() {
        if(top == 0) return -1;
        return ids[--top];
    }
    
    void free(int id) {
        if(id != -1) 
            ids[top++] = id;
    }
};








#define ACORN_DEBUG 1

namespace Acorn {

    bool ERROR_FLAG = false;
    std::string ERROR_MSG = "";

    template<typename... Args>
    void throw_error(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        ERROR_MSG = oss.str();
        ERROR_FLAG = true;
        print(red("ERROR: "),ERROR_MSG);
    }

    #if ACORN_DEBUG
        #define DEBUG_ONLY(x) x
    #else
        #define DEBUG_ONLY(x)
    #endif

    uint32_t hashBytes(const void* data, uint32_t size) {
        uint32_t hash = 5381;
        const uint8_t* bytes = (const uint8_t*)data;
        for(uint32_t i = 0; i < size; i++) {
            hash = ((hash << 5) + hash) + bytes[i];
        }
        return hash;
    }

    struct Ptr {
        Ptr() {}
        Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx) : pool(_pool), idx(_idx), sidx(_sidx) {}
        uint32_t pool = 0; //Pool it's at
        uint32_t idx = 0; //Column
        uint32_t sidx = 0; //Row

        inline bool operator==(const Ptr& other) const {return pool == other.pool && idx == other.idx && sidx == other.sidx;}
        inline bool operator!=(const Ptr& other) const {return !(*this == other);}
    };

    struct Ptr4 {
        Ptr4() {}
        Ptr4(uint32_t _midx, Ptr p) : midx(_midx), ptr(p) {}
        uint32_t midx = 0;
        Ptr ptr;
    };

    static const Ptr deadptr = {0,0,0};
    static Ptr dead_ref = {0,0,0};

    struct QCol {
        QCol() {}
        uint8_t* storage = nullptr;
        uint32_t size = 0;
        uint32_t capacity = 0;
        
        inline bool empty() {return size==0;}
        void reserve(uint32_t new_capacity) {
            if(new_capacity <= capacity) return;
            
            uint8_t* newPtr = new uint8_t[new_capacity];
            if(storage) memcpy(newPtr, storage, size);
            delete[] storage;
            storage = newPtr;
            capacity = new_capacity;
        }
        void resize(uint32_t new_size) {
            if (new_size > capacity) {
                reserve(new_size);
            }
            size = new_size;
        }
        void push(const void* element, uint32_t width) {
            uint32_t old_size = size;
            uint32_t new_size = size+width;
            if(new_size>=capacity) {
                reserve(new_size*2);
            }
            memcpy(&storage[old_size], element, width);
            size = new_size;
        }
        void push_default(uint32_t width) {
            size_t old_size = size;
            resize(old_size + width);
            memset(&storage[old_size], 0, width);
        }
        void insert(uint32_t index, const void* element, uint32_t width) {
            uint32_t byte_pos = index * width;
            uint32_t new_size = size + width;
            if(new_size >= capacity) reserve(new_size * 2);
            memmove(&storage[byte_pos + width], &storage[byte_pos], size - byte_pos);
            memcpy(&storage[byte_pos], element, width);
            size = new_size;
        }
        inline void* qget(uint32_t offset) {
            DEBUG_ONLY(if(offset>=size) {throw_error(red("col:qget "),"offset ",offset," out of bounds for size ",size);return nullptr;})
            return &storage[offset];
        }
        inline void* operator[](uint32_t index) {return qget(index);}
        inline void qset(uint32_t offset, const void* element, uint32_t width) {memcpy(&storage[offset], element, width);}
        void removeAt(uint32_t index, uint32_t width) {
            size_t byte_start = index * width;
            for(size_t i = byte_start; i < size - width; i++) {
                storage[i] = storage[i + width];
            }
            resize(size - width);
        }
        void clear() {size = 0;}
        void pop(void* out, uint32_t width) {
            memcpy(out, qget((size/width - 1) * width), width);
            resize(size - width);
        }
    };

    struct QString : QCol {
        QString() {}
        QString(QCol q) : QCol(q) {}
        char& at(uint32_t idx) {return *(char*)qget(idx);}
        char& operator[](uint32_t idx) {return *(char*)qget(idx);}
        void push(char c) {QCol::push((void*)&c,1);}
        uint32_t length() {return size;}

        void operator=(const std::string& s) {clear(); for(char c : s) push(c);}
        void operator=(const char* s) {clear(); while(*s) push(*s++);}
        bool operator==(const std::string& s) {
            if(size != s.length()) return false;
            return memcmp(storage, s.data(), size) == 0;
        }
        bool operator==(const char* s) {
            uint32_t len = strlen(s);
            if(size != len) return false;
            return memcmp(storage, s, size) == 0;
        }
        std::string to_std() {
            if(!storage) return "";
            return std::string((char*)storage, size);
        }
    };
    std::ostream& operator<<(std::ostream& os, QString& s) {
        if(s.storage) os.write((const char*)s.storage, s.size);
        return os;
    }


    struct CCol : QCol {
        CCol() {}
        CCol(uint32_t _size) : element_size(_size) {}
        CCol(QCol q) : QCol(q) {}
        uint32_t element_size = 1;
        uint32_t tag = 0;
        uint32_t hash = 0;
        uint32_t index = 0;
        bool live = true;

        inline uint32_t length() {return size / element_size;}
        void push(const void* element) {
            QCol::push(element,element_size);
        }
        void operator<<(const void* element) {push(element);}
        void push_default() {QCol::push_default(element_size);}
        void insert(uint32_t index, const void* element) {
            QCol::insert(index, element, element_size);
        }
        
        inline void* sget(uint32_t index) {
            DEBUG_ONLY(if(index*element_size>=size) {throw_error(red("col:sget "),"index ",index," out of bounds for size ",size,", elment size is ",element_size," tag is ",tag);return nullptr;})
            return &storage[index * element_size];
        }
        inline void* iget(uint32_t index, uint32_t offset) {
            DEBUG_ONLY(if(index*element_size+offset>=size) {throw_error(red("col:iget "),"index ",index," plus offset ",offset," out of bounds for size ",size);return nullptr;})
            return &storage[index * element_size + offset];
        }
        inline void set(uint32_t index, const void* element) {memcpy(&storage[index * element_size], element, element_size);}
        inline void iset(uint32_t index, uint32_t offset, const void* element, uint32_t width) {memcpy(&storage[index * element_size + offset], element, element_size);}
        void removeAt(uint32_t index) {QCol::removeAt(index,element_size);}
        void pop(void* out) {QCol::pop(out,element_size);}
    };

    struct QCellCol : QCol {
        QCellCol() {}
        QCellCol(QCol q) : QCol(q) {}
        CCol& get(uint32_t idx) {return *(CCol*)qget(idx*sizeof(CCol));}
        CCol& operator[](uint32_t idx) {return *(CCol*)qget(idx*sizeof(CCol));}
        void push(CCol c) {QCol::push((void*)&c,sizeof(CCol));}
        uint32_t length() {return size/sizeof(CCol);}
    };

    struct Col : CCol {
        Col() {}
        Col(uint32_t _size) :  CCol(_size) {}
        Col(CCol q) : CCol(q) {}
        bool heterogenous = false;
        QString label;
        QCellCol cells;
        
        inline void* get(uint32_t index) {
            if(heterogenous) {
                return qget(index);
            } else {
                return sget(index);
            }
        }
        inline void* operator[](uint32_t index) {return get(index);}
        inline void* last() {return get(size-1);}

        void qput(const void* element, const void* key, uint32_t key_size, uint32_t key_tag) {
            CCol c;
            c.element_size = key_size; 
            c.tag = key_tag;
            c.hash = hashBytes(key, key_size);
            c.index = length();
            c.push(key);
            push(element);
            cells.push(c);
        }
        void* get(const void* key, uint32_t size) {
            uint32_t h = hashBytes(key, size);
            for(int i = 0; i < cells.length(); i++) {
                CCol& c = cells[i];
                if(c.hash == h) {
                    if(memcmp(c.storage, key, size) == 0) { //Collison check against the stored key
                        return sget(c.index);
                    }
                }
            }
            return nullptr;
        }
        uint32_t getidx(const void* key, uint32_t size) {
            uint32_t h = hashBytes(key, size);
            for(int i = 0; i < cells.length(); i++) {
                CCol& c = cells[i];
                if(c.hash == h) {
                    if(memcmp(c.storage, key, size) == 0) { //Collison check against the stored key
                        return c.index;
                    }
                }
            }
            return 0;
        }
        bool hasKey(const void* key, uint32_t size) {
            uint32_t h = hashBytes(key, size);
            for(int i = 0; i < cells.length(); i++) {
                CCol& c = cells[i];
                if(c.hash == h && memcmp(c.storage, key, size) == 0) return true;
            }
            return false;
        }

        void put(const std::string& str, const void* element, uint32_t tag = 0) {qput(element,str.data(),str.length(),tag);}
        void* get(const std::string& str) {return get(str.data(), str.length());}
        bool hasKey(const std::string& str) {return hasKey(str.data(), str.length());}
        void put(uint64_t u64, const void* element, uint32_t tag = 0) {qput(element,(void*)&u64,8,tag);}
        void* get(uint64_t u64) {return get((void*)&u64, 8);}
        bool hasKey(uint64_t u64) {return hasKey((void*)&u64, 8);}
        void put(Ptr p, const void* element, uint32_t tag = 0) {qput(element, (void*)&p, sizeof(Ptr), tag);}
        void* get(Ptr p) {return get((void*)&p, sizeof(Ptr));}
        bool hasKey(Ptr p) {return hasKey((void*)&p, sizeof(Ptr));}
    };

    //Convience for ergonomic white/blacklist things
    struct _lookup {
        _lookup(list<std::string> init, bool _default_state) 
        : default_state(_default_state)  {
            for(auto s : init) {
                lookup[s] = !default_state;
            }
        }

        map<std::string,bool> lookup;
        bool default_state;

        bool operator[](const std::string& key) {
            return lookup.getOrDefault(key,default_state);
        }
    };

    uint32_t add_column(Col& col, size_t size = 0, uint32_t tag = 0) {
        Col ncol(size);
        ncol.tag = tag;
        col.push((void*)&ncol);
        return col.length()-1;
    }
    
    uint32_t note_value(Col& col, const std::string& key, uint32_t size, uint32_t tag) {
        uint32_t at = add_column(col, size, tag);
        (*(Col*)col.sget(at)).label = key;
        return at;
    }

    //Standard column create, use pooling means it will try to find a dead column first, tag sensitive means it will also ensure the column tag matches
    uint32_t create_column(Col& col, uint32_t size, uint32_t tag, bool use_pooling = true, bool tag_sensitive = false) {
        if(use_pooling) {
            for(int i=0;i<col.length();i++) {
                Col& ncol = *(Col*)col.sget(i);
                if(!ncol.live&&ncol.element_size==size&&(!tag_sensitive||ncol.tag==tag)) {
                    ncol.clear();
                    ncol.live = true;
                    return i;
                }
            }
        }
        add_column(col,size,tag);
        return col.length()-1;
    }
    //Creates a column from pool and intilizes it's memory if empty
    uint32_t push_column(Col& col, uint32_t size, uint32_t tag) {
        uint32_t at = create_column(col,size,tag);
        Col& ncol = *(Col*)col.sget(at);
        if(ncol.size<size) {
            ncol.resize(size);
        }
        return at;
    }
    void recycle_column(Col& col, uint32_t id) {
       (*(Col*)col.sget(id)).live = false;
    }



    static void write_qcol(std::ostream& out, QCol& col) {
        write_raw<uint32_t>(out, col.size);
        out.write((const char*)col.storage, col.size);
    }

    static QCol read_qcol(std::istream& in) {
        QCol col;
        uint32_t size = read_raw<uint32_t>(in);
        col.resize(size);
        in.read((char*)col.storage, col.size);
        return col;
    }

    static void write_ccol(std::ostream& out, CCol& col) {
        write_qcol(out,col);
        write_raw<uint32_t>(out, col.element_size);
        write_raw<uint32_t>(out, col.tag);
        write_raw<uint32_t>(out, col.hash);
        write_raw<bool>(out, col.live);
    }

    static CCol read_ccol(std::istream& in) {
        CCol col = read_qcol(in);
        col.element_size = read_raw<uint32_t>(in);
        col.tag = read_raw<uint32_t>(in);
        col.hash = read_raw<uint32_t>(in);
        col.live = read_raw<bool>(in);
        return col;
    }

    static void write_col(std::ostream& out, Col& col) {
        write_ccol(out,col);
        write_raw<bool>(out, col.heterogenous);
        write_qcol(out,col.cells);
        write_qcol(out,col.label);
    }

    static Col read_col(std::istream& in) {
        Col col = read_ccol(in);
        col.heterogenous = read_raw<bool>(in);
        col.cells = read_qcol(in);
        col.label = read_qcol(in);
        return col;
    }
}


namespace Acorn {   
    static int _ctx_dummy_index = 0;
    class Unit;
    struct Node;
    struct Value;

    map<uint32_t,std::string> labels;

    template<typename T>
    struct TCol : Col {
        TCol() : Col(sizeof(T)) {}
        TCol(Col c) : Col(c) {} 
        T& get(uint32_t idx) {return *(T*)Col::sget(idx);}
        void set(uint32_t idx, T val) {Col::set(idx,(void*)&val);}
        T& operator[](uint32_t idx) {return *(T*)Col::sget(idx);}
        void push(T t){Col::push((void*)&t);}
    };
    
    using TypeCol     = TCol<Col>;
    using TypeTypeCol = TCol<TypeCol>;
    TypeTypeCol types;

    inline void* resolve_ptr(const Ptr& ptr) {
        return types[ptr.pool][ptr.idx].get(ptr.sidx);
    }

    inline void* resolve_ptr(const Ptr& ptr, const uint32_t& idx) {
        return types[ptr.pool][idx].get(ptr.sidx);
    }

    inline Ptr& resolve_to_ptr(const Ptr& ptr) {
        return *(Ptr*)types[ptr.pool][ptr.idx].get(ptr.sidx);
    }

    inline Ptr& resolve_to_ptr(const Ptr& ptr, const uint32_t& idx) {
        return *(Ptr*)types[ptr.pool][idx].get(ptr.sidx);
    }

    inline Col& resolve_to_col(const Ptr& ptr) {
        return types[ptr.pool][ptr.idx];
    }

    inline Col& resolve_to_col(const Ptr& ptr, const uint32_t& idx) {
        return types[ptr.pool][idx];
    }

    inline Col& to_col(const Ptr& ptr) {
        return types[ptr.pool][ptr.idx];
    }

    inline Ptr get_ticket(uint32_t type_id, uint32_t size, uint32_t tag) {
        Ptr ticket{type_id,create_column(types[type_id],size,tag),0};
        return ticket;
    }

    std::string tag_to_str(uint32_t tag, void* data);

    list<Ptr> marked_ptrs;
    map<uint64_t,std::function<void(std::string&)>> ptr_colors;
    inline uint64_t Ptr_to_key(Ptr p) {
        return ((uint64_t)p.pool << 32) | (uint64_t)p.idx;
    }
    inline Ptr key_to_Ptr(uint64_t key) {
        return Ptr{(uint32_t)(key >> 32), (uint32_t)(key & 0xFFFFFFFF), 0};
    }
    uint64_t make_overload_key(uint32_t root, uint32_t right) {
        return ((uint64_t)root << 32) | right;
    }
    inline std::pair<uint32_t,uint32_t> decode_key(uint64_t key) {
        return std::make_pair<uint32_t,uint32_t>((uint32_t)(key >> 32), (uint32_t)(key & 0xFFFFFFFF));
    }

    void mark_error(Ptr ptr) {marked_ptrs << ptr;}


    struct string : Ptr {
        string() {}
        string(Ptr p) {pool=p.pool;idx=p.idx;sidx=p.sidx;}
        inline Col& col() {DEBUG_ONLY(if(idx>=types[pool].length()) {throw_error("invalid string col: ",idx," for type ",pool);}) return types[pool][idx]; }
        inline char at(uint32_t idx) {return *(char*)col()[idx];}
        inline uint32_t length() {return col().length();}
        inline void push(char c) { col().push(&c); }
        inline void push(const char* s, uint32_t len) {for(uint32_t i = 0; i < len; i++) col().push(&s[i]);}
        inline void push(const std::string& s) { push(s.data(), s.length()); }
        inline void operator=(const std::string& s){ col().clear(); push(s);}
        inline void operator=(string s){ col().clear(); push((const char*)s.col().storage, s.length());}
        inline void operator=(const char* s) { col().clear(); push(s, strlen(s)); }
        inline char& operator[](uint32_t idx) { return *(char*)col().get(idx); }
        std::string to_std() {Col& c = col(); DEBUG_ONLY(if(ERROR_FLAG) {return ERROR_MSG;}); return std::string((char*)c.storage, length());}
        inline int find(string look_for, int start_at, int nth_of = 1) {
            int found_at = -1;
            for(int i=start_at;i<col().length();i++) {
                bool match = true;
                for(int s=0;s<look_for.length();s++) {
                    if(*(char*)col()[i+s]!=look_for[s]) {
                        match = false;
                        break;
                    }
                }
                if(match) {
                    if(--nth_of<=0) {
                        found_at = i; break;
                    }
                }
            }
            return found_at;
        }
    };
    string get_string_ticket();

    struct type_and_value {
        uint32_t type;
        Ptr value;
    };

    uint32_t offsets_col = 0;
    uint32_t tags_col = offsets_col + 1;
    uint32_t sizes_col = tags_col + 1;
    uint32_t labels_col = sizes_col + 1;
    uint32_t subtags_col = labels_col + 1;
    uint32_t subsizes_col = subtags_col + 1;
    uint32_t ptrs_col = subsizes_col + 1;

    uint32_t overloads_col = ptrs_col + 1;

    struct _layout {
        _layout() {}
        _layout(Ptr p) : impl(p) {}
        Ptr impl = deadptr;
        map<std::string,uint32_t> label_to_index;
        uint32_t total_size = 0;

        list<uint32_t> offsets;
        list<uint32_t> tags;
        list<uint32_t> sizes;
        list<std::string> labels;

        list<uint32_t> subtags;
        list<uint32_t> subsizes;
        list<Ptr> ptrs;

        Col overloads = Col(sizeof(type_and_value));

        void add_overload(uint64_t key, uint32_t type, Ptr value) {
            type_and_value tnv{type, value};
            overloads.put(key,(void*)&tnv);
            resolve_to_col(impl,impl.idx+overloads_col).put(key,(void*)&tnv);
        }
        bool has_overload(uint64_t key) {
            return overloads.hasKey(key);
        }
        type_and_value get_overload(uint64_t key) {
            return *(type_and_value*)overloads.get(key);
        }


        uint32_t add_prop(uint32_t tag, uint32_t size, const std::string& label, uint32_t subtag = 0, uint32_t subsize = 0, Ptr ptr = {0,0,0}) {
            label_to_index.put(label,offsets.length());
            offsets << total_size;
            tags << tag;
            sizes << size;
            labels << label;
            subtags << subtag;
            subsizes << subsize;
            ptrs << ptr;

            resolve_to_col(impl,impl.idx+offsets_col).push((void*)&total_size);
            resolve_to_col(impl,impl.idx+tags_col).push((void*)&tag);
            resolve_to_col(impl,impl.idx+sizes_col).push((void*)&size);
            string label_ptr = get_string_ticket();
            label_ptr = label;
            resolve_to_col(impl,impl.idx+labels_col).push((void*)&label_ptr);
            resolve_to_col(impl,impl.idx+subtags_col).push((void*)&subtag);
            resolve_to_col(impl,impl.idx+subsizes_col).push((void*)&subsize);
            resolve_to_col(impl,impl.idx+ptrs_col).push((void*)&ptr);

            uint32_t old_size = total_size;
            total_size += size;
            return old_size;
        }
        void print_out() { //Make to_string later
            for(int i=0;i<offsets.length();i++) {
                print(i,": ",labels[i],": ",offsets[i],", ",Acorn::labels[tags[i]],", ",Acorn::labels[subtags[i]],"[",sizes[i],"]");
            }
            // for(auto e : overload.entrySet()) {
            //     auto keyl = decode_key(e.key);
            //     print(Acorn::labels[keyl.first]," ",Acorn::labels[keyl.second],"(",keyl.second,"): ",Acorn::labels[e.value.type]);
            // }
        }
    };
    map<uint32_t,_layout> layouts;

    void make_wrapper_for_layout(_layout& l, const std::string& name,  const std::string& output_path) {
        std::string s = "";
        std::string pad = "     ";
        s+="struct "+name+" : Ptr {\n";
        s += pad+name+"() {}\n";
        s += pad+name+"(uint32_t p, uint32_t i, uint32_t s) { pool = p; idx = i; sidx = s; }\n";
        s += pad+name+"(Ptr p) { pool = p.pool; idx = p.idx; sidx = p.sidx; }\n";
        for(int i=0;i<l.offsets.length();i++) {
            s+="\n";
            std::string type = labels[l.tags[i]];
            std::string label = l.labels[i];
            uint32_t offset = l.offsets[i];
            uint32_t size = l.sizes[i];

            bool is_compound = layouts.hasKey(l.tags[i]);
            bool is_ptr = l.tags[i]==ptr_id;

            if(is_ptr) {
                s += pad+pad_str("inline "+pad_str("Ptr&",12)+" "+label+"_ptr()",32)+"{return *(Ptr*)types[pool][idx].qget(sidx+"+std::to_string(offset)+"); }\n";
                s += pad+pad_str("inline "+pad_str("Col&",12)+" "+label+"_col()",32)+"{return  resolve_to_col("+label+"_ptr());}\n";
                s += pad+pad_str("inline "+pad_str("void",12)+" "+label+"(Ptr p)",32)+"{types[pool][idx].qset(sidx+"+std::to_string(offset)+", (void*)&p, "+std::to_string(size)+"); }\n";
            } else if(is_compound) {
                s += pad+pad_str("inline "+pad_str(type,12)+" "+label+"()",32)+"{return {pool, idx, sidx+"+std::to_string(offset)+"}; }\n";
                s += pad+pad_str("inline "+pad_str("void",12)+" "+label+"("+type+" t)",32)+"{types[pool][idx].qset(sidx+"+std::to_string(offset)+", types[t.pool][t.idx].qget(t.sidx), "+std::to_string(size)+"); }\n";
            } else {
                s += pad+pad_str("inline "+pad_str(type,12)+" "+label+"()",32)+"{return *("+type+"*)types[pool][idx].qget(sidx+"+std::to_string(offset)+"); }\n";
                s += pad+pad_str("inline "+pad_str("void",12)+" "+label+"("+type+" t)",32)+"{types[pool][idx].qset(sidx+"+std::to_string(offset)+", (void*)&t, "+std::to_string(size)+"); }\n";
            }
        }
        s+="};";
        editTextFile(output_path,[s](std::string& source){
            source += (source.empty()?"":"\n\n")+s;
        });
    }

    bool is_live(Ptr p) {return (p.pool!=0||p.idx!=0)&&p.pool<types.length();}

    uint32_t undefined_id = 0;
    uint32_t stages_id = 1;
    uint32_t ptr_id = 2;



    uint32_t add_type() {
        uint32_t at = types.length();
        TypeCol to_return;
        types.push(to_return);
        return at;
    }

    Ptr add_type_for_handle() {
        Ptr to_return{add_type(),0,0};
        return to_return;
    }

    uint32_t init_handler_type() {
        TypeCol t;
        uint32_t at = types.length();
        note_value(t,"UNDEFINED",0,0);
        t.get(0).push_default(); //UNDEFINED cell
        note_value(t,"stages",sizeof(Ptr),ptr_id);
        t.get(1).put("Layouts",(void*)&deadptr); //Layouts label
        note_value(t,"ptr",sizeof(Ptr),ptr_id);
        t.get(2).push_default(); //Layout of Ptr
        types.push(t);
        return at;
    }

    uint32_t handler_type_id = init_handler_type();
    
    uint32_t init_layout_type() {
        TypeCol t;
        uint32_t at = types.length();
        types.push(t);
        return at;
    }
    uint32_t layout_type_id = init_layout_type(); 

    uint32_t reg_id(const std::string& label) {
        uint32_t at = types[handler_type_id].length();
        note_value(types[handler_type_id],label,sizeof(Ptr),ptr_id);
        types[handler_type_id][at].push_default();
        labels.put(at,label);
        return at;
    }

    uint32_t prefix_ptr_id = reg_id("prefix_ptr"); uint32_t suffix_ptr_id = reg_id("suffix_ptr");
    uint32_t float_id = reg_id("float"); uint32_t prefix_float_id = reg_id("prefix_float"); uint32_t suffix_float_id = reg_id("suffix_float");
    uint32_t int_id = reg_id("int"); uint32_t prefix_int_id = reg_id("prefix_int"); uint32_t suffix_int_id = reg_id("suffix_int");
    uint32_t bool_id = reg_id("bool"); uint32_t prefix_bool_id = reg_id("prefix_bool"); uint32_t suffix_bool_id = reg_id("suffix_bool");
    uint32_t string_id = reg_id("string"); uint32_t prefix_string_id = reg_id("prefix_string"); uint32_t suffix_string_id = reg_id("suffix_string");
    uint32_t char_id = reg_id("char"); uint32_t prefix_char_id = reg_id("prefix_char"); uint32_t suffix_char_id = reg_id("suffix_char");
    uint32_t ptr4_id = reg_id("ptr4"); uint32_t prefix_ptr4_id = reg_id("prefix_ptr4"); uint32_t suffix_ptr4_id = reg_id("suffix_ptr4");
    size_t list_id = reg_id("list");
    size_t map_id = reg_id("map");
    size_t weakptr_id = reg_id("weakptr");
    size_t col_id = reg_id("col");
    
    uint32_t silenced_id = reg_id("SILENCED");
    uint32_t any_id = reg_id("any");
    uint32_t null_id = reg_id("null");
    size_t identifier_id = reg_id("IDENTIFIER");
    size_t object_id = reg_id("OBJECT");
    size_t literal_id = reg_id("LITERAL");
    
    size_t node_id = reg_id("node"); size_t prefix_node_id = reg_id("prefix_node"); size_t suffix_node_id = reg_id("suffix_node");
    size_t value_id = reg_id("value"); size_t prefix_value_id = reg_id("prefix_value"); size_t suffix_value_id = reg_id("suffix_value");
    size_t context_id = reg_id("context"); size_t prefix_context_id = reg_id("prefix_context"); size_t suffix_context_id = reg_id("suffix_context");

    size_t var_decl_id = reg_id("VAR_DECL");
    size_t func_call_id = reg_id("FUNC_CALL");
    size_t method_call_id = reg_id("METHOD_CALL");
    size_t function_id = reg_id("FUNCTION");
    size_t method_id = reg_id("METHOD");
    size_t func_decl_id = reg_id("FUNC_DECL");
    size_t type_decl_id = reg_id("TYPE_DECL");

    size_t tombstone_col = 0; 
    size_t refs_col = 0;

    Ptr add_layout_to_col(uint32_t type) {
        Ptr p = {layout_type_id,note_value(types[layout_type_id],labels[type]+" Offsets",4,int_id),0};
        note_value(types[layout_type_id],"Tags",4,int_id);
        note_value(types[layout_type_id],"Sizes",4,int_id);
        note_value(types[layout_type_id],"Labels",sizeof(Ptr),string_id);
        note_value(types[layout_type_id],"Subtags",4,int_id);
        note_value(types[layout_type_id],"Subsizes",4,int_id);
        note_value(types[layout_type_id],"Ptrs",sizeof(Ptr),ptr_id);
        note_value(types[layout_type_id],"Overloads",sizeof(Ptr4),ptr4_id);
        types[handler_type_id][type].set(0,(void*)&p);
        return p;
    }

    uint32_t make_store_type() {
        uint32_t at = add_type();
        return at;
    }
    
    uint32_t node_type_offset = 0;
    uint32_t node_sub_type_offset = 0;
    uint32_t node_name_offset = 0;
    uint32_t x_offset = 0;
    uint32_t y_offset = 0;
    uint32_t z_offset = 0;
    uint32_t node_value_offset = 0;
    uint32_t node_children_offset = 0;
    uint32_t node_quals_offset = 0;
    uint32_t node_node_table_offset = 0;
    uint32_t node_value_table_offset = 0;
    uint32_t node_scopes_offset = 0;
    uint32_t parent_offset = 0;
    uint32_t owner_offset = 0;
    uint32_t in_scope_offset = 0;
    uint32_t resolved_offset = 0;
    uint32_t node_opt_str_offset = 0;
    uint32_t mute_offset = 0;

    uint32_t value_type_offset = 0;
    uint32_t value_sub_type_offset = 0;
    uint32_t value_data_offset = 0;
    uint32_t address_offset = 0;
    uint32_t reg_offset = 0;
    uint32_t loc_offset = 0;
    uint32_t size_offset = 0;
    uint32_t sub_size_offset = 0;
    uint32_t value_quals_offset = 0;
    uint32_t value_sub_values_offset = 0;
    uint32_t type_scope_offset = 0;
    uint32_t store_offset = 0;

    uint32_t context_node_offset = 0;
    uint32_t context_qual_offset = 0;
    uint32_t context_left_offset = 0;
    uint32_t context_out_offset = 0;
    uint32_t context_root_offset = 0;
    uint32_t context_result_offset = 0;
    uint32_t context_value_offset = 0;
    uint32_t context_index_offset = 0;
    uint32_t context_state_offset = 0;
    uint32_t context_flag_offset = 0;
    uint32_t context_sub_offset = 0;
    uint32_t context_source_offset = 0;

    uint32_t init_node_type();
    uint32_t init_value_type();
    uint32_t init_context_type();

    uint32_t name_store_id = make_store_type();
    uint32_t node_type_id = init_node_type();
    uint32_t value_type_id = init_value_type();
    uint32_t context_type_id = init_context_type();
    uint32_t children_store_id = make_store_type();
    uint32_t quals_store_id = make_store_type();
    uint32_t node_table_store_id = make_store_type(); 
    uint32_t value_table_store_id = make_store_type(); 
    uint32_t scopes_store_id = make_store_type(); 
    uint32_t opt_str_store_id = make_store_type();
    uint32_t data_store_id = make_store_type();
    uint32_t sub_value_store_id = make_store_type();

    string get_string_ticket() {
        return get_ticket(name_store_id,1,char_id);
    }

    _layout& add_template(uint32_t for_type) {
        Ptr p = add_layout_to_col(for_type);
        _layout temp(p);
        layouts[for_type] = temp;
        return layouts.get(for_type);
    }

    uint32_t init_node_type() {
        uint32_t at = add_type();
        TypeCol& t = types[at];

        _layout& ntemp = add_template(node_id); //Node template
        node_type_offset = ntemp.add_prop(int_id,4,"type");
        node_sub_type_offset = ntemp.add_prop(int_id,4,"sub_type");
        node_name_offset = ntemp.add_prop(string_id,sizeof(Ptr),"name",char_id,1);
        x_offset = ntemp.add_prop(float_id,4,"x");
        y_offset = ntemp.add_prop(float_id,4,"y");
        z_offset = ntemp.add_prop(float_id,4,"z");
        node_value_offset = ntemp.add_prop(value_id,sizeof(Ptr),"value");
        node_children_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"children",node_id,sizeof(Ptr));
        node_quals_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"quals",node_id,sizeof(Ptr));
        node_node_table_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"node_table",node_id,sizeof(Ptr));
        node_value_table_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"value_table",value_id,sizeof(Ptr));
        node_scopes_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"scopes",node_id,sizeof(Ptr));
        parent_offset = ntemp.add_prop(node_id,sizeof(Ptr),"parent");
        owner_offset = ntemp.add_prop(node_id,sizeof(Ptr),"owner");
        in_scope_offset = ntemp.add_prop(node_id,sizeof(Ptr),"in_scope");
        resolved_offset = ntemp.add_prop(bool_id,1,"resolved");
        node_opt_str_offset = ntemp.add_prop(string_id,sizeof(Ptr),"opt_str");
        mute_offset = ntemp.add_prop(bool_id,1,"mute");

        return at;
    }

    uint32_t init_value_type() {
        uint32_t at = add_type();
        TypeCol& t = types[at];

        _layout& vtemp = add_template(value_id); //Value template
        value_type_offset = vtemp.add_prop(int_id,4,"type");
        value_sub_type_offset = vtemp.add_prop(int_id,4,"sub_type");
        value_data_offset = vtemp.add_prop(ptr_id,sizeof(Ptr),"data");
        address_offset = vtemp.add_prop(int_id,4,"address");
        reg_offset = vtemp.add_prop(int_id,4,"reg");
        loc_offset = vtemp.add_prop(int_id,4,"loc");
        size_offset = vtemp.add_prop(int_id,4,"size");
        sub_size_offset = vtemp.add_prop(int_id,4,"sub_size");
        value_quals_offset = vtemp.add_prop(ptr_id,sizeof(Ptr),"quals",node_id,sizeof(Ptr));
        value_sub_values_offset = vtemp.add_prop(ptr_id,sizeof(Ptr),"sub_values",value_id,sizeof(Ptr));
        type_scope_offset = vtemp.add_prop(node_id,sizeof(Ptr),"type_scope");
        store_offset = vtemp.add_prop(ptr_id,sizeof(Ptr),"store");

        return at;
    }

    uint32_t init_context_type() {
        uint32_t at = add_type();
        TypeCol& t = types[at];
        _layout& ctemp = add_template(context_id); //Context template
        context_node_offset = ctemp.add_prop(node_id,sizeof(Ptr),"node");
        context_qual_offset = ctemp.add_prop(node_id,sizeof(Ptr),"qual");
        context_left_offset = ctemp.add_prop(node_id,sizeof(Ptr),"left");
        context_out_offset = ctemp.add_prop(node_id,sizeof(Ptr),"out");
        context_root_offset = ctemp.add_prop(node_id,sizeof(Ptr),"root");
        context_result_offset = ctemp.add_prop(ptr_id,sizeof(Ptr),"result",node_id,sizeof(Ptr));
        context_value_offset = ctemp.add_prop(value_id,sizeof(Ptr),"value");
        context_index_offset = ctemp.add_prop(int_id,4,"index");
        context_state_offset = ctemp.add_prop(int_id,4,"state");
        context_flag_offset = ctemp.add_prop(bool_id,1,"flag");
        context_sub_offset = ctemp.add_prop(context_id,sizeof(Ptr),"sub");
        context_source_offset = ctemp.add_prop(string_id,sizeof(Ptr),"source",char_id,1);
        return at;
    }

    #define NAMED_PTRS 1

    std::string Ptr_as_string(Ptr p) {
        if(ERROR_FLAG||(p.pool>=types.length()||p.idx>=types[p.pool].length()||marked_ptrs.has(p))) {
            return red(std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx));
        }

        #if NAMED_PTRS
            std::string plabel = types[p.pool].label.empty()?std::to_string(p.pool):types[p.pool].label.to_std();
            std::string pidx = types[p.pool][p.idx].label.empty()?std::to_string(p.idx):types[p.pool][p.idx].label.to_std();
            std::string pstring = ""+plabel+"|"+pidx+"|"+std::to_string(p.sidx)+"";
            uint64_t key = Ptr_to_key(p);
        
            if(ptr_colors.hasKey(key)) {ptr_colors.get(key)(pstring);}
            return pstring;
        #else
            return ""+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx)+"";
        #endif
    }

    std::ostream& operator<<(std::ostream& os, Acorn::string& s) {
        os.write((const char*)s.col().storage, s.length());
        return os;
    }

    template<typename T>
    struct col_Ptr : Ptr {
        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but col_ptr was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline Col& col()                    {DEBUG_ONLY(if(safety_check("col_ptr:col")){static Col d; return d;}) return types[pool][idx];}
        inline uint32_t length()             {DEBUG_ONLY(if(safety_check("col_ptr:length")){return 0;}) return col().length();}
        inline bool empty()                  {DEBUG_ONLY(if(safety_check("col_ptr:empty")){return true;}) return col().empty();}
        inline void removeAt(uint32_t idx)   {DEBUG_ONLY(if(safety_check("col_ptr:removeAt")){return;}) col().removeAt(idx);}
        inline void clear()                  {DEBUG_ONLY(if(safety_check("col_ptr:clear")){return;}) col().clear();}
    
        inline T get(uint32_t idx)           {DEBUG_ONLY(if(safety_check("col_ptr:get")){return T(deadptr);}) void* ptr = col().get(idx); DEBUG_ONLY(if(safety_check("col_ptr:get:cast")){return T(deadptr);}) return T(*(Ptr*)ptr);}
        inline T operator[](uint32_t idx)    {return get(idx);}
        inline T last()                      {DEBUG_ONLY(if(safety_check("col_ptr:last")){return T(deadptr);}) return get(length()-1);}
    
        inline T take(uint32_t idx)          {DEBUG_ONLY(if(safety_check("col_ptr:take")){return T(deadptr);}) T val = get(idx); removeAt(idx); return val;}
        inline T pop()                       {DEBUG_ONLY(if(safety_check("col_ptr:pop")){return T(deadptr);}) Ptr p; col().pop(&p); return T(p);}
        inline void push(T t)                {DEBUG_ONLY(if(safety_check("col_ptr:push")){return;}) Ptr p(t); col().push(&p);}
        inline void operator<<(T t)          {push(t);}
        inline void insert( uint32_t idx, T t){DEBUG_ONLY(if(safety_check("col_ptr:insert")){return;}) Ptr p(t); col().insert(idx,&p);}
    
        inline bool hasKey(const std::string& key) {DEBUG_ONLY(if(safety_check("col_ptr:hasKey")){return false;}) return col().hasKey(key);}
        inline T get(const std::string& key) {DEBUG_ONLY(if(safety_check("col_ptr:get_key")){return T(deadptr);}) return T(*(Ptr*)col().get(key));}
        inline T operator[](const std::string& key) {return get(key);}
        inline void put(const std::string& key, T t) {DEBUG_ONLY(if(safety_check("col_ptr:put")){return;}) col().put(key, (void*)&t);}
    };
    using node_col  = col_Ptr<Node>;
    using value_col = col_Ptr<Value>;

    struct Value : public Ptr {
        Value() {}
        Value(Ptr p) { pool = p.pool; sidx = p.sidx; idx = p.idx;}

        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but value was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline uint32_t  type()                {DEBUG_ONLY(if(safety_check("value:type:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(value_type_offset);}
        inline void      type(uint32_t t)      {DEBUG_ONLY(if(safety_check("value:type:set")){return;}) types[pool][idx].qset(value_type_offset,(void*)&t,4);}
        inline uint32_t  sub_type()            {DEBUG_ONLY(if(safety_check("value:sub_type:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(value_sub_type_offset);}
        inline void      sub_type(uint32_t st) {DEBUG_ONLY(if(safety_check("value:sub_type:set")){return;}) types[pool][idx].qset(value_sub_type_offset,(void*)&st,4);}
        
        inline Ptr&      data_ptr()            {DEBUG_ONLY(if(safety_check("value:data_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(value_data_offset);}
        inline void      data_ptr(Ptr ptr)     {DEBUG_ONLY(if(safety_check("value:data_ptr:set")){return;}) resolve_to_col(*this).qset(value_data_offset,(void*)&ptr,sizeof(Ptr));}
        inline Col&      data_col()            {Ptr p = data_ptr(); return types[p.pool][p.idx];}
        
        inline uint32_t  address()             {DEBUG_ONLY(if(safety_check("value:address:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(address_offset);}
        inline void      address(uint32_t v)   {DEBUG_ONLY(if(safety_check("value:address:set")){return;}) types[pool][idx].qset(address_offset,(void*)&v,4);}
        inline int       reg()                 {DEBUG_ONLY(if(safety_check("value:reg:get")){return -1;}) return *(int*)types[pool][idx].qget(reg_offset);}
        inline void      reg(int i)            {DEBUG_ONLY(if(safety_check("value:reg:set")){return;}) types[pool][idx].qset(reg_offset,(void*)&i,4);}
        inline int       loc()                 {DEBUG_ONLY(if(safety_check("value:loc:get")){return -1;}) return *(int*)types[pool][idx].qget(loc_offset);}
        inline void      loc(int i)            {DEBUG_ONLY(if(safety_check("value:loc:set")){return;}) types[pool][idx].qset(loc_offset,(void*)&i,4);}
        
        inline uint32_t  size()                {DEBUG_ONLY(if(safety_check("value:size:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(size_offset);}
        inline void      size(uint32_t s)      {DEBUG_ONLY(if(safety_check("value:size:set")){return;}) types[pool][idx].qset(size_offset,(void*)&s,4);}
        inline uint32_t  sub_size()            {DEBUG_ONLY(if(safety_check("value:sub_size:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(sub_size_offset);}
        inline void      sub_size(uint32_t s)  {DEBUG_ONLY(if(safety_check("value:sub_size:set")){return;}) types[pool][idx].qset(sub_size_offset,(void*)&s,4);}
        
        inline Ptr&      quals_ptr()           {DEBUG_ONLY(if(safety_check("value:quals_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(value_quals_offset);}
        inline Col&      quals_col()           {Ptr& p = quals_ptr(); return types[p.pool][p.idx];}
        inline node_col  quals()               {return (node_col&)quals_ptr();}
        
        inline Ptr&       sub_values_ptr()     {DEBUG_ONLY(if(safety_check("value:sub_values_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(value_sub_values_offset);}
        inline Col&       sub_values_col()     {Ptr& p = sub_values_ptr(); return types[p.pool][p.idx];}
        inline value_col  sub_values()         {return (value_col&)sub_values_ptr();}
        
        inline Node      type_scope();
        inline void      type_scope(Ptr o)     {DEBUG_ONLY(if(safety_check("value:type_scope:set")){return;}) types[pool][idx].qset(type_scope_offset,(void*)&o,sizeof(Ptr));}
        inline Ptr&      store()               {DEBUG_ONLY(if(safety_check("value:store:get")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(store_offset);}
        inline void      store(Ptr p)          {DEBUG_ONLY(if(safety_check("value:store:set")){return;}) types[pool][idx].qset(store_offset,(void*)&p,sizeof(Ptr));}
    
        inline void setup(uint32_t _type, uint32_t _size, uint32_t _address = 0) {
            type(_type); size(_size); address(_address);
        }
    
        inline Ptr init_data() {
            Ptr dataptr{data_store_id, create_column(types[data_store_id], size(), type()), 0};
            types[data_store_id][dataptr.idx].push_default();
            types[pool][idx].qset(value_data_offset,(void*)&dataptr,sizeof(Ptr));
            return dataptr;
        }
    
        inline void set(void* data) {
            DEBUG_ONLY(if(safety_check("value:set")){return;})
            Ptr dataptr = data_ptr();
            if(!is_live(dataptr)) {
                dataptr = init_data();
            }
            // print("SET AT: ",Ptr_as_string(dataptr));
            // print("FROM: ",tag_to_str(types[dataptr.pool][dataptr.idx].tag,types[dataptr.pool][dataptr.idx].get(dataptr.sidx)));
            // print("TO: ",tag_to_str(types[dataptr.pool][dataptr.idx].tag,data));
            types[dataptr.pool][dataptr.idx].set(dataptr.sidx, data);
        }
    
        inline void* get() {
            DEBUG_ONLY(if(safety_check("value:get")){return nullptr;})
            Ptr dataptr = data_ptr();
            return types[dataptr.pool][dataptr.idx].get(dataptr.sidx);
        }

        inline void* sget() {
            DEBUG_ONLY(if(safety_check("value:sget")){return nullptr;})
            Ptr dataptr = data_ptr();
            return types[dataptr.pool][dataptr.idx].sget(dataptr.sidx);
        }
        
        inline void* qget() {
            DEBUG_ONLY(if(safety_check("value:qget")){return nullptr;})
            Ptr dataptr = data_ptr();
            return types[dataptr.pool][dataptr.idx].qget(dataptr.sidx);
        }

        inline void copy(Value o, bool is_deep) {
            Col& src = types[o.pool][o.idx];
            Col& dst = types[pool][idx];
            memcpy(dst.storage, src.storage, layouts[value_id].total_size);
            if(is_deep) {
                if(is_live(o.data_ptr())) {
                    init_data();
                    set(o.get());
                }

                Ptr qualsptr = get_ticket(quals_store_id, sizeof(Ptr), ptr_id);
                Col& new_quals = resolve_to_col(qualsptr);
                Col& old_quals = o.quals_col();
                new_quals.reserve(old_quals.size);
                memcpy(new_quals.storage, old_quals.storage, old_quals.size);
                new_quals.size = old_quals.size;
                types[pool][idx].qset(value_quals_offset,(void*)&qualsptr,sizeof(Ptr));
        
                Ptr subvalsptr = get_ticket(sub_value_store_id, sizeof(Ptr), ptr_id);
                Col& new_subvals = resolve_to_col(subvalsptr);
                Col& old_subvals = o.sub_values_col();
                new_subvals.reserve(old_subvals.size);
                memcpy(new_subvals.storage, old_subvals.storage, old_subvals.size);
                new_subvals.size = old_subvals.size;
                types[pool][idx].qset(value_sub_values_offset,(void*)&subvalsptr,sizeof(Ptr));
            }
        }

        int find_qual(uint32_t q_id);
        Node get_qual(uint32_t q_id);
    
        bool has_qual(uint32_t q_id) {
            return find_qual(q_id)!=-1;
        }
    };
    
    struct QNode : public Col {
        inline uint32_t& type()                   {return *(uint32_t*)&storage[node_type_offset];}
        inline void      type(uint32_t t)         {memcpy(&storage[node_type_offset],(void*)&t,4);}
        inline uint32_t& sub_type()               {return *(uint32_t*)&storage[node_sub_type_offset];}
        inline void      sub_type(uint32_t st)    {memcpy(&storage[node_sub_type_offset],(void*)&st,4);}
    };

    struct Node : public Ptr {
        Node() {}
        Node(Ptr p) { pool = p.pool; sidx = p.sidx; idx = p.idx;}
    
        inline QNode& toQ() {return (QNode&)types[pool][idx];}

        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but node was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline uint32_t  type()                {DEBUG_ONLY(if(safety_check("node:type:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(node_type_offset);}
        inline void      type(uint32_t t)      {DEBUG_ONLY(if(safety_check("node:type:set")){return;}) types[pool][idx].qset(node_type_offset,(void*)&t,4);}
        inline uint32_t  sub_type()            {DEBUG_ONLY(if(safety_check("node:sub_type:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(node_sub_type_offset);}
        inline void      sub_type(uint32_t st) {DEBUG_ONLY(if(safety_check("node:sub_type:set")){return;}) types[pool][idx].qset(node_sub_type_offset,(void*)&st,4);}
        
        inline Ptr&      name_ptr()            {DEBUG_ONLY(if(safety_check("node:name_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_name_offset);}
        inline Col&      name_col()            {Ptr& p = name_ptr(); return types[p.pool][p.idx];}
        inline string    name()                {return string(name_ptr());}
        inline void      name(std::string s)   {DEBUG_ONLY(if(safety_check("node:name:set")){return;}) name() = s;}
        
        inline float     x()                   {DEBUG_ONLY(if(safety_check("node:x:get")){return -1.0f;}) return *(float*)types[pool][idx].qget(x_offset);}
        inline void      x(float v)            {DEBUG_ONLY(if(safety_check("node:x:set")){return;}) types[pool][idx].qset(x_offset,(void*)&v,4);}
        inline float     y()                   {DEBUG_ONLY(if(safety_check("node:y:get")){return -1.0f;}) return *(float*)types[pool][idx].qget(y_offset);}
        inline void      y(float v)            {DEBUG_ONLY(if(safety_check("node:y:set")){return;}) types[pool][idx].qset(y_offset,(void*)&v,4);}
        inline float     z()                   {DEBUG_ONLY(if(safety_check("node:z:get")){return -1.0f;}) return *(float*)types[pool][idx].qget(z_offset);}
        inline void      z(float v)            {DEBUG_ONLY(if(safety_check("node:z:set")){return;}) types[pool][idx].qset(z_offset,(void*)&v,4);}
        
        inline Ptr       value_ptr()           {DEBUG_ONLY(if(safety_check("node:value_ptr")){return deadptr;}) return *(Ptr*)types[pool][idx].qget(node_value_offset);}
        inline Value     value()               {return Value(value_ptr());}
        inline void      value(Ptr ptr)        {DEBUG_ONLY(if(safety_check("node:value:set")){return;}) types[pool][idx].qset(node_value_offset,(void*)&ptr,sizeof(Ptr));}
        
        inline Ptr&      children_ptr()        {DEBUG_ONLY(if(safety_check("node:children_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_children_offset);}
        inline Col&      children_col()        {Ptr& p = children_ptr(); return types[p.pool][p.idx];}
        inline node_col  children()            {return (node_col&)children_ptr();}
        inline void      children(node_col l)  {DEBUG_ONLY(if(safety_check("node:children:set")){return;}) types[pool][idx].qset(node_children_offset,(void*)&l,sizeof(Ptr));}
        
        inline Ptr&      quals_ptr()           {DEBUG_ONLY(if(safety_check("node:quals_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_quals_offset);}
        inline Col&      quals_col()           {Ptr& p = quals_ptr(); return types[p.pool][p.idx];}
        inline node_col  quals()               {return (node_col&)quals_ptr();}
    
        inline Ptr&        node_table_ptr()    {DEBUG_ONLY(if(safety_check("node:node_table_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_node_table_offset);}
        inline Col&        node_table_col()    {Ptr& p = node_table_ptr(); return types[p.pool][p.idx];}
        inline node_col    node_table()        {return (node_col&)node_table_ptr();}
        
        inline Ptr&        value_table_ptr()   {DEBUG_ONLY(if(safety_check("node:value_table_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_value_table_offset);}
        inline Col&        value_table_col()   {Ptr& p = value_table_ptr(); return types[p.pool][p.idx];}
        inline value_col   value_table()       {return (value_col&)value_table_ptr();}
        
        inline Ptr&      scopes_ptr()          {DEBUG_ONLY(if(safety_check("node:scopes_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_scopes_offset);}
        inline Col&      scopes_col()          {Ptr& p = scopes_ptr(); return types[p.pool][p.idx];}
        inline node_col  scopes()              {return (node_col&)scopes_ptr();}
        
        inline Ptr&  parent_ptr()              {DEBUG_ONLY(if(safety_check("node:parent_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(parent_offset);}
        inline Node  parent()                  {return Node(parent_ptr());}
        inline void  parent(Ptr p)             {DEBUG_ONLY(if(safety_check("node:parent:set")){return;}) types[pool][idx].qset(parent_offset,(void*)&p,sizeof(Ptr));}
        
        inline Ptr&  owner_ptr()               {DEBUG_ONLY(if(safety_check("node:owner_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(owner_offset);}
        inline Node  owner()                   {return Node(owner_ptr());}
        inline void  owner(Ptr p)              {DEBUG_ONLY(if(safety_check("node:owner:set")){return;}) types[pool][idx].qset(owner_offset,(void*)&p,sizeof(Ptr));}
        
        inline Ptr&  in_scope_ptr()            {DEBUG_ONLY(if(safety_check("node:in_scope_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(in_scope_offset);}
        inline Node  in_scope()                {return Node(in_scope_ptr());}
        inline void  in_scope(Ptr p)           {DEBUG_ONLY(if(safety_check("node:in_scope:set")){return;}) types[pool][idx].qset(in_scope_offset,(void*)&p,sizeof(Ptr));}
                
        inline Ptr&   opt_str_ptr()            {DEBUG_ONLY(if(safety_check("node:opt_str_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_opt_str_offset);}
        inline Col&   opt_str_col()            {Ptr& p = opt_str_ptr(); return types[p.pool][p.idx];}
        inline string opt_str()                {return string(opt_str_ptr());}
        
        inline bool  mute()                    {DEBUG_ONLY(if(safety_check("node:mute:get")){return false;}) return *(bool*)types[pool][idx].qget(mute_offset);}
        inline void  mute(bool b)              {DEBUG_ONLY(if(safety_check("node:mute:set")){return;}) types[pool][idx].qset(mute_offset,(void*)&b,1);}
    
        inline bool  resolved()                {DEBUG_ONLY(if(safety_check("node:resolved:get")){return false;}) return *(bool*)types[pool][idx].qget(resolved_offset);}
        inline void  resolved(bool b)          {DEBUG_ONLY(if(safety_check("node:resolved:set")){return;}) types[pool][idx].qset(resolved_offset,(void*)&b,1);}
    
        inline void copy(Node o) {
            Col& src = types[o.pool][o.idx];
            Col& dst = types[pool][idx];
            memcpy(dst.storage, src.storage, layouts[node_id].total_size);
        }

        int find_qual(uint32_t q_id) {
            for(int i=0;i<quals().length();i++){ 
                if(quals()[i].type()==q_id) {return i;}
            }
            return -1;
        } 
    
        int find_qual_in_value(uint32_t q_id) {
            if(is_live(value()))
                return value().find_qual(q_id);
            return -1;
        }
        
        bool has_qual(size_t q_id, bool check_value = true) {
            if(check_value&&find_qual_in_value(q_id)!=-1) return true;
            if(find_qual(q_id)!=-1) return true;
            return false;
        }
    };

    inline Node Value::type_scope() {return Node(*(Ptr*)types[value_type_id][idx].qget(type_scope_offset));}
    int Value::find_qual(uint32_t q_id) {
        for(int i=0;i<quals().length();i++){ 
            if(quals()[i].type()==q_id) {return i;}
        }
        return -1;
    } 
    Node Value::get_qual(uint32_t q_id) {
        int q_at = find_qual(q_id);
        if(q_at!=-1) {
            return quals()[q_at];
        }
        return deadptr;
    } 

    Node make_node(uint32_t type = 0, uint32_t sub_type = 0, std::string name = "", float x = -1.0f, float y = -1.0f, float z = -1.0f,
        Value value = deadptr, Ptr childrenptr = deadptr, Ptr qualsptr = deadptr, Ptr nodetableptr = deadptr, 
        Ptr valuetableptr = deadptr, Ptr scopesptr = deadptr, Ptr parent = deadptr, Ptr owner = deadptr, 
        Ptr in_scope = deadptr, std::string opt_str = "", bool mute = false, bool resolved = false) 
    {
        Node n;
        n.pool = node_type_id;
        n.idx = push_column(types[node_type_id], layouts[node_id].total_size,node_id);
        n.sidx = 0;
        Col& col = types[node_type_id][n.idx];
        col.heterogenous = true;

        col.qset(node_type_offset,(void*)&type,4);
        col.qset(node_sub_type_offset,(void*)&sub_type,4);

        Ptr nameptr = get_ticket(name_store_id,sizeof(char),char_id);
        col.qset(node_name_offset, (void*)&nameptr,sizeof(Ptr));
        for(auto c : name) types[nameptr.pool][nameptr.idx].push((void*)&c);

        col.qset(x_offset, (void*)&x,4);
        col.qset(y_offset, (void*)&y,4);
        col.qset(z_offset, (void*)&z,4);

        col.qset(node_value_offset, (void*)&value,sizeof(Ptr));
    
        if(!is_live(childrenptr)) childrenptr = get_ticket(children_store_id,sizeof(Ptr),ptr_id);
        col.qset(node_children_offset, (void*)&childrenptr,sizeof(Ptr));
    
        if(!is_live(qualsptr)) qualsptr = get_ticket(quals_store_id,sizeof(Ptr),ptr_id);
        col.qset(node_quals_offset, (void*)&qualsptr,sizeof(Ptr));
        
        if(!is_live(nodetableptr)) nodetableptr = get_ticket(node_table_store_id,sizeof(Ptr),ptr_id);
        col.qset(node_node_table_offset, (void*)&nodetableptr,sizeof(Ptr));
    
        if(!is_live(valuetableptr)) valuetableptr = get_ticket(value_table_store_id,sizeof(Ptr),ptr_id);
        col.qset(node_value_table_offset, (void*)&valuetableptr,sizeof(Ptr));

        if(!is_live(scopesptr)) scopesptr = get_ticket(scopes_store_id,sizeof(Ptr),ptr_id);
        col.qset(node_scopes_offset, (void*)&scopesptr,sizeof(Ptr));
        
        col.qset(parent_offset, (void*)&parent,sizeof(Ptr));
        col.qset(owner_offset, (void*)&owner,sizeof(Ptr));
        col.qset(in_scope_offset, (void*)&in_scope,sizeof(Ptr));

        Ptr optstrptr = get_ticket(opt_str_store_id,sizeof(char),char_id);
        col.qset(node_opt_str_offset, (void*)&optstrptr,sizeof(Ptr));
        for(auto c : opt_str) types[optstrptr.pool][optstrptr.idx].push((void*)&c);

        col.qset(mute_offset, (void*)&mute,1);
        col.qset(resolved_offset, (void*)&resolved,1);

        return n;
    }

    Node make_node(uint32_t type, std::string name, Value value, Ptr in_scope) {
        return make_node(type,0,name,-1.0f,-1.0f,-1.0f,value,deadptr,deadptr,deadptr,deadptr,deadptr,deadptr,deadptr,in_scope);
    }

    void recycle_column(Ptr p) {
        recycle_column(types[p.pool], p.idx);
    }

    void recycle_node(Node n);

    void recycle_value(Value v) {
        if(is_live(v)&&resolve_to_col(v).live) {
            for(int i=0;i<v.quals().length();i++) {
                recycle_node(v.quals()[i]);
            }
            recycle_column(v.quals_ptr());

            for(int i=0;i<v.sub_values().length();i++) {
                recycle_value(v.sub_values()[i]);
            }
            recycle_column(v.sub_values_ptr());

            recycle_column(v.data_ptr());
        }
    }

    //Recycles everything
    void recycle_node(Node n) {
        if(is_live(n)&&resolve_to_col(n).live) {
            for(int i=0;i<n.children().length();i++) {
                recycle_node(n.children()[i]);
            }
            recycle_column(n.children_ptr());

            for(int i=0;i<n.scopes().length();i++) {
                recycle_node(n.scopes()[i]);
            }
            recycle_column(n.scopes_ptr());

            for(int i=0;i<n.quals().length();i++) {
                recycle_node(n.quals()[i]);
            }
            recycle_column(n.quals_ptr());

            recycle_column(n.name_ptr());
            recycle_value(n.value());
            recycle_column(n.node_table_ptr());
            recycle_column(n.value_table_ptr());
            recycle_column(n.opt_str_ptr());
            recycle_column(n);
        }
    }

    //Doesn't recycle the value or children or scopes or quals
    void soft_recycle_node(Node n) {
        recycle_column(n.name_ptr());
        recycle_column(n.children_ptr());
        recycle_column(n.quals_ptr());
        recycle_column(n.node_table_ptr());
        recycle_column(n.value_table_ptr());
        recycle_column(n.scopes_ptr());
        recycle_column(n.opt_str_ptr());
        recycle_column(n);
    }

    Value make_value(uint32_t type = 0, uint32_t size = 0, uint32_t address = 0, uint32_t sub_type = 0, 
        uint32_t sub_size = 0, Ptr type_scope = deadptr, Ptr data = deadptr, Ptr quals = deadptr, 
        Ptr sub_values = deadptr, Ptr store = deadptr, int reg = -1, int loc = -1) 
    {
        Value v;
        v.pool = value_type_id;
        v.idx = push_column(types[value_type_id], layouts[value_id].total_size, value_id);
        v.sidx = 0;
        Col& col = types[value_type_id][v.idx];
        col.heterogenous = true;
    
        col.qset(value_type_offset, (void*)&type, 4);
        col.qset(value_sub_type_offset, (void*)&sub_type, 4);
        col.qset(size_offset, (void*)&size, 4);
        col.qset(sub_size_offset, (void*)&sub_size, 4);
        col.qset(address_offset, (void*)&address, 4);
        col.qset(reg_offset, (void*)&reg, 4);
        col.qset(loc_offset, (void*)&loc, 4);
    
        col.qset(value_data_offset, (void*)&data, sizeof(Ptr));
        col.qset(type_scope_offset, (void*)&type_scope, sizeof(Ptr));
        col.qset(store_offset, (void*)&store, sizeof(Ptr));
    
        if(!is_live(quals)) quals = get_ticket(quals_store_id, sizeof(Ptr), ptr_id);
        col.qset(value_quals_offset, (void*)&quals, sizeof(Ptr));
    
        if(!is_live(sub_values)) sub_values = get_ticket(sub_value_store_id, sizeof(Ptr), ptr_id);
        col.qset(value_sub_values_offset, (void*)&sub_values, sizeof(Ptr));
    
        return v;
    }



    struct Context : public Ptr {
        Context() {}
        Context(Ptr p) { pool = p.pool; sidx = p.sidx; idx = p.idx; }
    
        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but context was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline Ptr&     node_ptr()           {DEBUG_ONLY(if(safety_check("context:node:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_node_offset);}
        inline Node     node()               {return Node(node_ptr());}
        inline void     node(Ptr p)          {DEBUG_ONLY(if(safety_check("context:node:set")){return;}) types[pool][idx].qset(context_node_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     qual_ptr()           {DEBUG_ONLY(if(safety_check("context:qual:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_qual_offset);}
        inline Node     qual()               {return Node(qual_ptr());}
        inline void     qual(Ptr p)          {DEBUG_ONLY(if(safety_check("context:qual:set")){return;}) types[pool][idx].qset(context_qual_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     left_ptr()           {DEBUG_ONLY(if(safety_check("context:left:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_left_offset);}
        inline Node     left()               {return Node(left_ptr());}
        inline void     left(Ptr p)          {DEBUG_ONLY(if(safety_check("context:left:set")){return;}) types[pool][idx].qset(context_left_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     out_ptr()            {DEBUG_ONLY(if(safety_check("context:out:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_out_offset);}
        inline Node     out()                {return Node(out_ptr());}
        inline void     out(Ptr p)           {DEBUG_ONLY(if(safety_check("context:out:set")){return;}) types[pool][idx].qset(context_out_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     root_ptr()           {DEBUG_ONLY(if(safety_check("context:root:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_root_offset);}
        inline Node     root()               {return Node(root_ptr());}
        inline void     root(Ptr p)          {DEBUG_ONLY(if(safety_check("context:root:set")){return;}) types[pool][idx].qset(context_root_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     value_ptr()          {DEBUG_ONLY(if(safety_check("context:value:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_value_offset);}
        inline Value    value()              {return Value(value_ptr());}
        inline void     value(Ptr p)         {DEBUG_ONLY(if(safety_check("context:value:set")){return;}) types[pool][idx].qset(context_value_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     result_ptr()         {DEBUG_ONLY(if(safety_check("context:result:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_result_offset);}
        inline Col&     result_col()         {Ptr& p = result_ptr(); return types[p.pool][p.idx];}
        inline node_col result()             {return (node_col&)result_ptr();}
        inline void     result(Ptr p)        {DEBUG_ONLY(if(safety_check("context:result:set")){return;}) types[pool][idx].qset(context_result_offset,(void*)&p,sizeof(Ptr));}
    
        inline int&     index()              {DEBUG_ONLY(if(safety_check("context:index:get")){return _ctx_dummy_index;}) return *(int*)types[pool][idx].qget(context_index_offset);}
        inline void     index(int i)         {DEBUG_ONLY(if(safety_check("context:index:set")){return;}) types[pool][idx].qset(context_index_offset,(void*)&i,4);}
    
        inline Ptr&     sub_ptr()            {DEBUG_ONLY(if(safety_check("context:sub:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_sub_offset);}
        inline Context  sub()                {return Context(sub_ptr());}
        inline void     sub(Ptr p)           {DEBUG_ONLY(if(safety_check("context:sub:set")){return;}) types[pool][idx].qset(context_sub_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     source_ptr()         {DEBUG_ONLY(if(safety_check("context:source:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_source_offset);}
        inline Col&     source_col()         {Ptr& p = source_ptr(); return types[p.pool][p.idx];}
        inline string   source()             {return string(source_ptr());}
        inline void     source(Ptr p)        {DEBUG_ONLY(if(safety_check("context:source:set")){return;}) types[pool][idx].qset(context_source_offset,(void*)&p,sizeof(Ptr));}
        inline void     source(std::string s){DEBUG_ONLY(if(safety_check("context:source:set")){return;}) source() = s;}
    
        inline uint32_t state()              {DEBUG_ONLY(if(safety_check("context:state:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(context_state_offset);}
        inline void     state(uint32_t s)    {DEBUG_ONLY(if(safety_check("context:state:set")){return;}) types[pool][idx].qset(context_state_offset,(void*)&s,4);}
    
        inline bool     flag()               {DEBUG_ONLY(if(safety_check("context:flag:get")){return false;}) return *(bool*)types[pool][idx].qget(context_flag_offset);}
        inline void     flag(bool b)         {DEBUG_ONLY(if(safety_check("context:flag:set")){return;}) types[pool][idx].qset(context_flag_offset,(void*)&b,1);}
    };


    Context make_context(Ptr result = deadptr, Ptr source = deadptr) {
        Context c;
        c.pool = context_type_id;
        c.idx = push_column(types[context_type_id], layouts[context_id].total_size, context_id);
        c.sidx = 0;
        Col& col = types[context_type_id][c.idx];
        col.heterogenous = true;
    
        Ptr dead_node = deadptr;
        Ptr dead_value = deadptr;
    
        col.qset(context_node_offset,   (void*)&dead_node,  sizeof(Ptr));
        col.qset(context_qual_offset,   (void*)&dead_node,  sizeof(Ptr));
        col.qset(context_left_offset,   (void*)&dead_node,  sizeof(Ptr));
        col.qset(context_out_offset,    (void*)&dead_node,  sizeof(Ptr));
        col.qset(context_root_offset,   (void*)&dead_node,  sizeof(Ptr));
        col.qset(context_value_offset,  (void*)&dead_value, sizeof(Ptr));
        col.qset(context_sub_offset,    (void*)&dead_node,  sizeof(Ptr));
    
        if(!is_live(result)) result = get_ticket(children_store_id, sizeof(Ptr), ptr_id);
        col.qset(context_result_offset, (void*)&result, sizeof(Ptr));
        
        if(!is_live(source)) source = get_ticket(name_store_id, sizeof(char), char_id);
        col.qset(context_source_offset, (void*)&source, sizeof(Ptr));
    
        uint32_t zero = 0; bool f = false;
        col.qset(context_index_offset,  (void*)&zero, 4);
        col.qset(context_state_offset,  (void*)&zero, 4);
        col.qset(context_flag_offset,   (void*)&f,    1);
    
        return c;
    }

    void recycle_context(Context ctx) {
        recycle_column(ctx);
    }
    void deep_recycle_context(Context ctx) {
        // recycle_column(ctx.result_ptr()); //Because this is often somebodies children
        // recycle_column(ctx.source_ptr());
        recycle_column(ctx);
    }
    
    using Handler = std::function<void(Context&)>;

    struct Stage : q_object {
        Stage() {
            default_function = [](Context& ctx){};
        }

        map<uint32_t,Handler> handlers;
        Handler default_function;

        std::string label;

        bool has(uint32_t key){
            return handlers.hasKey(key);
        }

        Handler& run(uint32_t key){
            return handlers.getOrDefault(key,default_function);
        }

        Handler& getOrDefault(uint32_t key, Handler& fallback){
            return handlers.getOrDefault(key,fallback);
        }

        Handler& operator[](uint32_t key) {
            return handlers[key];
        }
    };

    struct Watcher {
        Watcher(){};
        Watcher(std::string _label) : label(_label) {};
        std::string label = "";
        Handler stagestart = nullptr;
        Handler prefix = nullptr;
        Handler suffix = nullptr;
        Handler stagend = nullptr;
        Handler logger = nullptr;
    };  

    std::string tag_to_str(uint32_t tag, void* data) {
        DEBUG_ONLY(if(ERROR_FLAG) {return "ERROR";})
        if(tag==int_id) {
            return std::to_string(*(int*)data);
        } else if(tag==float_id) {
            return std::to_string(*(float*)data);
        } else if(tag==bool_id) {
            return (*(bool*)data?"true":"false");
        } else if(tag==char_id) {
            return std::string(1,*(char*)data);
        } else if(tag==string_id) {
            Ptr ptr = *(Ptr*)data;
            if(ptr.pool>=types.length()) {
                return red("STRING ERROR "+std::to_string(ptr.pool)+"|"+std::to_string(ptr.idx)+"|"+std::to_string(ptr.sidx));
            }
            std::string content = string(ptr).to_std();
            return Ptr_as_string(ptr)+"> \""+escape_string(content)+"\"";
        } else if(tag==ptr_id) {
            return Ptr_as_string(*(Ptr*)data);
        } else if(tag==ptr_id||tag==node_id||tag==value_id||tag==context_id) {
            return Ptr_as_string(*(Ptr*)data);
        } else if(tag==ptr4_id) {
            Ptr4 p = *(Ptr4*)data;
            std::string s = labels[p.midx]+"> "+Ptr_as_string(p.ptr);
            return s;
        } else if(tag==list_id||tag==col_id) {
            Ptr ptr = *(Ptr*)data;
            std::string s = Ptr_as_string(ptr)+"> [";
            Col& col = resolve_to_col(ptr);
            for(int i=0;i<col.length();i++) {
                s+=tag_to_str(col.tag,col[i]);
                if(i<col.length()-1) {s+=", ";}
            }
            s+="]";
            return s;
        } else if(tag==map_id) {
            Ptr ptr = *(Ptr*)data;
            std::string s = Ptr_as_string(ptr)+"> [";
            Col& col = resolve_to_col(ptr);
            // for(auto e : col.cells.entrySet()) {
            //     s+="{"+e.key+": "+tag_to_str(col.tag,col[e.value])+"}";
            // }
            s+="]";
            return s;
        } else {
            return labels[tag]+"?";
        }
    }
    
    std::string disassemble(uint32_t instr) {
        if(instr==0) return "NULL";
        if(instr==0xD65F03C0) return "ret";

        uint32_t op9 = (instr >> 23) & 0b111111111;
        uint32_t op7 = (instr >> 24) & 0b11111110;

        if(op9 == 0b010100101) { // MOVZ sf=0
            int rd    = instr & 0b11111;
            int imm16 = (instr >> 5) & 0b1111111111111111;
            return "movz " + std::to_string(rd) + " " + std::to_string(imm16) + " 0";
        }
        if(op9 == 0b110100101) { // MOVZ sf=1
            int rd    = instr & 0b11111;
            int imm16 = (instr >> 5) & 0b1111111111111111;
            return "movz " + std::to_string(rd) + " " + std::to_string(imm16) + " 1";
        }


        if((instr & 0b01111111100000000000000000000000) == 0b01110010100000000000000000000000) {
            int rd    =  instr & 0b11111;
            int imm16 = (instr >> 5) & 0b1111111111111111;
            int hw    = (instr >> 21) & 0b11;
            int sf    = (instr >> 31) & 1;
            return "movk " + std::to_string(rd) + " " + std::to_string(imm16) + " " + std::to_string(hw*16) + " " + std::to_string(sf);
        }

        if((instr & 0b01111111001000000000001111100000) == 0b00101010000000000000001111100000) {
            int rd = instr & 0b11111;
            int rm = (instr >> 16) & 0b11111;
            int sf = (instr >> 31) & 1;
            return "mov " + std::to_string(rd) + " " + std::to_string(rm) + " " + std::to_string(sf);
        }

        if((instr & 0b01111111111000000000000000000000) == 0b00001011000000000000000000000000) {
            int rd = instr & 0b11111;
            int rn = (instr >> 5) & 0b11111;
            int rm = (instr >> 16) & 0b11111;
            int sf = (instr >> 31) & 1;
            return "add " + std::to_string(rd) + " " + std::to_string(rn) + " " + std::to_string(rm) + " " + std::to_string(sf);
        }

        if((instr & 0b01111111111000000000000000000000) == 0b00011011000000000000000000000000) {
            int rd = instr & 0b11111;
            int rn = (instr >> 5) & 0b11111;
            int rm = (instr >> 16) & 0b11111;
            int sf = (instr >> 31) & 1;
            return "mul " + std::to_string(rd) + " " + std::to_string(rn) + " " + std::to_string(rm) + " " + std::to_string(sf);
        }

        if((instr & 0b11111111110000000000000000000000) == 0b10111001010000000000000000000000) {
            int rt     = instr & 0b11111;
            int rn     = (instr >> 5) & 0b11111;
            int offset = ((instr >> 10) & 0b111111111111) * 4;
            return "ldr32 " + std::to_string(rt) + " " + std::to_string(rn) + " " + std::to_string(offset);
        }

        if((instr & 0b11111111110000000000000000000000) == 0b11111001010000000000000000000000) {
            int rt     = instr & 0b11111;
            int rn     = (instr >> 5) & 0b11111;
            int offset = ((instr >> 10) & 0b111111111111) * 8;
            return "ldr " + std::to_string(rt) + " " + std::to_string(rn) + " " + std::to_string(offset);
        }

        if((instr & 0b11111111110000000000000000000000) == 0b10111001000000000000000000000000) {
            int rt     = instr & 0b11111;
            int rn     = (instr >> 5) & 0b11111;
            int offset = ((instr >> 10) & 0b111111111111) * 4;
            return "str32 " + std::to_string(rt) + " " + std::to_string(rn) + " " + std::to_string(offset);
        }

        if((instr & 0b11111111110000000000000000000000) == 0b11111001000000000000000000000000) {
            int rt     = instr & 0b11111;
            int rn     = (instr >> 5) & 0b11111;
            int offset = ((instr >> 10) & 0b111111111111) * 4;
            return "str " + std::to_string(rt) + " " + std::to_string(rn) + " " + std::to_string(offset);
        }



        return "   ?   ";
    }

    std::string print_columnar_table(list<list<std::string>> lines) {
        list<uint32_t> widths;
        uint32_t longest_row = 0;
        for(int l=0;l<lines.length();l++) {
            list<std::string>& line = lines[l];
            uint32_t widest_row = 0;
            for(int i=0;i<line.length();i++) {
                if(i==0&&line[i].empty()) {line[i] = std::to_string(l);}
                if(line[i].length()>widest_row) {widest_row = line[i].length();}
            }
            widths << widest_row;
            if(line.length()>longest_row) {longest_row = line.length();}
        }

        std::string to_return = "";
        int lpadlen = digit_count(longest_row)+1;
        for(int r=0;r<longest_row;r++) {
            if(r==1) {
                for(int l=0;l<lines.length();l++) {
                    to_return+=std::string(widths[l]+lpadlen+3,'-')+"<|>";
                }
                to_return+="\n";
            }

            for(int l=0;l<lines.length();l++) {
                std::string line = "";
                if(lines[l].length()>r) {line = lines[l][r];}
                std::string rownum = std::to_string(r-1); //Minus 1 because indexes start at 0                    
                if(r==0) { //If a header
                    std::string column = std::to_string(l);
                    if(line==column) {
                        to_return += center_pad(line, widths[l]+lpadlen+3) + " | ";
                    } else {
                        to_return+=std::string(lpadlen-column.length(),' ')+column+" : ";
                        to_return += center_pad(line, widths[l]) + " | ";
                    }
                } else {
                    if(!line.empty()) {
                        to_return+=std::string(lpadlen-rownum.length(),' ')+rownum+" : ";
                        std::string padding(widths[l]-line.length(),' ');
                        to_return += line+padding+" | ";
                    } else {
                        to_return += center_pad("X",widths[l]+lpadlen+3)+" | ";
                    }
                }

            }
            to_return+="\n";

            if(longest_row>1&&r==longest_row-1) {
                for(int l=0;l<lines.length();l++) {
                    to_return+=std::string(widths[l]+lpadlen+3,'=')+"/ \\";
                }
            }
        }
        return to_return;
    }

    list<list<std::string>> type_to_lines(TypeCol& t) {
        list<list<std::string>> lines;
        list<uint32_t> dtypes;
        for(int c=0;c<t.length();c++) {
            Col& col = t[c];
            list<std::string> subline;
            subline << col.label.to_std()+(col.live?"":" [FREE]");
            if(col.heterogenous) {
                if(layouts.hasKey(col.tag)) {
                    _layout& l = layouts.get(col.tag);
                    for(int o=0;o<l.offsets.length();o++) {
                        std::string line = "";
                        line+=pad_str(l.labels[o]+": ",12);
                        line+=tag_to_str(l.tags[o],col.qget(l.offsets[o]));
                        subline << line;
                    }
                } else {
                    print(red("core::type_to_string unable to print heteregenous column of type "+labels[col.tag]+" because no layout was found"));
                }
            } else {
                for(int r=0;r<col.length();r++) {
                    std::string line = ""; //v turn this into a 'show key as string' tag eventually and replace this cruft
                    if(col.label=="stages") { //From the handler type
                        std::string cell_label = ""; //col.get_cell_label(r);
                        if(!cell_label.empty()) line+=cell_label;
                        else line+="REIMPLMENT CELL KEYS LATER";
                    } else {
                        line+=tag_to_str(col.tag,col[r]);
                    }
                    subline << line;
                }
            }
            lines << subline;
        }
        return lines;
    }

    std::string type_to_string(TypeCol& t) {
        return print_columnar_table(type_to_lines(t));
    }


    list<list<std::string>> TypeCol_to_lines(TypeCol& t) {
        list<list<std::string>> lines;
        for(int c=0;c<t.length();c++) {
            Col& col = t[c];
            list<std::string> subline;
            subline << col.label.to_std();
            if(col.heterogenous) {
                if(layouts.hasKey(col.tag)) {
                    _layout& l = layouts.get(col.tag);
                    for(int o=0;o<l.offsets.length();o++) {
                        std::string line = "";
                        line+=pad_str(l.labels[o]+": ",12);
                        line+=tag_to_str(l.tags[o],col.qget(l.offsets[o]));
                        subline << line;
                    }
                } else {
                    print(red("core::TypeCol_to_lines unable to print heteregenous column of type "+labels[col.tag]+" because no layout was found"));
                }
            } else {
                for(int r=0;r<col.length();r++) {
                    std::string line = "";
                    line+=tag_to_str(col.tag,col[r]);
                    subline << line;
                }
            }
            lines << subline;
        }
        return lines;
    }

    void print_column(Col& col) {
        print("COL: ",col.label," TAG: ",labels[col.tag]," [",std::to_string(col.length()),"]");
        if(col.heterogenous) {
            if(layouts.hasKey(col.tag)) {
                _layout& l = layouts.get(col.tag);
                for(int o=0;o<l.offsets.length();o++) {
                    std::string line = "";
                    line+=pad_str(l.labels[o]+": ",12);
                    line+=tag_to_str(l.tags[o],col.qget(l.offsets[o]));
                    print(line);
                }
            } else {
                print(red("core::print_column unable to print heteregenous column of type "+labels[col.tag]+" because no layout was found"));
            }
        } else {
            for(int i=0;i<col.length();i++) {
                print(i,": ",tag_to_str(col.tag,col[i]));
            }
        }
    }

    void dump_unit(bool clear_dump) {
        if(clear_dump) writeFile("mixos-acorn/tests/printout.txt","");

        for(int t=0;t<types.length();t++) {
            std::string to_print = "";
            to_print += "TYPE "+std::to_string(t)+" "+types[t].label.to_std()+":\n";
            to_print += type_to_string(types[t]);
            to_print += "\n\n\n";

            editTextFile("mixos-acorn/tests/printout.txt",[to_print](std::string& source){
                source+=to_print;
            });
        }
    }

    std::string value_info(Value value) {
        DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(value)),red(" while another error was active")); return "";})

        std::string to_return = "";
        to_return += cyan("["+Ptr_as_string(value)+"]")+
        + "(type: " + green(labels[value.type()])
        + (value.reg()!=-1?", reg: "+std::to_string(value.reg()):"");
        if(is_live(value.data_ptr())) { //For post-mortems we want to see the adress, so it needs to be computed first, before the error
            std::string ptr_addr = Ptr_as_string(value.data_ptr());
            if(resolve_to_col(value.data_ptr()).empty()) {
                to_return += ", value: "+gray("empty")+" @"+ptr_addr;
            } else {
                to_return += ", value: "+gray(tag_to_str(value.type(),value.get()))+" @"+ptr_addr;
            }
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(value)),red(" but the value was invalid")); return to_return;})
        }
        to_return += (value.sub_type()!=0?", sub_type: "+labels[value.sub_type()]:"")
        + (is_live(value.type_scope())?", type_scope: "+blue(Ptr_as_string(value.type_scope())):"")
        + (value.size()!=0?", size: "+std::to_string(value.size()):"")
        + (value.sub_size()!=0?", sub_size: "+std::to_string(value.sub_size()):"")
        + (value.address()!=0?", address: "+std::to_string(value.address()):"")
        + (value.loc()!=-1?", loc: "+std::to_string(value.loc()):"")
        + (is_live(value.store())?", store: "+Ptr_as_string(value.store()):"")
        + (!value.sub_values().empty()?", sub: "+std::to_string(value.sub_values().length()):"");
        if(!value.quals().empty()) {
            to_return += ", Quals: ";
            for(int i=0;i<value.quals().length();i++) {
                to_return += labels[value.quals()[i].type()]+(i!=value.quals().length()-1?", ":"");
            }
        }
        to_return += ")";
        return to_return;
    }
    
    std::string node_info(Node node) {
        DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(node)),red(" while another error was active")); return "";})

        std::string to_return = "";
        to_return += blue(Ptr_as_string(node)+" ")
        + labels[node.type()]
        + (node.sub_type()==0?"":":"+labels[node.sub_type()])
        + (node.name().length()==0?"":" "+green(escape_string(node.name().to_std()))+" ") 
        + (is_live(node.value())?value_info(node.value()):"")
        + (node.x()!=-1.0f?"("+std::to_string((int)node.x())+","+std::to_string((int)node.y())+")":"")
        + (!node.children().empty()?"[C:"+std::to_string(node.children().length())+"]":"")
        + (!node.scopes().empty()?"[S:"+std::to_string(node.scopes().length())+"]":"")
        + (is_live(node.owner())?"[O:"+blue(Ptr_as_string(node.owner()))+"]":"")
        + (is_live(node.in_scope())?"{"+node.in_scope().name().to_std()+"}":"");
        if(!node.quals().empty()) {
            to_return += "[Q: ";
            for(int i=0;i<node.quals().length();i++) {
                if(node.quals()[i].mute()) {
                    to_return += italic_str(Ptr_as_string(node.quals()[i])+">"+labels[node.quals()[i].type()]);
                } else {
                    to_return += Ptr_as_string(node.quals()[i])+">"+labels[node.quals()[i].type()];
                }
                to_return+=(i!=node.quals().length()-1?", ":"");
            }
            to_return += "]";
        }
        return to_return;
    }

    #define ACORN_MUTE_TABLES 1

    std::string node_to_string(Node node, int depth = 0, int index = 0, bool print_sub_scopes = false, std::string sigil = "") {
        DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(node)),red(" while another error was active")); return "";})
        std::string indent(depth * 2, ' ');
        std::string to_return = "";
        
        to_return += indent + sigil + std::to_string(index) + ": " + node_info(node);
    
        #if !ACORN_MUTE_TABLES 
        if(node.value_table().length()>0) {
            to_return += "\n" + indent + "   Value table:";
            QCellCol cells = node.value_table().col().cells;
            for(int i=0;i<cells.length();i++) {
                to_return += "\n" + indent + "     Key: "+std::to_string(cells.get(i).hash)+" | "+value_info(node.value_table().get(cells.get(i).index));
            }
        }
        if(node.node_table().length()>0) {
            to_return += "\n" + indent + "   Node table:";
            QCellCol cells = node.node_table().col().cells;
            for(int i=0;i<cells.length();i++) {
                to_return += "\n" + indent + "     Key: "+std::to_string(cells.get(i).hash)+" | "+node_info(node.node_table().get(cells.get(i).index));
            }
        }
        #endif
    


        // if(!node->opt_str.empty()) {
        //     to_return +=  "\n" + indent + "  Opt_str: " + node->opt_str;
        // }

        if(!node.children().empty()) {
            for(int i=0;i<node.children().length();i++) {
                if(is_live(node.children()[i])) {
                    if(node.children()[i].idx==node.idx) {
                        to_return+="\n "+indent+red("  self refrence");
                    } else {
                        to_return += "\n " + node_to_string(node.children()[i], depth + 1, i, print_sub_scopes,"c");
                    }
                }
                else {
                    to_return += "\n" + indent + "[NULL CHILD] "+node_info(node.children()[i]);
                }
            }
        }

        if(!node.scopes().empty()) {
            //to_return +=  "\n" + indent + "   Scopes: " + std::to_string(node.scopes().length());
            int i = 0;
            for(int s=0;s<node.scopes().length();s++) {
                Node scope = node.scopes()[s];
                if(scope.owner().idx==node.idx) {
                    to_return += "\n " + node_to_string(scope, depth + 1, s, print_sub_scopes,"s");
                }
                else {
                    to_return += "\n"+indent+"  s"+std::to_string(s)+": "+node_info(scope);
                }
            }
        }
    
        return to_return;
    }

    bool init_type_pool() {
        labels[undefined_id] = "UDEFINED";
        labels[ptr_id] = "Ptr";

        types[handler_type_id].label = "handlers";
        types[layout_type_id].label = "layouts";
        types[node_type_id].label = "nodes";
        types[value_type_id].label = "values";
        types[context_type_id].label = "contexts";
        types[name_store_id].label = "names";
        types[children_store_id].label = "children";
        types[quals_store_id].label = "quals";
        types[node_table_store_id].label = "node table";
        types[value_table_store_id].label = "value table";
        types[scopes_store_id].label = "scopes";
        types[opt_str_store_id].label = "opt_str";
        types[data_store_id].label = "data";
        types[sub_value_store_id].label = "sub_value";



        add_template(ptr_id);
        add_template(string_id); 

        // writeFile("mixos-acorn/tests/printout2.txt","");
        // make_wrapper_for_layout(layouts[node_id],"Node","mixos-acorn/tests/printout2.txt");
        return true;
    }
    bool type_pool_intilized = init_type_pool();

    Node scan_for_node(const std::string& label, Node from) {
        for(int i=0;i<from.children().length();i++) {
            //print("CHECKING: ",from.children()[i].name().to_std());
            if(from.children()[i].name().to_std()==label) {
                return from.children()[i];
            }
            Node found = scan_for_node(label,from.children()[i]);
            if(is_live(found)) {
                return found;
            }
        }
        for(int i=0;i<from.scopes().length();i++) {
            Node found = scan_for_node(label,from.scopes()[i]);
            if(is_live(found)) {
                return found;
            }
        }
        return deadptr;
    }

    void test_acorn() {
        Node n = make_node();
        Node m = make_node();
        n.name("NODE N"); m.name("NODE M");
        print("NODES WITH 2 NODES: ",types[node_type_id].length());
        print("NAMES WITH 2 NODES: ",types[name_store_id].length());
        print("CHILDREN WITH 2 NODES: ",types[children_store_id].length());
        recycle_node(n);
        print("NODES AFTER RECYCLE: ",types[node_type_id].length());
        print("NAMES AFTER RECYCLE: ",types[name_store_id].length());
        print("CHILDREN AFTER RECYCLE: ",types[children_store_id].length());
        Node c = make_node();
        print("NODES WITH 2 NODES: ",types[node_type_id].length());
        print("NAMES WITH 2 NODES: ",types[name_store_id].length());
        print("CHILDREN WITH 2 NODES: ",types[children_store_id].length());
        c.name("C");
        print(c.name().to_std());
    }

    void deep_copy_node(Node n, Node o, map<uint32_t,Value>& value_alias_table, map<uint32_t,Node>& node_alias_table) {
        n.type(o.type());
        n.sub_type(o.sub_type());
        n.name(o.name().to_std());
        n.x(o.x());
        n.y(o.y());
        n.z(o.z());
        n.mute(o.mute());
        n.resolved(o.resolved());
    
        n.children().clear();
        for(int i = 0; i < o.children().length(); i++) {
            Node newc = make_node();
            deep_copy_node(newc, o.children()[i], value_alias_table, node_alias_table);
            n.children() << newc;
        }
    
        n.quals().clear();
        for(int i = 0; i < o.quals().length(); i++) {
            Node newq = make_node();
            deep_copy_node(newq, o.quals()[i], value_alias_table, node_alias_table);
            n.quals() << newq;
        }
    
        n.scopes().clear();
        for(int i = 0; i < o.scopes().length(); i++) {
            Node news = make_node();
            deep_copy_node(news, o.scopes()[i], value_alias_table, node_alias_table);
            n.scopes() << news;
        }
    
        if(value_alias_table.hasKey(o.value().idx)) {
            Value aliased = value_alias_table.get(o.value().idx);
            n.value(aliased);
            if(is_live(aliased.type_scope())) {
                if(!n.scopes().empty()) {
                    Node ntyscope = aliased.type_scope();
                    n.scopes().col().set(0,(void*)&ntyscope);
                }
                else
                    n.scopes() << aliased.type_scope();
            }
        } else {
            if(is_live(o.value())) {
                if(!is_live(n.value())) {
                    n.value(make_value());
                }
                n.value().copy(o.value(),true);
            }
        }
    
        types[n.pool][n.idx].qset(node_value_table_offset,
            types[o.pool][o.idx].qget(node_value_table_offset), sizeof(Ptr));
        types[n.pool][n.idx].qset(node_node_table_offset,
            types[o.pool][o.idx].qget(node_node_table_offset), sizeof(Ptr));
    
        n.parent(o.parent_ptr());
        n.owner(o.owner_ptr());
        n.in_scope(o.in_scope_ptr());
        n.opt_str() = o.opt_str().to_std();
    }

    Node copy_as_token(Node node) {
        Node copy = make_node(node.type(),0,node.name().to_std(),node.x(),node.y(),node.z());
        copy.mute(true);
        for(int i=0;i<node.quals().length();i++) {
            Node q = node.quals()[i];
            if(q.mute()) {copy.quals() << q;}
        }
        return copy;
    }

    Node turn_into_token(Node node) {
        node.mute(true);
        return node;
    }


    void walk_nodenet(Node root, std::function<void(Node)> func) {
        func(root);
        for(int i=0;i<root.children().length();i++) walk_nodenet(root.children()[i],func);
        for(int i=0;i<root.quals().length();i++) walk_nodenet(root.quals()[i],func);
        for(int i=0;i<root.scopes().length();i++) if(root.scopes()[i].owner()==root) walk_nodenet(root.scopes()[i],func);
        if(is_live(root.value())) {
            for(int i=0;i<root.value().quals().length();i++) walk_nodenet(root.value().quals()[i],func);
        }
    }
        
    struct Unit : public q_object {
        Unit() {init();}

        map<std::string,g_ptr<Stage>> stages;
        Node unit_root = deadptr;
        std::string unit_label = "";
    
        Stage& reg_stage(std::string label) {
            g_ptr<Stage> new_stage = make<Stage>();
            new_stage->label = label;
            stages.put(label,new_stage);

            Ptr p(0,0,false); //Just a dead pointer
            types[handler_type_id][stages_id].put(label,(void*)&p);

            return *new_stage.getPtr();
        }

        Stage& print_handlers = reg_stage("printing");


        std::string value_as_string(Value v) {
            Context ctx = make_context(); ctx.value(v);
            print_handlers.run(v.type())(ctx);
            deep_recycle_context(ctx);
            return ctx.source().to_std();
        }
    
        Stage& a_handlers = reg_stage("assembling");
        Stage& s_handlers = reg_stage("scoping");
        Stage& t_handlers = reg_stage("typing");
    
        Stage& d_handlers = reg_stage("discovering");
        Stage& r_handlers = reg_stage("resolving");
        Stage& e_handlers = reg_stage("evaluating");
    
        Stage& m_handlers = reg_stage("modeling");
        Stage& i_handlers = reg_stage("inspecting");
        Stage& x_handlers = reg_stage("executing");
    
        Stage* active_stage;
        list<Watcher> watchers;
        std::string GLOBAL_MSG = "";
        void log_to_watcher(Context& ctx, const std::string& msg) {
            GLOBAL_MSG = msg;
            for(auto& w : watchers) {if(w.logger) w.logger(ctx);}
            GLOBAL_MSG = "";
        }
    
        void start_logged_stage(Stage& stage) {
            active_stage = &stage;
            DEBUG_ONLY(for(auto& w : watchers) {if(w.stagestart) w.stagestart((Context&)dead_ref);})
        }
        void end_logged_stage() {
            DEBUG_ONLY(for(auto& w : watchers) {if(w.stagend) w.stagend((Context&)dead_ref);})
        }

        void start_stage(Stage& stage) {
            active_stage = &stage;
        }
    
        void start_stage(g_ptr<Stage> stage_ptr) {
            start_stage(*stage_ptr.getPtr());
        }

        void start_stage(Stage* stage_ptr) {
            start_stage(*stage_ptr);
        }

        virtual void init() {
            DEBUG_ONLY(
                Watcher def("core");
                def.stagestart = [this](Context& ctx){
                    if(active_stage) {
                        newline(active_stage->label);
                    }
                };
                def.prefix = [this](Context& ctx){
                    newline(active_stage->label+": "+node_info(ctx.node()));
                };
                def.suffix = [this](Context& ctx){
                    if(ERROR_FLAG) {log(red("Marked "+Ptr_as_string(ctx.node())+" as error")); mark_error(ctx.node());}
                    log(green("After: "),node_info(ctx.node()));
                    endline();
                };
                def.stagend = [this](Context& ctx){
                    endline();
                };
                watchers << def;
            )
        }
    
        virtual Node process(std::string path) {
            return deadptr;
        }
    
        virtual void run(Node root) {
            
        }
    
        bool standard_travel_pass(Node root, Context sub = deadptr);

        inline void standard_process(Context& ctx, uint32_t type) {
            DEBUG_ONLY(for(auto& w : watchers) {if(w.prefix) w.prefix(ctx);})
            active_stage->run(type)(ctx);
            DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
        }

        inline void standard_process(Context& ctx) {
            standard_process(ctx,ctx.node().type());
        }
    
        void process_node(Context& ctx, Node node) {
            Node saved_node = ctx.node();
            Context saved_sub = ctx.sub();
            ctx.node(node);
            standard_process(ctx);
            ctx.node(saved_node);
            ctx.sub(saved_sub);
        }
    
        void process_node(Context& ctx, Node node, Node left) {
            Node saved_node = ctx.node();
            Node saved_left = ctx.left();
            Context saved_sub = ctx.sub();
            ctx.node(node);
            ctx.left(left);
            standard_process(ctx);
            ctx.node(saved_node);
            ctx.left(saved_left);
            ctx.sub(saved_sub);
        }
    
        void process_node(Node node, Node left) {
            Context ctx = make_context();
            process_node(ctx,node,left);
            deep_recycle_context(ctx);
        }
    
        void standard_sub_process_node(Node root) {
            Context ctx = make_context();
            ctx.node(root);
            standard_sub_process(ctx);
            deep_recycle_context(ctx);
        }

        void standard_sub_process(Context& ctx) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a sub_process while an error was flagged")); return;})
            node_col children = ctx.node().children();
            Context sub_ctx = make_context(children,ctx.source_ptr());
            sub_ctx.root(ctx.node());
            sub_ctx.sub(ctx.sub());

            int& i = sub_ctx.index();
            while(i < sub_ctx.result().length()) {
                if(i==0) {
                    process_node(sub_ctx, sub_ctx.result().get(i));
                } else {
                    process_node(sub_ctx, sub_ctx.result().get(i), sub_ctx.result().get(i-1));
                }

                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})

                i++;
            }
            ctx.flag(sub_ctx.flag());
            recycle_context(sub_ctx);
        }

        void backwards_sub_process(Context& ctx) { 
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a backwards sub_process while an error was flagged")); return;})
            node_col children = ctx.node().children();
            Context sub_ctx = make_context(children,ctx.source_ptr());
            sub_ctx.root(ctx.node());
            sub_ctx.sub(ctx.sub());
            int& i = sub_ctx.index();
            i = children.length()-1;
            while(i >= 0) {
                if(i==children.length()-1) {
                    process_node(sub_ctx, sub_ctx.result().get(i));
                } else {
                    process_node(sub_ctx, sub_ctx.result().get(i), sub_ctx.result().get(i+1));
                }
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                i--;
            }
            ctx.flag(sub_ctx.flag());
            recycle_context(sub_ctx);
        }

        //resolve_to_col(sub_ctx).qset(context_source_offset,(void*)&ctx.source_ptr(),sizeof(Ptr));

        void fire_quals(Context& ctx, Value value) {
            Value saved_value = ctx.value();
            ctx.value(value);
            for(int q=0;q<value.quals().length();q++) {
                Node qual = value.quals()[q];
                if(qual.mute()) continue;
                ctx.qual(qual);
                if(active_stage->has(qual.type()+1))
                    (*active_stage)[qual.type()+1](ctx);
            }
            ctx.value(saved_value);
        }
        void fire_quals(Context& ctx, Node node) {
            Node saved_node = ctx.node();
            ctx.node(node);
            for(int q=0;q<node.quals().length();q++) {
                Node qual = node.quals()[q];
                if(qual.mute()) continue;
                ctx.qual(qual);
                if(active_stage->has(qual.type()+2))
                    (*active_stage)[qual.type()+2](ctx);
            }
            ctx.node(saved_node);
        }

        void standard_qual_process(Context& ctx) {
            for(int n=0;n<2;n++) {
                Context sub_ctx = make_context(n==0?ctx.node().quals():ctx.node().value().quals());
                int& i = sub_ctx.index();
                sub_ctx.root(ctx.node());
                sub_ctx.sub(ctx.sub());
                while(i < sub_ctx.result().length()) {
                    sub_ctx.qual(sub_ctx.result().get(i));
                    if(n==0) {
                        sub_ctx.node(ctx.node());
                    } else {
                        sub_ctx.value(ctx.node().value());
                    }
                    standard_process(sub_ctx,sub_ctx.qual().type());
                    sub_ctx.left(sub_ctx.result().get(i));
                    i++;
                }
                recycle_context(sub_ctx);
            }
        }
    
        void standard_direct_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a direct pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Direct pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children);
            int& i = ctx.index();
            ctx.root(root);
            while(i < ctx.result().length()) {
                ctx.node(ctx.result().get(i));
                standard_process(ctx);
                ctx.left(ctx.result().get(i));
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                i++;
            }
    
            node_col scopes = root.scopes();
            for(int i = 0; i<scopes.length(); i++) {
                standard_direct_pass(scopes.get(i));
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
            }
            endline();
            deep_recycle_context(ctx);
        }

        void standard_resolving_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a resolving pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Resolving pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children);
            int& i = ctx.index();
            ctx.root(root);
            while(i < ctx.result().length()) {    //Process all nodes with scopes first (like any declerations)
                if(!ctx.result()[i].scopes().empty()) {
                    ctx.node(ctx.result()[i]);
                    standard_process(ctx);
                    DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                    ctx.left(ctx.result()[i]);
                }
                i++;
            }
            i = 0;
            while(i < ctx.result().length()) {    //Then process nodes without scopes
                if(ctx.result()[i].scopes().empty()) {
                    ctx.node(ctx.result()[i]);
                    standard_process(ctx);
                    DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                    ctx.left(ctx.result()[i]);
                }
                i++;
            }
            i = 0;
            while(i < ctx.result().length()) {    //Then the children of nodes with scopes
                if(!ctx.result()[i].scopes().empty()) {
                    standard_sub_process_node(ctx.result()[i]);
                }
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                i++;
            }
            i = 0;
            while(i < ctx.result().length()) {    //Then finnaly the subscopes
                if(!ctx.result()[i].scopes().empty()) {
                    for(int s = 0;s<ctx.result()[i].scopes().length();s++) {
                        if(ctx.result()[i].scopes()[s].owner().idx==ctx.result()[i].idx) {
                            standard_resolving_pass(ctx.result()[i].scopes()[s]);
                            DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                        }
                    }
                }
                i++;
            }
            endline();
            deep_recycle_context(ctx);
        }

        void standard_backwards_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a backwards pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Backwards pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children);
            int& i = ctx.index();
            ctx.root(root);
            i = ctx.result().length()-1;
            while(i >= 0) {
                ctx.node(ctx.result().get(i));
                standard_process(ctx);
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                node_col scopes = ctx.result().get(i).scopes();
                for(int s = 0; s<scopes.length(); s++) {
                    if(is_live(scopes.get(s).owner())&&scopes.get(s).owner().idx==ctx.result().get(i).idx) {
                        memory_backwards_pass(scopes.get(s));
                        DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                    }
                }
                ctx.left(ctx.result().get(i));
                i--;
            }
            endline();
            deep_recycle_context(ctx);
        }

        void memory_backwards_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a memory backwards pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Backwards pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children);
            int& i = ctx.index();
            ctx.root(root);
            i = ctx.result().length()-1;
            while(i >= 0) {
                ctx.node(ctx.result().get(i));
                node_col scopes = ctx.result().get(i).scopes();
                for(int s = 0; s<scopes.length(); s++) {
                    if(is_live(scopes.get(s).owner())&&scopes.get(s).owner().idx==ctx.result().get(i).idx) {
                        memory_backwards_pass(scopes.get(s));
                        DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                    }
                }
                standard_process(ctx);
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                ctx.left(ctx.result().get(i));
                i--;
            }
            endline();
            deep_recycle_context(ctx);
        }
    };

    //Returns true if flagged for a return/break
    bool Unit::standard_travel_pass(Node root, Context sub) {
        DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a travel pass while an error was flagged")); return true;})
        node_col children = root.children();
        newline("Travel pass over "+std::to_string(children.length())+" nodes");
        Context ctx = make_context(children,is_live(sub)?sub.source_ptr():deadptr);
        int& i = ctx.index();
        ctx.root(root);
        ctx.sub(sub);
        while(i < ctx.result().length()) {
            ctx.node(ctx.result().get(i));
            standard_process(ctx);
            ctx.left(ctx.result().get(i));
            DEBUG_ONLY(if(ERROR_FLAG) {endline(); return true;})
            if(ctx.flag()) { //This is the return/break process
                endline();
                deep_recycle_context(ctx);
                return true;
            }
            i++;
        }
        endline();
        deep_recycle_context(ctx);
        return false;
    }



    static void write_TypeCol(std::ostream& out, TypeCol& type) {
        write_col(out, type);
        for(int c = 0; c < type.length(); c++) {
            Col& col = type[c];
            write_col(out, col);
        }
    }
    
    static TypeCol read_TypeCol(std::istream& in) {
        TypeCol type = read_col(in);
        for(uint32_t i = 0; i < type.length(); i++) {
            Col col = read_col(in);
            type.set(i,col);
        }
        return type;
    }

    static void write_TypeTypeCol(std::ostream& out, TypeTypeCol& col) {
        write_col(out, col);
        for(int i = 0; i < col.length(); i++) {
            write_TypeCol(out,col[i]);
        }
    }

    static TypeTypeCol read_TypeTypeCol(std::istream& in) {
        TypeTypeCol col = read_col(in);
        for(uint32_t p = 0; p < col.length(); p++) {
            col.set(p,read_TypeCol(in));
        }
        return col;
    }

    void init_from_disk(std::istream& in) {
        labels.clear();
        layouts.clear();

        TypeCol& h = types[handler_type_id];
        for(int i = 0; i < h.length(); i++) {
            Col& handler_col = h[i];

            labels.put(i,handler_col.label.to_std());

            if(handler_col.empty()) continue;
            Ptr lptr = *(Ptr*)handler_col.sget(0);
            if(!is_live(lptr)) continue;
    
            _layout l(lptr);
    
            Col& offsets_c  = resolve_to_col(lptr, lptr.idx + offsets_col);
            Col& tags_c     = resolve_to_col(lptr, lptr.idx + tags_col);
            Col& sizes_c    = resolve_to_col(lptr, lptr.idx + sizes_col);
            Col& labels_c   = resolve_to_col(lptr, lptr.idx + labels_col);
            Col& subtags_c  = resolve_to_col(lptr, lptr.idx + subtags_col);
            Col& subsizes_c = resolve_to_col(lptr, lptr.idx + subsizes_col);
            Col& ptrs_c     = resolve_to_col(lptr, lptr.idx + ptrs_col);
            l.overloads     = resolve_to_col(lptr, lptr.idx + overloads_col);
    
            uint32_t count = offsets_c.size / sizeof(uint32_t);
            for(uint32_t f = 0; f < count; f++) {
                uint32_t offset  = *(uint32_t*)offsets_c.sget(f);
                uint32_t tag     = *(uint32_t*)tags_c.sget(f);
                uint32_t size    = *(uint32_t*)sizes_c.sget(f);
                Ptr      label_p = *(Ptr*)labels_c.sget(f);
                uint32_t subtag  = *(uint32_t*)subtags_c.sget(f);
                uint32_t subsize = *(uint32_t*)subsizes_c.sget(f);
                Ptr      ptr     = *(Ptr*)ptrs_c.sget(f);
    
                std::string label_str = string(label_p).to_std();
                l.label_to_index.put(label_str, l.offsets.length());
                l.offsets  << offset;
                l.tags     << tag;
                l.sizes    << size;
                l.labels   << label_str;
                l.subtags  << subtag;
                l.subsizes << subsize;
                l.ptrs     << ptr;
            }
            l.total_size = l.offsets.length() > 0 
                ? l.offsets.last() + l.sizes.last() 
                : 0;
                
            layouts[i] = l;
        }
    }

    void save_acorn(const std::string& path) {
        auto out = openWriteStream(path);
        write_TypeTypeCol(out,types);

    }
    void load_acorn(const std::string& path) {
        auto in = openReadStream(path);
        types = read_TypeTypeCol(in);
        init_from_disk(in);
        ERROR_FLAG = false;
    }
}