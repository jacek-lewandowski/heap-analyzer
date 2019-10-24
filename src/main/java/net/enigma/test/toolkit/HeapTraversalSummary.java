package net.enigma.test.toolkit;

import java.util.Objects;
import java.util.StringJoiner;

public class HeapTraversalSummary {
    public final long totalSize;
    public final long totalCount;
    public final long markedSize;
    public final long markedCount;

    public HeapTraversalSummary(long totalSize, long totalCount, long markedSize, long markedCount) {
        this.totalSize = totalSize;
        this.totalCount = totalCount;
        this.markedSize = markedSize;
        this.markedCount = markedCount;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        HeapTraversalSummary that = (HeapTraversalSummary) o;
        return totalSize == that.totalSize &&
                totalCount == that.totalCount &&
                markedSize == that.markedSize &&
                markedCount == that.markedCount;
    }

    @Override
    public int hashCode() {
        return Objects.hash(totalSize, totalCount, markedSize, markedCount);
    }

    @Override
    public String toString() {
        return new StringJoiner(", ", HeapTraversalSummary.class.getSimpleName() + "[", "]")
                .add("totalSize=" + totalSize)
                .add("totalCount=" + totalCount)
                .add("markedSize=" + markedSize)
                .add("markedCount=" + markedCount)
                .toString();
    }

}
