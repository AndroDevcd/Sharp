# Roadmap 

#### Version 0.3.0
- Add built in functions 
    - List processing
    - Reflection
- Add generics option Generic<?> to have the type be forced to extend object
    - The class with ? should be processed at compile time to assign objects that follow this rule alike
    - also allow for class specification i.e `generic<t ? my_class>` 
    - name it a generic qualifier since the type will not be known
    - casts to generic qualifier will not work as the built in class is not of that type
    - if the user tried generic qualifier cast at high level we need to process it otherwise as mentioned above will happen
    - do not let user cast from object to generic qualifier because a specific type must be known at time of cssting
- [X] Allow for importing of star (*) 
    - import std.*; imports all of the standard library
- Add functionality for AOT Compiler for functions and Inject them directly in the exe
- [X] Have automatic lookup for enums
- [X] Allow for expression in first argument of for loop
- Add asm volatile() argument to allow optimzer to modify the injected code
- standard asm() code will not be modified by compiler unlexss there needs to be a
  address readjustment due to code being removed above the asm() block
from changing the source code
- Limit naming of declirations such as fields named as methods or classes
- Figure out weird bug with Unixfile system processing char when normalizing file path
- [X] Add support for inline anaonymous functions
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
- [X] modify syntax to allow for a much more user friendly and easy design for type creation and
variable assignments

```javascript

class House {
    rooms : int;
    address := "";
    hasBasement : bool;
    value : int = 0; // variables are now written as follows <identifier> `:` [<uType>] [`=` <value>] `;`
}

def foo(num : var) {
    x := 9; // value is inferred type set to var
    y := num; // same concept as above
    z : int = num; 
    
    // we now change the inferred fn pointer assign symbol from to `:=` from `=` 
    fn calc := def ?(x : var, y : var) : var {
        c = 10; // could not resolve c out of scope
        return Math.sqrt(x)+y;
    };
    
    // regular typed fn pointer assignments stay the same
    fn calc2(var) = def ?(z : var) { };
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
- [X] (Fuckin donne!!!!!) fix bug that deadlocks lock statements due to return and goto
```javascript
lock(self) {
    return 1; // lock is not released
}
```

- [X] support function pointers as global variables
- fix exception system problems
    * Extreemly slow
    * finally block execute has problems with returning from the function in question

- [X] give attribute local a meaning to where it forces fields and methods to only be used exclusivley
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

- [X] add new options for bootstrap compiler object dumping
    - "-obj" to dump object file (file will be outputted with no debugging info and mangled
    - "-obj:d" for demangling code to show names
    - "-obj:x" for displaying the compiled high level assembly syntax i.e. "mov fp+2, %ebx"
    - "-obj:i" for debugging info i.e "line numbers, file locations, and address locations i.e [0x34a]
    - "--obj"  for file object dump options
    - Allow for the user to chain together options i.e "-obj:ix"

- Create new syntax for Assembler in SHarp compiler to be more like a modern assembly language
- Create a "CodeAst" Block structure when compiling to allow for insightful data as well as code optimization
- add support for file resource pointers
-- This allows for the user to pull down data in a file in blocks instead of all at once
---- This will however pose a risk for threads leaving file resources open if there is a deadlock or he thread gets shut down
---- Implement same code to close all file handles on thread shutdown
---- Below is a proposed solution for adding this feature

```javascript
class Thread {
    ...
    List<File*> fileHandles;
};

void VirtualMachine::sysInterrupt(int64_t interrupt) {
    ...
    case INC_FILE_POINTER:
        // do your work here
        break; // we may have many sysInterrupts() to handle the functionality
}

```

- Convert programming language to support 32 bit systems
-- Max integer value is "2,147,483,632‬"
--- Limit max class size, function, and field size possible

- add  with and apply
with:
```javascript

class car {
    vin : string;
    wheels : int;
}

def main() {
    audi := new car { "ABCD", 4 };
    
    // both calls are null safe
    // with allows code to be in a block while using instance in with()
    with(audi) as c { // default is `it`
        c.vin = "ZYXC";
        print("car " + c);
    } else {
        // if its null then do this
    }   
    
    // have hem both return a value thats automatically returned on exit of scope
    // mainly used for setting only the values in the class in apply()
    apply(new car()) {
        vin = "";
        wheels = 8;
    } else {
        // if null do this
    }
}
```

- [X] add extention functions
- [X] add keyword "ext" use case "ext class BaseClass { ... }"
     - extention classes cant be instantiated
- [X] make interfaces extension classes by default!
- [X] add keyword "mutate" use case "mutate string : parcellable { ... }" you are efectivley adding new functality to the class inline
- [X] add keyword "stable" use case "stable class string { ... '"

- add reflection system

- add use block
    - use abstracts out the scope of what you are using to only use the vars provided in the use block
    - an anonymous funcion is generated and the value returned
    - does not worry about nor handles closures
    - use cannot be assigned to function pointers as it returns the value not 
        the ptr to the function.

```javascript

