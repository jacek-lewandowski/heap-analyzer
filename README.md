# Test Toolkit

This agent and its accompanying Java library provides functionality which may be helpful to track heap memory usage
of Java programs.

All examples can be run by executing `gradle run`

## Tagging objects

Each Java object can have a tag which is a 64 bit number. Tags can be used to store extra information about objects
without changing or explicitly referencing those objects. Test Toolkit internally uses tags for heap memory usage 
scanning and for exploring referents paths. In particular, calling those methods may change tags you previously set
on some objects to some meaningless values. However, some bits which have special meaning will survive (for example
`MARKER` bit or `SKIP_REFS_FROM` bit, they will be explained below, also see utils.h for more information).

#### Example - raw tag manipulation

```java
Object o = new Object();
System.out.printf("tag of o = %d%n", TestToolkit.instance.getTag(o));

TestToolkit.instance.setTag(o, 100);
System.out.printf("tag of o after setTag(o, 100) = %d%n", TestToolkit.instance.getTag(o));

// set MARKER bit on the object
TestToolkit.instance.markObject(o);
System.out.printf("tag of o after marking it = %d%n", TestToolkit.instance.getTag(o));

// set SKIP_REFS_FROM bit on class objects matching the provided signature

Object o1 = new Object(), o2 = new Object(), o3 = new Object();
TestToolkit.instance.setTag(o1, 1);
TestToolkit.instance.setTag(o2, 2);
TestToolkit.instance.setTag(o3, 2);
System.out.printf("tags of o1, o2, o3 = %d, %d, %d%n",
        TestToolkit.instance.getTag(o1), TestToolkit.instance.getTag(o2), TestToolkit.instance.getTag(o3));

// set tag to 3 for all objects which have tag 2
TestToolkit.instance.setTag(2, 3);
System.out.printf("tags of o1, o2, o3 after setTag(2, 3) = %d, %d, %d%n",
        TestToolkit.instance.getTag(o1), TestToolkit.instance.getTag(o2), TestToolkit.instance.getTag(o3));

// set tag 10 for all objects
TestToolkit.instance.setTag(10);
System.out.printf("tags of o1, o2, o3 after setTag(0) = %d, %d, %d%n",
        TestToolkit.instance.getTag(o1), TestToolkit.instance.getTag(o2), TestToolkit.instance.getTag(o3));

System.out.printf("tags of String.class, StringBuilder.class, StringBuffer = %d, %d, %d%n",
        TestToolkit.instance.getTag(String.class),
        TestToolkit.instance.getTag(StringBuilder.class),
        TestToolkit.instance.getTag(StringBuffer.class));

// set SKIP_REFS_FROM bit on all classes whose signatures contain string '/StringBu'
TestToolkit.instance.skipRefsFromClassesBySubstring("/StringBu");
System.out.printf("tags of String.class, StringBuilder.class, StringBuffer after skipRefsFromClassesBySubstring('/StringBu') = %d, %d, %d%n",
        TestToolkit.instance.getTag(String.class),
        TestToolkit.instance.getTag(StringBuilder.class),
        TestToolkit.instance.getTag(StringBuffer.class));
```

produces output similar to:

```
tag of o = 0
tag of o after setTag(o, 100) = 100
tag of o after marking it = 4294967396
tags of o1, o2, o3 = 1, 2, 2
tags of o1, o2, o3 after setTag(2, 3) = 1, 3, 3
tags of o1, o2, o3 after setTag(0) = 10, 10, 10
tags of String.class, StringBuilder.class, StringBuffer after setTag(0) = 10, 10, 10
tags of String.class, StringBuilder.class, StringBuffer after setTag(0) = 10, 68719476746, 68719476746
```

## Heap memory usage

A method which scans heap returns information about the number of total traversed objects and total size they use on 
heap. Additionally, it returns similar summary of objects which were previously marked. Note that these summaries are
done for objects which are reachable from heap roots, that is, which are not subject to garbage collection. There are
however some exceptions - phantom, weak and soft references are treated the same as any other reference and even if some 
object is references only from such a wrapper, it will be counted in the summary. Continue reading to learn how to 
overcome that problem.  

The example below shows the heap summaries with and without marked objects.

#### Example - heap memory usage

```java
HeapTraversalSummary hs1 = TestToolkit.instance.traverseHeap();
System.out.println(hs1);

byte[] array = new byte[10000]; // now we have a new object on heap

HeapTraversalSummary hs2 = TestToolkit.instance.traverseHeap();
System.out.println("after creating array of 10000 bytes " + hs2);

TestToolkit.instance.markObject(array); // marking interesting object
HeapTraversalSummary hs3 = TestToolkit.instance.traverseHeap();
System.out.println("after marking that array " + hs3);

array = null;
HeapTraversalSummary hs4 = TestToolkit.instance.traverseHeap();
System.out.println("after dereferencing that array " + hs4);
```

