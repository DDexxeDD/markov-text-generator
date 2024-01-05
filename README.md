# Markov Text Generator

Generate text using markov chains.

This was a little test project to play with markov chains. There is little to no error handling, and probably a bunch of unaccounted for edge cases.

Only basic ascii text is supported for input.


## Building

You will need [meson](https://mesonbuild.com/Getting-meson.html) to build this project.

**Build**


```
meson setup build
meson compile -C build
```

This will create the directory 'build'.
In the 'build' directory the executable will be called 'markov'.


## Running

```
./build/markov
```

The input text used is "books/defiant agents - andre norton.txt".
This is hardcoded so you need to run the binary from the top directory.


## License

The code in this repository is licensed under Creative Commons 0 (CC0).

Licenses for included libraries are in their respective folders in 'external'.
