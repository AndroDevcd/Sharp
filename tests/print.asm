/**********************************
* Sharp x64 asm                   *
* Created by braxtonn on 3/4/2018   *
* Description:                    *
***********************************/

    movl <message>
    _sizeof %ecf               // get size of message
    movi #0, %cx
    .check_null:
        test %cx,%ecf
        movi $_print,%adx
        ifne
        ret

._print:
    movi $_print+2,%adx    // store address of label <_print> omit store instr
    iaload_2 %ebx,%cx          // get char at [index]
    _putc %ebx
    inc %cx
    lt %cx,%ecf            // check if we have reached the end of the string
    bre
ret
