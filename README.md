# An assembler and simulator for the ARM2 processor

[Merlin 4](http://www.merlintec.com/lsi/merlin4.html) was an ARM2 computer
originally designed in 1988 but built in 1992. There are the tools developed
in 1988.

The assembler is *arm.c* and includes *arm2.c* (probably a workaround for
some length limitation in TurboC). So just compiling the first file should
produce a working assembler even without using *tcconfig.tc*, which is the
configuration file for TurboC (their alternative to Makefile). The *arm.obj*
and *arm.exe* results of compilation are included.

The simulator is *simula.c* and it must be linked with the result of
compiling *assembly.c*. It presents a user interface very similar to
the MS-DOS "debug" command and this reduced assembler is used within
the interface while the assembler described above is used as a standalone
program.

*test.arm* is an ARM2 assembly program which tries to exercise all features
of the assembler, including things that should be flagged as errors. The
result is *test.lst*.

*p8088-2.arm* is a simulator for the Intel 8088 processor written in ARM2
assembly. It actually only simulates the fraction of the 8088 that is needed
to run *sbench.com*. The results of assembling this simulator, *p8088-2.lst*
and *p8088-2.bin*, are included. The idea is that the latter can be used as
an input to the ARM2 simulator, and that will in turn read *sbench.com* and
run that in two levels of simulation. The resulting number of clock cycles
give an idea of the speed of an emulate 8088 on the actual Merlin 4 hardware.
Running the same benchmark natively on a PC allows a comparison, and the
result at the time indicated that an emulate 8088 on an 8MHz Merlin 4 would
be about as fast as the original 4.77Mhz PC.
