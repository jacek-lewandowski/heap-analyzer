package net.enigma.test.toolkit;

public class TestToolkitAgent
{
    /**
     * Counts the number of all instances on heap (including those which are not referenced any more)
     * of the given class
     *
     * @param cls class
     *
     * @return the total number of instances on heap
     */
    public static native int countInstances(Class<?> cls);

    /**
     * Counts the number of references to the given object from instances reachable from the heap roots
     *
     * @param object object to be investigated
     *
     * @return the number of live references
     */
    public static native int countLiveReferences(Object object);

    /**
     * Counts the total size of live instances with a given tag
     *
     * @param tag tag
     *
     * @return the size in bytes
     */
    public static native long countSizeOfLiveTaggedObjects(long tag);

    /**
     * Force full garbage collection
     */
    public static native void forceGC();

    /**
     * Set the tag on object
     *
     * @param object object
     * @param tag tag, where 0 is considered as no-tag
     */
    public static native void setTag(Object object, long tag);

    /**
     * Retrieves object tag
     *
     * @param object object
     *
     * @return tag, where 0 is considered as no-tag
     */
    public static native long getTag(Object object);
}
