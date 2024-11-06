import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantLock;

public class NestedReadWriteLock implements ReadWriteLock {
    // Counters and flags to maintain the number of readers,
    // writers and writers waiting for exclusive write access.
    int readers, waitingWriters;
    boolean writer;
    // Locks to be used: one for the counters 
    // and two for actual use.
    Lock lock, readLock, writeLock;
    // Conditions to be used: one each for 
    // the readers and writers.
    Condition readCondition, writeCondition;

    public NestedReadWriteLock() {
        // Initialize counters and flags
        readers = waitingWriters = 0;
        writer = false;
        // Create locks
        lock = new ReentrantLock();
        readLock = new ReadLock();
        writeLock = new WriteLock();        
        // Create conditions
        readCondition = lock.newCondition();
        writeCondition = lock.newCondition();
    }

    public Lock readLock() {
        return readLock;
    }

    public Lock writeLock() {
        return writeLock;
    }

    protected class ReadLock implements Lock {
        public void lock() {
            lock.lock();
            try {
                // Wait until all writers and waiting writers are serviced
                while (writer || waitingWriters > 0) {
                    readCondition.await();
                }
                // Add to number of readers
                readers++;
            } finally {
                lock.unlock();
            }
        }

        public void unlock() {
            lock.lock();
            try {
                // Decrement number of readers
                readers--;
                // Signal a single waiting writer thread to wake up
                // if it exists and there are no more readers
                if (readers == 0 && waitingWriters > 0) {
                    writeCondition.signal();
                }
            } finally {
                lock.unlock();
            }
        }
        
    }

    protected class WriteLock implements Lock {

        // Threads must acquire the read lock first
        public void lock() {
            lock.lock();
            try {
                // Decrement the number of readers
                readers--;
                // Increment the number of waiting writers
                waitingWriters++;
                // Wait until I am the only reader
                while (readers > 0 || writer) {
                    writeCondition.await();
                }
                // Decrement the number of waiting writers
                waitingWriters--;
                // Become a writer with exclusive write access
                writer = true;
            } finally {
                lock.unlock();
            }
        }

        public void unlock() {
            lock.lock();
            try {
                // Unset writer flag
                writer = false;
                if (waitingWriters > 0) {
                    // First, wake up waiting writers if any
                    writeCondition.signal();
                } else {
                    // Otherwise, wake up all readers
                    readCondition.signalAll();
                }
                // Increment the number of readers
                readers++;
            } finally {
                lock.unlock();
            }
        }

    }

}