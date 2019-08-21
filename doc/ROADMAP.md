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
- Allow for expression in first argument of for loop --done
- Add asm volatile() argument to allow optimzer to modify the injected code
- standard asm() code will not be modified by compiler unlexss there needs to be a
  address readjustment due to code being removed above the asm() block
from changing the source code
- Limit naming of declirations such as fields named as methods or classes
- Figure out weird bug with Unixfile system processing char when normalizing file path
- Add support for inline anaonymous functions -- done
    - Syntax:
```javascript
def foo() {
    
    var x = 1, y = 3.14, c = 10;
    
    // calculate some value
    fn calc = def ?(var x, var y) : var {
        c = 10; // could not resolve c out of scope
        return Math.sqrt(x)+y;
    };
    calc(x,y);
}
```

- the concept of  the use statement is that the use directive
creates an inline anonymous function to be used directly with your 
code. The inline function has no notion of the surrounding scope
around it. I create the corresponding name $anonymous1 etc. in the class or
global scope and translate it to ``var result = $anonymous1(x, y);``

- Implement the use of faster code to write for loops
-- anonymous for loop
```javascript
    for < 10:
        print("single line");

    for >= 100: {
       print("multi-line");
    }

    for 100: { // operator < is assumed to be the poerator
       print("multi-line");
    }
```
- fix bug that deadlocks lock statements due to return and goto
```javascript
lock(self) {
    return 1; // lock is not released
}
```

- support function pointers as global variables
- fix exception system problems
    * Extreemly slow
    * finally block execute has problems with returning from the function in question

- give attribute local a meaning to where it forces fields and methods to only be used exclusivley
  for the file it was created in. All other files will be considered out of scope
  
  ```
  in c++
  class Field {
    ...
    string scopeLimiter; // if this in not empty check it for  fields and methods of global scope only
  }
  
  in Sharp
  local def foo() {
    ...
  }
  
  def main() {
    foo(); // only main.sharp file has access to foo
  }
  ```

- Allow for type inference on fields that could possibly be of multiple types
```javascript
def foo(??name) {
    if(name is var) {
        name = 9;
    }
((SomeClass)name).work();

}
```