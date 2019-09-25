# Change List
This file contains all the various changes specifically pertaining to the syntax made to sharp. 
For each stable build of sharp pushed to master there will be a corresponding version; However, this file is
not guaranteed to change after every release as language syntax changes are an arduous task.


##### Version 0.2.581 (September 25, 2019)

   ###### Inline initialization
```javascript
    mod app;

    import std;
    
    class House {
        var windows;
        var bathrooms = 2;
        string address;
        var bedrooms;
        string securityProvider;
        
        // old way of initialization via constructor no longer need 
        public House(var windows, var baths, string loc, var beds, string prov)
        {
            self->windows=windows;
            bathrooms=baths;
            address=loc;
            bedrooms=beds;
            securityProvider=prov;
        }
    }
    
    def oldWay() {
        House house = new House(6, 1.5, "1805 Hennepin Ave N, Glencoe, MN 55336", 3, "ADT");
        ...
    }
    
    def newWay() {
        // initilization option 1
        House house = new House 
            {
                6, 
                1.5,
                 "1805 Hennepin Ave N, Glencoe, MN 55336", 
                 3, 
                 "ADT"
            };
        
        
        // initilization option 2
        house = new House 
            {
                windows = 6,
                address = "1805 Hennepin Ave N, Glencoe, MN 55336",
                bedrooms = 3,
                securityProvider = "ADT"
            }; // notice that bathrooms was not added in the initialization
    }
    
    def main(object[] args) {
        oldWay();
        newWay();
    }
```

The above code displays multiple ways on how to initialize a class. The old way required the user to create a constructor and initialize the fields inside the constructor
and then on creation of the object call said constructor to setup the fields. The new way however no longer requires the creation of a constructor and instead allows for a much faster 
initialization of a class in-terms of write time and runtime without having to worry about the overhead of a constructor.

With this new feature there are several things to note as listed below:

   * This new way of initialization does not completely invalidate the need for constructor in the language as there are still a few crucial use cases for requiring a constructor such as 
     calling functions and or base class methods during initialization;
   * Inline class initialization automatically sets default values to fields where they are otherwise not listed such as in example option #2 in `newway()` 
   * Base fields can also be initialized as well as any base default values are automatically initialized if not added in lower level classes.
   * Inline initializations do not include a function call, therefore under a big load where millions of objects are being created there is a noticeable performance difference between 
     the new way and old way to initialize a class
   * There are 2 main ways to inline initialize a class
        - Automatic mode where each value you pass must match the field type in the same order the fields are layed out in the class from top to bottom.
          However if you have default values that you want to respect then manual should be your preferred choice.
        - Manual mode where you must specifically describe which fields are being initialized and set to what values.
   * This new style of class initialization is ony strongly recommended for simple objects as using this for more complex types such as `std#Thread` can cause 
     exceptions and unexpected behavior on your code.
     
     
   ###### Anonymous Functions
```javascript
    mod app;

    import std;
    
    // old way
    def foo(object[] args) : var {
        println("I'm a thread!");
        return 0;
    }
    
    def foo2() {
        println("I am foo2!");
    }
    
    def main(object[] args) {
        Thread thread = new Thread("foo", foo);
        thread.start(null);
        
        // new way
        thread = new Thread("foo" def ?(object[] args) : var {
            println("I'm an anonymous thread!");
            return 0;
        });
        thread.start(null);
        
        // you can still code it this way to be more verbose
        fn ptr(Object[]) = foo; // old way
        
        // inferred function pointers
        fn ptr2 = foo; // new way option 1 has signature `def (object[]){}`
        // ptr2 = foo2; // not allowed as the compiler has already inferred the type of function pointer in `ptr2`
        fn ptr3 = def ?(object[] args) { println("I'm anonymous!"); }; // new way option 2
        
        ptr(null);
        ptr2(null);
        ptr3(null);
    }
```

The above code displays a new way to create quick functions that may perform trivial tasks and would not want to exposed to the outside world
for one reason or another. The design of anonymous functions were developed in mind with a low friction syntax that allows you to as you code evolves 
and would like to transfer a anonymous function to a globally accessed one and vise versa, to not be an inconvenient task like it is in other languages such as c++.

This new feature also brings with it an easier way to create and initialize function pointers within the language;
Instead of constantly having to write which type of function pointer you are trying to create, you can simply infer
the type of functions you are expected to set to the pointer by allowing the compiler to figure out the type and assign it to the pointer.
Once the type is inferred it cannot be changed and must be assigned only by functions that match the inferred signature.
