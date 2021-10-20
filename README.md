
  
<a name="logo"/>    
<div align="center">    
<a href="https://github.com/" target="_blank">    
<img src="https://github.com/AndroDevcd/Sharp/blob/master/doc/logo.png?raw=true" alt="Sharp Logo" width="210" height="142"></img>    
</a>    
</div>    
    
# The Sharp Programming Language 

Sharp is an open source a modern object-oriented programming language 
that was designed to improve the speed and efficiency when writing robust software.     
    
Sharp's syntax design is elegantly simple and easy to read. Allowing you to focus more on what the code
is doing rather than how to write the code.   
```javascript    
// Ths is a hello world program!
mod app;    
    
// lets say hi 3 times!
def main() {    
   for < 3:
      print("hello, world!");    
}    
```    

The standard library comes with a comprehensive lightweight co-routines library `task`
that makes handling work in the background simple to put together but looks good as well.
  
```javascript  
mod app; 

// import the threadding library
import (
    std.io,
    std.io.task
)
  
def heavy_computation() {
    // simulate heavy work being done
    sleep(3000);
}

def lots_of_math() : int {
    // simulate heavy work being done
    sleep(1000);
    return 6;
}

def main() {  
    // here we create a background task to do some work
   launch = { -> 
        heavy_computation();
   }

   // launch a task that you can get the answer at a later time
   answer := deferred_task<int>.for_result(
       with_timeout(1500),
       { obs : observable<int> ->
           obs.post(lots_of_math());
       }
   );
    
   sleep(700);
   println("you're answer is ${answer.response}"); // block main thread until answer is posted
}  
```  

Sharp is the first ever object-orientated to offer a built-in high performance
dependency injection system directly within the language.

```javascript
mod main;

class user_client {
    def create(name: string) {
        println("created $name!");
    }   
}

class user_api {
    private client: user_client;
        
    user_api(uc: user_client) {
        client = uc;
    }

    def create_user(name: string) {
        client.create(name);
    }
}

// inject value of user client into global variable
inject api : user_api;

def main() {
    // use the Api!
    api.create_user("james");
}

// create a component for the user api
component user_component {
    single { new user_api(get()) }
    factory { new user_client() }
}

``` 

The language also supports several other convenient features that improve 
the overall experience with programming in the language.

```javascript
mod app;
  
class list<T> {
    // inferred type system using `:=`
    size := 0;
    data : T[];
}

/*
* Extension functions
*/
def list.last() : T {
    return data[size-1];
}


/*
* Function pointers
*/
def main() { /* ... */ }
main_ptr := main;

/*
* inline function declarations
*/
count := 1;
def inc_count() := count++;

/*
* Type aliasing
*/
alias var as Dollar
money : Dollar = 1.52;

/*
* Anonymous arrays
*/
num_list := { 1, 2, 3 };

/*
* Typed Dictionaries
*/
response_messages := { as <int, string>
    200 : "200 error",
    500 : "500 Error",
    405 : "405 Error"    
};

/*
* Anonymous Dictionaries
*/
days := {
    0 : "Monday",
    1 : "Tuesday",
    2 : "Wednesday",
    3 : "Thursday",
    4 : "Friday",
    5 : "Saturday",
    6 : "Sunday"
};


/*
* Anonymous functions
*/
fn total_size := { data : object[] -> (var)
    total := 0;
    for i < data: {
        total += sizeof(data[i]);
    }
    
    return total;
};

/*
* Class mutations
*
* Class list now has the field initialSize
* and constructor, class mutation acts just like
* an advanced version of extension functions
*/
mutate class list {
    initialSize : var = 100;
    
    List(data : T[]) {
        // ...
    }   
}
// and so much more !!
```

Sharp has a multi-language inspired syntax taking the beauty of languages such 
as JavaScript, python, Java, lua, Go, and C#. While expressing it in a more concise way. 
The language supports anonymous functions that make it easy to abstract away blocks of code without the need to declare a bunch of on-time use functions in your code.  
  
```javascript
mod app;  
  
def main() {  
   var num;  
   for < 100: {  
      num = num*Math.sqrt(num)/2;
   }  
  
   fn someComputation = { num : var -> (nil)  
      // we are in a completly seperate code space  
      // this code space knows nothing of the x variable  
      // unless explicitly passed in  
  
      print("num = " + num);  
   };  

   someComputation(num);
}  
```  

## Getting Started

Thdocumentation below explains how to get you started with using sharp.

#### Downloading

To download Sharp go to ``releases`` and download the latest release of Sharp. Unpack the folder revealing the contents inside. Sharp is currently supporting 64 & 32 bit linux and windows operating systems, below covers how to install sharp on both systems:

#### Installing on Windows

Navigate to the folder ``{YOUR_DIR}\release-pkg\bin\x64\win``.  Create the folder ``C:/Program Files/Sharp/bin``.  Place the Sharp binaries in the bin folder. Add following folder to your PATH:

```
PATH = {OTHER_PATH_DIRS}; C:/Program Files/Sharp/bin/;
```

Windows Powershell is the preferred method to use sharp but you may use windows CMD as well.
Next cd into the directory of your sharp build to the folder lib and run the commands below using powershell

```
mkdir "C:/Program Files/Sharp/include/"
cd {SHARP_INSTALL_PATH}/lib/support/0.2.9/
cp -r * "C:/Program Files/Sharp/include/"
Sharpc test.sharp -o test -s -R
Sharp test
```

After running the commands above if you receive the final output as ``Hello, World!``` Then Sharp has been installed properly and your good to go!

#### Installing on linux

Navigate to the folder ``{YOUR_DIR}\release-pkg\``.  Installing sharp on linux is very simple simply run the command below:

```
chmod +x linux-install.bash
./linux-install.bash
```

After running the commands above if you receive the final output as ``Hello, World!``` Then Sharp has been installed properly and your good to go!

#### Programming Ideas
After you install Sharp if you are having trouble coming up with programming ideas then you can head over to the 'examples/' directory to see a list
of sample programs using various parts of the Sharp programming language.

