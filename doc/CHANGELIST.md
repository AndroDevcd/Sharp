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
    
##### Version 0.2.9 (September 1, 2020)
    # Bug Fixes
        * Several seg faults fixed
        * Function pointers are no longer compatible with object types
        * Huge ammount of compiler code generation bugs have been fixed 
        * Several code optimizer bugs have been fixed in regards to if statements
        * Fixed bug related to the `lock` statement not properly releasing control
        * Fixed massive bug with the code obfuscation system crashing the runtime system

    # QoL Improvements
        * Significantly improved Garbage collector ~2x gain in performance
        * Improved task scheduling api for faster and more efficent scheduling
        * Increased stability in the multithreadding system
        * Several internal memory leaks fixed
        * Maximum executable class size limit increased from 262k - 16.7m
        * Exe file updated to version 5 (because of instruction resizing)
        
    # New Additions
        * 32bit operating system support offically (for linux) added
        * 'is' keyword added for dynamic runtime type checking
        * wait & notify system added for inter-thread communication
        * New Sharp native bridge added to allow for interfacing with c++

    # Known Issues
        * JIT is not currently supported on any platform
        * The Profiler is not supported
        * Dictionaries are not fully supported by the lang

    runtime version: 2.6.0
    compiler version: 0.2.9

    Backwards compatibility support tested Version: (n/a)
    
##### Version 0.2.805 (June 19, 2020)
    # Bug Fixes
        * Major stability improvements
        * Fixed several null pointer exceptions with file sys stl
        * Fixed major bugs in the compiler

    # QoL Improvements
        * Compiler v2 development has begun. This compiler comes with several bug fixes as well as several other updates and improvementas listed below.
            * New compiler comes with:
                ** Vastly improved code optimization and generation system
                ** Foundation for 32 & 64 bit support added
                ** Significantly improved exception system
                ** Newly upgraded multithreadding system for better code sincronization
                ** Better compiler error messages
                ** Improved compiler and VM performance
                ** Cleaner design for less internal bugs
                ** All code generated is now properly execution safe and should not cause seg faults
                ** New language design for a better seamless experiance 

    # New Additions
        * Implemented 'local' keyword in the language
        * Made several changes to the syntax
        * Completly new standard library re-written from scratch (new multithreadding task system)

    # Known Issues
        * JIT is not currently supported on any platform

    Backwards compatibility support tested Version: (n/a)
    
##### Version 0.2.587 (September 30, 2019)
    # Bug Fixes
        * Fixed several null checks incorrectly being applied to objects and could have lead to a seg fault
        * Fixed major control flow statement bug when setting default values on class variables causing seg faults
        * Fixed seg fault on out of memory exception event
        * Fixed bug in JIT when creating an array of classes

    # QoL Improvements
        * Improved the speed and efficency of casting via class address lookups
        * Further improved memory footprint/allocation performance of objects in Sharp. Objecects are now fully optimized (48 bytes -> 40 bytes)
        * Updated stl file system with an easier to use interface and better performance

    # New Additions
        *

    # Known Issues
        *
        
    # Comments
        * This update serves as a hotfix/general update that fixes some crucial bugs in the language as well as some small improvments 
        * As a result of the current state of the language the speed profiler will not be added to this update as it is not supported by the JIT system as of now

    Backwards compatibility support tested Version: (0.2.422+)

##### Version 0.2.581 (September 25, 2019)
    # Bug Fixes
        * Fixed templates inside of classes compiler bug
        * Fixed new expression not catching edge case where there is no cunstructor provided i.e. House h = new House;
        * Function pointers were not allowed to be created at global scope or local scope
        * `thread_local` will now be allowed on function pointers
        * Function pointers should now properly work on generic classes
        * Fixed several errors in the Sharp compiler assembler for writing inline assembly
        * Fixed compiler bug preventing implementation of delegate function through multiple interfaces
        * Fixed bug in Sharp standard library file system that prevented creation of directory when the new dir was not already in-place

    # QoL Improvements
        * Updated the Assembler to allow for high level assembly programming with the newly added instructions
        * Added the JIT back to the language with significant performance boots (64bit windows is ony supported as of now)
        * Memory footprint/allocation performance of objects in Sharp were optimized (72 bytes -> 48 bytes)
        * Mutex locks are now released on objects when they are destroyed by the GC

    # New Additions
        * Multiple compiler options changes
            * Added compiler option to specifically disable certain warnings individually
            * Moved compiler warning options to a seperate function (you now have to use [--hw] to see all warning options)
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
        * Currently the Interpreter and JIT system dont like each other so all code on first boot of every program will be compiled down to machine code
            - As a result expect slightly longer program runtimes. However if you wish use `Time` class in `std.kernel` to test the time execution of your code if you desire
            - Linux is unaffected as the JIT currently only runs on windows
                
    # Comments
        * As a result of the current state of the language the speed profiler will not be added to this update as it is not supported by the JIT system as of now
                    

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