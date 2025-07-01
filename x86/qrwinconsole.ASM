;how to use:
;make sure qrcodes.inc is in the same folder
;as this file, then compile this file, then
;call the executable from the command line
;giving 1 argument, a file name
;and optionally a 2nd argument, a number

;the contents of the file will be encoded
;as a QR code and the code will be rendered
;in the console

;if the number argument is 0 or greater,
;it is used as the ECI indicator number
;to indicate the encoding of the input file
;and of the QR code




format PE console ;GUI 4.0
entry start

include 'win32a.inc'


section '.data' data readable writeable


    hStdOut         dd ?
    singleChar      db 'A'
    charsWritten    dd ?


  argc        dd 0
  argv        dd 0
    filenameptr dd 0
    hFile       dd ?
    bytesRead   dd ?
    buffer_size = 8000
    buffer      rb buffer_size


  int_value    dd -1

qrcodes = 1
include 'qrcodes.inc'


section '.code' code readable executable


start:

    ; Get handle to STD_OUTPUT_HANDLE
    push -11
    call    [GetStdHandle]
    mov     [hStdOut], eax

    invoke  GetCommandLineW
    mov esi, eax
    push    argc
    push    esi
    call    [CommandLineToArgvW]
    mov     [argv], eax
    mov     ebx, [argc]
    cmp ebx, 2
    jge @f
    push    0
    call    [ExitProcess]
@@:
    mov     esi, [argv]
    mov     eax, [esi + 4]
    mov dword [filenameptr], eax
    cmp ebx, 3
    jnge nonumber
    mov     esi, [argv]
    mov     eax, [esi + 8]
    push    10                ; base = 10
    push    0                 ; endptr = NULL
    push    eax               ; wide string to convert
    call    [wcstol]
    cmp eax, -1
    jng @f
    mov     [int_value], eax ; store result
@:
nonumber:



    ; HANDLE hFile = CreateFileA("input.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    push    0                      ; hTemplateFile
    push    FILE_ATTRIBUTE_NORMAL ; dwFlagsAndAttributes
    push    OPEN_EXISTING         ; dwCreationDisposition
    push    0                      ; lpSecurityAttributes
    push    FILE_SHARE_READ       ; dwShareMode
    push    GENERIC_READ          ; dwDesiredAccess
    push    dword [filenameptr] ;filename               ; lpFileName
    call    [CreateFileW]
    cmp     eax, INVALID_HANDLE_VALUE
    je      error
    mov     [hFile], eax
    ; ReadFile(hFile, buffer, buffer_size, &bytesRead, NULL);
    push    0               ; lpOverlapped
    push    bytesRead       ; lpNumberOfBytesRead
    push    buffer_size     ; nNumberOfBytesToRead
    push    buffer          ; lpBuffer
    push    [hFile]         ; hFile
    call    [ReadFile]
    test    eax, eax
    jz      error
    ; CloseHandle(hFile)
    push    [hFile]
    call    [CloseHandle]


    push -1
    push dword [int_value]
    push dword [bytesRead]
    push buffer
    call makeQR
    cmp eax, -1
    je error
    ;makeQR succeeded, EAX = size, matrix=RGBA bitmap
    push eax
    mov eax, [esp]
    call blankline
    mov eax, [esp]
    call blankline
    mov eax, [esp]
    call blankline
    mov eax, [esp]
    call blankline
    mov edx, [esp]
    push dword [size]
    push 0
outputloop1:
    mov byte [singleChar],"#"
    call writechar
    call writechar
    call writechar
    call writechar
outputloop2:
    mov ecx, [esp]
    mov byte [singleChar],"#"
    cmp dword [matrix + ecx * 4], 5
    ja @f
    mov byte [singleChar]," "
    @@:
    call writechar
    inc dword [esp]
    mov ecx, [esp]
    cmp ecx, [esp+4]
    jl outputloop2

    mov byte [singleChar],"#"
    call writechar
    call writechar
    call writechar
    call writechar
    mov byte [singleChar],10
    call writechar

    mov eax, [size]
    add dword [esp + 4], eax
    mov eax, [esp + 4]
    cmp eax, [size2]
    jle outputloop1
    pop eax
    pop eax
    mov eax, [esp]
    call blankline
    mov eax, [esp]
    call blankline
    mov eax, [esp]
    call blankline
    mov eax, [esp]
    call blankline
    pop eax

    ; ExitProcess(0)
    push    0
    call    [ExitProcess]

error:
    ; ExitProcess(1)
    push    1
    call    [ExitProcess]

writechar:
    ; WriteConsoleA(hStdOut, &singleChar, 1, &charsWritten, NULL)
    push eax
    push    0               ; lpReserved
    push    charsWritten    ; lpNumberOfCharsWritten
    push    1               ; nNumberOfCharsToWrite
    push    singleChar      ; lpBuffer
    push    [hStdOut]       ; hConsoleOutput
    call    [WriteConsoleA]
    pop eax
    ret
blankline:
    add eax, 7
    mov byte [singleChar],"#"
    blankloop:
    call writechar
    dec eax
    cmp eax, 0
    jge blankloop
    mov byte [singleChar],10
    call writechar
    ret


qrcodes = 0
include 'qrcodes.inc'


section '.idata' import data readable writeable

        library kernel32, 'kernel32.dll', \
                shell32,  'shell32.dll', \
                msvcrt,   'msvcrt.dll'

        import kernel32, \
            GetStdHandle, 'GetStdHandle', \
            WriteConsoleA, 'WriteConsoleA', \
            CreateFileW, 'CreateFileW', \
            ReadFile, 'ReadFile', \
            CloseHandle, 'CloseHandle', \
            GetCommandLineW, 'GetCommandLineW', \
            ExitProcess, 'ExitProcess'

        import  kernel,\
                GetModuleHandle,'GetModuleHandleA',\
                ExitProcess,'ExitProcess'

        import shell32, \
               CommandLineToArgvW, 'CommandLineToArgvW'

        import msvcrt, \
               wcstol, 'wcstol'