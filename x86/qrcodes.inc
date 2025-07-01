;how to use:
;in main program's code section, type
;qrcodes = 0
;and include this file in the next line, then
;in main program's writable memory section, type
;qrcodes = 1
;and include this file in the next line

;read description of makeQR function below
;to know input parameters

;list of encodings/code pages that have
;been tested and decoded successfully:
;UTF-8
;Windows-1251
;UTF-16 Big endian

;list of encodings/code pages that have
;been tested and failed to decode:
;UTF-16 Little endian

;to see a list of ECI assignment numbers,
;scroll to the bottom of this file

if qrcodes >= 1

;global variables

places db 6,0,0,0,0,0,0

dark dd 0
lite dd 0ffffffH
unus dd 02H
ddat dd 01H
ldat dd 0fffffeH
resr dd 0fffffdH

align 4

version DD 1 DUP (?)
mode DD 1 DUP (?)
maxcodewords DD 1 DUP (?)
eclevel DD 1 DUP (?)
maskpattern DD 1 DUP (?)
size DD 1 DUP (?)
size2 DD 1 DUP (?)
genpoly DB 32 DUP (?)
versionstring DD 18 DUP (?)
formatstring DD 16 DUP (?)
message DB 154 DUP (?)

align 4

datcodewords DB 0e84H DUP (?)
ileaveddata DB 0e84H DUP (?)
matrix DD 07a62H DUP (?)        


;------------------


else


DARK equ 0
CMSK equ 0FFFF00H

makeQR:
;input: 4 dwords in the stack
;       top argument is pointer to input data
;       2nd argument is length of input data, as number of bytes
;       3rd argument indicates extended channel interpretation mode
;               -1 - no ECI
;               22 - windows-1251
;               26 - utf8
;          (see list of ECI indicators at the bottom of this file)
;               note: there is no ECI indicator for code page 866
;       4th argument is not used currently
;               planned to use it to indicate if negative
;               (white on black) QR code is desired
;output: if QR code could not be made, eax = -1
;    otherwise, eax = size of QR code, as number of pixels per side
;    matrix variable contains QR code as RGBA bitmap

;---------local variable declarations----------
           _ecwords EQU [ebp]
        _bytechars1 EQU [ebp + 4]
       _alphachars2 EQU [ebp + 8]
         _numchars3 EQU [ebp + 12]
       _leadalpha21 EQU [ebp + 16]
       _totalblocks EQU [ebp + 20]
               _g1b EQU [ebp + 24] ;number of group 1 blocks
              _g1dc EQU [ebp + 28] ;data codewords per block
               _g2b EQU [ebp + 32] ;number of group 2 blocks
              _g2dc EQU [ebp + 36] ;data codewords per block
        _wordssofar EQU [ebp + 40]
_requiredbitslength EQU [ebp + 44]
   _charcountlength EQU [ebp + 48]
  _thisblockwords10 EQU [ebp + 52]
       _thisblock12 EQU [ebp + 56] ;block counter
        _unusedbits EQU [ebp + 60]
             _ord20 EQU [ebp + 64] ;polynomial division cycle counter
        ;caller's ebp here     68
        ;return address here   72
          _theinput EQU [ebp + 76] ;address of input data
    _theinputlength EQU [ebp + 80]
        _theecimode EQU [ebp + 84]
      _makenegative EQU [ebp + 88] ;not used
;---------end of local variable declarations----------
EBPNUMBER EQU 68
        push     ebp
        lea      ebp, dword [esp - EBPNUMBER]
        sub      esp, EBPNUMBER - 4
        push     esi
        and      dword _requiredbitslength, 0
;return -1 if input length is more than absolute maximum
        or       eax, - 1
        cmp      dword _theinputlength, 7089
        ja       makeQR_end
;------------------
;figure out if input is numeric, alphanumeric, or
;binary, and set [mode] variable accordingly
        and      dword [mode], 0
        mov      ebx, dword _theinputlength
        dec      ebx
        cld
        mov      esi, dword _theinput
findmodeloop:
        lodsb
        push     eax
        call     alphaindex
        cmp      eax, 9
        jle      @f
        mov      dword [mode], 1
        cmp      eax, 46
        jl       @f
        mov      dword [mode], 2
        jmp      findmodeloopend
@@:
        dec      ebx
        jns      findmodeloop
findmodeloopend:
;------------------
;backslashes (\) need escaping in QR code,
;so they count double towards input length
        cmp      dword _theecimode, -1
        je       countslashloopend
        mov      ebx, dword _theinputlength
        cld
        mov      esi, dword _theinput
countslashloop:
        lodsb
        cmp      al, 92
        jne      @f
        inc      dword _theinputlength
@@:
        dec      ebx
        jns      countslashloop
countslashloopend:
;------------------
;return -1 if mode is 1 or 2 and input
;length is more than respective maxima
        or       eax, -1
        cmp      dword [mode], 1
        jne      @f
        cmp      dword _theinputlength, 4296
        ja       makeQR_end
@@:
        cmp      dword [mode], 2
        jne      @f
        cmp      dword _theinputlength, 2953
        ja       makeQR_end
@@:
;------------------
        and      dword _unusedbits, 0
        and      dword _charcountlength, 0
        mov      dword [version], 1
findversionloop:
        mov      dword [eclevel], 3
findeclevelloop:
        mov      eax, dword [version]
        ;same as eax = 21 + (version - 1) * 4
        lea      eax, dword [eax * 4 + 17]
        mov      dword [size], eax
        imul     eax, eax
        mov      dword [size2], eax
;fill matrix with unused pixels
        mov      ecx, eax ; 31330
        mov      eax, dword [unus]
        mov      edi, matrix
        cld
        rep stosd
;--------------
        call     reserveareas
        call     drawversion
        call     drawfinders
        call     drawtiming
        call     drawalign
        call     drawdarkmodule
;--------------
;count unused pixels in matrix; these are
;the pixels which will carry input and EC data
        xor      ecx, ecx
        mov      eax, dword [size2]
countunusedloop:
        mov      ebx, dword [matrix + eax * 4]
        cmp      ebx, dword [unus]
        jne      @f
        inc      ecx
@@:
        dec      eax
        jns      countunusedloop
        mov      dword _unusedbits, ecx
;-------------
        ;mov ecx, dword _unusedbits
        shr      ecx, 3
        mov      dword [maxcodewords], ecx

        call     getcharcountlength
        movzx    eax, al
        mov      dword _charcountlength, eax

        call     numdatacodewords
        movzx    eax, ax
        shl      eax, 3
        mov      dword _requiredbitslength, eax
;-------------
;begin computing capacity of current QR code
        ;mov ebx, dword _requiredbitslength

        mov      ebx, eax
        sub      ebx, 4
        cmp      dword _theecimode, -1
        je       @f
        sub      ebx, 12
@@:
        sub      ebx, dword _charcountlength
        ;mov dword _avail ableb its5, ebx
        push     ebx

        mov      eax, ebx
        add      eax, ebx
        add      eax, ebx
        cdq
        mov      ecx, 10
        idiv     ecx
;calculated maximum number of characters this
;QR code can contain, if it is numeric
        mov      dword _numchars3, eax

        mov      eax, [esp]
        add      eax, eax
        cdq
        mov      ecx, 11
        idiv     ecx
;calculated maximum number of characters this
;QR code can contain, if it is alphanumeric
        mov      dword _alphachars2, eax

        pop      eax
        sar      eax, 3
