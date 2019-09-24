# Change List
This file contains all the various changes made to sharp. 
For each stable build of sharp pushed to master there will be a corresponding version.

Backwards Compatibility:
For backwards compatibility support only the version posted below each release is the guaranteed supported version and above,
any other versions below that is considered deprecated and no longer supported. Backwards compatibility is only in reference
to the ability to run older code that has been compiled by the older framework and can be successfully ran on the new system
Any other compatibility issues/changes will be otherwise noted in the release notes specifically.

##### Version 0.2.422 (August 12, 2019)
    # Bug Fixes
        *

    # QoL Improvements
        *

    # New Additions
        *

    # Known Issues
        *

    Backwards compatibility support tested Version: ()

##### Version 0.2.422 (August 12, 2019)
    # Bug Fixes
        * Fixed templates inside of classes compiler bug
        * Fixed new expression not catching edge case where there is no cunstructor provided i.e. House h = new House;
        * Function pointers were not allowed to be created at global scope or local scope
        * `thread_local` will now be allowed on function pointers
        * Function pointers should now properly work on generic classes
        * Fixed several errors in the Sharp compiler assembler for writing inline assembly
        * Fixed compiler bug preventing implementation of delegate function through multiple interfaces

    # QoL Improvements
        * Updated the Assembler to allow for high level assembly programming with the newly added instructions
        * Added the JIT back to the language with significant performance boots (64bit windows is ony supported as of now)
        * Memory footprint/allocation performance of objects in Sharp were optimized (72 bytes -> 48 bytes)
        * Mutex locks are now released on objects when they are destroyed by the GC

    # New Additions
        * Multiple compiler options changes
            * Added compiler option to specifically disable certain warnings individually
            * Moved compiler warning options to a seperate function (you now have to use [-hw] to see all warning options)
        * Constructors are no longer needed as classes can now be initalized inline without the use of constructors using new initilization feature see (LanguageChanges.md) for more information 
        * New VM instructions
            * Added to x64 VM Instruction `dup` for stack duplication
            * Added to x64 VM Instruction `popobj_2` for direct stack object assignment
            * Added to x64 VM Instruction `swap` for stack swapping
        * Added anomyous functions to the language
        * Added new thread syncronization system to prevent threads from shutting down with active mutex locks causing deadlocks on threads

    # Known Issues
        * Returning/jumping out of scope of a lock(){} code section results in a deadlock on that object
        * The speed and memory profiler does not work with the JIT system as of now and will be added in a later update
        * Sub classes do not work in Generic classes
        * The new library class added to the standard library `HashMap` is extreemly slow and buggy 

    Backwards compatibility support tested Version: (0.2.422+)

##### Version 0.2.422 (August 12, 2019)
    # Bug Fixes
        * Added a check to prevent native variables from being assigned objects values i.e { object o =  new Object(); var i = o; }
        * Fixed holes and edge cases in Template classes and sub template classes
        * Standard Library Thread file was incorrectly starting and creating threads
        * Various other small fixes

    # QoL Improvements
        * Changed publishing procedure to a more detailed breakdown of the changes made for better insights on the language's progress
          as well as backwards compatibility support will now be implemented

    # New Additions
        * "thread_local" & "local" attributes added to language
        * JIT Compiler first pass added to significantly increase performance
            - x86/64 & ARM will be only supported for a while
        * Internal stack size can now be set on the runtime engine to allow of bigger or smaller stack space in "interpreted" mode

    # Known Issues
        * JIT compiler is very buggy therefore it is disabled for now
        * Templates inside of classes currently do not work

    # Comments
        * I will now pick back up the project to continue development and make it better than the language has ever been

    Backwards compatibility support tested Version: (0.2.422+)

##### Version 0.2.372 (August 6, 2018)
* Added dynamic threshold updating in GC
* More bug fixes and improvements
* Added sharp speed profiler
* Began support for gui processing

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