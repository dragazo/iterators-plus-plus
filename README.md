# Iterators++

Iterators++ is a library written in standard C++14 that adds all sorts of wacky and useful iterator types that do fun and convenient things.

I always thought it was kinda weird that C++, despite having iterators since always, never really did anything interesting with them. I mean think about it: the only unusual iterators in the standard library are i/o iterators, which are actually really interesting as a design choice. But in C++ anything that has the proper interface can be used as an iterator, so why not make some cool things to exploit that?

Iterators++ is still highly-experimental - very few features are currently implemented and even then I still need to do more thorough testing and development work on them. If you do use this library at this early stage, the interfaces should (mostly) stay the same, but I will likely change some type/function names around to shorten them at some point.