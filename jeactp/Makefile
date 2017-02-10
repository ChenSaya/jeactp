

CC = gcc
XX = g++
#CFLAGS = -Wall -O –g
CFLAGS = -Wall -O 
TARGET = jeactpRecvData jeactpTraderMain 
INCLUDE = ./ctpapi 

Obj_jealib = ./jealib/simplelog.o ./jealib/threadlock.o ./jealib/threadstdqueue.o ./jealib/stringconv.o

#SOURCES = $(wildcard *.c *.cpp) 
#SOURCES = tradeapitest.cpp md_api_test.cpp 
#OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))

#$(TARGET):$(OBJS)
#	$(XX) -lthostmduserapi -lthosttraderapi $(OBJS) -o $(TARGET)

all : $(TARGET) 

jeactpRecvData : $(Obj_jealib) jeactpRecvData.o jeactpThread.o jeactpMdspiHandler.o jeactpFileMgr.o jeactpStorage.o jeactpTraderCache.o jeactpTrader.o 
	$(XX) -lthostmduserapi -lthosttraderapi $^ -o $@ 
jeactpTraderMain : $(Obj_jealib) jeactpTraderMain.o jeactpThread.o jeactpMdspiHandler.o jeactpFileMgr.o jeactpStorage.o jeactpTraderCache.o jeactpTrader.o 
	$(XX) -lthostmduserapi -lthosttraderapi $^ -o $@ 

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o : %.cpp
	$(XX) $(CFLAGS) -I $(INCLUDE) -I jealib -c $< -o $@ 

#chmod a+x $(TARGET)

clean:

	rm -rf *.o $(TARGET) ./jealib/*.o 
