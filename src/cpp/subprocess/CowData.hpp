/** @cond PRIVATE */
#include <cstdint>
#include <memory>

// experemnting with this to reduce amount of copies for some APIs
namespace subprocess {
    class RefCount {
    public:
        int release()   { return --mCount; }
        int retain()    { return ++mCount; }
        int refCount()  { return mCount; }
    private:
        int mCount=1;
    };
    class Data {
    public:
        typedef uint8_t byte;

        void reserve(ssize_t size);
    private:
        byte*       mData       = nullptr;
        ssize_t     mSize       = 0;
        ssize_t     mCapacity   = 0;
    };
    class CowData {
    public:
        typedef uint8_t data_type;
        void reserve(ssize_t size) { mData.resize(size); }

    private:
        std::shared_ptr<std::string> mData;
    };
}
/** @endcond */