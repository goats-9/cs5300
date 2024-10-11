template<class T>
class Snapshot {
public:
  virtual void update(int l, T v) = 0;
  virtual std::vector<T> snapshot() = 0;
};