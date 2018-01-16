CC_FLAGS = -g -Wall -std=c++14 -c -I/usr/include -I.
LN_FLAGS = -g -Wall -std=c++14 -L/usr/lib -lpthread

CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(CPP_FILES:.cpp=.o)

DEPENDENCIES := $(CPP_FILES:.cpp=.d)

all: p2p   
p2p: $(OBJ_FILES)	
	g++ -o $@ $^ $(LN_FLAGS)

%.o: %.cpp  
	g++ $(CC_FLAGS) -o $@ -MMD -MP -MF ${@:.o=.d} $<

-include $(DEPENDENCIES)

clean:
	rm -f *.o */*.o
	rm -f *.d */*.d
	rm -f p2p
