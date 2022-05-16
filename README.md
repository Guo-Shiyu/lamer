# Lamer
  A header-only, high-precision latency measurement library, default nanosecond and up to cpu-clock-cycle,  with customizable dump interface.     
  + x86_64 Linux currently only.
  + C++ 17 required

# Quick Start
~~~ c++
#include "lamer.hpp"

using lamer::NanoMeasurer;

int main(int argc, char** argv)
{
    NanoMeasurer<> meur;

    meur.record("point a here!");
    std::cout << "hello world!" << std::endl;
    meur.record("point b!");

    meur.record();
    meur.record("point c hahah");
    
    meur.dump(std::cout);

    return 0;
}

// Example Output:
// 
// hello world!
// lame info (default dumper):
// precision: nanosec      sample: 4
// 
// 
// stamp(s.ns)             comment
// ------------------------------------
// 3.985534551             point a here !
// 3.985574928             point b!
// 3.985574989
// 3.985575019             point C hahah 
~~~

# API Doc 

## 0. Concept Overview 

## 1. Struct & Class

### Detector 
  Fn. Used to get timepoint.

### Piece 
  Struct. A record of a timepoint with comment and mark.

### Cache 
  Struct. A internal (stack) cached struct for pieces.

### Measurer   
  Struct. A warpped structure that supplys function interfaces.   

### Dumper
  Fn. Format data in cache to given output stream.  

  + Specialize default dumper template to have customed format.

## 2. Macro 
  define the followings 'Declare Macro' before include statement to have extra compile time check.   
  ( Better performance may be obtained. )   

  + STRICT_MEMORY_ALIGN 

## 3. Recommended Usuage
  + todo 

## 4. Advise 
  + nanosec, record cost 150ns / call (in my pc)
  + release compile, optmize, template specialize confilt
