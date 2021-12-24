# histProfiler

Very lightweight performance profiler based on inserting timestamps sampling in to the tested code.
{
  Begin(“part1 of my code”)
  …
  End(“part1 of my code”)
}

It accumulates these samples in histograms where X-axis represent measured time in specified units and Y-axis represent number of times it took the code to run.

The amount of memory  used by the profiler is constant and the CPU usage is minimal – a lookup in a std::unordered_map and some arithmetics. So it can run for a very long time.

The accumulated data can be dumped to std::out or a text file. 

To visualize the data, 1) copy&paste it into excel,  2) select all. 2) insert your favorite chart. See example in the docs/charts.png.

profiler.h/cpp contain the interface and all the implementation.
main.cpp and tests/test_*.cpp – contain several examples.
