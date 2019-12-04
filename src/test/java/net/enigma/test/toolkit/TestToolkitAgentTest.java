package net.enigma.test.toolkit;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import java.util.Arrays;
import java.util.concurrent.atomic.AtomicReference;

import static net.enigma.test.toolkit.TestToolkit.instance;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

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
        assertTrue(instance.isAgentAvailable());
    }

    @Before
    public void init() {
        instance.setTag(0);
    }

    @Test
    public void testGetSetTag() throws Throwable {
        Object o = new Object();
        long tag = instance.getTag(o);
        assertEquals(0, tag);

        instance.setTag(o, 1);
        tag = instance.getTag(o);
        assertEquals(1, tag);
    }

    @Test
    public void testSetOfManyObjects() {
        Object[] array = new Object[10];

        for (int i = 0; i < array.length; i++) {
            array[i] = new Object();
            instance.setTag(array[i], (i + 10) / 2);
        }

        assertTags(array, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9);
        instance.setTag(5, 25);

        assertTags(array, 25, 25, 6, 6, 7, 7, 8, 8, 9, 9);

        instance.setTag(3);
        assertTags(array, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3);
        assertEquals(3, instance.getTag(array));
    }

    @Test
    public void testFlaggingObjectsWithMarkerBit() {
        Object o = new Object();
        instance.setTag(123);
        assertEquals(123, instance.getTag(o));

        instance.markObject(o);
        long tag = instance.getTag(o);
        assertEquals(123L, tag & 0xFFFFFFFFL);
        assertEquals(1L, tag >> 32L);
    }

    @Test
    public void testFlaggingClassesWithSkipRefsFromBit() {
        instance.setTag(String.class, 11);
        instance.setTag(StringBuilder.class, 12);
        instance.setTag(StringBuffer.class, 13);
        assertTags(new Object[]{String.class, StringBuilder.class, StringBuffer.class}, 11, 12, 13);

        instance.skipRefsFromClassesBySubstring("/StringBu");
        long skipRefsFromBit = 0x1000000000L;
        assertTags(new Object[]{String.class, StringBuilder.class, StringBuffer.class}, 11, 12 | skipRefsFromBit, 13 | skipRefsFromBit);
    }

    @Test
    public void testTraverseHeap() {
        byte[][] array = new byte[100][];
        HeapTraversalSummary hs1 = TestToolkit.instance.traverseHeap();
        assertTrue(hs1.totalCount > 0);
        assertTrue(hs1.totalSize > 0);

        for (int i = 0; i < array.length; i++) array[i] = new byte[200];
        HeapTraversalSummary hs2 = TestToolkit.instance.traverseHeap();
        assertEquals(100, hs2.totalCount - hs1.totalCount, 30);
        assertEquals(20000, hs2.totalSize - hs1.totalSize, 6000);

        byte[][] extraRef = array;
        HeapTraversalSummary hs3 = TestToolkit.instance.traverseHeap();
        assertEquals(100, hs3.totalCount - hs1.totalCount, 30);
        assertEquals(20000, hs3.totalSize - hs1.totalSize, 6000);

        for (int i = 0; i < array.length / 2; i++) array[i] = null;
        HeapTraversalSummary hs4 = TestToolkit.instance.traverseHeap();
        assertEquals(50, hs2.totalCount - hs4.totalCount, 15);
        assertEquals(10000, hs2.totalSize - hs4.totalSize, 3000);
    }

    @Test
    public void testTraverseHeapWithMarkedObjects() {
        byte[][] array = new byte[100][];
        HeapTraversalSummary hs1 = TestToolkit.instance.traverseHeap();
        assertEquals(hs1.markedCount, 0);
        assertEquals(hs1.markedSize, 0);

        for (int i = 0; i < array.length; i++) array[i] = new byte[200];
        HeapTraversalSummary hs2 = TestToolkit.instance.traverseHeap();
        assertEquals(hs2.markedCount, 0);
        assertEquals(hs2.markedSize, 0);


        Arrays.stream(array).forEach(instance::markObject);
        HeapTraversalSummary hs3 = TestToolkit.instance.traverseHeap();
        assertEquals(hs3.markedCount, 100);
        assertEquals(hs3.markedSize, 21600);

        for (int i = 0; i < array.length / 2; i++) array[i] = null;
        HeapTraversalSummary hs4 = TestToolkit.instance.traverseHeap();
        assertEquals(hs4.markedCount, 50);
        assertEquals(hs4.markedSize, 10800);

        byte[][] extraRef = array;
        HeapTraversalSummary hs5 = TestToolkit.instance.traverseHeap();
        assertEquals(hs5.markedCount, 50);
        assertEquals(hs5.markedSize, 10800);
    }

    @Test
    public void testTraverseHeapWithClassExclusion() {
        byte[] o = new byte[1000];
        instance.markObject(o);
        HeapTraversalSummary hs1 = TestToolkit.instance.traverseHeap();
        assertEquals(1, hs1.markedCount);
        assertEquals(1016, hs1.markedSize);

        AtomicReference<byte[]> ref = new AtomicReference<>(o);
        HeapTraversalSummary hs2 = TestToolkit.instance.traverseHeap();
        assertEquals(1, hs2.markedCount);
        assertEquals(1016, hs2.markedSize);

        o = null;
        HeapTraversalSummary hs3 = TestToolkit.instance.traverseHeap();
        assertEquals(1, hs3.markedCount);
        assertEquals(1016, hs3.markedSize);

        instance.skipRefsFromClassesBySubstring("/AtomicReference");
        HeapTraversalSummary hs4 = TestToolkit.instance.traverseHeap();
        assertEquals(0, hs4.markedCount);
        assertEquals(0, hs4.markedSize);

        o = ref.get();
        HeapTraversalSummary hs5 = TestToolkit.instance.traverseHeap();
        assertEquals(1, hs5.markedCount);
        assertEquals(1016, hs5.markedSize);
    }

    private void assertTags(Object[] array, long... tags) {
        for (int i = 0; i < tags.length; i++) {
            assertEquals(String.format("[%d]", i), tags[i], instance.getTag(array[i]));
        }
    }

}
