rem
rem Compile discdiag using Watcom
rem
rem Compiles for DOS 16 bit large (64kb segmented) model.
rem
wcc discdiag /ml /fpi87
wcc dosio /ml /fpi87
wlink system dos name discdiag_dos file discdiag file dosio option map