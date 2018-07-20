# Change List
This file contains all the various changes made to sharp. 
For each stable build of sharp pushed to master there will be a corresponding version.

##### Version 0.2.372 (July 19, 2018)
* Added dynamic threshold updating in GC
* More bug fixes and improvments
* Added sharp speed profiler code

##### Version 0.2.343 (July 13, 2018)

- All objects now inherit Std#Object class to allow for baseline support
for basic operators used in std#Dictionary<>
* Major performance improvements to overall language
    - Anti-lag system implemented to GC to minimize pausing of threads during allocation
    - Added various optimizations to the garbage collector that prevent the thread from slowing down your program by a factor of 10+ 
    - Calling function using preallocated list instead of calling malloc() and free()
* Major bug fixes to system causing silent (SEGV) Seg faults
* Compiler in Sharp began
* Global variables and methods added
* Threading support added