produces output similar to:

```
HeapTraversalSummary[totalSize=441064, totalCount=8557, markedSize=0, markedCount=0]
after creating array of 10000 bytes HeapTraversalSummary[totalSize=452448, totalCount=8593, markedSize=0, markedCount=0]
after marking that array HeapTraversalSummary[totalSize=452608, totalCount=8596, markedSize=10016, markedCount=1]
after dereferencing that array HeapTraversalSummary[totalSize=442736, totalCount=8598, markedSize=0, markedCount=0]
```

We can prune some paths in the reference graph by skipping traversals from objects of classes flagged with 
`SKIP_REFS_FROM` bit. This bit can be set either explicitly by calling one of `setTag` methods, or with use of handy
method `skipRefsFromClassesBySubstring`. All classes whose signatures contain the provided string will be automatically
flagged with `SKIP_REFS_FROM` bit. This method can be called multiple times for different patterns.

The example below shows that after excluding `AtomicReference` class, objects references by its instances are not 
counted in the summaries. Note that this can be used to exclude references from `WeakReference` or `SoftReference` if 
needed.

#### Example - excluding classes

```java
HeapTraversalSummary hs1 = TestToolkit.instance.traverseHeap();
System.out.println(hs1);

AtomicReference<List<byte[]>> ref = new AtomicReference<>(Collections.singletonList(new byte[10000]));
TestToolkit.instance.markObject(ref.get().get(0));
HeapTraversalSummary hs2 = TestToolkit.instance.traverseHeap();
System.out.println("after creating AtomicReference which indirectly references array of 10000 bytes " + hs2);

TestToolkit.instance.skipRefsFromClassesBySubstring("AtomicReference");
HeapTraversalSummary hs3 = TestToolkit.instance.traverseHeap();
System.out.println("after marking skipping all references from all AtomicReference instances " + hs3);
```

produces output similar to:

```
HeapTraversalSummary[totalSize=442856, totalCount=8599, markedSize=0, markedCount=0]
after creating AtomicReference which indirectly references array of 10000 bytes HeapTraversalSummary[totalSize=453952, totalCount=8625, markedSize=10016, markedCount=1]
after marking skipping all references from all AtomicReference instances HeapTraversalSummary[totalSize=150400, totalCount=3982, markedSize=0, markedCount=0]
```

## Debugging references

Another method is designed to debug references leading to the marked objects (referents of marked object). It also 
applies only to the live objects, reachable from heap roots. The references are printed in a tree-like structure to 
standard error stream. 

#### Example - debug references

```java
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

TestToolkit.instance.markObject(array);
TestToolkit.instance.debugReferences(0, 3);
```

produces output similar to:

```
Object (10016 bytes) of [B, referenced from:
 ├── Lnet/enigma/test/toolkit/HeapTraversalSummary;, stack local: thread: 1/2, Lnet/enigma/test/toolkit/TestToolkitExamples;.debugReferencesExample(TestToolkitExamples.java:112)
 ├── Lnet/enigma/test/toolkit/TestToolkitExamples$1;, field: val$array  (TestToolkitExamples.java:98)
 │   ├── Lnet/enigma/test/toolkit/HeapTraversalSummary;, thread
 │   ├── Lnet/enigma/test/toolkit/HeapTraversalSummary;, stack local: thread: 10/1, Lnet/enigma/test/toolkit/TestToolkitExamples$1;.run(TestToolkitExamples.java:104)
 │   ├── Lnet/enigma/test/toolkit/HeapTraversalSummary;, stack local: thread: 1/2, Lnet/enigma/test/toolkit/TestToolkitExamples;.debugReferencesExample(TestToolkitExamples.java:112)
 │   ├── [Ljava/lang/Thread;, array element: [1]
 │   │   ├── Ljava/lang/ThreadGroup;, field: threads 
 ├── Ljava/util/Collections$SingletonList;, field: element  (Collections.java:4803)
 │   ├── Ljava/util/concurrent/atomic/AtomicReference;, field: value 
 │   │   ├── Lnet/enigma/test/toolkit/HeapTraversalSummary;, stack local: thread: 1/2, Lnet/enigma/test/toolkit/TestToolkitExamples;.debugReferencesExample(TestToolkitExamples.java:112)
```

The two parameters of this method can be used to limit the amount of data which is printed. If there are many marked
objects, we may consider printing references to only those objects whose size is greater than some threshold.
