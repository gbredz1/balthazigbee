#pragma once
#include <memory>

template <typename T>
class Singleton {
  protected:
    Singleton() = default;
    ~Singleton() = default;
    Singleton(Singleton const &) = delete;
    void operator=(Singleton const &) = delete;

  public:
    // static T &instance() {
    //     static T instance;
    //     return instance;
    // }

    // static std::shared_ptr<T> instance() {
    //     static std::shared_ptr<T> instance{new T};
    //     return instance;
    // }

    static std::shared_ptr<T> instance() {
        static std::shared_ptr<T> instance;
        if (instance == nullptr) {
            instance.reset(new T()); // Create new instance, assign to unique_ptr.
        }
        return instance;
    }
};
