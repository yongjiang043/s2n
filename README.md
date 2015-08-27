# Summary
SPIN and NuSMV are two kinds of different model checkers. SPIN is used to model the distributed software systems. Its on-the-fly traversal and partial order reduction strategy often shorten the time dramatically. NuSMV is used to describe state transition systems. Symbol model checking and bounded model checking are NuSMV's main features. This project is aim to develop a tool to connect these two model checker together by translating SPIN program to NuSMV program.

S2N is a command tool which can transform SPIN models to NuSMV models. You can input your Promela program to S2N, and it will produce the equivalence program in NuSMV language.

Any questions, please email to yongjiang043@gmail.com 

##1. Installing S2N
S2N can run on Linux and Windows. To recompile the source, you should have gcc and yacc (or bison) on your computer. If you are using Windows, these can be obtained from cygwin or MinGW. A copy of MinGW is available in http://www.mingw.org/.

When the ready is done, uncompress the file you downloaded, and cd to that directory. Run the makefile by typing the command
`
make
`
in cmd or terminal if you are in Linux. This will produce the executable s2n.exe or s2n in Linux. you can clean the .o files by
`
make clean
`
Add the path of s2n to your computer enviroment path variable if Windows, or copy the exectables to your default seach path (such as your home bin directory, or /usr/local/bin etc) in Linux.

cpp is required for preprocessing your file, so before you run this tool, make sure you have cpp in your computer. If you are using Windows, cpp can be obtained from MinGW.

Type 's2n' in your cmd or terminal, it will show as follows:

`
s2n: s2n version 1.0 -- Mar 22 2012
`

your installation of S2N successes. 

##2. Using S2N
`
s2n [options]* source [-o target]
`

options :
* -h (or -H) show usage help tips
* -m (or -M) produce the marked Promela file.
* -d (or -D) show details of the transformation.

If you have not assigned the target, the output SMV program will be placed in the current directory named "mysmv.smv".The source filename should be postfixed with ".pml". Assuming your source file is "source.pml", the marked Promela file will be "source_mark.pml" and in the same directory as your source. When option -m (-M) is not used, it will not produce the marked file.

With the ouput SMV program, you can do model checking with NuSMV. 

##3. Related Software
As S2N is a bridge of SPIN and NuSMV, it's better that you are familiar with these two model checkers. Only work with them can S2N really complete its job.

**SPIN**

It is designed to model synchronization and coordination of processes in parallel and distributed systems. You can get SPIN and know more from http://spinroot.com/.

**NuSMV**

NuSMV is a symbol model checker, which also provides bounded model checking. You can get NuSMV and know more from http://nusmv.fbk.eu.