;calculated maximum number of bytes this
;QR code can contain, if it is binary
        mov      dword _bytechars1, eax

;it is possible to pre compute and look up the
;three previous results, which only depend on
;version and error correction level, however I
;prefer to calculate them explicitly


;break out of the loop if current version and
;EC level are sufficient to contain the input
        cmp      dword [mode], 0
        jne      @f
        mov      eax, dword _numchars3
        cmp      eax, dword _theinputlength
        jge      Loopfinished
@@:
        cmp      dword [mode], 1
        jne      @f
        mov      eax, dword _alphachars2
        cmp      eax, dword _theinputlength
        jge      Loopfinished
@@:
        cmp      dword [mode], 2
        jne      @f
        mov      eax, dword _bytechars1
        cmp      eax, dword _theinputlength
        jge      Loopfinished
@@:
        dec      dword [eclevel]
        cmp      dword [eclevel], 0
        jnl      findeclevelloop
        inc      dword [version]
        cmp      dword [version], 40
        jng      findversionloop
Loopfinished:
;--------------
;return -1 if version is greater than 40, which is
;the maximum for QR codes
;this should never happen, because of the prior checks
;regarding input length
        or       eax, - 1
        cmp      dword [version], 40
        ja       makeQR_end
;--------------
;reset data structures in case 
;they have been used previously
        cld
        mov      ecx, 5 ; 929
        xor      eax, eax
        mov      edi, datcodewords
        rep stosd
        mov      ecx, 5 ; 929
        mov      edi, ileaveddata
        rep stosd
;---------------
;add ECI mode indicator if necessary
;see page 24 of ISO/IEC 18004:2015
        cmp      dword _theecimode, 999999
        jng      @f
        mov      dword _theecimode, -1
@@:
        cmp      dword _theecimode, -1
        je       eciskip
        push     dword 4
        push     dword 7
        push     datcodewords
        call     appendtobitset
        mov      eax, dword _theecimode
        cmp      eax, 127
        jg       @f
        push     8
        and      eax, 127
        jmp      addeci
@@:
        cmp      eax, 16383
        jg       @f
        push     16
        and      eax, 16383
        or       eax, 32768
        jmp      addeci
@@:
        push     24
        and      eax, 2097151
        or       eax, 12582912
addeci:
        push     eax
        push     datcodewords
        call     appendtobitset
eciskip:
;-----------------
;add mode indicator
        mov      eax, dword [mode]
        movzx    eax, byte [modeindicators + eax]
        push     4
        push     eax
        push     datcodewords
        call     appendtobitset
;add length-of-input marker
        push     dword _charcountlength
        push     dword _theinputlength
        push     datcodewords
        call     appendtobitset
;-------------------
;prepare registers for data acquisition
        cld
        mov      esi, dword _theinput
        mov      edi, esi
        add      edi, dword _theinputlength
;-------------------
;mode 0: acquire numeric data
        cmp      dword [mode], 0
        jne      mode0loopend
        ;cld
        ;mov esi, dword _theinput
        ;mov edi, esi
        ;add edi, dword _theinputlength
mode0loop:
        mov      ebx, edi
        sub      ebx, esi
        lodsb
        sub      al, 48
        cmp      ebx, 1
        jne      @f
        push     4
        jmp      mode0next
@@:
        cmp      ebx, 2
        jne      @f
        imul     eax, eax, 10
        mov      ebx, eax
        lodsb
        sub      al, 48
        add      eax, ebx
        push     7
        jmp      mode0next
@@:
        cmp      ebx, 3
        jl       mode0next
        imul     eax, eax, 100
        mov      ebx, eax
        lodsb
        sub      al, 48
        imul     eax, eax, 10
        add      ebx,eax
        lodsb
        sub      al, 48
        add      eax, ebx
        push     10
mode0next:
        push     eax
        push     datcodewords
        call     appendtobitset
        cmp      esi, edi
        jnge     mode0loop
mode0loopend:
;-------------------
;mode 1: acquire alphanumeric data
        cmp      dword [mode], 1
        jne      skipmode1
        ;cld
        ;mov esi, dword _theinput
        ;mov edi, esi
        ;add edi, dword _theinputlength
mode1loop:
        mov      ebx, edi
        sub      ebx, esi
        lodsb
        push     eax
        call     alphaindex
        cmp      ebx, 2
        jnae     @f
        imul     eax, 45
        mov      edx, eax
        lodsb
        push     eax
        call     alphaindex
        add      eax, edx
        push     dword 11
        jmp      mode1next
@@:
        push     dword 6
mode1next:
        push     eax
        push     dword datcodewords
        call     appendtobitset
        cmp      esi, edi
        jb       mode1loop
skipmode1:
;-------------------
;mode 2: acquire binary data
        cmp      dword [mode], 2
        jne      skipmode2
        ;cld
        ;mov esi, dword _theinput
        ;mov edi, esi
        ;add edi, dword _theinputlength
mode2loop:
        lodsb
        push     dword 8
        push     eax
        push     datcodewords
;if ECI mode is not default and input contains a
;backslash, escape it in the QR code by doubling it up
        cmp      dword _theecimode, -1
        je       @f
        cmp      eax, 92
        jne      @f
        push     dword 8
        push     eax
        push     dword datcodewords
        call     appendtobitset
@@:
        call     appendtobitset
        cmp      esi, edi
        jb       mode2loop
skipmode2:
;----------------
;if number of payload bits so far is less than capacity,
;!!!  append four 0s to terminate  !!! (emphasis intentional)
;or append 0s up to full capacity, whichever happens first
        mov      eax, dword _requiredbitslength
        sub      eax, dword [datcodewords + 4]
        cmp      eax, 4
        jle      @f
        mov      al, 4
@@:
        push     eax
        push     0
        push     datcodewords
        call     appendtobitset
;if number of payload bits so far is not multiple of 8,
;append 0s until it is
        mov      al, 8
        sub      al, byte [datcodewords + 8]
        and      eax, 7
        jz       @f
        push     eax
        push     0
        push     datcodewords
        call     appendtobitset
@@:
;-------------
;add repeating padding {236, 17} if necessary to fill capacity
        mov      esi, 236
        mov      edi, 17
        push     ebp
        mov      ebp, dword _requiredbitslength
        sar      ebp, 3
        sub      ebp, dword [datcodewords]
paddingloop:
        cmp      ebp, 0
        jng      paddingloopend
        push     8
        push     esi
        push     datcodewords
        call     appendtobitset
        xchg     esi, edi
        dec      ebp
        jmp      paddingloop
paddingloopend:
        pop      ebp
;---------------------
;prepare some local variables
        call     numblocksingroup2; movzx    eax, al
        mov      dword _g2b, eax
        call     numblocksingroup1; movzx    eax, al
        mov      dword _g1b, eax
        mov      eax, dword _g1b
        add      eax, dword _g2b
        mov      dword _totalblocks, eax
        call     group1datacodewords; movzx    eax, al
        mov      dword _g1dc, eax
        call     group2datacodewords; movzx    eax, al
        mov      dword _g2dc, eax
        call     eccodewordsperblock
        mov      dword _ecwords, eax
        and      dword _wordssofar, 0
        
        push     dword _ecwords
        call     makeECgenerator
        and      dword _thisblock12, 0
processblockloop:
        mov      ebx, dword _g1dc
        mov      eax, dword _thisblock12
        cmp      eax, dword _g1b
        jl       @f
        mov      ebx, dword _g2dc
