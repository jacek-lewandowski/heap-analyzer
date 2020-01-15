package net.enigma.test.toolkit;

public abstract class HeapAnalyzer {
    public static final HeapAnalyzer instance;

    static {
        HeapAnalyzer instanceLocal;
        try {

            HeapAnalyzerAgent.init();
            instanceLocal = new HeapAnalyzerNativeDelegate();
        } catch (Error err) {
            System.err.println("Looks like there is no HeapAnalyzer agent present, using a dummy implementation");
            instanceLocal = new HeapAnalyzer() {
            };
        }
        instance = instanceLocal;
    }

    private HeapAnalyzer() {
        // make it sealed
    }

    private static class HeapAnalyzerNativeDelegate extends HeapAnalyzer {
        @Override
        public boolean isAgentAvailable() {
            return true;
        }

        @Override
        public synchronized HeapTraversalSummary traverseHeap() {
            return HeapAnalyzerAgent.traverseHeap();
        }

        @Override
        public synchronized void dumpReferences(long sizeThreshold, int depth) {
            HeapAnalyzerAgent.dumpReferences(sizeThreshold, depth);
        }

        @Override
        public synchronized void forceGC() {
            HeapAnalyzerAgent.forceGC();
        }

        @Override
        public void setTag(Object object, long tag) {
            if (object == null)
                throw new IllegalArgumentException("object cannot be null");
            HeapAnalyzerAgent.setTag(object, tag);
        }

        @Override
        public long getTag(Object object) {
            if (object == null)
                throw new IllegalArgumentException("object cannot be null");
            return HeapAnalyzerAgent.getTag(object);
        }

        @Override
        public void setTag(long curTag, long newTag) {
            if (curTag == 0)
                throw new IllegalArgumentException("curTag cannot be 0");

            HeapAnalyzerAgent.setTag(curTag, newTag);
        }

        @Override
        public void setTag(long newTag) {
            HeapAnalyzerAgent.setTag(0, newTag);
        }

        @Override
        public void markObject(Object obj) {
            HeapAnalyzerAgent.markObject(obj);
        }

        @Override
        public void unmarkObject(Object obj) {
            HeapAnalyzerAgent.unmarkObject(obj);
        }

        @Override
        public void skipRefsFromClassesBySubstring(String pattern) {
            HeapAnalyzerAgent.skipRefsFromClassesBySubstring(pattern);
        }
    }

    public boolean isAgentAvailable() {
        return false;
    }

    public HeapTraversalSummary traverseHeap() {
        return new HeapTraversalSummary(0, 0, 0, 0);
    }

    public void dumpReferences(long sizeThreshold, int depth) {
        // no-op
    }

    /**
     * @see HeapAnalyzerAgent#forceGC()
     */
    public void forceGC() {
        // no-op
    }

    /**
     * @see HeapAnalyzerAgent#setTag(Object, long)
     */
    public void setTag(Object object, long tag) {
        // no-op
    }

    /**
     * @see HeapAnalyzerAgent#getTag(Object)
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
     * @see HeapAnalyzerAgent#markObject(Object)
     */
    public void markObject(Object obj) {
        // no-op
    }

    /**
     * @see HeapAnalyzerAgent#unmarkObject(Object)
     */
    public void unmarkObject(Object obj) {
        // no-op
    }

    /**
     * @see HeapAnalyzerAgent#skipRefsFromClassesBySubstring(String)
     */
    public void skipRefsFromClassesBySubstring(String pattern) {
        // no-op
    }
}
