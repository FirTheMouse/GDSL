# Introduction

GDSL helps you build programming languages.
The project didn't start with this goal in mind, it was originally meant to be just a domain specific language for a GUI library, but my projects have a pesky habit of becoming infrastructure. 
The thesis underlying it today is that compilers can be simple without forcing constraints onto the user. While other frameworks like Racket or MLIR have opinions about what you build, GDSL is content to run it.
It also has no opinions about how your disk space should be spent.

I wasn't seeking to address any particular pain point, I build in my own ecosystem and only realized later that such a tool would be of use to others, thus why I've dressed up neatly in a repo and written this README.

# Installation

Clone the repo from GitHub.
Copy `core/GDSL.hpp` into your project and include it.
That's it.

# Getting Started

In the future I may make a tutorial series showing how to make a language from start to finish with GDSL, though at this stage there's still a lot of smoothing and polishing to be done before I can spend the time to write/record such a tutorial. 
For now, I'll equip those of you who are more technically adventurous with the core insights.

TAST DRE MIX, these are the three parts that compose a GDSL. 
TAST tokenizes and parses to make a structure other stages will consume. This is where you go from 'big string' to 'compiled structure'. I prefer to front-load a lot of resolution into my T stage, but by no means do you have to do the same. The T stage can just be for sucking up qualifiers and merging children. 

DRE resolves and annotates the TAST structure so execution can run smoothly. This is where sizes can be calculated, call sites formulated, and optimization passes run. The e stage in particular is home to potential, it runs backwards and its designed for all your error handling and optimization passes (though you can always emit errors earlier, later, or in my case, not at all).

MIX actually executes your code. I haven't invested effort into building a proper MIX for either of the demo modules yet (it's only been about 6 weeks as of writing this), so I just use the x stage and run as an interpreted tree. If you want to emit to LLVM (which is a MIX in of itself + an e stage) you can write the handlers for that. If you want to emit to native, use the m stage to model your memory layout, I stage to emit instructions, x stage to run the stream. If you want a JIT, m stage to arrange memory, I stage to trace and produce the operations, x stage to run them in a stream.

The best examples of how to make a language are just looking at what I've made so far, with GDSL-LISP and GDSL-C. Feel free to scan through, the code is quite short and hopefully fairly readable. These are only demo modules, not full implementations.


# What If I'm not Building

If you're only here to take a look at the concepts and claim, then you just want to run the files directly, not build your own GDSL. 
Head over to the main.cpp, and comment/uncomment the include module you want (I'll be making this smoother in the future, as I understand not everyone is used to commenting include paths to switch implementations). Then just run the command below to compile:

g++ -std=c++17 -O2 -I. main.cpp -o gdsl

And then run the test for the module:

Run ctest (with GDSL-C):
./gdsl ./modules/tests/ctest.gld

Run lisptest (with GDSL-LISP):
./gdsl ./modules/tests/lisptest.gld

You can turn PRINT_ALL on/off in the kernel to see more or less, and you can add your own logging wherever you'd like.

# Logging

GDSL uses my Span system, it's quite simple, just three commands:

newline: this takes a string for its label, and will start a new line. Lines are what your logs will get organized over, they also time themselves. Each time you see a big indent in the logs, that's a line starting.

endline: very important, you call this when you want to end a line, it will then pop it from the stack. This will also return the time from the line, and you can format this into a string with ftime().

log: this takes any args and will create a message to place under the current line, so you can do: log("hello"," world",32); and that'll work just fine.

At the end, run span->print_all(); and there's your log.
In the future I'll add severity levels and such, but for now it just a way to organize your prints and store things.

# Utility Modules

Because GDSL modules are just C++ headers, you can make, download, and include helpers. I made two, Q-AST and Q-Tokenizer, as examples showing how these could look.
If you write a cool MIX system, or some useful tools for yourself, please share it with others! A central place to host such modules is on the roadmap.