@@:
;determined how many codewords this block must contain
        mov      dword _thisblockwords10, ebx

;clear the array that is used to
;calculate error correction codewords
        cld
        mov      ecx, 76 ; 154
        xor      eax, eax
        mov      edi, message
        rep stosw

;copy this block's data to the array in reverse order
;with leading 0s required for next computation
        mov      edi, message
        add      edi, dword _ecwords
        mov      ecx, dword _thisblockwords10
        mov      esi, datcodewords + 9
        add      esi, dword _wordssofar
;also increase the counter of codewords processed so far
        add      dword _wordssofar, ecx
        dec      ecx
        add      esi, ecx
@@:
        movsb
        dec      esi
        dec      esi
        dec      ecx
        jns      @b

;----------------
        mov      eax, dword _thisblockwords10
        mov      ecx, dword _ecwords
        lea      eax, dword [ecx + eax - 1]
        mov      dword _ord20, eax
errorcorrectionloop:
        mov      eax, dword _ord20
        movzx    eax, byte [message + eax]
        test     eax, eax
;if lead exponent is 0, skip this polynomial division cycle
        jz       errorcorrectionnext

;calculate lead value for this polynomial division cycle
        mov      eax, dword _ord20
        movzx    eax, byte [message + eax]
        mov      al, byte [mod285antilogs + eax]
        mov      byte _leadalpha21, al

;perform one cycle of dividing this block's codewords by the
;EC generator polynomial, using Galois Field 256 arithmetic
        mov      ecx, dword _ord20
        mov      edi, dword _ecwords
        sub      ecx, edi
polydivisionloop:
        movzx    eax, byte [genpoly + edi]
        movzx    edx, byte _leadalpha21
        add      eax, edx
        cdq
        mov      esi, 255
        idiv     esi
        movzx    eax, byte [mod285logs + edx]
        mov      edx, edi
        movzx    ebx, byte [message + edx + ecx]
        xor      ebx, eax
        mov      byte [message + edi + ecx], bl
        dec      edi
        jns      polydivisionloop
;------------------
errorcorrectionnext:
        dec      dword _ord20
        mov      eax, dword _ord20
        cmp      eax, dword _ecwords
        jnl      errorcorrectionloop

;store error correction data
        mov      edi, dword _ecwords
        mov      esi, dword message
        dec      edi
        add      esi, edi
        std
@@:
        lodsb
        push     8
        push     eax
        push     datcodewords
        call     appendtobitset
        dec      edi
        jns      @b
        cld
;------------
        inc      dword _thisblock12
        mov      eax, dword _thisblock12
        cmp      eax, dword _totalblocks
        jnge     processblockloop
;------------
;interleave data
        xor      esi, esi
ileavegroup1loop:
        xor      edi, edi
ileavegroup1_innerloop:
        mov      ecx, edi
        sub      ecx, dword _g1b
        cmp      edi, dword _g1b
        jnle @f
        xor      ecx, ecx
@@:
        mov      eax, edi
        imul     eax, dword _g1dc
        add      eax, esi
        movzx    eax, byte [datcodewords + eax + ecx + 9]
        push     8
        push     eax
        push     ileaveddata
        call     appendtobitset
        inc      edi
        cmp      edi, dword _totalblocks
        jnge     ileavegroup1_innerloop

        inc      esi
        cmp      esi, dword _g1dc
        jnge     ileavegroup1loop


        cmp      dword _g2dc, 0
        jle      skipgroup2

;interleave data from group 2
        mov      esi, 1
group2loop:
        mov      ecx, esi
        mov      eax, dword _g1dc
        imul     eax, dword _g1b
        imul     ecx, dword _g2dc
        movzx    eax, byte [datcodewords + eax + ecx + 8]
        push     8
        push     eax
        push     ileaveddata
        call     appendtobitset
        inc      esi
        cmp      esi, dword _g2b
        jng      group2loop
skipgroup2:

;interleave error correction data
        mov      eax, dword _g1b
        mov      ecx, dword _g2b
        imul     eax, dword _g1dc
        imul     ecx, dword _g2dc
        add      eax, ecx
        push     eax; startecwords
        call     eccodewordsperblock
        push     eax; bk
        push     0; n
        push     0; i
ix EQU dword [esp]
jx EQU dword [esp + 4]
bk EQU [esp + 8]
startecwords EQU [esp + 12]
ileaveloop2:
        mov      ix, 0
ileaveloop1:
        xor      edx,edx
        mov      eax, ix
        imul     dword bk
        add      eax, startecwords
        mov      ecx, jx
        movzx    eax, byte [datcodewords + 9 + eax + ecx]
        push     8
        push     eax
        push     ileaveddata
        call     appendtobitset
        inc      ix
        mov      eax, ix
        cmp      eax, dword _totalblocks
        jnge     ileaveloop1
        inc      jx
        mov      eax, jx
        cmp      eax, bk
        jnge     ileaveloop2
        sub      esp, 16 ; discard last 4 local variables
;------------------
;if there are any unused pixels left over,
;fill the corresponding data with 0s
        mov      eax, dword _unusedbits
        sub      eax, dword [ileaveddata + 4]
        cmp      eax, 0
        jle      @f
        push     dword eax
        push     dword 0
        push     ileaveddata
        call     appendtobitset
@@:
;-----------------
;draw interleaved data,
;find which mask pattern minimizes penalty score,
;apply mask,
;return size of QR code
        call     drawdata
        call     minimalpenaltymask
        mov      dword [maskpattern], eax
        push     dword eax
        call     mask
        mov      eax, dword [size]
makeQR_end:
        pop      esi
        add      ebp, EBPNUMBER
        leave
        ret 16


drawdata:
;input: nothing
;output: matrix is modified
col EQU edx
row EQU ebx
ix EQU ecx
godown EQU edi
oddcol EQU ebp
        push     ebp
        xor      godown, godown
        xor      ix, ix
        mov      col, dword [size]
        dec      col
        mov      row, col
drawdataloop:

        mov      eax, row
        mul      byte [size]
        add      eax, col
        mov      esi, dword [matrix + eax * 4]
        cmp      esi, dword [unus]
        jne      skipdraw
        push     ebx
        push     ecx
        push     eax
        mov      eax, ix
        call     getdrawbit
        pop      ecx
        mov      dword [matrix + ecx * 4], eax
        pop      ecx
        pop      ebx
        inc      ix
skipdraw:
        mov      oddcol,col
        and      oddcol,1
        cmp      col, 6
        jge      @f
        xor      oddcol, 1
@@:
        cmp      oddcol, 1
        je       isoddcol
        dec      col
        jmp      testrow0
isoddcol:
        inc      col
        and      godown, 1
        jz       @f
        inc      row
        jmp      testrow0
@@:
        dec      row
testrow0:
        cmp      row, 0
        jge      testrowsize
        or       godown, 1
        xor      row, row
        cmp      col, 8
        jne      @f
        dec      col
@@:
        dec      col
        dec      col
testrowsize:
        cmp      row, dword [size]
        jnae     drawdataloopend
        xor      godown, godown
        mov      row, dword [size]
        dec      row
        dec      col
        dec      col
        cmp      col, 5
        jne      @f
        dec      col
@@:
        cmp      col, 6
        jne      drawdataloopend
        dec      col
        dec      col
drawdataloopend:
        cmp      ix, dword [datcodewords + 4]
        jnae     drawdataloop
        pop      ebp
        ret




mask:
;input: eax = mask type
;output: matrix is modified
        pop      ecx
        pop      eax
        push     ecx
