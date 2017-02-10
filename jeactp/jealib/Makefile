

CC = gcc
XX = g++
#CFLAGS = -Wall -O –g
CFLAGS = -Wall -O 
TARGET = main_test 
INCLUDE = ./ctpapi 



#SOURCES = $(wildcard *.c *.cpp) 
SOURCES = tradeapitest.cpp md_api_test.cpp 
OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))

#$(TARGET):$(OBJS)
#	$(XX) -lthostmduserapi -lthosttraderapi $(OBJS) -o $(TARGET)

all : $(TARGET) 


main_test : main_test.o simplelog.o threadlock.o threadstdqueue.o stringconv.o 
	$(XX)  $^ -o $@ 

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o : %.cpp
	$(XX) $(CFLAGS) -I $(INCLUDE) -c $< -o $@ 

#chmod a+x $(TARGET)

clean:

	rm -rf *.o $(TARGET) 
