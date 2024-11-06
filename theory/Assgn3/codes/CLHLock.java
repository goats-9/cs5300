public class CLHLock {
    public boolean isLocked() {
        QNode tnode = tail.get();
        return tnode.locked;   
    }
}