;remove the above to change calling convention
col EQU ecx
row EQU edx
        push     eax
        push     0
push eax
        call     drawformat
        xor      col, col
maskloopcol:
        xor      row, row
masklooprow:
        mov      eax, row
        mul      cl
        push     eax ;save result: row * column
        push     edx
        cdq
        mov      ebx, 3
        idiv     ebx
        mov      eax, edx
        pop      edx
        push     eax; save result: (row * column) mod 3
        mov      eax, dword [esp + 12]
        cmp      al, 7
        jg       @f
        jmp      dword [maskbjumps + eax * 4]
@@:
        pop      ebx
        jmp      endmasks
bmask0:
        mov      ebx, row
        add      ebx, col
        and      bl, 1
        xor      ebx, 1
        jmp      bmaskdone
bmask1:
        mov      ebx, row
        and      bl, 1
        xor      ebx, 1
        jmp      bmaskdone
bmask2:
        mov      eax, col
        mov      bl, 3
        div      bl
        xor      ebx, ebx
        cmp      ah, 0
        setz     bl
        jmp      bmaskdone
bmask3:
        mov      eax, col
        add      eax, row
        mov      bl, 3
        div      bl
        xor      ebx, ebx
        cmp      ah, 0
        setz     bl
        jmp      bmaskdone
bmask4:
        mov      eax, col
        mov      ebx, 3
        div      bl
        mov      ebx, row
        shr      ebx, 1
        add      bl, al
        and      bl, 1
        xor      ebx, 1
        jmp      bmaskdone
bmask5:
        mov      eax, dword [esp]
        mov      ebx, dword [esp+4]
        and      ebx, 1
        add      ebx, eax
        setz     bl
        jmp      bmaskdone
bmask6:
        mov      eax, dword [esp]
        mov      ebx, dword [esp+4]
        and      ebx, 1
        add      ebx, eax
        and      ebx, 1
        setz     bl
        jmp      bmaskdone
bmask7:
        mov      eax, dword [esp]
        mov      ebx, row
        add      ebx, col
        and      ebx, 1
        add      ebx, eax
        and      ebx, 1
        setz     bl
bmaskdone:
        pop      eax ;discard result
        pop      eax ;discard result
        mov      dword [esp], ebx
        mov      eax, row
        imul     byte [size]
        add      eax, col
        mov      ebx, eax
        mov      eax, dword [matrix + ebx * 4]
        cmp      eax, dword [unus]
        jne      @f
        mov      eax, dword [ldat]
@@:
        and      dword [esp], 1
        jz       endflip
        cmp      eax, dword [ddat]
        jne      @f
        mov      eax, dword [ldat]
        jmp      endflip
@@:
        cmp      eax, dword [ldat]
        jne      endflip
        mov      eax, dword [ddat]
endflip:
        mov      dword [matrix + ebx * 4], eax
        inc      row
        cmp      row, dword [size]
        jnge     masklooprow
        inc      col
        cmp      col, dword [size]
        jnge     maskloopcol
endmasks:
        pop      eax
        pop      eax
        ret
maskbjumps:
        dd bmask0
        dd bmask1
        dd bmask2
        dd bmask3
        dd bmask4
        dd bmask5
        dd bmask6
        dd bmask7


getratiopenalty:
;input: nothing
;output: eax = penalty score from B/W ratio of QR code
darks EQU ebx
        xor      darks, darks
        mov      ecx, dword [size2]
countdarksloop:
        mov      eax, dword [matrix + ix * 4]
        and      eax, CMSK
        jnz      @f
        inc      darks
@@:
        dec      ecx
        jns      countdarksloop

        imul     eax, darks, 100
        mov      ecx, DWORD   [size2]
        cdq
        idiv     ecx
        sub      eax, 50
        jns      @f
        neg      eax
@@:
        xor      ecx, ecx
        cmp      eax, 5
        jle      @f
ratioloop:
        inc      ecx
        sub      eax, 5
        cmp      eax, 5
        jg       ratioloop
@@:
        imul     eax, ecx, 10
        ret


gettotalpenalty:
;input: nothing
;output: eax = total penalty score of QR code
        call     getblockpenalty
        push     eax
        call     getstraightpenalty
        push     eax
        call     getsequencepenalty
        push     eax
        call     getratiopenalty
        pop      ebx
        add      eax, ebx
        pop      ebx
        add      eax, ebx
        pop      ebx
        add      eax, ebx
        ret



getsequencepenalty:
;input: nothing
;output: eax = penalty score for rule 3 of current QR code
ix EQU ebx
penalty EQU esi
col EQU dword [esp]
row EQU dword [esp + 4]
        xor      penalty, penalty
        push     0
        push     0
seqloopA:
        mov      col, 10
seqloopB:
        xor      edi, edi
        xor      ix, ix
seqloopC:
        mov      eax, col
        sub      eax, ix
        mov      ecx, row
        imul     ecx, dword [size]
        add      eax, ecx
        mov      eax, dword [matrix + eax * 4]
        and      eax, CMSK
        cmp      eax, dword [sequence + ix * 4]
        jne      @f
        inc      edi
@@:
        cmp      eax, dword [sequence + ix * 4 + 16]
        jne      @f
        add      edi, 16
@@:
        mov      eax, col
        sub      eax, ix
        imul     eax, dword [size]
        add      eax, row
        mov      eax, dword [matrix + eax * 4]
        and      eax, CMSK
        cmp      eax, dword [sequence + ix * 4]
        jne      @f
        add      edi, 256
@@:
        cmp      eax, dword [sequence + ix * 4 + 16]
        jne      @f
        add      edi, 4096
@@:
        inc      ix
        cmp      ix, 10
        jng      seqloopC
        push     ecx
        xor      ecx, ecx
incpenaltyloop:
        mov      eax, edi
        shr      eax, cl
        and      eax, 15
        cmp      eax, 11
        jl       @f
        inc      penalty
@@:
        add      cl, 4
        cmp      cl, 12
        jle      incpenaltyloop
        pop      ecx
        inc      col
        mov      eax, col
        cmp      eax, dword [size]
        jnge     seqloopB
        inc      row
        mov      eax, row
        cmp      eax, dword [size]
        jnge     seqloopA
        pop      eax
        pop      eax
        imul     eax, penalty, 40
        ret



minimalpenaltymask:
;input: nothing
;output: al = number of mask pattern that minimizes
;             penalty score for current QR code
        mov      ecx,0
        sub      esp, 8
mpmloop:
        push     ecx
        mov      eax, ecx
        push     eax
        call     mask
        call     gettotalpenalty
        pop      ecx
        cmp      ecx, 0
        jne      @f
        mov      dword [esp], ecx
        mov      dword [esp + 4], eax
@@:
        mov      edx, dword [esp + 4]
        cmp      eax, edx
        jge      @f
        mov      dword [esp + 4], eax
        mov      dword [esp], ecx
@@:
        push     ecx
        mov      eax, ecx
        push     eax
        call     mask
        pop      ecx
        inc      ecx
        cmp      ecx, 8
        jl       mpmloop
        pop      eax
        pop      edx
        ret


