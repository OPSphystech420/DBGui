//
// Made by OPSphystech420 2024 (c)
//

#ifndef SINGLETON_H
#define SINGLETON_H

template<typename Template>
class Singleton
{
public:
    static Template& GetInstance()
    {
        static Template Instance{};
        return Instance;
    }

protected:
    Singleton() { }
    ~Singleton() { }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;
};

#include <memory>
#include <mutex>
#include <atomic>

template<typename T>
class SingletonDestructPattern
{
public:
    
    static T& GetInstance()
    {
        T* tmp = instance_.load(std::memory_order_acquire);
        
        if (!tmp)
        {
            
            std::lock_guard<std::mutex> lock(instanceMutex_);
            tmp = instance_.load(std::memory_order_relaxed);
            
            if (!tmp)
            {
                std::unique_ptr<T, Deleter> newInstance(new T());
                tmp = newInstance.get();
                instance_.store(tmp, std::memory_order_release);
                instanceOwner_.reset(newInstance.release());
            }
        }
        return *tmp;
    }

    static void DestroyInstance()
    {
        std::lock_guard<std::mutex> lock(instanceMutex_);
        
        if (instanceOwner_)
        {
            instanceOwner_.reset();
            instance_.store(nullptr, std::memory_order_release);
        }
    }

protected:
    
    SingletonDestructPattern() = default;
    virtual ~SingletonDestructPattern() = default;

    SingletonDestructPattern(const SingletonDestructPattern&) = delete;
    SingletonDestructPattern& operator=(const SingletonDestructPattern&) = delete;

    SingletonDestructPattern(SingletonDestructPattern&&) = delete;
    SingletonDestructPattern& operator=(SingletonDestructPattern&&) = delete;

private:

    struct Deleter
    {
        void operator()(T* ptr)
        {
            delete ptr;
        }
    };

    static std::atomic<T*> instance_;
    static std::mutex instanceMutex_;
    
    static std::unique_ptr<T, Deleter> instanceOwner_;
};

template<typename T>
std::atomic<T*> SingletonDestructPattern<T>::instance_{nullptr};

template<typename T>
std::unique_ptr<T, typename SingletonDestructPattern<T>::Deleter> SingletonDestructPattern<T>::instanceOwner_{nullptr};

template<typename T>
std::mutex SingletonDestructPattern<T>::instanceMutex_;


#endif // SINGLETON_H

