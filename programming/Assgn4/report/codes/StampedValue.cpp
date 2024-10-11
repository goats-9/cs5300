template<class T>
struct StampedValue {
  T value;        // Value stored
  uint16_t stamp; // Timestamp
  uint16_t id;    // Thread id
  
  StampedValue(T val, uint16_t ts = 0, uint16_t tid = 0) : value(val), stamp(ts), id(tid) {}

  bool operator == (const StampedValue<T> stval) const {
    return value == stval.value and stamp == stval.stamp and id == stval.id;
  }
};
