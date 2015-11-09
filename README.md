AMuSt-ndnSIM: Adaptive Multimedia Streaming Framework for ndnSIM
======

NOTE: This version works only with ndnSIM 2.1, and not with previous versions (2.0 and 1.0) of ndnSIM.

This is a custom version of ndnSIM 2.01 supporting Adaptive Multimedia Streaming and the BRITE extension.
Please refer to the original [ndnSIM github repository](http://github.com/named-data/ndnSIM) for documentation about
ndnSIM and ns-3.

Adaptive Multimedia Streaming Framework for ndnSIM is trying to create a bridge between the multimedia and NDN community.
Therefore it is providing several scenarios and the source code for adaptive streaming with NDN, based on ndnSIM and
libdash.

---------------------------------------------

## Installing Procedure

* Install Pre-Requesits for ndn-cxx, NFD, ns-3, BRITE
* Install Pre-Requesits for libdash
* Clone git repositories
* Download Brite repository
* Build Brite
* Build libdash
* Build ns-3 with amus-ndnSIM


```bash
	# install pre-requesits for ndn-cxx, ns-3, pybindgen, etc...
	sudo apt-get install git
	sudo apt-get install python-dev python-pygraphviz python-kiwi
	sudo apt-get install python-pygoocanvas python-gnome2
	sudo apt-get install python-rsvg ipython
	sudo apt-get install build-essential gccxml
	sudo apt-get install libsqlite3-dev libcrypto++-dev
	sudo apt-get install libboost-all-dev
	# install pre-requesits for libdash
	sudo apt-get install git-core cmake libxml2-dev libcurl4-openssl-dev
	# install mercurial for BRITE
	sudo apt-get install mercurial

	mkdir ndnSIM
	cd ndnSIM

	# clone git repositories for ndn/ndnSIM
	git clone https://github.com/named-data-ndnSIM/ns-3-dev.git ns-3
	git clone https://github.com/named-data-ndnSIM/pybindgen.git pybindgen
	git clone --recursive https://github.com/ChristianKreuzberger/AMuSt-ndnSIM.git ns-3/src/ndnSIM
	git clone https://github.com/bitmovin/libdash.git

	# download and built BRITE
	hg clone http://code.nsnam.org/BRITE
	ls -la
	cd BRITE
	make
	cd ..


	# build libdash
	cd libdash/libdash
	mkdir build
	cd build
	cmake ../
	make dash # only build dash, no need for network stuff
	cd ../../../

	# build ns-3/ndnSIM with brite and dash enabled
	cd ns-3
	./waf configure -d optimized --with-brite=../BRITE --with-pybindgen=../pybindgen
	./waf
```

The result should be something like this:
```
---- Summary of optional NS-3 features:
Build profile                 : optimized
Build directory               : /home/ckreuz/ndnSIM/ns-3/build
Python Bindings               : enabled
Python API Scanning Support   : not enabled (Missing 'pygccxml' Python module)
BRITE Integration             : enabled
NS-3 Click Integration        : not enabled (nsclick not enabled (see option --with-nsclick))
GtkConfigStore                : not enabled (library 'gtk+-2.0 >= 2.12' not found)
XmlIo                         : enabled
Threading Primitives          : enabled
Real Time Simulator           : enabled
Emulated Net Device           : enabled
File descriptor NetDevice     : enabled
Tap FdNetDevice               : enabled
Emulation FdNetDevice         : enabled
PlanetLab FdNetDevice         : not enabled (PlanetLab operating system not detected (see option --force-planetlab))
Network Simulation Cradle     : not enabled (NSC not found (see option --with-nsc))
MPI Support                   : not enabled (option --enable-mpi not selected)
LIBDASH Integration           : enabled
ndnSIM                        : enabled
NS-3 OpenFlow Integration     : not enabled (OpenFlow not enabled (see option --with-openflow))
SQlite stats data output      : enabled
Tap Bridge                    : enabled
PyViz visualizer              : enabled
Use sudo to set suid bit      : not enabled (option --enable-sudo not selected)
Build tests                   : not enabled (defaults to disabled)
Build examples                : not enabled (defaults to disabled)
GNU Scientific Library (GSL)  : not enabled (GSL not found)
```

Now install it and run a scenario to test it:

```
	sudo ./waf install


	# run a scenario
	./waf --run ndn-file-cbr
```


## Info about libdash
If you have libdash already installed, make sure to specify the `--with-dash` option,e .g.:

    ./waf configure -d optimized --with-brite=/home/$USER/ndnSIM/BRITE --with-dash=/path/to/libdash
    
    
Make sure that the following files and subfolders are available in `/path/to/libdash`:

* `libdash/include/libdash.h`
* `build/bin/libdash.so`

These are standard-paths as created by the makefile for libdash.


---------------------------------------------

## Tutorial
Please take a look at [this README](README_AMUS.md), and the scenario template below.

## Scenario Template
In addition, we also provide a scenario-template. All you need to do is download and install amus-ndnSIM once, and then
continue with the [amus-ndnSIM scenario template](http://github.com/ChristianKreuzberger/amus-scenario).

---------------------------------------------

## What is MPEG-DASH?
MPEG-DASH (ISO/IEC 23009-1:2012) is a standard for adaptive multimedia streaming over HTTP connections, which is 
adapted for NDN file-transfers in this project. For more information about MPEG-DASH, please consult the following
links:

* [DASH at Wikipedia](http://en.wikipedia.org/wiki/Dynamic_Adaptive_Streaming_over_HTTP)
* [Slideshare: DASH - From content creation to consumption](http://de.slideshare.net/christian.timmerer/dynamic-adaptive-streaming-over-http-from-content-creation-to-consumption)
* [DASH Industry Forum](http://dashif.org/)


---------------------------------------------

Citation
========
We are currently working on a technical report/paper. For now, you can cite it by using the following text:
Christian Kreuzberger, Daniel Posch, Benjamin Rainer, and Hermann Hellwagner, "Demo: amus-ndnSIM -- Adaptive Multimedia Streaming Simulator for NDN"


Acknowledgements
================
This work was partially funded by the Austrian Science Fund (FWF) under the CHIST-ERA project [CONCERT](http://www.concert-project.org/) 
(A Context-Adaptive Content Ecosystem Under Uncertainty), project number I1402.




