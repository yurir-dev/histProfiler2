from IPython.display import clear_output
from matplotlib import pyplot as plt
import numpy as np
#%matplotlib inline

import time
import pdb
import struct
from datetime import datetime
from matplotlib.animation import FuncAnimation

def nanosToTimeUnits(nanos):
    timeUnits = f"{nanos} nanos per bucket"
    if nanos == 1:
        timeUnits = "nanoseconds"
    elif nanos == 1000:
        timeUnits = "microseconds"
    elif nanos == 1000000:
        timeUnits = "milliseconds"
    elif nanos == 1000000000:
        timeUnits = "seconds"
    return timeUnits


class HeaderHist:
    def __init__(self, numBuckets, numSamples, minSample, maxSample, overflows, sum_, xAxisDesc='', desc=''):
        self.numBuckets = numBuckets
        self.numSamples = numSamples
        self.minSample = minSample
        self.maxSample = maxSample
        self.overflows = overflows
        self._sum = sum_

        #pdb.set_trace()
        self.description = desc
        self.xAxisDescription = xAxisDesc

        self.mean = self._sum / self.numSamples if self.numSamples > 0 else 0
        self.mean = round(self.mean, 2)

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

    def getXAxisName(self):
        return self.xAxisDescription
    
    def getNumBuckets(self):
        return self.numBuckets

    def description(self):
        return self.description
    
    def stats(self):
        return f"samples: {self.numSamples}, min: {self.numSamples}, max: {self.maxSample}, mean: {self.mean}, #overflows: {self.overflows}"
    
class HeaderTimeHist:
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

        self.timeUnits = nanosToTimeUnits(self.samplesPerBucket)

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

    def getXAxisName(self):
        return self.timeUnits
    
    def getNumBuckets(self):
        return self.numBuckets

    def description(self):
        return self.description
    
    def stats(self):
        return f"samples: {self.numSamples}, min: {self.minSampleUnits}, max: {self.maxSampleUnits}, mean: {self.mean}, #overflows: {self.overflows}"

class HeaderRateCounter:
    def __init__(self, numBuckets, nanosPerBucket, currentIndex, desc=''):
        self.numBuckets = numBuckets
        self.nanosPerBucket = nanosPerBucket
        self.currentIndex = currentIndex
        self.description = desc

        self.timeUnits = nanosToTimeUnits(self.nanosPerBucket)

    def __sub__(self, other):
        return self

    def getXAxisName(self):
        return self.timeUnits
    
    def getNumBuckets(self):
        return self.numBuckets

    def description(self):
        return self.description
    
    def stats(self):
        return f"currentIndex: {self.currentIndex}"
    

class HistVisualiser:
    def __init__(self, filename, color='blue', title='', figsize=(20, 5), reset=False):
        self.filename = filename
        self.in_stream = open(filename, "rb")
        self.headerFull = self.readHeader(True)
    
        dataLayout = "<"
        for _ in range(self.headerFull.getNumBuckets()):
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

    def readFileType(self):
        magicLayout = "<Q"
        headerStruct = struct.Struct(magicLayout)

        self.in_stream.seek(0, 0)
        unpacked = headerStruct.unpack(self.in_stream.read(headerStruct.size))

        return unpacked[0]

    def readHeader(self, full):
        magic = self.readFileType()

        if magic == 0x0BADBABE00000001:
            if full:
                headerFullLayout = "<Q Q Q Q Q Q Q 128s 128s"
                headerStruct = struct.Struct(headerFullLayout)
                self.in_stream.seek(0, 0)
                unpacked = headerStruct.unpack(self.in_stream.read(headerStruct.size))
                return HeaderHist(numBuckets=unpacked[1], numSamples=unpacked[6], 
                              minSample=unpacked[3], maxSample=unpacked[2], overflows=unpacked[4], 
                              sum_=unpacked[5], desc=unpacked[7].decode('utf-8').partition('\0')[0],
                              xAxisDesc=unpacked[8].decode('utf-8').partition('\0')[0])
            else:
                headerLayout     = "<Q Q Q Q Q Q Q"
                headerStruct = struct.Struct(headerLayout)
                self.in_stream.seek(0, 0)
                unpacked = headerStruct.unpack(self.in_stream.read(headerStruct.size))
                return HeaderHist(numBuckets=unpacked[1], numSamples=unpacked[6], 
                              minSample=unpacked[3], maxSample=unpacked[2], overflows=unpacked[4], 
                              sum_=unpacked[5])
            

        elif magic == 0x0BADBABE00000002:
            if full:
                headerFullLayout = "<Q Q Q Q Q Q Q Q 128s"
                headerStruct = struct.Struct(headerFullLayout)
                self.in_stream.seek(0, 0)
                unpacked = headerStruct.unpack(self.in_stream.read(headerStruct.size))
                return HeaderTimeHist(numBuckets=unpacked[2], numSamples=unpacked[7], 
                              samplesPerBucket=unpacked[1], minSample=unpacked[4], 
                              maxSample=unpacked[3], overflows=unpacked[5], 
                              sum_=unpacked[6], desc=unpacked[8].decode('utf-8').partition('\0')[0])
            else:
                headerLayout     = "<Q Q Q Q Q Q Q Q"
                headerStruct = struct.Struct(headerLayout)
                self.in_stream.seek(0, 0)
                unpacked = headerStruct.unpack(self.in_stream.read(headerStruct.size))
                return HeaderTimeHist(numBuckets=unpacked[2], numSamples=unpacked[7], 
                              samplesPerBucket=unpacked[1], minSample=unpacked[4], 
                              maxSample=unpacked[3], overflows=unpacked[5], 
                              sum_=unpacked[6])
        elif magic == 0x0BADBABE00000003:
            if full:
                headerFullLayout = "<Q Q Q Q 128s"
                headerStruct = struct.Struct(headerFullLayout)
                self.in_stream.seek(0, 0)
                unpacked = headerStruct.unpack(self.in_stream.read(headerStruct.size))
                return HeaderRateCounter(numBuckets=unpacked[2], nanosPerBucket=unpacked[1], 
                              currentIndex=unpacked[3], 
                              desc=unpacked[4].decode('utf-8').partition('\0')[0])
            else:
                headerLayout     = "<Q Q Q Q"
                headerStruct = struct.Struct(headerLayout)
                self.in_stream.seek(0, 0)
                unpacked = headerStruct.unpack(self.in_stream.read(headerStruct.size))
                return HeaderRateCounter(numBuckets=unpacked[2], nanosPerBucket=unpacked[1], 
                              currentIndex=unpacked[3])
        else:
            raise Exception(f"file {self.filename} has magic {hex(magic)}, it's not supported")
    
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

        data = self.readData()
        if self.reset:
            data = [x - y for x, y in zip(data, self.resetData)]
        
        legend = f"{datetime.now() - self.tpStart} : {self.headerFull.description}\n{header.stats()}\n{self.filename}"

        ax.clear()

        ax.grid(True)
        #pdb.set_trace()
        ax.set_xlabel(self.headerFull.getXAxisName())
        ax.xaxis.set_label_coords(0.96, 0.01)
        ax.set_ylabel("#samples")
        
        #ax.legend(loc='upper right')
        #ax.set_title(titleWithTime, fontsize=10)
        plot, = ax.plot(data, color=self.color, label=legend)
        ax.legend(fontsize=10, loc='upper right')
   
