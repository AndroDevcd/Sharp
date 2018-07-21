# Roadmap 

#### Version 0.3.0
- Add built in functions 
    - List processing
    - Reflection
- Add generics option Generic<?> to have the type be forced to extend object
    - The class with ? should be processed at compile time to assign objects that follow this rule alike
- Allow for importing of star (*) 
    - import std.*; imports all of the standard library
- Have automatic lookup for enums
- Allow for expression in first argument of for loop
- Add asm volatile() argument to prevent optimzer 
from changing the source code
- Limit naming of declirations such as fields named as methods or classes
- Figure out weird bug with Unixfile system processing char when normalizing file path