def main() {
    x : var = 1, y = 2;
    sum := use x, y -> (var) {
        return x + y;
    }
}

```
    
- [X] add `alias` keyword to refrence anything utype in the language
    - syntax is `alias <utype> as <identifier>`
    - considering it's a utype we just process it as such and return the utype including the code that is in it
    - scope aliasis by current scope, as well as use access for aliases to be able to use them
    
- add closures for function calls to access local stack vars t hiher scope
    - local vars must be an object primitives cannot be accessed
    - create a class when building closures i.e `_str_closure#1234` and add in all the fields that the closure contains to be accessed later
    - only store local vars in closure nothing else
    - set the code to init the closure class just before when the lambda function was created
    
    
- Add generic functions
-- Add support for generic extension functions as well
```javascript
def<T> templateFun() : T {
    // do stuff
}

def<T> T.templateFun() : T {
    // do stuff
}

def main() {
    templateFun<var>();
    str := "";
    str.templateFun<string>();
}
```

- add dictionaries
```javascript

response_messages := 
        { as <int, string>
            200: "OK",
            403: "Access forbidden",
            404: "File not found",
            500: "Internal server error"
        };
```

- add base generics
    - class generic<t base some_class> { }
    - add support for high level casting and assignment of the objects
    
    
- [X] add when statement
 
 ```javascript

def main() {
    myStr := "hello";
    when (myStr) {
        "hi" -> {
        
        }    
        "hello" -> {
        
        }
        else {
        
        }
    }   

    when {
        myStr == "hi" -> {
        
        }
        myStr == "hello" -> {
            
        }
        else {
        
        }
    }
}
```

- [X] add support for operator []

```javascript

class foo {
    data : var[];
    
    init {
        data = { 1, 2, 3 };
    }    

    def operator[](index : _int64) : var {
        return data[index];
    }
}

def main() {
    _foo = new foo();
    val := _foo[0];
}
```

- add string compression on output files
--- Format oes as fllows

=== Compression Cunk Begin ===
12:Hello, World,2:hi,4:hello...
=== Compression Chuck End ===

to bring down file exe size

- [X] add builtin obfuscation of code to the lang
-- Obfuscate classes, fields, and method names in code
--- It will be as follows
```javascript

class car {
    vin : string;
    hp : _int8;
    color : vehicle_color;
    wheels: _int8;

    def foo(i : int) {}
}

enum vehicle_color {
    red,
    blue,
    silver,
    black,
    pink
}

// explicitly obvuscate these fields & enum
obfuscate {
    car.vin,
    car.hp,
    vehicle_color
}

// explicitly declare to not obvuscate these fields
obfuscate -keep {
    car.wheels,
    car.foo(int)
}

// some options have different formatting
obfuscate -keeppackagenames {
    "std.io",
    "std.math"
}

```

- for each field, an alias is created and the actual name is stored in the class as a random name
- add option -obf to compiler to obfuscate everything 
- dont perform code obvuscation until code genertion step in the compiler

- [X] add support for using function pointers as utypes

```javascript

ptr : (int, string)(nil) = some_fun;

def foo() : ()(char) {

}

```

- add object serialization

```javascript
mod main;

class worker {
    salary : int;
    title: string;
    name : string;
}

def main() {
    john := new worker { 50_000, "Developer", "John" }
    serialized_worker : string = serialize(john);
    john = deserialize(serialized_worker) as worker;
}

```

- add `is` keyword to check for instances

```javascript
mod main;

def main() {
    message : object  = "Hi";
    if(message is string.class) {
        
    }
}

```

- add new constructor decl style

```javascript
mod main;

class list<t>(
 private maxSize: var,
 private data: t[]
) {
    // ..
}

// here is an example of calling a base class constructor with the new syntax
class dog(private size : int) base animal(size) {
 // ..
}


def main() {
    _list = new list<int>(10, { 1, 2, 3 });
}
```

the class `list` now has a field called maxSize declared because parameters with access specifiers
are declared member variables and the ones with out are considered parameters

- [X] implement static inits for classes

```javascript
mod main;

class Player {
    static init {
        //... code for when the singleton is created
    }

}
```

- add a `$internal_func_static_init()` inside of the class if there is a static field, function, or init block found by the compiler
- just like regular init static init will be compiled after fields
- move code for static variable initialization to that function
- do not allow static init to be called at global scope
- for each class call the respective function in `srt_init()` for setting up each class & pass the instance of MOVG instruction to the function because technically it will be an instance function

