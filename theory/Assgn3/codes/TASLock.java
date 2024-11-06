public class TASLock implements Lock {
    public boolean isLocked() {
        return state.get();
    }
}