reserveareas:
;input: nothing
;output: some parts of matrix are marked as reserved
;        so the data drawing algorithm will skip them
        push     dword [resr]
        push     9
        push     0
        push     0
        call     fillbox
        push     dword [resr]
        push     8
        mov      eax, dword [size]
        sub      eax, 8
        push     eax
        push     1
        call     fillbox
        push     dword [resr]
        push     8
        push     1
        mov      eax, dword [size]
        sub      eax, 8
        push     eax
        call     fillbox
        cmp      dword [version], 7
        jl       @f
        push     dword [resr]
        push     3
        mov      eax, dword [size]
        sub      eax, 11
        push     eax
        push     0
        call     fillbox
        push     dword [resr]
        push     3
        mov      eax, dword [size]
        sub      eax, 11
        push     eax
        push     3
        call     fillbox
        push     dword [resr]
        push     3
        push     0
        mov      eax, dword [size]
        sub      eax, 11
        push     eax
        call     fillbox
        push     dword [resr]
        push     3
        push     3
        mov      eax, dword [size]
        sub      eax, 11
        push     eax
        call     fillbox
@@:
        ret


drawformat:
;input: eax = type of mask
;output: formatstring array is modified
;        format info is drawn in matrix
;uses eax ebx ecx edx esi
        pop      ecx
        pop      eax
        push     ecx
ix EQU ecx
        xor      ix, ix
        mov      ebx, dword [eclevel]
        shl      ebx, 3
        add      eax, ebx
dfloop1:
        mov      ebx, 1
        shl      ebx, cl
        movzx    edx, word [formatstrings + eax * 2]
        mov      esi, dword [dark]
        and      ebx, edx
        jnz      @f
        mov      esi, dword [lite]
@@:
        mov      ebx, 14
        sub      ebx, ix
        mov      dword [formatstring + ebx * 4], esi
        inc      ix
        cmp      ix, 14
        jle      dfloop1

row EQU edx
        xor      ix, ix
        mov      row, dword [size]
dfloop2:
        dec      row
        mov      eax, row
        imul     byte [size]
        mov      ebx, dword [formatstring + ix * 4]
        inc      ix
        mov      dword [matrix + 32 + eax * 4], ebx
        cmp      row, 7
        jne      @f
        dec      row
@@:
        mov      ebx, dword [size]
        sub      ebx, 7
        cmp      row, ebx
        jne      @f
        mov      row, 9
@@:
        cmp      row, 0
        jge      dfloop2

col EQU edx
        xor      ix, ix
        xor      col, col
        mov      eax, dword [size]
        shl      eax, 5
dfloop3:
        mov      ebx, dword [formatstring + ix * 4]
        inc      ix
        mov      dword [matrix  + eax + col * 4], ebx
        cmp      col, 5
        jne      @f
        inc      col
@@:
        cmp      col, 7
        jne      @f
        mov      col, dword [size]
        sub      col, 9
@@:
        inc      col
        cmp      col, dword [size]
        jnge     dfloop3
        ret


drawtiming:
;input: nothing
;output: timing pattern is drawn in matrix
        mov      eax, 8
        mov      ebx, dword [size]
        mov      esi, ebx
        sub      ebx, eax
dtloop:
        mov      ecx, dword [dark]
        test     eax, 1
        jz       @f
        mov      ecx, dword [lite]
@@:
        mov      edx, 6
        imul     edx, esi
        add      edx, eax
        mov      [matrix + edx * 4], ecx
        mov      edx, eax
        imul     edx, esi
        add      edx, 6
        mov      [matrix + edx * 4], ecx
        inc      eax
        cmp      eax, ebx
        jl       dtloop
        ret


fillbox:
;input: 4 dwords in the stack: startx, starty, boxsize, content
;output: a filled box is drawn in matrix
        pop      esi; return address
        pop      edi
        pop      eax
        pop      ecx
        pop      edx
        push     esi; save return address
        push     ebp; save previous base pointer
        mov      ebp, edi
        shl      ebp, 2
        mov      esi, eax
        add      edi, ecx
        shl      edi, 2
        add      esi, ecx
        movzx    ecx, byte [size]
        mul      cl
        imul     esi, ecx
fbloop:
        mov      ebx, ebp
@@:
        mov      [matrix + ebx + eax * 4], edx
        add      ebx, 4
        cmp      edi, ebx
        jg       @b
        add      eax, ecx
        cmp      esi, eax
        jg       fbloop
        pop      ebp; restore previous base pointer
        ret


numblocksingroup2:
;input: nothing
;output: eax = number of blocks in group 2 of QR code
        mov      edx, 2
        jmp      lbt
group1datacodewords:
;input: nothing
;output: eax = number of data codewords in group 1 of QR code
        mov      edx, 1
        jmp      lbt
numblocksingroup1:
;input: nothing
;output: eax = number of blocks in group 1 of QR code
        mov      edx, 0
lbt:
        mov      eax, dword [version]
        dec      eax
        imul     eax, eax, 12
        imul     ecx, dword [eclevel], 3
        add      eax, edx
        movzx    eax, byte [bigtable + ecx + eax]
        ret


group2datacodewords:
;input: nothing
;output: eax = number of data codewords in group 2 of QR code
        call     numblocksingroup2
        test     eax, eax
        jz       @f
        call     group1datacodewords
        inc      eax
@@:
        ret


eccodewordsperblock:
;input: nothing
;output: eax = number of EC codewords per block
        call     numdatacodewords
        mov      esi, DWORD [maxcodewords]
        sub      esi, eax
        call     numblocksingroup1
        mov      edi, eax
        call     numblocksingroup2
        add      edi, eax
        mov      eax, esi
        cdq
        idiv     edi
        ret


numdatacodewords:
;input: nothing
;output: eax = number of data codewords in QR code
        call     group1datacodewords
        mov      esi, eax
        call     numblocksingroup1
        imul     esi, eax
        call     group2datacodewords
        mov      edi, eax
        call     numblocksingroup2
        imul     edi, eax
        add      esi, edi
        mov      eax, esi
        ret

alphaindex:
;input: al
;output: eax = index of input in alphamode table, or 46 if not found
        pop      ecx
        pop      eax
        push     ecx
        xor      ecx, ecx
@@:
        cmp      [alphamode + ecx], al
        je       @f
        inc      cl
        cmp      cl, 46
        jl       @b
@@:
        mov      eax, ecx
        ret


mem7 db 7

drawalign:
;input: nothing
;output: alignment patterns (small squares) are drawn in matrix
        mov      eax, dword [version]
        cmp      al, 1
        jle      drawalign_end
        push     eax
        shl      al, 2
        add      al, 10
        mov      byte [places + 1], al
        pop      eax
        mov      ebx, eax
        idiv     byte [mem7]
        and      eax, 255
        cmp      al, 0
        je       startloop
        cmp      al, 1
        jg       case2
        inc      bl
        jmp      @f
case2:
        cmp      al, 2
        jg       case3
        sub      bl, 4
        jmp      @f
case3:
        cmp      al, 3
        jg       case4
        sub      bl, 10
@@:
        shl      bl, 1
        jmp      da_endcases
case4:
        cmp      al, 4
        jg       case5
        sub      bl, 28
        jmp      @f
case5:
        sub      bl, 34
@@:
        mov      bl, byte [diffs + ebx]
da_endcases:
        mov      ecx, 1
@@:
        mov      dl, byte [places + ecx]
        sub      dl, bl
        mov      byte [places + 1 + ecx], dl
        inc      ecx
        cmp      ecx, eax
        jle      @b
startloop:
        xor      ecx, ecx
da_loop1:
        xor      edx, edx
da_loop2:
        cmp      cl, 0
        jne      @f
        cmp      dl, 1
        jle      da_continue
@@:
        cmp      cl, 1
        jne      @f
        cmp      dl, 0
        je       da_continue
