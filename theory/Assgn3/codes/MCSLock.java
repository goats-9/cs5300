public class MCSLock {
    public boolean isLocked() {
        QNode node = tail.get();
        return node != null && node.locked;
    } 
}
