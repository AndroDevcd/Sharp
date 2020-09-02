
  
<a name="logo"/>    
<div align="center">    
<a href="https://github.com/" target="_blank">    
<img src="https://github.com/AndroDevcd/Sharp/blob/master/doc/logo.png?raw=true" alt="Sharp Logo" width="210" height="142"></img>    
</a>    
</div>    
    
# The Sharp Programming Language 

Sharp is an open source a modern object-oriented programming language that aims to bridge the gap between modern expressive programming paradigms and strictly typed rigid languages like Java and C#. My goal is to provide the usefulness of an object oriented programming language, while holding the simplicity of an expressive programming language without having to sacrifice performance.     
    
Sharp is elegantly simple.    
```javascript    
mod app;    
    
def main() {    
    print("hello, world!");    
}    
```    
    
The syntax in Sharp is simple, easy to read, and beautiful allowing you to focus more on what the code
is doing rather that the syntax 
  
```javascript  
mod app;  
 
def compute() {
    // TODO...
}

def main() {  
    
   for < 10:
    compute();
}  
```  

The language library comes standard with a comprehensive lightweight multi-threading library `task`
that makes handling work in the background simple to put together but looks good as well.
  
```javascript  
mod app; 

// import the threadding library
import std.io; 
import std.io.task; 
  
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
       { obs : object ->
           ((observable<int>)obs).post(lots_of_math());
       }
   );
    
   sleep(700);
   println("you're answer is ${answer.response}"); // block main thread until answer is posted
}  
```  

Sharp supports several convient features that improve the overall experience with programming in the language.

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
fn main_ptr := main;

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
* an advanced version of extention functions
*/
mutate class list {
    initialSize : var = 100;
    
    List(data : T[]) {
        // ...
    }   
}
// and more !!

def main() {  
    
   for < 10:
    compute();
}  
```

Sharp has a multi-language inspired syntax taking the beauty of languages such as JavaScript, python, Java, Go, and C#. While expressing it in a more concise way. sharp supports anonymous functions that make it easy to abstract away blocks of code without the need to declare a bunch of on-time use functions in your code.  
  
```javascript
mod app;  
  
def main() {  
   var num, x;  
   for(var i = 0; i < 100; i++) {  
      num = num*Math.sqrt(num)/2;
      x++;  
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

The above code is a simple illustration of how Sharp's low friction syntax allows you to evolve your code overtime as your requirements change
from local code, to anonymous function, to a globally accessed static function and more.
Although being an object oriented language, sharp unlike other languages don't force you to explicitly follow that model of programming.
It's understood that the old languages of today such as java, c# and more require alot of boilerplate code to be written thus taking away developer time
. Below is just one simple way sharp can remove the menotinous task usually encountered in you day-to-day development flow.

```javascript
mod app;  

// here we create a data class o hold inforation about a vehicle
class Vehicle {
    licensePlate := ""; // we dont need to specify types if we dont want to the compiler figures it out for us
    wheels := 4; // default value of 4 wheels
}

// here we abstract the vehicle class into a truck  
class Truck base Vehicle {
    make := "", model = "";
    color : Color;
    modelYear := 1997;
}

def main() {  
   car := new Truck {
            make = "Ford",
            model = "F-150",
            color = Color.SPACE_GREY,
            modelYear = 2024,
            licensePlate = "ZSY 7CH9"
            // wheels = 4 // wheels is already set to default value of 4 so we dont need this statement
        }; // Object Truck is created inline and assigned the values instead of calling a constructor
    
    // alternativley you can create classes like this as well
    veh := new Vehicle {"ZSY 7CH9"}; // creates a vehicle with 4 wheels and a license plate number
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