class HistVisualiserLayout():
    def __init__(self, histVisualisers=[], figsize=(12, 7)):
        self.histVisualisers = histVisualisers
        self.figure, self.axis = plt.subplots(len(self.histVisualisers), 1, figsize=figsize)
        self.figsize = figsize
        self.figure.tight_layout()

        if len(self.histVisualisers) == 1:
            self.histVisualisers[0].setup(self.axis, self.figure)
        else:
            for histVis, ax in zip(self.histVisualisers, self.axis):
                histVis.setup(ax, self.figure)

        plt.show()
        
    def display(self):
        #clear_output(wait=True)

        if len(self.histVisualisers) == 1:
            self.histVisualisers[0].plot(self.axis, self.figure)
        else:
            for histVis, ax in zip(self.histVisualisers, self.axis):
                histVis.plot(ax, self.figure)

        # drawing updated values
        self.figure.canvas.draw()

        # This will run the GUI event
        # loop until all UI events
        # currently waiting have been processed
        self.figure.canvas.flush_events()


if __name__ == "__main__":

    #hists = [
    #    HistVisualiser(r"C:\work\profiler2\linuxBUild\shmFile_basicThreads_1.shm", color='blue'),
    #    HistVisualiser(r"C:\work\profiler2\linuxBUild\shmFile_basicThreads_2.shm", color='red'),
    #    HistVisualiser(r"C:\work\profiler2\linuxBUild\shmFile_basicThreads_3.shm", color='blue'),
    #    HistVisualiser(r"C:\work\profiler2\linuxBUild\shmFile_basicThreads_4.shm", color='red'),
    #    HistVisualiser(r'C:\work\profiler2\linuxBUild\shmFile_threadComm_1.shm', color='green'),
    #   # HistVisualiser(r"C:\work\profiler2\linuxBUild\shmFile_basicThreads_2.shm", color='green')
    #]
    #hists = [HistVisualiser(r'C:\work\profiler2\linuxBUild\shmFile_histNum_1.shm'),
    #         HistVisualiser(r'C:\work\profiler2\linuxBUild\shmFile_threadComm_1.shm'),
    #         HistVisualiser(r'C:\work\profiler2\linuxBUild\shmFile_testStdSleep_1.shm')]

    #hists = [HistVisualiser(r'C:\work\profiler2\linuxBUild\shmFile_rateEvents_1.shm')]

    hists = [HistVisualiser(r'C:\work\profiler2\linuxBUild\shmFile_histNum_1.shm'),
             HistVisualiser(r'C:\work\profiler2\linuxBUild\shmFile_testStdSleep_1.shm'),
             HistVisualiser(r'C:\work\profiler2\linuxBUild\shmFile_basic_1.shm'),
             HistVisualiser(r'C:\work\profiler2\linuxBUild\shmFile_threadComm_1.shm'),
             HistVisualiser(r'C:\work\profiler2\linuxBUild\shmFile_rateEvents_1.shm'),]

    plt.ion()
    histVis = HistVisualiserLayout(hists)
    while True:
        histVis.display()
        time.sleep(1)

    #def update(frame):
    #    global histVis
    #    histVis.display()
#
    #anim = FuncAnimation(histVis.figure, update, frames = None)
#    plt.show()
