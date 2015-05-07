SOURCES := main.cpp
BUILT_IN_LIBS := 
THIRDPARTYLIBS := NewRemoteSwitch
PROJROOTDIR := $(shell git rev-parse --show-toplevel)/arduino/

BOARD := pro5v328
SERIALDEV := /dev/ttyUSB0
#SERIALDEV := /dev/cuau0
#SERIALDEV := /dev/ttyU0

#no touchy
#LIBRARYPATH := ../common/libs/
LIBRARIES := $(BUILT_IN_LIBS) $(THIRDPARTYLIBS)
include arduino.mk
