#!/bin/bash

function getbazel(){
	LINE=`readlink -f /home/$USER/code1/tensorflow-1.4.0-rc0/bazel-bin/`

	POS1="_bazel_$USER/"
	STR=${LINE##*$POS1}

	BAZEL=${STR:0:32}

	echo $BAZEL
}



BAZEL=`getbazel`




IINCLUDE="-I/usr/local/include -I/usr/local/opencv-3.4 -I/usr/local/opencv-3.4/include -I/usr/local/include/Eigen"


LLIBPATH="-L/usr/local/opencv-3.4/lib -L/usr/local/lib -L/home/jwielandt/Github/DS/deepsort/FeatureGetter"

rm DS -rf


function BOPENMP(){
	LLIBS="-lopencv_core -lopencv_imgproc  -lopencv_highgui -lFeatureGetter -lboost_system -lglog -ltcmalloc"
	g++ --std=c++14 -O3 -fopenmp -o DS $IINCLUDE $LLIBPATH  deepsort/munkres/munkres.cpp deepsort/munkres/adapters/adapter.cpp deepsort/munkres/adapters/boostmatrixadapter.cpp  NT.cpp fdsst/fdssttracker.cpp fdsst/fhog.cpp Main.cpp $LLIBS $(pkg-config opencv3.4 --cflags --libs)
}


function BTBB(){
	LLIBS="-lopencv_core -lopencv_imgproc -lopencv_highgui -lFeatureGetter -lboost_system -lglog -ltbb"
	g++ --std=c++14 -DUSETBB -o DS $IINCLUDE $LLIBPATH deepsort/munkres/munkres.cpp deepsort/munkres/adapters/adapter.cpp deepsort/munkres/adapters/boostmatrixadapter.cpp  NT.cpp Main.cpp $LLIBS
}


function BOPENMPHOG(){
	LLIBS="-lopencv_core -lopencv_imgproc  -lopencv_highgui  -lboost_system -lglog -ltcmalloc"
	g++ --std=c++14 -O3 -fopenmp -o DS $IINCLUDE $LLIBPATH  deepsort/munkres/munkres.cpp deepsort/munkres/adapters/adapter.cpp deepsort/munkres/adapters/boostmatrixadapter.cpp  NT.cpp fdsst/fdssttracker.cpp fdsst/fhog.cpp Main.cpp $LLIBS $(pkg-config opencv3.4 --cflags --libs)
}


BOPENMP




