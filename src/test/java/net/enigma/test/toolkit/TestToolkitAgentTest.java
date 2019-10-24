package net.enigma.test.toolkit;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import java.util.function.Supplier;

import static org.junit.Assert.*;

// The real tests still need to be implemented
public class TestToolkitAgentTest {
    public StaticTestClass tc;

    public static class StaticTestClass {
        byte[] bytes;
        StaticTestClass other;
    }

    public class InnerClass {
        long attr;
        StaticTestClass other;
    }

    @BeforeClass
    public static void initClass() {
        TestToolkitAgent.init();
    }

    @Before
    public void init() {
        TestToolkitAgent.setTag(0, 0);
    }

    @Test
    public void testTraverseHeap() {
        HeapTraversalSummary hs = TestToolkitAgent.traverseHeap();
        assertEquals(0, hs.markedCount);
        assertEquals(0, hs.markedSize);
        assertNotEquals(0, hs.totalCount);
        assertNotEquals(0, hs.totalSize);

        StaticTestClass tc = new StaticTestClass();
        tc.bytes = new byte[100];
        TestToolkitAgent.markObject(tc);
        hs = TestToolkitAgent.traverseHeap();
        assertEquals(1, hs.markedCount);
        assertTrue(hs.markedSize > 0 && hs.markedSize < 100);
        assertNotEquals(0, hs.totalCount);
        assertNotEquals(0, hs.totalSize);

        TestToolkitAgent.markObject(tc.bytes);
        hs = TestToolkitAgent.traverseHeap();
        assertEquals(2, hs.markedCount);
        assertTrue(hs.markedSize > 100);
        assertNotEquals(0, hs.totalCount);
        assertNotEquals(0, hs.totalSize);
    }

    @Test
    public void testGetSetTag() throws Throwable {
        long[] array = new long[10];
        long tag = TestToolkitAgent.getTag(array);
        assertEquals(0, tag);
        TestToolkitAgent.setTag(array, 1);
        tag = TestToolkitAgent.getTag(array);
        assertEquals(1, tag);
    }


    @Test
    public void testSetTag2() {
        StaticTestClass obj1 = new StaticTestClass();
        StaticTestClass obj2 = new StaticTestClass();
        StaticTestClass obj3 = new StaticTestClass();
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

    @Test
    public void testDebugReferences() {
        StaticTestClass c = new StaticTestClass();
        tc = c;
        StaticTestClass c2 = new StaticTestClass();
        c2.other = c;
        InnerClass c3 = new InnerClass();
        c3.other = c;

        Supplier<StaticTestClass> lambda = () -> c;

        TestToolkitAgent.markObject(c);
        TestToolkitAgent.debugReferences(10, 5);
    }

   @Test
   public void testSkipRefs() {
       TestToolkitAgent.skipRefsFromClassesBySubstring("net/enigma");
   }
}
