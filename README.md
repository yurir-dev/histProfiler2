# histProfiler

Very lightweight performance profiler based on inserting timestamps sampling in to the tested code.
{
  Begin(“part1 of my code”)
  …
  End(“part1 of my code”)
}

It accumulates these samples in histograms, the amount of memory used by the profiler is constant and the CPU usage is minimal – a lookup in a std::unordered_map and some arithmetics.
The accumulated data can be dumped to std::out or a text file. 

To visualize the data, 1) copy&paste it into excel,  2) select all. 2) insert your favorite chart. See the charts.png.

profiler.h/cpp contain the interface and all the implementation.
main.cpp – contains several test examples.