@@:
        push     edx
        push     ecx
        push     dword [dark]
        push     1
        movzx    edx, byte [places + edx]
        push     edx
        movzx    ecx, byte [places + ecx]
        push     ecx
        dec      edx
        dec      ecx
        push     dword [lite]
        push     3
        push     edx
        push     ecx
        dec      ecx
        dec      edx
        push     dword [dark]
        push     5
        push     edx
        push     ecx
        call     fillbox
        call     fillbox
        call     fillbox
        pop      ecx
        pop      edx
da_continue:
        inc      dl
        cmp      dl, 7
        jge      @f
        cmp      byte [places + edx], 0
        je       @f
        jmp      da_loop2
@@:
        inc      cl
        cmp      cl, 7
        jge      @f
        cmp      byte [places + ecx], 0
        je       @f
        jmp      da_loop1
@@:
drawalign_end:
        ret

drawdarkmodule:
;input: nothing
;output: fixed dark module is drawn in matrix
        push     dword [dark]
        push     dword 1
        mov      eax, dword [size]
        sub      eax, 8
        push     eax
        push     dword 8
        call     fillbox
        ret

getcharcountlength:
;input: nothing
;output: eax = length of character count marker, in bits
        mov      eax, [mode]
        xor      ebx, ebx
        cmp      [version], 9
        setg     bl
        cmp      [version], 26
        setg     cl
        add      bl, cl
        shl      bl, 2
        add      ebx, charcountlengths
        xlatb
        ret


appendtobitset:
;input: ebx = pointer to bitset
;       eax = data to add
;       ecx = number of low-order bits from eax to add
;output: bitset is modified
        pop      ecx
        pop      ebx
        pop      eax
        xchg     ecx, dword [esp]
;remove the above to change calling convention
mbs EQU ebx
numbits EQu ecx
        push     ebp
        dec      numbits
appendloop:
        mov      edx, eax
        shr      edx, cl
        push     numbits
        mov      ecx, 7
        sub      cl, byte [mbs + 8]
        mov      ebp, 1
        shl      ebp, cl
        mov      ecx, dword [mbs]
        add      ecx, 9
        xchg     ebp, ecx
        and      edx, 1
        jz       @f
        or       byte [mbs + ebp], cl
        jmp      appenddone
@@:
        not      ecx
        and      byte [mbs + ebp], cl
appenddone:
        pop      numbits
        inc      dword [mbs + 4]
        inc      byte [mbs + 8]
        and      byte [mbs + 8], 7
        jnz      @f
        inc      dword [mbs]
@@:
        dec      numbits
        cmp      numbits, 0
        jge      appendloop
        pop      ebp
        ret


drawfinders:
;input: nothing
;output: finder patterns (large squares) are drawn in matrix
        push     dword [lite]
        push     8
        push     0
        push     0
        call     fillbox
        push     dword [dark]
        push     7
        push     0
        push     0
        call     fillbox
        push     dword [lite]
        push     5
        push     1
        push     1
        call     fillbox
        push     dword [dark]
        push     3
        push     2
        push     2
        call     fillbox
        push     dword [lite]
        push     8
        mov      eax, dword [size]
        sub      eax, 8
        push     eax
        push     0
        call     fillbox
        push     dword [dark]
        push     7
        mov      eax, dword [size]
        sub      eax, 7
        push     eax
        push     0
        call     fillbox
        push     dword [lite]
        push     5
        mov      eax, dword [size]
        sub      eax, 6
        push     eax
        push     1
        call     fillbox
        push     dword [dark]
        push     3
        mov      eax, dword [size]
        sub      eax, 5
        push     eax
        push     2
        call     fillbox
        push     dword [lite]
        push     8
        push     0
        mov      eax, dword [size]
        sub      eax, 8
        push     eax
        call     fillbox
        push     dword [dark]
        push     7
        push     0
        mov      eax, dword [size]
        sub      eax, 7
        push     eax
        call     fillbox
        push     dword [lite]
        push     5
        push     1
        mov      eax, dword [size]
        sub      eax, 6
        push     eax
        call     fillbox
        push     dword [dark]
        push     3
        push     2
        mov      eax, dword [size]
        sub      eax, 5
        push     eax
        call     fillbox
        ret


getdrawbit:
;input: eax = index of desired bit from ileaveddata
;output: eax = [ddat] if bit is 1, [ldat] otherwise
;pop ebx
;pop eax
;pop eax
;push ebx
;remove the above to change calling convention
        mov      ebx, eax
        shr      eax, 3
        and      ebx, 7
        mov      ecx, 7
        sub      ecx, ebx
        mov      bl, byte [ileaveddata + 9 + eax]
        shr      bx, cl
        mov      eax, dword [ddat]
        and      ebx, 1
        jnz      @f
        mov      eax, dword [ldat]
@@:
        ret

drawversion:
;input: nothing
;output: version info is drawn in matrix
        mov      ebx, dword [version]
        cmp      bx, 7
        jnge     drawversion_end
        sub      bx, 7
        mov      ebx, dword [versionstrings + ebx * 4]
        xor      ecx, ecx
drawverloop:
        mov      eax, 1
        shl      ax, cl
        and      ax, bx
        mov      eax, dword [dark]
        jnz      @f
        mov      eax, dword [lite]
@@:
        mov      dword [versionstring + ecx * 4], eax
        inc      ecx
        cmp      ecx, 17
        jng      drawverloop
        xor      ecx, ecx
        xor      ebx, ebx   ;col
        mov      edx, dword [size]
        sub      dl, 8
        push     edx
putversion1:
        mov      edx, dword [size]
        sub      edx, 11    ;row
putversion2:
        mov      eax, edx
        mul      byte [size]
        add      eax, ebx
        push     ebx
        mov      ebx, dword [versionstring + ecx * 4]
        mov      dword [matrix + eax * 4], ebx
        pop      ebx
        push     edx
        mov      eax, ebx
        mul      byte [size]
        add      eax, edx
        mov      edx, dword [versionstring + ecx * 4]
        mov      dword [matrix + eax * 4], edx
        inc      cx
        pop      edx
        inc      edx
        cmp      edx, dword [esp]
        jl       putversion2
        inc      bx
        cmp      bx, 6
        jl       putversion1
        pop      edx
drawversion_end:
        ret

makeECgenerator:
;input: eax = desired order of EC generator polynomial
;output: genpoly array is modified
        pop      ecx
        pop      eax
        push     ecx
;remove the above to change calling convention
        mov      [genpoly], 0
        push     eax
        cld
        mov      ecx, 31
        mov      edi, genpoly
        xor      eax, eax
        rep stosb
        mov      ecx, 2
ecgenloop:
        push     ecx
        mov      eax, ecx
        dec      eax
        push     eax
        std
        mov      esi, genpoly - 1
        add      esi, ecx
        mov      edi, esi
        inc      edi
        rep movsb
        mov      al, [genpoly]
        add      eax, [esp]
        cmp      eax, 255
        jnge     @f
        sub      eax, 255
        cmp      eax, 255
        jnge     @f
        sub      eax, 255
@@:
        mov      [genpoly], al
        inc      cl
xorloop:
        mov      ebx, genpoly
        mov      eax, ecx
        xlatb
        mov      dh, al
        mov      al, cl
        inc      eax
        xlatb
        add      eax, [esp]
        cmp      eax, 255
        jnge     @f
        sub      eax, 255
        cmp      eax, 255
        jnge     @f
        sub      eax, 255
