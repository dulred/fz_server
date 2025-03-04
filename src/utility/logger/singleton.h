#ifndef FZ_SINGLETON_H
#define FZ_SINGLETON_H

#define MY_DISABLE_COPY_MOVE(Class)             \
    Class(const Class &) = delete;              \
    Class &operator=(const Class &) = delete;   \
    Class(Class &&) = delete;                   \
    Class &operator=(Class &&) = delete;

namespace fz {

template <typename DerivedClass>
struct Singleton {
    static DerivedClass& instance() {
        static DerivedClass s_instance;
        return s_instance;
    }

protected:
    Singleton()  = default;
    ~Singleton() = default;

private:
    MY_DISABLE_COPY_MOVE(Singleton)
};

}  // namespace fz

#endif  //FZ_SINGLETON_H
