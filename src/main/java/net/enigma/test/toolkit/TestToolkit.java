package net.enigma.test.toolkit;

public abstract class TestToolkit
{
    public static final TestToolkit instance;

    static
    {
        TestToolkit instanceLocal = null;
        try
        {
            TestToolkitAgent.getTag(TestToolkit.class);
            instanceLocal = new TestToolkitNativeDelegate();
        }
        catch (Error err)
        {
            System.err.println("Looks like there is no TestToolkit agent present, using a dummy implementation");
            instanceLocal = new TestToolkit() {};
        }
        instance = instanceLocal;
    }

    private TestToolkit()
    {
        // make it sealed
    }

    private static class TestToolkitNativeDelegate extends TestToolkit
    {
        @Override
        public boolean isAgentAvailable()
        {
            return true;
        }

        @Override
        public int countInstances(Class<?> cls)
        {
            return TestToolkitAgent.countInstances(cls);
        }

        @Override
        public int countLiveReferences(Object object)
        {
            return TestToolkitAgent.countLiveReferences(object);
        }

        @Override
        public long countLiveTaggedObjects(long tag, boolean debugReferences)
        {
            return TestToolkitAgent.countLiveTaggedObjects(tag, debugReferences);
        }

        @Override
        public long countSizeOfLiveTaggedObjects(long tag, boolean debugReferences)
        {
            return TestToolkitAgent.countSizeOfLiveTaggedObjects(tag, debugReferences);
        }

        @Override
        public void forceGC()
        {
            TestToolkitAgent.forceGC();
        }

        @Override
        public void setTag(Object object, long tag)
        {
            TestToolkitAgent.setTag(object, tag);
        }

        @Override
        public long getTag(Object object)
        {
            return TestToolkitAgent.getTag(object);
        }

        @Override
        public void setTag(long curTag, long newTag)
        {
            if (curTag == 0)
                throw new IllegalArgumentException("curTag cannot be 0");

            TestToolkitAgent.setTag(curTag, newTag);
        }

        @Override
        public void setTag(long newTag)
        {
            TestToolkitAgent.setTag(0, newTag);
        }
    }

    public boolean isAgentAvailable()
    {
        return false;
    }

    /**
     * Counts the number of all instances on heap (including those which are not referenced any more)
     * of the given class
     *
     * @param cls class
     *
     * @return the total number of instances on heap
     */
    public int countInstances(Class<?> cls)
    {
        // no-op
        return 0;
    }

    /**
     * Counts the number of references to the given object from instances reachable from the heap roots
     *
     * @param object object to be investigated
     *
     * @return the number of live references
     */
    public int countLiveReferences(Object object)
    {
        // no-op
        return 0;
    }

    /**
     * Counts the total number of live instances with a given tag
     *
     * @param tag tag
     *
     * @return the number of live instances
     */
    public long countLiveTaggedObjects(long tag, boolean debugReferences)
    {
        // no-op
        return 0;
    }

    /**
     * Counts the total size of live instances with a given tag
     *
     * @param tag tag
     *
     * @return the size in bytes
     */
    public long countSizeOfLiveTaggedObjects(long tag, boolean debugReferences)
    {
        // no-op
        return 0;
    }

    /**
     * Force full garbage collection
     */
    public void forceGC()
    {
        // no-op
    }

    /**
     * Set the tag on object
     *
     * @param object object
     * @param tag tag, where 0 is considered as no-tag
     */
    public void setTag(Object object, long tag)
    {
        // no-op
    }

    /**
     * Retrieves object tag
     *
     * @param object object
     *
     * @return tag, where 0 is considered as no-tag
     */
    public long getTag(Object object)
    {
        // no-op
        return 0;
    }

    /**
     * Sets the tag {@code newTag} on all objects tagged with {@code curTag}
     *
     * @param curTag tag that has to be set on the object to update that object's tag, cannot be 0
     * @param newTag tag to be set on selected objects objects
     */
    public void setTag(long curTag, long newTag)
    {
        // no-op
    }

    /**
     * Sets the tag {@code newTag} on all objects.
     *
     * @param newTag tag to be set on all objects
     */
    public void setTag(long newTag)
    {
        // no-op
    }
}
