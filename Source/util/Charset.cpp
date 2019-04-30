#include "Charset.h"

bool IsCSourceChar(int c)
{
    static bool isCSourceChar[] = {
        true, // \0 - NUL (null)
        false, // \1 - SOH (start of heading)
        false, // \2 - STX (start of text)
        false, // \3 - ETX (end of text)
        false, // \4 - EOT (end of transmission)
        false, // \5 - ENQ (enquiry)
        false, // \6 - ACK (acknowledge)
        false, // \7 - BEL (bell)
        false, // \8 - BS  (backspace)
        true, // \9 - TAB (horizontal tab)
        true, // \10 - LF  (NL line feed, new line)
        false, // \11 - VT  (vertical tab)
        false, // \12 - FF  (NP form feed, new page)
        true, // \13 - CR  (carriage return)
        false, // \14 - SO  (shift out)
        false, // \15 - SI  (shift in)
        false, // \16 - DLE (data link escape)
        false, // \17 - DC1 (device control 1)
        false, // \18 - DC2 (device control 2)
        false, // \19 - DC3 (device control 3)
        false, // \20 - DC4 (device control 4)
        false, // \21 - NAK (negative acknowledge)
        false, // \22 - SYN (synchronous idle)
        false, // \23 - ETB (end of trans. block)
        false, // \24 - CAN (cancel)
        false, // \25 - EM  (end of medium)
        false, // \26 - SUB (substitute)
        false, // \27 - ESC (escape)
        false, // \28 - FS  (file separator)
        false, // \29 - GS  (group separator)
        false, // \30 - RS  (record separator)
        false, // \31 - US  (unit separator)
        true, // \32 - SPACE
        true, // \33 - !
        true, // \34 - "
        true, // \35 - #
        true, // \36 - $
        true, // \37 - %
        true, // \38 - &
        true, // \39 - '
        true, // \40 - (
        true, // \41 - )
        true, // \42 - *
        true, // \43 - +
        true, // \44 - ,
        true, // \45 - -
        true, // \46 - .
        true, // \47 - /
        true, // \48 - 0
        true, // \49 - 1
        true, // \50 - 2
        true, // \51 - 3
        true, // \52 - 4
        true, // \53 - 5
        true, // \54 - 6
        true, // \55 - 7
        true, // \56 - 8
        true, // \57 - 9
        true, // \58 - :
        true, // \59 - ;
        true, // \60 - <
        true, // \61 - =
        true, // \62 - >
        true, // \63 - ?
        true, // \64 - @
        true, // \65 - A
        true, // \66 - B
        true, // \67 - C
        true, // \68 - D
        true, // \69 - E
        true, // \70 - F
        true, // \71 - G
        true, // \72 - H
        true, // \73 - I
        true, // \74 - J
        true, // \75 - K
        true, // \76 - L
        true, // \77 - M
        true, // \78 - N
        true, // \79 - O
        true, // \80 - P
        true, // \81 - Q
        true, // \82 - R
        true, // \83 - S
        true, // \84 - T
        true, // \85 - U
        true, // \86 - V
        true, // \87 - W
        true, // \88 - X
        true, // \89 - Y
        true, // \90 - Z
        true, // \91 - [
        true, // \92 - '\'
        true, // \93 - ]
        true, // \94 - ^
        true, // \95 - _
        true, // \96 - `
        true, // \97 - a
        true, // \98 - b
        true, // \99 - c
        true, // \100 - d
        true, // \101 - e
        true, // \102 - f
        true, // \103 - g
        true, // \104 - h
        true, // \105 - i
        true, // \106 - j
        true, // \107 - k
        true, // \108 - l
        true, // \109 - m
        true, // \110 - n
        true, // \111 - o
        true, // \112 - p
        true, // \113 - q
        true, // \114 - r
        true, // \115 - s
        true, // \116 - t
        true, // \117 - u
        true, // \118 - v
        true, // \119 - w
        true, // \120 - x
        true, // \121 - y
        true, // \122 - z
        true, // \123 - {
        true, // \124 - |
        true, // \125 - }
        true, // \126 - ~
        false, // \127 - DEL
    };
    return 0 <= c && c <= 127 && isCSourceChar[c];
}
