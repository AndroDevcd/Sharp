
<a name="logo"/>  
<div align="center">  
<a href="https://github.com/" target="_blank">  
<img src="https://github.com/AndroDevcd/Sharp/blob/master/doc/logo.png?raw=true" alt="Sharp Logo" width="210" height="142"></img>  
</a>  
</div>  
  
# The Sharp Programming Language  
Sharp is an open source a modern object-oriented programming language that aims to bridge the gap between modern expressive programming paradigms   
and strictly typed rigid languages like Java and C#. My goal is to provide the usefulness of an object oriented programming language, while holding the simplicity of an expressive programming language   
without having to sacrifice performance.   
  
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

Sharp makes for loops easy by allowing the programmer to not have to care about creating iteration tracking variables unless absolutely necessary.

```javascript
mod app;

def main() {
	for -> 10:
		println("im looping!");
}
```
 the above illustration creates an **anonymous for loop** that does the tracking or loop iteration for you

Sharp has a multi-language inspired syntax taking the beauty of languages such as JavaScript, python, Java, and C#. While expressing it in a more concise way. sharp supports anonymous functions that make it easy to abstract away blocks of code without the need to have thousands of external functions in your code.

```javascript
mod app;

def main() {
	var num;
	for(var i = 0; i < 100; i++) {
		num = num*Math.sqrt(num)/2;
	}

	fn someComputation = use -> num: {
		// we sre in a completly seperate code space
		// this soce space know nothing of the num variable
		// unless explicitly passed in

		print("num = " + num);
	};
}
```

##### Some thoughts on language design
Sharp does not force you to be object oriented if you don''t want to like other languages force you to be. Of course some things done such as creating threads require some state to be stored and persist through the lifetime of your application but Sharp allows you to be either more expressive or object-oriented or both.