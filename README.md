
  
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
    
Considering that sharp is an Object oriented programming language it is immediately familiar to Java and C++ developers.  
  
```javascript  
mod app;  
  
def foo(object[] args) : var {  
   print("I ran a thread!");  
}  
  
def main() {  
   new Thread("foo", foo).start();  
}  
```  
  
Sharp's syntax is minimalistic simple and easy to use  
  
```javascript  
mod app;  
  
def main() {  
   for < 10:  
      println("im looping!");  
}  
```  

The above illustration creates an **anonymous** for loop that does the tracking or loop iteration for you  
  
Sharp has a multi-language inspired syntax taking the beauty of languages such as JavaScript, python, Java, Go, and C#. While expressing it in a more concise way. sharp supports anonymous functions that make it easy to abstract away blocks of code without the need to have thousands of external functions in your code.  
  
```javascript  
mod app;  
  
def main() {  
   var num, x;  
   for(var i = 0; i < 100; i++) {  
      num = num*Math.sqrt(num)/2;
      x++;  
   }  
  
   fn someComputation = use -> num: {  
      // we are in a completly seperate code space  
      // this code space knows nothing of the x variable  
      // unless explicitly passed in  
  
      print("num = " + num);  
   };  
}  
```  

Although being an object oriented language, sharp makes it very easy for using classes for simple operations
by effectivley elimminating the need to create constructors for initalizing any given class.

```javascript
mod app;  

class Vehicle {
    string licensePlate; // licensePlate, make, & model are all string types
    int wheels = 4; // wheels has a default value of 4
}
  
class Truck : Vehicle {
    string make, model;
    Color color;
    int modelYear = 1997; // modelYear has a default value of 1997
}

def main() {  
   Truck car = new Truck {
            make = "Ford",
            model = "F-150",
            color = Color.SPACE_GREY,
            modelYear = 2020,
            licensePlate = "ZSY 7CH9"
            // wheels = default // wheels is already set to default value of 4 so we dont need this statement
        }; // no need for constructor in Vehicle or Truck as we create this object inline and assign the values
    
        /**
        * The above code translates to      
        * Truck tmp = new Truck();          
        * tmp.make = ..;
        * tmp.model = ..;
        * tmp.color = ..;
        * tmp.modelYear = ..;
        * tmp.licensePlate = ..;
        * tmp.wheels = ..;
        *
        * car = tmp; tmp = null;
        *
        */
  
   fn draw = use -> car: {  
      // we want to draw the provided vehicle
     // to the screen
      
      drawObject(car);
   };  
}  
```
  
##### Some thoughts on language design  
Sharp does not force you to be object oriented if you don''t want to like other languages force you to be. Of course some things done such as creating threads require some state to be stored and persist through the lifetime of your application; However Sharp allows you to be either more expressive or object-oriented or both.

#### Cool Stuff
Check out the snake game I made In Sharp under the link: https://github.com/AndroDevcd/Sharp/blob/remastered/projects/snake/main.sharp

```
%%%%%%%%%%%%%%%%%%%%%%
%                    %
%                    %
%                    %
%                    %
%                    %
%                    %
%                    %
%                    %
%                    %
%     <ooo           %
%        o    ooo    %
%x       o    o      %
%        oooooo      %
%                    %
%                    %
%                    %
%                    %
%                    %
%                    %
%                    %
%%%%%%%%%%%%%%%%%%%%%%

score: 140

commands: up, down, left, right, Q: quit, p: pause
p
```
