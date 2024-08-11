template<class T>
class Counter {
    /// 64-bit atomic unsigned counter.
    std::atomic<T> ctr;
public:
    Counter(T n = 0);

    T get();

    T getAndIncrement(T inc = 1);
};