@@:
        mov      ebx, mod285logs
        xlatb
        mov      dl, al
        mov      al, dh
        xlatb
        xor      al, dl
        mov      ebx, mod285antilogs
        xlatb
        mov      byte [genpoly + ecx], al
        inc      cl
        cmp      ecx, dword [esp + 4]
        jl       xorloop
        pop      ecx
        pop      ecx
        inc      cl
        cmp      cl, byte [esp]
        jle      ecgenloop
        pop      eax
end2:
        cld
        ret

getblockpenalty:
;input: nothing
;output: eax = penalty score from squares
        mov      esi, 0
        mov      ebx, [size]
        dec      bl
        dec      bl
sqloop2:
        mov      ecx, [size]
        dec      cl
        dec      cl
sqloop:
        mov      eax, ecx
        imul     byte [size]
        add      eax, ebx
        mov      edx, [matrix + eax * 4]
        and      edx, CMSK
        mov      edi, edx
        mov      edx, [matrix + 4 + eax * 4]
        and      edx, CMSK
        cmp      edx, edi
        jne      @f
        add      eax, [size]
        mov      edx, [matrix + eax * 4]
        and      edx, CMSK
        cmp      edx, edi
        jne      @f
        mov      edx, [matrix + 4 + eax * 4]
        and      edx, CMSK
        cmp      edx, edi
        jne      @f
        add      esi, 3
@@:
        dec      ecx
        cmp      ecx, 0
        jge      sqloop
        dec      ebx
        cmp      ebx, 0
        jge      sqloop2
        mov      eax, esi
        ret

getstraightpenalty:
;input: nothing
;output: eax = penalty score of qr code for
;        straight runs of identical pixels
        mov      esi, 0 ; penalty
        mov      edi, 3 ; flags 1=new row, 2= new column
        mov      ecx, dword [size]
        sub      ecx, 5
gsploop2:
        mov      ebx, dword [size]
        sub      ebx, 5
gsploop1:
        mov      eax, ecx
        imul     byte [size]
        add      eax, ebx
        push     ecx
        mov      edx, dword [matrix + eax * 4]
        and      edx, CMSK
        mov      ecx, dword [matrix + 4 + eax * 4]
        and      ecx, CMSK
        cmp      edx, ecx
        jne      nostraightrow
        mov      ecx, dword [matrix + 8 + eax * 4]
        and      ecx, CMSK
        cmp      edx, ecx
        jne      nostraightrow
        mov      ecx, dword [matrix + 12 + eax * 4]
        and      ecx, CMSK
        cmp      edx, ecx
        jne      nostraightrow
        mov      ecx, dword [matrix + 16 + eax * 4]
        and      ecx, CMSK
        cmp      edx, ecx
        jne      nostraightrow
        inc      esi
        test     edi, 1
        jz       @f
        inc      esi
        inc      esi
@@:
        and      edi, 2
        jmp      @f
nostraightrow:
        or       edi, 1
@@:
        pop      ecx
        mov      eax, ebx
        imul     byte [size]
        add      eax, ecx
        push     ecx
        mov      edx, dword [matrix + eax * 4]
        and      edx, CMSK
        add      eax, dword [size]
        mov      ecx, dword [matrix + eax * 4]
        and      ecx, CMSK
        cmp      edx, ecx
        jne      nostraightcol
        add      eax, dword [size]
        mov      ecx, dword [matrix + eax * 4]
        and      ecx, CMSK
        cmp      edx, ecx
        jne      nostraightcol
        add      eax, dword [size]
        mov      ecx, dword [matrix + eax * 4]
        and      ecx, CMSK
        cmp      edx, ecx
        jne      nostraightcol
        add      eax, dword [size]
        mov      ecx, dword [matrix + eax * 4]
        and      ecx, CMSK
        cmp      edx, ecx
        jne      nostraightcol
        inc      esi
        test     edi, 2
        jz       @f
        inc      esi
        inc      esi
@@:
        and      edi, 1
        jmp      @f
nostraightcol:
        or       edi, 2
@@:
        pop      ecx
        dec      ebx
        cmp      ebx, 0
        jge      gsploop1
        mov      edi, 3
        dec      ecx
        cmp      ecx, 0
        jge      gsploop2
        mov      eax, esi
        ret


kolibrios_drawtowindow:
;input: nothing
        call     reducematrix
        mov      eax, 7
        mov      ebx, matrix
        mov      ecx, [size]
        shl      ecx, 16
        add      ecx, [size]
        mov      edx, 40
        shl      edx, 16
        add      edx, 80
        int      0x40
        ret


reducematrix:
;input: nothing
;reduces matrix from rgba to 3-byte rgb
;for use in KolibriOS system function 7
        mov      ecx, [size2]
        mov      eax, matrix + 3
        mov      ebx, matrix + 4
        xor      edx, edx
@@:
        mov      dl, [ebx]
        mov      [eax], dl
        inc      eax
        inc      ebx
        mov      dl, [ebx]
        mov      [eax], dl
        inc      eax
        inc      ebx
        mov      dl, [ebx]
        mov      [eax], dl
        inc      eax
        inc      ebx
        inc      ebx
        dec      ecx
        jnz      @b
        ret


versionstrings dd 0x7C94, 0x85BC, 0x9A99, 0xA4D3, 0xBBF6
        dd 0x0C762, 0x0D847, 0x0E60D, 0x0F928, 0x10B78, 0x1145D
        dd 0x12A17, 0x13532, 0x149A6, 0x15683, 0x168C9, 0x177EC
        dd 0x18EC4, 0x191E1, 0x1AFAB, 0x1B08E, 0x1CC1A, 0x1D33F
        dd 0x1ED75, 0x1F250, 0x209D5, 0x216F0, 0x228BA, 0x2379F
        dd 0x24B0B, 0x2542E, 0x26A64, 0x27541, 0x28C69

formatstrings dw 0x77C4, 0x72F3, 0x7DAA, 0x789D, 0x662F
        dw 0x6318, 0x6C41, 0x6976, 0x5412, 0x5125, 0x5E7C 
        dw 0x5B4B, 0x45F9, 0x40CE, 0x4F97, 0x4AA0, 0x355F 
        dw 0x3068, 0x3F31, 0x3A06, 0x24B4, 0x2183, 0x2EDA
        dw 0x2BED, 0x1689, 0x13BE, 0x1CE7, 0x19D0, 0x762
        dw 0x255, 0xD0C, 0x83B

mod285logs db 1, 2, 4, 8, 16, 32, 64, 128, 29, 58, 116, 232
        db 205, 135, 19, 38, 76, 152, 45, 90, 180, 117, 234, 201
        db 143, 3, 6, 12, 24, 48, 96, 192, 157, 39, 78, 156, 37
        db 74, 148, 53, 106, 212, 181, 119, 238, 193, 159, 35
        db 70, 140, 5, 10, 20, 40, 80, 160, 93, 186, 105, 210
        db 185, 111, 222, 161, 95, 190, 97, 194, 153, 47, 94
        db 188, 101, 202, 137, 15, 30, 60, 120, 240, 253, 231
        db 211, 187, 107, 214, 177, 127, 254, 225, 223, 163, 91
        db 182, 113, 226, 217, 175, 67, 134, 17, 34, 68, 136, 13
        db 26, 52, 104, 208, 189, 103, 206, 129, 31, 62, 124, 248
        db 237, 199, 147, 59, 118, 236, 197, 151, 51, 102, 204
        db 133, 23, 46, 92, 184, 109, 218, 169, 79, 158, 33, 66
        db 132, 21, 42, 84, 168, 77, 154, 41, 82, 164, 85, 170
        db 73, 146, 57, 114, 228, 213, 183, 115, 230, 209, 191
        db 99, 198, 145, 63, 126, 252, 229, 215, 179, 123, 246
        db 241, 255, 227, 219, 171, 75, 150, 49, 98, 196, 149
        db 55, 110, 220, 165, 87, 174, 65, 130, 25, 50, 100, 200
        db 141, 7, 14, 28, 56, 112, 224, 221, 167, 83, 166, 81
        db 162, 89, 178, 121, 242, 249, 239, 195, 155, 43, 86
        db 172, 69, 138, 9, 18, 36, 72, 144, 61, 122, 244, 245
        db 247, 243, 251, 235, 203, 139, 11, 22, 44, 88, 176, 125
        db 250, 233, 207, 131, 27, 54, 108, 216, 173, 71, 142, 1

