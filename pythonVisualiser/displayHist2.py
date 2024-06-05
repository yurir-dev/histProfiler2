from IPython.display import clear_output
from matplotlib import pyplot as plt
import numpy as np
#%matplotlib inline

import time
import struct
from datetime import datetime
from matplotlib.animation import FuncAnimation


class Header:
    def __init__(self, numBuckets, numSamples, samplesPerBucket, minSample, maxSample, overflows, sum_, desc=''):
        self.numBuckets = numBuckets
        self.numSamples = numSamples
        self.samplesPerBucket = samplesPerBucket
        self.minSample = minSample
        self.maxSample = maxSample
        self.overflows = overflows
        self._sum = sum_
        self.description = desc

        self.mean = self._sum / self.numSamples if self.numSamples > 0 else 0
        self.mean = round(self.mean, 2)
        
        self.minSampleUnits = self.minSample / self.samplesPerBucket if self.samplesPerBucket > 0 else 0
        self.maxSampleUnits = self.maxSample / self.samplesPerBucket if self.samplesPerBucket > 0 else 0

    def __sub__(self, other):
        #self.numBuckets -= other.numBuckets
        self.numSamples -= other.numSamples
        #self.samplesPerBucket -= other.samplesPerBucket
        #self.minSample -= other.minSample
        #self.maxSample -= other.maxSample
        self.overflows -= other.overflows
        self._sum -= other._sum
        
        self.mean = self._sum / self.numSamples if self.numSamples > 0 else 0
        self.mean = round(self.mean, 2)
        
        #self.minSampleUnits = self.minSample / self.samplesPerBucket if self.samplesPerBucket > 0 else 0
        #self.maxSampleUnits = self.maxSample / self.samplesPerBucket if self.samplesPerBucket > 0 else 0

        return self

class HistVisualiser:
    def __init__(self, filename, color='blue', title='', figsize=(20, 5), reset=False):
        self.filename = filename
        self.in_stream = open(filename, "rb")
        self.headerFull = self.readHeader(True)
        
        self.timeUnits = f"{self.headerFull.samplesPerBucket} nanos per bucket"
        if self.headerFull.samplesPerBucket == 1:
            self.timeUnits = "nanoseconds"
        elif self.headerFull.samplesPerBucket == 1000:
            self.timeUnits = "microseconds"
        elif self.headerFull.samplesPerBucket == 1000000:
            self.timeUnits = "milliseconds"
        elif self.headerFull.samplesPerBucket == 1000000000:
            self.timeUnits = "seconds"
    
        dataLayout = "<"
        for _ in range(self.headerFull.numBuckets):
            dataLayout += 'Q ' 
        self.dataStruct = struct.Struct(dataLayout)
        
        self.color = color
        self.title = title
        self.figsize = figsize
        self.reset = reset
        self.resetData = None

        if self.reset:
            self.in_stream.seek(4096, 0)
            buffer = self.in_stream.read(self.dataStruct.size)
            self.resetData = dataStruct.unpack(buffer)

        self.tpStart = datetime.now()

    def readHeader(self, full):
        headerFullLayout = "<Q Q Q Q Q Q Q Q 128s"
        headerLayout     = "<Q Q Q Q Q Q Q Q"
        
        headerStruct = struct.Struct(headerFullLayout) if full else struct.Struct(headerLayout)
        
        self.in_stream.seek(0, 0)
        unpacked = headerStruct.unpack(self.in_stream.read(headerStruct.size))
        
        if full:
            return Header(numBuckets=unpacked[2], numSamples=unpacked[7], 
                          samplesPerBucket=unpacked[1], minSample=unpacked[4], 
                          maxSample=unpacked[3], overflows=unpacked[5], 
                          sum_=unpacked[6], desc=unpacked[8].decode('utf-8').partition('\0')[0])
        else:
            return Header(numBuckets=unpacked[2], numSamples=unpacked[7], 
                          samplesPerBucket=unpacked[1], minSample=unpacked[4], 
                          maxSample=unpacked[3], overflows=unpacked[5], 
                          sum_=unpacked[6])
        #magic = histHeader[0]
    
    def readData(self):
        self.in_stream.seek(4096, 0)
        buffer = self.in_stream.read(self.dataStruct.size)
        return self.dataStruct.unpack(buffer)
    
    def setup(self, ax, fig):
        ax.axis('auto')
        

    def plot(self, ax, fig):
        header = self.readHeader(full=False)
        if self.reset:
            header -= self.headerFull

        legend = f"{self.headerFull.description} - #samples: {header.numSamples}, min: {header.minSampleUnits}, max: {header.maxSampleUnits}, mean: {header.mean}, #overflows: {header.overflows}"
    
        data = self.readData()
        if self.reset:
            data = [x - y for x, y in zip(data, self.resetData)]
        
        legendTimeFileName = f"{datetime.now() - self.tpStart} : {legend}\n{self.filename}"

        ax.clear()

        ax.grid(True)
        ax.set_xlabel(self.timeUnits)
        ax.xaxis.set_label_coords(1.05, 0)
        ax.set_ylabel("#samples")
        
        #ax.legend(loc='upper right')
        #ax.set_title(titleWithTime, fontsize=10)
        plot, = ax.plot(data, color=self.color, label=legendTimeFileName)
        ax.legend(fontsize=10, loc='upper right')
   
class HistVisualiserLayout():
    def __init__(self, histVisualisers=[], figsize=(12, 7)):
        self.histVisualisers = histVisualisers
        self.figure, self.axis = plt.subplots(len(self.histVisualisers), 1, figsize=figsize)
        self.figsize = figsize
        #self.figure.tight_layout()

        for histVis, ax in zip(self.histVisualisers, self.axis):
            histVis.setup(ax, self.figure)

        plt.show()
        
    def display(self):
        #clear_output(wait=True)

        for histVis, ax in zip(self.histVisualisers, self.axis):
            histVis.plot(ax, self.figure)

        # drawing updated values
        self.figure.canvas.draw()

        # This will run the GUI event
        # loop until all UI events
        # currently waiting have been processed
        self.figure.canvas.flush_events()


if __name__ == "__main__":

    hists = [
        HistVisualiser(r"C:\work\profiler2\linuxBUild\shmFile_basicThreads_1.shm", color='blue'),
        HistVisualiser(r"C:\work\profiler2\linuxBUild\shmFile_basicThreads_2.shm", color='red'),
        HistVisualiser(r"C:\work\profiler2\linuxBUild\shmFile_basicThreads_3.shm", color='blue'),
        HistVisualiser(r"C:\work\profiler2\linuxBUild\shmFile_basicThreads_4.shm", color='red'),
       # HistVisualiser(r"C:\work\profiler2\linuxBUild\shmFile_basicThreads_2.shm", color='green')
    ]
    plt.ion()
    histVis = HistVisualiserLayout(hists)
    while True:
        histVis.display()

    #def update(frame):
    #    global histVis
    #    histVis.display()
#
    #anim = FuncAnimation(histVis.figure, update, frames = None)
#    plt.show()
