### Testing Sharp

You can build a sharp application by using the Sharpc.exe:

``
Sharpc.exe <std-libs> <your-files>
``

Sharp currently does not have a way to search and compile the std library files so any 
std lib file that is used must be included in the compiler

Your code can be compiled Optimized and unoptimized so be sure to use that!

### Viewing the output
You can view the output of your source code by using the option ``-objdmp`` to see what was generated

To run your code simple type ``Sharp.exe <your-exe>``