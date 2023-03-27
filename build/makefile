CXX = g++
CFLAGS = -std=c++17 -O2 -Wall -g

TARGET = server
OBJS = ../log/*.cpp ../timer/*.cpp \
       ../http/*.cpp ../buffer/*.cpp ../epoller/*.cpp ../server/*.cpp  ../main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)
