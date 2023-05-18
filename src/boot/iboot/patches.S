.global _main
.text

_main:
    mov x8, 64
    mov x2, msgLen
    adr x1, msg
    mov x0, 1
    svc 0

    mov x8, 93
    svc 0

.data

msg: .ascii "Hello, world\n"
msgLen = . - msg