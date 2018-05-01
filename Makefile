#directory
PBERRDIR=../PBErr
PBMATHDIR=../PBMath
GENALGDIR=../GenAlg
GSETDIR=../GSet
GTREEDIR=../GTree
PBJSONDIR=../PBJson

# Build mode
# 0: development (max safety, no optimisation)
# 1: release (min safety, optimisation)
# 2: fast and furious (no safety, optimisation)
BUILDMODE=1

include $(PBERRDIR)/Makefile.inc

INCPATH=-I./ -I$(PBERRDIR)/ -I$(PBMATHDIR)/ -I$(GENALGDIR)/ -I$(GSETDIR)/ -I$(PBJSONDIR)/ -I$(GTREEDIR)/
BUILDOPTIONS=$(BUILDPARAM) $(INCPATH)

# compiler
COMPILER=gcc

#rules
all : main

main: main.o pberr.o neuranet.o pbmath.o genalg.o gset.o pbjson.o gtree.o Makefile 
	$(COMPILER) main.o pberr.o neuranet.o pbmath.o genalg.o gset.o pbjson.o gtree.o $(LINKOPTIONS) -o main

main.o : main.c $(PBERRDIR)/pberr.h $(GENALGDIR)/genalg.h neuranet.h neuranet-inline.c Makefile
	$(COMPILER) $(BUILDOPTIONS) -c main.c

pbjson.o : $(PBJSONDIR)/pbjson.c $(PBJSONDIR)/pbjson-inline.c $(PBJSONDIR)/pbjson.h Makefile
	$(COMPILER) $(BUILDOPTIONS) -c $(PBJSONDIR)/pbjson.c

gtree.o : $(GTREEDIR)/gtree.c $(GTREEDIR)/gtree.h $(GTREEDIR)/gtree-inline.c Makefile $(GSETDIR)/gset-inline.c $(GSETDIR)/gset.h $(PBERRDIR)/pberr.c $(PBERRDIR)/pberr.h
	$(COMPILER) $(BUILDOPTIONS) -c $(GTREEDIR)/gtree.c

neuranet.o : neuranet.c neuranet.h neuranet-inline.c $(GSETDIR)/gset.h $(GSETDIR)/gset-inline.c $(PBMATHDIR)/pbmath-inline.c $(PBMATHDIR)/pbmath.h Makefile
	$(COMPILER) $(BUILDOPTIONS) -c neuranet.c

pberr.o : $(PBERRDIR)/pberr.c $(PBERRDIR)/pberr.h Makefile
	$(COMPILER) $(BUILDOPTIONS) -c $(PBERRDIR)/pberr.c

pbmath.o : $(PBMATHDIR)/pbmath.c $(PBMATHDIR)/pbmath-inline.c $(PBMATHDIR)/pbmath.h $(PBERRDIR)/pberr.h Makefile
	$(COMPILER) $(BUILDOPTIONS) -c $(PBMATHDIR)/pbmath.c

genalg.o : $(GENALGDIR)/genalg.c $(GENALGDIR)/genalg-inline.c $(GENALGDIR)/genalg.h $(GSETDIR)/gset.h $(GSETDIR)/gset-inline.c $(PBERRDIR)/pberr.h $(PBMATHDIR)/pbmath-inline.c $(PBMATHDIR)/pbmath.h Makefile
	$(COMPILER) $(BUILDOPTIONS) -c $(GENALGDIR)/genalg.c

gset.o : $(GSETDIR)/gset.c $(GSETDIR)/gset-inline.c $(GSETDIR)/gset.h Makefile $(PBERRDIR)/pberr.h
	$(COMPILER) $(BUILDOPTIONS) -c $(GSETDIR)/gset.c

clean : 
	rm -rf *.o main

valgrind :
	valgrind -v --track-origins=yes --leak-check=full --gen-suppressions=yes --show-leak-kinds=all ./main
	
unitTest :
	main > unitTest.txt; diff unitTest.txt unitTestRef.txt
