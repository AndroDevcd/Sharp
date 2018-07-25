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
- Add support for inline anaonymous functions
    - Syntax:
```javascript
def foo() {
    
    var x = 1, y = 3.14, c = 10;
    
    // calculate some value
    var result = use x, y : var {
        c = 10; // could not resolve c out of scope
        return Math.sqrt(x)+y;
    }
}
```

- the concept of  the use statement is that the use directive
creates an inline anonymous function to be used directly with your 
code. The inline function has no notion of the surrounding scope
around it. I create the corresponding name $anonymous1 etc. in the class or
global scope and translate it to ``var result = $anonymous1(x, y);``

- Implement the use of faster code to write for loops
```javascript

```