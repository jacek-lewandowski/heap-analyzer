package net.enigma.test.toolkit;

public abstract class TestToolkit {
    public static final TestToolkit instance;

    static {
        TestToolkit instanceLocal;
        try {

            TestToolkitAgent.init();
            instanceLocal = new TestToolkitNativeDelegate();
        } catch (Error err) {
            System.err.println("Looks like there is no TestToolkit agent present, using a dummy implementation");
            instanceLocal = new TestToolkit() {
            };
        }
        instance = instanceLocal;
    }

    private TestToolkit() {
        // make it sealed
    }

    private static class TestToolkitNativeDelegate extends TestToolkit {
        @Override
        public boolean isAgentAvailable() {
            return true;
        }

        @Override
        public synchronized HeapTraversalSummary traverseHeap() {
            return TestToolkitAgent.traverseHeap();
        }

        @Override
        public synchronized void debugReferences(long sizeThreshold, int depth) {
            TestToolkitAgent.debugReferences(sizeThreshold, depth);
        }

        @Override
        public synchronized void forceGC() {
            TestToolkitAgent.forceGC();
        }

        @Override
        public void setTag(Object object, long tag) {
            TestToolkitAgent.setTag(object, tag);
        }

        @Override
        public long getTag(Object object) {
            return TestToolkitAgent.getTag(object);
        }

        @Override
        public void setTag(long curTag, long newTag) {
            if (curTag == 0)
                throw new IllegalArgumentException("curTag cannot be 0");

            TestToolkitAgent.setTag(curTag, newTag);
        }

        @Override
        public void setTag(long newTag) {
            TestToolkitAgent.setTag(0, newTag);
        }

        @Override
        public void markObject(Object obj) {
            TestToolkitAgent.markObject(obj);
        }

        @Override
        public void skipRefsFromClassesBySubstring(String pattern) {
            TestToolkitAgent.skipRefsFromClassesBySubstring(pattern);
        }
    }

    public boolean isAgentAvailable() {
        return false;
    }

    public HeapTraversalSummary traverseHeap() {
        return new HeapTraversalSummary(0, 0, 0, 0);
    }

    public void debugReferences(long sizeThreshold, int depth) {
        // no-op
    }

    /**
     * @see TestToolkitAgent#forceGC() 
     */
    public void forceGC() {
        // no-op
    }

    /**
     * @see TestToolkitAgent#setTag(Object, long) 
     */
    public void setTag(Object object, long tag) {
        // no-op
    }

    /**
     * @see TestToolkitAgent#getTag(Object) 
     */
    public long getTag(Object object) {
        // no-op
        return 0;
    }

    /**
     * Sets the tag {@code newTag} on all objects tagged with {@code curTag}
     *
     * @param curTag tag that has to be set on the object to update that object's tag, cannot be 0
     * @param newTag tag to be set on selected objects objects
     */
    public void setTag(long curTag, long newTag) {
        // no-op
    }

    /**
     * Sets the tag {@code newTag} on all objects.
     *
     * @param newTag tag to be set onł all objects
     */
    public void setTag(long newTag) {
        // no-op
    }

    /**
     * @see TestToolkitAgent#markObject(Object) 
     */
    public void markObject(Object obj) {
        // no-op
    }

    /**
     * @see TestToolkitAgent#skipRefsFromClassesBySubstring(String)
     */
    public void skipRefsFromClassesBySubstring(String pattern) {
        // no-op
    }
}
