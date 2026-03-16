# Introduction

GDSL is a single C++ header that lets you build a programming language by writing handlers: small functions that define how your language tokenizes, parses, and executes. Two demo implementations are included: a C subset and a Lisp subset, both running on the same unmodified handler system to show what this looks like in practice.

# Installation

Clone the github repo and include 'core/GDSL.hpp' in your project. 
If you're just interested in running tests or inspecting the demo implementations, simply clone and run it from the main.cpp as described below.

# Commands

From the main.cpp the chosen demo module can be commented/uncommented.

Compile:

g++ -std=c++17 -O2 -I. main.cpp -o gdsl

This has only been tested on a Mac, so Windows or Linux systems may require a different command. 

Run ctest (for GDSL-C):

./gdsl ./modules/tests/ctest.gld

Run lisptest (for GDSL-LISP):

./gdsl ./modules/tests/lisptest.gld

You can turn PRINT_ALL on/off in the kernel to see more or less.

# Getting Started

If you're interested in building your own language with GDSL, or creating a module for it, the process can be split into 3 stages.

TAST: Tokenizer, a_stage, scoping, t_stage. This is where the structure of your language gets built.

DRE: Discovery, resolution, error. Where most of the complex logic lives operating on the finished structure.

MIX: Model, interpret, execute. Lowering the code for running on the chosen system.

Most languages and tools already focus on one stage, Python is a very TAST heavy language, Rust focuses on the DRE, and LLVM is a MIX. Most programmers when making a language attempt to do all three, though these days it's very common to offload the MIX to a system like LLVM or MLIR. 
You do not have to try to do everything, the demo modules I've created are partially to provide reference, but also bridges through creating a TAST. GDSL-C in particular contains the systems you'll need for things like scoping, Pratt parsing, identifier disambiguation, and qualifier attachment. 

If you're new to compilers, the demo modules are the best starting point, read through GDSL-C for a complete example of how the stages connect, if you feel lost, feel free to reach out to me personally.

# Example

A walkthrough calculator module is in progress, check back soon.


