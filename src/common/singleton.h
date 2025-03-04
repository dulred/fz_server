#ifndef FZ_SINGLETON_H
#define FZ_SINGLETON_H

namespace fz
{
    namespace common
    {
        template <typename T>
        class Singleton
        {
        public:
            static T* instance()
            {
                static T instance;
                return &instance;
            }
        private:
            Singleton() = default;
            Singleton(const Singleton<T>&) = delete;
            Singleton<T>& operator = (const Singleton<T>&) = delete;
            ~Singleton() = default;
        };

        #define SINGLETON(classname) \
            friend class Singleton<classname>; \
            private:    \
                classname(const classname&) = delete; \
                classname& operator = (const classname&) = delete; \
                // classname() = default; \
                // ~classname() = default; \
            
    } // namespace common
    
} // namespace fz


#endif