package net.enigma.test.toolkit;

import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;

public class HeapAnalyzerExamples {
    public static void taggingExample() {
        System.out.println("\nTagging example\n================================");
        HeapAnalyzer.instance.setTag(0); // reset tags for all objects to 0

        Object o = new Object();
        System.out.printf("tag of o = %d%n", HeapAnalyzer.instance.getTag(o));

        HeapAnalyzer.instance.setTag(o, 100);
        System.out.printf("tag of o after setTag(o, 100) = %d%n", HeapAnalyzer.instance.getTag(o));

        HeapAnalyzer.instance.markObject(o);
        System.out.printf("tag of o after marking it = %d%n", HeapAnalyzer.instance.getTag(o));

        Object o1 = new Object(), o2 = new Object(), o3 = new Object();
        HeapAnalyzer.instance.setTag(o1, 1);
        HeapAnalyzer.instance.setTag(o2, 2);
        HeapAnalyzer.instance.setTag(o3, 2);
        System.out.printf("tags of o1, o2, o3 = %d, %d, %d%n",
                HeapAnalyzer.instance.getTag(o1), HeapAnalyzer.instance.getTag(o2), HeapAnalyzer.instance.getTag(o3));

        // set tag to 3 for all objects which have tag 2
        HeapAnalyzer.instance.setTag(2, 3);
        System.out.printf("tags of o1, o2, o3 after setTag(2, 3) = %d, %d, %d%n",
                HeapAnalyzer.instance.getTag(o1), HeapAnalyzer.instance.getTag(o2), HeapAnalyzer.instance.getTag(o3));

        // set tag 10 for all objects
        HeapAnalyzer.instance.setTag(10);
        System.out.printf("tags of o1, o2, o3 after setTag(0) = %d, %d, %d%n",
                HeapAnalyzer.instance.getTag(o1), HeapAnalyzer.instance.getTag(o2), HeapAnalyzer.instance.getTag(o3));

        System.out.printf("tags of String.class, StringBuilder.class, StringBuffer = %d, %d, %d%n",
                HeapAnalyzer.instance.getTag(String.class),
                HeapAnalyzer.instance.getTag(StringBuilder.class),
                HeapAnalyzer.instance.getTag(StringBuffer.class));

        // set SKIP_REFS_FROM bit on all classes whose signatures contain string '/StringBu'
        HeapAnalyzer.instance.skipRefsFromClassesBySubstring("/StringBu");
        System.out.printf("tags of String.class, StringBuilder.class, StringBuffer after skipRefsFromClassesBySubstring('/StringBu') = %d, %d, %d%n",
                HeapAnalyzer.instance.getTag(String.class),
                HeapAnalyzer.instance.getTag(StringBuilder.class),
                HeapAnalyzer.instance.getTag(StringBuffer.class));
    }

    public static void heapTraversalExample1() {
        System.out.println("\nHeap traversal example 1\n================================");
        HeapAnalyzer.instance.setTag(0); // reset tags for all objects to 0

        HeapTraversalSummary hs1 = HeapAnalyzer.instance.traverseHeap();
        System.out.println(hs1);

        byte[] array = new byte[10000]; // now we have a new object on heap

        HeapTraversalSummary hs2 = HeapAnalyzer.instance.traverseHeap();
        System.out.println("after creating array of 10000 bytes " + hs2);

        HeapAnalyzer.instance.markObject(array); // marking interesting object
        HeapTraversalSummary hs3 = HeapAnalyzer.instance.traverseHeap();
        System.out.println("after marking that array " + hs3);

        array = null;
        HeapTraversalSummary hs4 = HeapAnalyzer.instance.traverseHeap();
        System.out.println("after dereferencing that array " + hs4);
    }

    public static void heapTraversalExample2() {
        System.out.println("\nHeap traversal example 2\n================================");
        HeapAnalyzer.instance.setTag(0); // reset tags for all objects to 0

        HeapTraversalSummary hs1 = HeapAnalyzer.instance.traverseHeap();
        System.out.println(hs1);

        AtomicReference<List<byte[]>> ref = new AtomicReference<>(Collections.singletonList(new byte[10000]));
        HeapAnalyzer.instance.markObject(ref.get().get(0));
        HeapTraversalSummary hs2 = HeapAnalyzer.instance.traverseHeap();
        System.out.println("after creating AtomicReference which indirectly references array of 10000 bytes " + hs2);

        HeapAnalyzer.instance.skipRefsFromClassesBySubstring("AtomicReference");
        HeapTraversalSummary hs3 = HeapAnalyzer.instance.traverseHeap();
        System.out.println("after marking skipping all references from all AtomicReference instances " + hs3);
    }

    public static void debugReferencesExample() {
        System.out.println("\nDebug references example\n================================");
        HeapAnalyzer.instance.setTag(0); // reset tags for all objects to 0

        byte[] array = new byte[10000];
        AtomicReference<List<byte[]>> ref = new AtomicReference<>(Collections.singletonList(array));
        Thread t = new Thread("example thread") {
            @Override
            public void run() {
                int i =0;
                while (true) {
                    i = i + array.length;
                    try { Thread.sleep(100); } catch (InterruptedException e) { return; }
                }
            }
        };
        t.setDaemon(true);
        t.start();

        HeapAnalyzer.instance.markObject(array);
        HeapAnalyzer.instance.debugReferences(0, 3);
    }

    public static void main(String[] args) {
        taggingExample();
        heapTraversalExample1();
        heapTraversalExample2();
        debugReferencesExample();
    }

}
