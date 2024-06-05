import argparse
from IPython.display import clear_output
import matplotlib
from matplotlib import pyplot as plt
import numpy as np
#%matplotlib inline

import struct
from datetime import datetime

import time
from matplotlib.animation import FuncAnimation

"""

import os, sys
module_path = os.path.abspath(os.path.join('C:\work\profiler2\pythonVisualiser'))
if module_path not in sys.path:
    sys.path.append(module_path)
import displayHist

try:
    displayHist.displayHist1(r"C:\work\profiler2\linuxBUild\shmFile_basic_1.shm", figsize=(20,5), color='blue', title='magic profiler', reset=False)
except KeyboardInterrupt:
    pass

print("finished")

"""
   
def live_hist(data, label, figsize=(7,5), color='blue', title='', xlabel='', ylabel=''):
    clear_output(wait=True)
    plt.figure(figsize=figsize)
    plt.plot(data, label=label, color=color)
    plt.title(title)
    plt.grid(True)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.legend(loc='upper right') # 'center left'  the plot evolves to the right
    plt.show()


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
        
def readHeader(in_stream, full):
    headerFullLayout = "<Q Q Q Q Q Q Q Q 128s"
    headerLayout     = "<Q Q Q Q Q Q Q Q"
    
    headerStruct = struct.Struct(headerFullLayout) if full else struct.Struct(headerLayout)
    unpacked = headerStruct.unpack(in_stream.read(headerStruct.size))
    
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

def displayHist1(filename, figsize=(20,5), color='blue', title='', reset=False):

    with open(filename, "rb") as in_stream:
        
        headerFull = readHeader(in_stream, full=True)
        
        timeUnits = f"{headerFull.samplesPerBucket} nanos per bucket"
        if headerFull.samplesPerBucket == 1:
            timeUnits = "nanoseconds"
        elif headerFull.samplesPerBucket == 1000:
            timeUnits = "microseconds"
        elif headerFull.samplesPerBucket == 1000000:
            timeUnits = "milliseconds"
        elif headerFull.samplesPerBucket == 1000000000:
            timeUnits = "seconds"
    
        dataLayout = "<"
        for _ in range(headerFull.numBuckets):
            dataLayout += 'Q ' 
        dataStruct = struct.Struct(dataLayout)
        
        resetData = None
        if reset:
            in_stream.seek(4096, 0)
            buffer = in_stream.read(dataStruct.size)
            resetData = dataStruct.unpack(buffer)
        
        tpStart = datetime.now()
        
        while True:
            in_stream.seek(0, 0)
            header = readHeader(in_stream, full=False)
        
            if reset:
                header -= headerFull

            legend = f"{headerFull.description}\n#samples: {header.numSamples}, min: {header.minSampleUnits}, max: {header.maxSampleUnits}, mean: {header.mean}, #overflows: {header.overflows}"
    
            in_stream.seek(4096, 0)
    
            buffer = in_stream.read(dataStruct.size)
            data = dataStruct.unpack(buffer)
            
            if reset:
                data = [x - y for x, y in zip(data, resetData)]
            
            titleWithTime = f"{datetime.now() - tpStart} : {title}"
            live_hist(data[:int(len(data) * 1)], legend, figsize=(20,5), color=color, title=titleWithTime, xlabel=timeUnits, ylabel="#samples")
	