mod285antilogs db 0, 0, 1, 25, 2, 50, 26, 198, 3, 223, 51, 238
        db 27, 104, 199, 75, 4, 100, 224, 14, 52
        db 141, 239, 129, 28, 193, 105, 248, 200, 8, 76, 113, 5
        db 138, 101, 47, 225, 36, 15, 33, 53, 147, 142, 218, 240
        db 18, 130, 69, 29, 181, 194, 125, 106, 39, 249, 185, 201
        db 154, 9, 120, 77, 228, 114, 166, 6, 191, 139, 98, 102, 221
        db 48, 253, 226, 152, 37, 179, 16, 145, 34, 136, 54, 208
        db 148, 206, 143, 150, 219, 189, 241, 210, 19, 92, 131, 56
        db 70, 64, 30, 66, 182, 163, 195, 72, 126, 110, 107, 58, 40
        db 84, 250, 133, 186, 61, 202, 94, 155, 159, 10, 21, 121
        db 43, 78, 212, 229, 172, 115, 243, 167, 87, 7, 112, 192
        db 247, 140, 128, 99, 13, 103, 74, 222, 237, 49, 197, 254, 24
        db 227, 165, 153, 119, 38, 184, 180, 124, 17, 68, 146, 217
        db 35, 32, 137, 46, 55, 63, 209, 91, 149, 188, 207, 205, 144
        db 135, 151, 178, 220, 252, 190, 97, 242, 86, 211, 171, 20
        db 42, 93, 158, 132, 60, 57, 83, 71, 109, 65, 162, 31, 45, 67
        db 216, 183, 123, 164, 118, 196, 23, 73, 236, 127, 12, 111
        db 246, 108, 161, 59, 82, 41, 157, 85, 170, 251, 96, 134, 177
        db 187, 204, 62, 90, 203, 89, 95, 176, 156, 169, 160, 81, 11
        db 245, 22, 235, 122, 117, 44, 215, 79, 174, 213, 233, 230
        db 231, 173, 232, 116, 214, 244, 234, 168, 80, 88, 175

;bigtable (blocks by version and ec level)
;rowmajor - version
;rowminor - eclevel
;columns: number of blocks in group 1,
;         number of data codewords in each of group 1's blocks,
;         number of blocks in group 2,
bigtable db 1,19,0,1,16,0,1,13,0,1,9,0,1,34,0,1,28,0,1,22,0
        db 1,16,0,1,55,0,1,44,0,2,17,0,2,13,0,1,80
        db 0,2,32,0,2,24,0,4,9,0,1,108,0,2,43,0,2,15,2,2,11
        db 2,2,68,0,4,27,0,4,19,0,4,15,0,2,78,0,4
        db 31,0,2,14,4,4,13,1,2,97,0,2,38,2,4,18,2,4,14,2,2
        db 116,0,3,36,2,4,16,4,4,12,4,2,68,2,4,43
        db 1,6,19,2,6,15,2,4,81,0,1,50,4,4,22,4,3,12,8,2,92
        db 2,6,36,2,4,20,6,7,14,4,4,107,0,8,37,1,8
        db 20,4,12,11,4,3,115,1,4,40,5,11,16,5,11,12,5,5,87
        db 1,5,41,5,5,24,7,11,12,7,5,98,1,7,45,3
        db 15,19,2,3,15,13,1,107,5,10,46,1,1,22,15,2,14,17
        db 5,120,1,9,43,4,17,22,1,2,14,19,3,113,4,3
        db 44,11,17,21,4,9,13,16,3,107,5,3,41,13,15,24,5,15
        db 15,10,4,116,4,17,42,0,17,22,6,19,16,6,2
        db 111,7,17,46,0,7,24,16,34,13,0,4,121,5,4,47,14,11
        db 24,14,16,15,14,6,117,4,6,45,14,11,24,16
        db 30,16,2,8,106,4,8,47,13,7,24,22,22,15,13,10,114
        db 2,19,46,4,28,22,6,33,16,4,8,122,4,22,45
        db 3,8,23,26,12,15,28,3,117,10,3,45,23,4,24,31,11
        db 15,31,7,116,7,21,45,7,1,23,37,19,15,26,5
        db 115,10,19,47,10,15,24,25,23,15,25,13,115,3,2,46
        db 29,42,24,1,23,15,28,17,115,0,10,46,23,10
        db 24,35,19,15,35,17,115,1,14,46,21,29,24,19,11,15
        db 46,13,115,6,14,46,23,44,24,7,59,16,1,12
        db 121,7,12,47,26,39,24,14,22,15,41,6,121,14,6,47
        db 34,46,24,10,2,15,64,17,122,4,29,46,14,49
        db 24,10,24,15,46,4,122,18,13,46,32,48,24,14,42,15
        db 32,20,117,4,40,47,7,43,24,22,10,15,67,19
        db 118,6,18,47,31,34,24,34,20,15,61

charcountlengths db 10,9,8,8,12,11,16,10,14,13,16,12

modeindicators db 1,2,4,8,7

diffs db 24,24,26,26,26,28,28

alphamode db '0123456789ABCDEFGHIJKL'
        db 'MNOPQRSTUVWXYZ $%*+-./:', 0
            
DARK equ 0
CMSK equ 0FFFF00H

;1 dark, 1 light, 3 dark, 1 light, 1 dark
;preceded or followed by 4 light
sequence dd CMSK, CMSK, CMSK, CMSK
             dd DARK, CMSK, DARK, DARK, DARK, DARK
             dd CMSK, DARK, CMSK, CMSK, CMSK, CMSK


end if



;List of (some) ECI assignment numbers

;number Code page or encoding   
;0      Code page 437   
;1      ISO/IEC 8859-1          Latin-1
;2      Code page 437   
;3      ISO/IEC 8859-1          Latin-1
;4      ISO/IEC 8859-2          Latin-2
;5      ISO/IEC 8859-3          Latin-3
;6      ISO/IEC 8859-4          Latin-4
;7      ISO/IEC 8859-5          Latin/Cyrillic
;20     Shift JIS
;22     Windows-1251            Latin/Cyrillic
;25     UTF-16                  Big endian
;26     UTF-8
;27     ISO/IEC 646:1991 IRV    US-ASCII
;28     Big5
;29     GB/T 2312
;30     KS X 1001
;31     GBK
;32     GB 18030
;33     UTF-16                  Little endian
;34     UTF-32                  Big endian
;35     UTF-32                  Little endian
;899    8-bit binary data