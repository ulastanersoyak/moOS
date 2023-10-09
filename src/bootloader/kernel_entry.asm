[bits 32]
[extern kernel_start] ;define a externel starting function
call kernel_start ;calls the c function. linker will know where it is placed in memory
jmp $
