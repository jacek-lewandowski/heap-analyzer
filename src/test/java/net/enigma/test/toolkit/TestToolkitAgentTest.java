package net.enigma.test.toolkit;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class TestToolkitAgentTest
{
    @Test
    public void testGetSetTag() throws Throwable {
        long[] array = new long[10];
        long tag = TestToolkitAgent.getTag(array);
        assertEquals(0, tag);
        TestToolkitAgent.setTag(array, 1);
        tag = TestToolkitAgent.getTag(array);
        assertEquals(1, tag);
    }

    public static class TestClass {
        long attr;
    }

    @Test
    public void testCountInstances() throws Throwable {
        int n0 = TestToolkitAgent.countInstances(TestClass.class);
        TestClass obj = new TestClass();
        int n1 = TestToolkitAgent.countInstances(TestClass.class);
        assertEquals(1, n1 - n0);
    }

    @Test
    public void testLiveReferences() throws Throwable {
        TestClass obj = new TestClass();
        assertEquals(1, TestToolkitAgent.countLiveReferences(obj));
        TestClass ref = obj;
        assertEquals(2, TestToolkitAgent.countLiveReferences(obj));
        obj = null;
        assertEquals(1, TestToolkitAgent.countLiveReferences(obj));
    }

    @Test
    public void testCountSizeOfLiveTaggedObjects()
    {
        TestClass obj = new TestClass();
        assertEquals(0, TestToolkitAgent.countSizeOfLiveTaggedObjects(1, false));
        TestToolkitAgent.setTag(obj, 2);
        assertEquals(0, TestToolkitAgent.countSizeOfLiveTaggedObjects(1, false));
        assertEquals(24, TestToolkitAgent.countSizeOfLiveTaggedObjects(2, false));
        TestToolkitAgent.setTag(obj, 1);
        assertEquals(24, TestToolkitAgent.countSizeOfLiveTaggedObjects(1, false));
        assertEquals(0, TestToolkitAgent.countSizeOfLiveTaggedObjects(2, false));
    }

    @Test
    public void testCountLiveTaggedObjects()
    {
        TestClass obj = new TestClass();
        assertEquals(0, TestToolkitAgent.countLiveTaggedObjects(1, false));
        TestToolkitAgent.setTag(obj, 2);
        assertEquals(0, TestToolkitAgent.countLiveTaggedObjects(1, false));
        assertEquals(1, TestToolkitAgent.countLiveTaggedObjects(2, false));
        TestToolkitAgent.setTag(obj, 1);
        assertEquals(1, TestToolkitAgent.countLiveTaggedObjects(1, false));
        assertEquals(0, TestToolkitAgent.countLiveTaggedObjects(2, false));
    }

    @Test
    public void testSetTag2()
    {
        TestClass obj1 = new TestClass();
        TestClass obj2 = new TestClass();
        TestClass obj3 = new TestClass();
        TestToolkitAgent.setTag(obj2, 2);
        TestToolkitAgent.setTag(obj3, 3);

        assertEquals(2, TestToolkitAgent.getTag(obj2));
        TestToolkitAgent.setTag(2, 4);
        assertEquals(4, TestToolkitAgent.getTag(obj2));

        assertEquals(3, TestToolkitAgent.getTag(obj3));
        TestToolkitAgent.setTag(0, 5);
        assertEquals(5, TestToolkitAgent.getTag(obj1));
        assertEquals(5, TestToolkitAgent.getTag(obj2));
        assertEquals(5, TestToolkitAgent.getTag(obj3));
    }
}
