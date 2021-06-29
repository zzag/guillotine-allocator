# guillotine allocator

This is a simple to use texture atlas allocator library.


## Example usage

```cpp
KGuillotineAllocator::Allocator allocator(size);
auto allocation1 = allocator.allocate(QSize(100, 50));
auto allocation2 = allocator.allocate(QSize(50, 60));

allocator.deallocate(allocation1.id);
allocator.deallocate(allocation2.id);
```
