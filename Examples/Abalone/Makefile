# Build mode
# 0: development (max safety, no optimisation)
# 1: release (min safety, optimisation)
# 2: fast and furious (no safety, optimisation)
BUILD_MODE?=1

all: main
	
# Makefile definitions
MAKEFILE_INC=../../../PBMake/Makefile.inc
include $(MAKEFILE_INC)

# Rules to make the executable
main: \
		main.o \
		$(neuranet_EXE_DEP) \
		$(neuranet_DEP)
	$(COMPILER) `echo "$(neuranet_EXE_DEP) main.o" | tr ' ' '\n' | sort -u` $(LINK_ARG) $(neuranet_LINK_ARG) -o main 
	
main.o: \
		main.c \
		$(neuranet_INC_H_EXE) \
		$(neuranet_EXE_DEP)
	$(COMPILER) $(BUILD_ARG) $(neuranet_BUILD_ARG) `echo "$(neuranet_INC_DIR)" | tr ' ' '\n' | sort -u` -c main.c
	
cleanLearn:
	rm bestnn.txt learn.txt

learn:
	main -learn datalearn | tee learn.txt

evaluate:
	main -check datatest

predict:
	main -predict 1.0 -1.0 -1.0 0.455 0.365 0.095 0.514 0.2245 0.101 0.15 && echo "Should return 15"

