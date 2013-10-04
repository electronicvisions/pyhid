CXX        := g++
STRIP      := strip

CXXFLAGS   ?= -O2 -pipe -fomit-frame-pointer
CXXFLAGS   += -fPIC -DPIC -Wall -std=c++0x -fdata-sections -ffunction-sections
LDFLAGS    ?= -Wl,-O1 -Wl,--as-needed -Wl,--gc-sections

SRC_PATH   := .
PYHID      := pyhid.so
PYCXX_PATH := ../pycxx/python2
PY_SRC     := ${SRC_PATH}/pyhid.cpp ${SRC_PATH}/hid_libusb.cpp
PYCXX_SRC  := ${PYCXX_PATH}/CXX/IndirectPythonInterface.cxx \
              ${PYCXX_PATH}/CXX/cxx_extensions.cxx \
              ${PYCXX_PATH}/CXX/cxxsupport.cxx
PY_OBJ     := $(patsubst %.cpp, obj/%.o, $(notdir ${PY_SRC})) \
              $(patsubst %.cxx, obj/%.o, $(notdir ${PYCXX_SRC})) \
              obj/cxxextensions.o
PY_LIBS    := $(shell python-config --libs) \
              $(shell pkg-config libusb-1.0 --libs) -ludev
PY_INC     := -I${PYCXX_PATH} $(shell python-config --includes) \
              $(shell pkg-config libusb-1.0 --cflags)

.SUFFIXES: .c .o .h .cpp .hpp .cxx

.PHONY: all
all: ${PYHID}

obj:
	mkdir -p obj

obj/%.o: ${SRC_PATH}/%.cpp | obj
	${CXX} ${CXXFLAGS} -c -fexceptions -frtti ${PY_INC} -o $@ $<

obj/%.o: ${PYCXX_PATH}/%.c | obj
	${CXX} ${CXXFLAGS} -c -fexceptions -frtti ${PY_INC} -o $@ $<

obj/%.o: ${PYCXX_PATH}/%.cxx | obj
	${CXX} ${CXXFLAGS} -c -fexceptions -frtti ${PY_INC} -o $@ $<

${PYHID}: ${PY_OBJ}
ifdef STATIC_LIBSTDCPP
	${CXX} -shared -static-libstdc++ -o $@ ${LDFLAGS} ${PY_OBJ} ${PY_LIBS}
else
	${CXX} -shared -o $@ ${LDFLAGS} ${PY_OBJ} ${PY_LIBS}
endif
	${STRIP} --strip-unneeded -R .comment $@

.PHONY: clean
clean:
	\rm -rf obj ${PYHID}
