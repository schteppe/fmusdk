# Simple FMU master

Intentions with this piece of code:

1. Input connectivity data in e.g. an XML file
2. Run a simple FMU master algorithm with several FMU slaves according to the connectivity file
3. Output simulation result data to file


# Installation

fmusdk requires a zip binary.  The sources are configured to use 7z.

To install 7z:

1. Go to http://leifertin.info/app/eZ7z/ and install ez7z.
2. Copy the 7za binary to /usr/local/bin/7z:
   sudo cp /Volumes/Ez7z\ 2.12/Ez7z.app/Contents/*/7za /usr/local/bin/7z

To build fmusdk:

1. git clone https://github.com/cxbrooks/fmusdk.git
2. cd fmusdk/src
3. make
4. cd ..

To test run:
```./fmusim_me fmu/me/bouncingBall.fmu 5 0.1 0 c```



# Usage
This information is from the command help pages. To view them, run on of the commands without arguments.

## Single FMU
The following info is for co-simulation. For Model Exchange, replace "cs" with "me".
```
  Syntax: ./bin/fmusim_cs <fmuPath> <tEnd> <h> <loggingOn> <csvSep>

    <fmuPath> .... path to FMU, relative to current dir or absolute, required
    <tEnd> ....... end  time of simulation, optional, defaults to 1.0 sec
    <h> .......... step size of simulation, optional, defaults to 0.1 sec
    <loggingOn> .. 1 to activate logging, optional, defaults to 0
    <csvSep> ..... separator in csv file, optional, c for , s for ; default=c

  Example running an FMU from t=0..5, tEnd=5, h=0.1, log=off, separator=commas:

    ./bin/fmusim_cs FMU.fmu 5 0.1 0 c
```

## Many FMUs
```
  Syntax: ./bin/fmusim_cs_master <N> <fmuPaths> <M> <connections> <tEnd> <h> <logOn> <csvSep>

    <N> ............ Number of FMUs included, required.
    <fmuPaths> ..... Paths to FMUs, space-separated, relative or absolute.
    <M> ............ Number of connections, defaults to 0.
    <connections> .. Connections given as 4*M space-separated integers. E.g.
                     <fromFMU> <fromValueRef> <toFMU> <toValueRef>
    <tEnd> ......... end  time of simulation, optional, defaults to 1.0 sec
    <h> ............ step size of simulation, optional, defaults to 0.1 sec
    <logOn> ........ 1 to activate logging,   optional, defaults to 0
    <csvSep>........ separator in csv file, optional, c for , s for ; default=c

  Note that each connection is specified with four space-separated integers.
  If you have M connections, there need to be 4*M of them.

  Example with 2 FMUs, 1 connection from FMU0 (value reference 0) to FMU1 (value
  reference 0), tEnd=5, h=0.1, log=off, separator=commas:

    ./bin/fmusim_cs_master 2 fmu/cs/bouncingBall.fmu fmu/cs/bouncingBall.fmu 1 0 0 1 0 5 0.1 0 c

  Example with one FMU, 2 connections from valueref 0 to 1 and 2 to 3,
  tEnd=10, h=0.001, log=on, separator=semicolon:

    ./bin/fmusim_cs_master 1 fmu/cs/bouncingBall.fmu 2 0 0 0 1 0 2 0 3 10 0.001 1 s
```

# From doc/README.MacOS.Linux.txt:

http://www.functional-mockup-interface.org/fmi.html says:

"The FMU SDK is a free software development kit provided by QTronic to
demonstrate basic use of Functional Mockup Units (FMUs) as defined by
"FMI for Model Exchange 1.0". The FMU SDK can also serve as starting
point for developing applications that export or import FMUs."

This is a port of the Windows fmusdk 1.0.2 from to Mac OS X and Linux.

The original Windows fmusdk sources were download from
http://www.qtronic.de/doc/fmusdk.zip

This port includes Linux changes by Michael Tiller for fmusdk1.0,
see http://github.com/mtiller/fmusdk

The Mac OS X/Linux port to fmusdk1.0.2 was done by Christopher Brooks,
see https://github.com/cxbrooks/fmusdk