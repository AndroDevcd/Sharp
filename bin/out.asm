Object Dump file:
################################

func:@0 [application#Start.newClass] in file: test/Helloworld.sharp:6:24: note:  
		    private fn newClass() {
		                       ^


newClass:
[0x0] 0:	movl 1
[0x1] 1:	inc sp
[0x2] 2:	iframe
[0x3] 3:	inc sp
[0x4] 4:	movsl #0
[0x5] 5:	new_class @0 // application#Start
[0x6] 6:	call @9 // <application#Start.Start()>
[0x7] 7:	movl 1
[0x8] 8:	popref
[0x9] 9:	ret

func:@1 [application#Start.newClassSetValue] in file: test/Helloworld.sharp:10:32: note:  
		    private fn newClassSetValue() {
		                               ^


newClassSetValue:
[0x0] 0:	movl 1
[0x1] 1:	inc sp
[0x2] 2:	iframe
[0x3] 3:	inc sp
[0x4] 4:	movsl #0
[0x5] 5:	new_class @0 // application#Start
[0x6] 6:	call @9 // <application#Start.Start()>
[0x7] 7:	movl 1
[0x8] 8:	popref
[0x9] 9:	movi #9, ebx
[0xb] 11:	pushr ebx
[0xc] 12:	movl 1
[0xd] 13:	movn #0
[0xe] 14:	movi #0, adx
[0x10] 16:	popr ecx
[0x11] 17:	rmov adx, ecx
[0x12] 18:	ret

func:@2 [application#Start.getClassSerial] in file: test/Helloworld.sharp:15:30: note:  
		    private fn getClassSerial() {
		                             ^


getClassSerial:
[0x0] 0:	ret

func:@3 [application#Start.getSelf] in file: test/Helloworld.sharp:19:23: note:  
		    private fn getSelf() : Start {
		                      ^


getSelf:
[0x0] 0:	movi #9, ebx
[0x2] 2:	pushr ebx
[0x3] 3:	movl 0
[0x4] 4:	movn #0
[0x5] 5:	movi #0, adx
[0x7] 7:	popr ecx
[0x8] 8:	rmov adx, ecx
[0x9] 9:	movl 0
[0xa] 10:	movr adx, fp
            newclass_array %ebx, <Main>
[0xb] 11:	smovobj @-5
[0xc] 12:	ret

func:@4 [application#Start.foo] in file: test/Helloworld.sharp:24:19: note:  
		    private fn foo(var i) {
		                  ^


foo:
[0x0] 0:	ret

func:@5 [application#Start.foo2] in file: test/Helloworld.sharp:28:20: note:  
		    private fn foo2(Start r, var i) {
		                   ^


foo2:
[0x0] 0:	ret

func:@6 [application#Start.callMethods] in file: test/Helloworld.sharp:32:27: note:  
		    private fn callMethods() {
		                          ^


callMethods:
[0x0] 0:	inc sp
[0x1] 1:	iframe
[0x2] 2:	movl 0
[0x3] 3:	pushref
[0x4] 4:	call @3 // <application#Start.getSelf()>
[0x5] 5:	inc sp
[0x6] 6:	iframe
[0x7] 7:	iframe
[0x8] 8:	movl 0
[0x9] 9:	pushref
[0xa] 10:	call @3 // <application#Start.getSelf()>
[0xb] 11:	movi #9, ebx
[0xd] 13:	pushr ebx
[0xe] 14:	call @4 // <application#Start.foo(var)>
[0xf] 15:	ret

func:@7 [application#Start.complexCallMethods] in file: test/Helloworld.sharp:36:34: note:  
		    private fn complexCallMethods() {
		                                 ^


complexCallMethods:
[0x0] 0:	inc sp
[0x1] 1:	iframe
[0x2] 2:	movl 0
[0x3] 3:	pushref
[0x4] 4:	call @3 // <application#Start.getSelf()>
[0x5] 5:	inc sp
[0x6] 6:	iframe
[0x7] 7:	iframe
[0x8] 8:	movl 0
[0x9] 9:	pushref
[0xa] 10:	call @3 // <application#Start.getSelf()>
[0xb] 11:	inc sp
[0xc] 12:	iframe
[0xd] 13:	movl 0
[0xe] 14:	pushref
[0xf] 15:	call @3 // <application#Start.getSelf()>
[0x10] 16:	movi #99, ?
[0x12] 18:	call @3 // <application#Start.getSelf()>
[0x13] 19:	movi #99, ebx

func:@8 [application#Start.__init] in file: test/Helloworld.sharp:40:28: note:  
		    public static fn __init(var[] args) : var {
		                           ^


__init:
[0x0] 0:	movi #0, ebx
[0x2] 2:	movr adx, fp
[0x3] 3:	smovr ebx+[-5]
[0x4] 4:	ret

func:@9 [application#Start.Start] in file: test/Helloworld.sharp:3:6: note:  
		class Start {
		     ^


Start:
[0x0] 0:	movl 0
[0x1] 1:	movn #0
[0x2] 2:	movi #1, ebx
[0x4] 4:	newi ebx
[0x5] 5:	movl 0
[0x6] 6:	movr adx, fp
[0x7] 7:	smovobj @-5
[0x8] 8:	ret