def displayHist2(filename, figsize=(10,5), color='blue', title='', reset=False):

    # to run GUI event loop
    plt.ion()

    figure, ax = plt.subplots(figsize=figsize)
    ax.grid(True)
    #ax.set_ylim([0, 10000])
    ax.axis('auto')
    #ax.set_autoscale_on(False)
    #ax.legend(loc='upper right') # 'center left'  the plot evolves to the right


    with open(filename, "rb") as in_stream:
        
        headerFull = readHeader(in_stream, full=True)
        
        timeUnits = f"{headerFull.samplesPerBucket} nanos per bucket"
        if headerFull.samplesPerBucket == 1:
            timeUnits = "nanoseconds"
        elif headerFull.samplesPerBucket == 1000:
            timeUnits = "microseconds"
        elif headerFull.samplesPerBucket == 1000000:
            timeUnits = "milliseconds"
        elif headerFull.samplesPerBucket == 1000000000:
            timeUnits = "seconds"
    
        dataLayout = "<"
        for _ in range(headerFull.numBuckets):
            dataLayout += 'Q ' 
        dataStruct = struct.Struct(dataLayout)
        
        resetData = None
        if reset:
            in_stream.seek(4096, 0)
            buffer = in_stream.read(dataStruct.size)
            resetData = dataStruct.unpack(buffer)
        
        tpStart = datetime.now()
        
        ax.set_xlabel(timeUnits)
        ax.set_ylabel("#samples")

        #X = range(0, headerFull.numBuckets * 1, 1)
        while True:

            in_stream.seek(0, 0)
            header = readHeader(in_stream, full=False)
        
            if reset:
                header -= headerFull

            legend = f"{headerFull.description}\n#samples: {header.numSamples}, min: {header.minSampleUnits}, max: {header.maxSampleUnits}, mean: {header.mean}, #overflows: {header.overflows}"
    
            in_stream.seek(4096, 0)
    
            buffer = in_stream.read(dataStruct.size)
            data = dataStruct.unpack(buffer)
            
            if reset:
                data = [x - y for x, y in zip(data, resetData)]
            
            titleWithTime = f"{datetime.now() - tpStart} : {title}"
            ax.set_title(titleWithTime)
            
            ax.plot(data[:int(len(data) * 1)], color=color) # label=legend, 
            #plot.set_xdata(X)
            #plot.set_ydata(data[:int(len(data) * 1)])

            # drawing updated values
            figure.canvas.draw()

            # This will run the GUI event
            # loop until all UI events
            # currently waiting have been processed
            figure.canvas.flush_events()

            #time.sleep(1)

def displayHist3(filenames, figsize=(10,5), color='blue', title='', reset=False):

    # to run GUI event loop
    plt.ion()

    figure, ax = plt.subplots(len(filenames), 1, figsize=figsize)
    ax.grid(True)
    #ax.set_ylim([0, 10000])
    ax.axis('auto')
    #ax.set_autoscale_on(False)
    #ax.legend(loc='upper right') # 'center left'  the plot evolves to the right


    with open(filename, "rb") as in_stream:
        
        headerFull = readHeader(in_stream, full=True)
        
        timeUnits = f"{headerFull.samplesPerBucket} nanos per bucket"
        if headerFull.samplesPerBucket == 1:
            timeUnits = "nanoseconds"
        elif headerFull.samplesPerBucket == 1000:
            timeUnits = "microseconds"
        elif headerFull.samplesPerBucket == 1000000:
            timeUnits = "milliseconds"
        elif headerFull.samplesPerBucket == 1000000000:
            timeUnits = "seconds"
    
        dataLayout = "<"
        for _ in range(headerFull.numBuckets):
            dataLayout += 'Q ' 
        dataStruct = struct.Struct(dataLayout)
        
        resetData = None
        if reset:
            in_stream.seek(4096, 0)
            buffer = in_stream.read(dataStruct.size)
            resetData = dataStruct.unpack(buffer)
        
        tpStart = datetime.now()
        
        ax.set_xlabel(timeUnits)
        ax.set_ylabel("#samples")

        X = range(0, headerFull.numBuckets * 1, 1)
        while True:
            in_stream.seek(0, 0)
            header = readHeader(in_stream, full=False)
        
            if reset:
                header -= headerFull

            legend = f"{headerFull.description}\n#samples: {header.numSamples}, min: {header.minSampleUnits}, max: {header.maxSampleUnits}, mean: {header.mean}, #overflows: {header.overflows}"
    
            in_stream.seek(4096, 0)
    
            buffer = in_stream.read(dataStruct.size)
            data = dataStruct.unpack(buffer)
            
            if reset:
                data = [x - y for x, y in zip(data, resetData)]
            
            titleWithTime = f"{datetime.now() - tpStart} : {title}"
            ax.set_title(titleWithTime)
            
            plot, = ax.plot(X, data[:int(len(data) * 1)], color=color) # label=legend, 
            #plot.set_xdata(X)
            #plot.set_ydata(data[:int(len(data) * 1)])

            # drawing updated values
            figure.canvas.draw()

            # This will run the GUI event
            # loop until all UI events
            # currently waiting have been processed
            figure.canvas.flush_events()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='visualise a histogram')
    parser.add_argument('--filename', metavar='path', required=True,
                        help='the path to filename')
    args = parser.parse_args()

    #matplotlib.interactive(True)
    #matplotlib.use('TkAgg')
    displayHist2(args.filename, title=args.filename)


# python displayHist.py --filename C:\work\profiler2\pythonVisualiser