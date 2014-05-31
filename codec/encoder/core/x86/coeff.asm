;*!
;* \copy
;*     Copyright (c)  2010-2013, Cisco Systems
;*     All rights reserved.
;*
;*     Redistribution and use in source and binary forms, with or without
;*     modification, are permitted provided that the following conditions
;*     are met:
;*
;*        * Redistributions of source code must retain the above copyright
;*          notice, this list of conditions and the following disclaimer.
;*
;*        * Redistributions in binary form must reproduce the above copyright
;*          notice, this list of conditions and the following disclaimer in
;*          the documentation and/or other materials provided with the
;*          distribution.
;*
;*     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
;*     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
;*     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
;*     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
;*     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
;*     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
;*     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
;*     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
;*     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
;*     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
;*     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
;*     POSSIBILITY OF SUCH DAMAGE.
;*
;*
;*  memzero.asm
;*
;*  Abstract
;*     cavlc
;*
;*  History
;*      09/08/2010 Created
;*
;*
;*************************************************************************/

%include "asm_inc.asm"



%ifdef X86_32
SECTION .rodata align=16

align 16
sse2_b8 db 8, 8, 8, 8, 8, 8, 8, 8

ALIGN  16
sse2_b_1 db -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1

align 16
byte_1pos_table:
    db 0,0,0,0,0,0,0,0, ;0
    db 0,0,0,0,0,0,0,1, ;1
    db 1,0,0,0,0,0,0,1, ;2
    db 1,0,0,0,0,0,0,2, ;3
    db 2,0,0,0,0,0,0,1, ;4
    db 2,0,0,0,0,0,0,2, ;5
    db 2,1,0,0,0,0,0,2, ;6
    db 2,1,0,0,0,0,0,3, ;7
    db 3,0,0,0,0,0,0,1, ;8
    db 3,0,0,0,0,0,0,2, ;9
    db 3,1,0,0,0,0,0,2, ;10
    db 3,1,0,0,0,0,0,3, ;11
    db 3,2,0,0,0,0,0,2, ;12
    db 3,2,0,0,0,0,0,3, ;13
    db 3,2,1,0,0,0,0,3, ;14
    db 3,2,1,0,0,0,0,4, ;15
    db 4,0,0,0,0,0,0,1, ;16
    db 4,0,0,0,0,0,0,2, ;17
    db 4,1,0,0,0,0,0,2, ;18
    db 4,1,0,0,0,0,0,3, ;19
    db 4,2,0,0,0,0,0,2, ;20
    db 4,2,0,0,0,0,0,3, ;21
    db 4,2,1,0,0,0,0,3, ;22
    db 4,2,1,0,0,0,0,4, ;23
    db 4,3,0,0,0,0,0,2, ;24
    db 4,3,0,0,0,0,0,3, ;25
    db 4,3,1,0,0,0,0,3, ;26
    db 4,3,1,0,0,0,0,4, ;27
    db 4,3,2,0,0,0,0,3, ;28
    db 4,3,2,0,0,0,0,4, ;29
    db 4,3,2,1,0,0,0,4, ;30
    db 4,3,2,1,0,0,0,5, ;31
    db 5,0,0,0,0,0,0,1, ;32
    db 5,0,0,0,0,0,0,2, ;33
    db 5,1,0,0,0,0,0,2, ;34
    db 5,1,0,0,0,0,0,3, ;35
    db 5,2,0,0,0,0,0,2, ;36
    db 5,2,0,0,0,0,0,3, ;37
    db 5,2,1,0,0,0,0,3, ;38
    db 5,2,1,0,0,0,0,4, ;39
    db 5,3,0,0,0,0,0,2, ;40
    db 5,3,0,0,0,0,0,3, ;41
    db 5,3,1,0,0,0,0,3, ;42
    db 5,3,1,0,0,0,0,4, ;43
    db 5,3,2,0,0,0,0,3, ;44
    db 5,3,2,0,0,0,0,4, ;45
    db 5,3,2,1,0,0,0,4, ;46
    db 5,3,2,1,0,0,0,5, ;47
    db 5,4,0,0,0,0,0,2, ;48
    db 5,4,0,0,0,0,0,3, ;49
    db 5,4,1,0,0,0,0,3, ;50
    db 5,4,1,0,0,0,0,4, ;51
    db 5,4,2,0,0,0,0,3, ;52
    db 5,4,2,0,0,0,0,4, ;53
    db 5,4,2,1,0,0,0,4, ;54
    db 5,4,2,1,0,0,0,5, ;55
    db 5,4,3,0,0,0,0,3, ;56
    db 5,4,3,0,0,0,0,4, ;57
    db 5,4,3,1,0,0,0,4, ;58
    db 5,4,3,1,0,0,0,5, ;59
    db 5,4,3,2,0,0,0,4, ;60
    db 5,4,3,2,0,0,0,5, ;61
    db 5,4,3,2,1,0,0,5, ;62
    db 5,4,3,2,1,0,0,6, ;63
    db 6,0,0,0,0,0,0,1, ;64
    db 6,0,0,0,0,0,0,2, ;65
    db 6,1,0,0,0,0,0,2, ;66
    db 6,1,0,0,0,0,0,3, ;67
    db 6,2,0,0,0,0,0,2, ;68
    db 6,2,0,0,0,0,0,3, ;69
    db 6,2,1,0,0,0,0,3, ;70
    db 6,2,1,0,0,0,0,4, ;71
    db 6,3,0,0,0,0,0,2, ;72
    db 6,3,0,0,0,0,0,3, ;73
    db 6,3,1,0,0,0,0,3, ;74
    db 6,3,1,0,0,0,0,4, ;75
    db 6,3,2,0,0,0,0,3, ;76
    db 6,3,2,0,0,0,0,4, ;77
    db 6,3,2,1,0,0,0,4, ;78
    db 6,3,2,1,0,0,0,5, ;79
    db 6,4,0,0,0,0,0,2, ;80
    db 6,4,0,0,0,0,0,3, ;81
    db 6,4,1,0,0,0,0,3, ;82
    db 6,4,1,0,0,0,0,4, ;83
    db 6,4,2,0,0,0,0,3, ;84
    db 6,4,2,0,0,0,0,4, ;85
    db 6,4,2,1,0,0,0,4, ;86
    db 6,4,2,1,0,0,0,5, ;87
    db 6,4,3,0,0,0,0,3, ;88
    db 6,4,3,0,0,0,0,4, ;89
    db 6,4,3,1,0,0,0,4, ;90
    db 6,4,3,1,0,0,0,5, ;91
    db 6,4,3,2,0,0,0,4, ;92
    db 6,4,3,2,0,0,0,5, ;93
    db 6,4,3,2,1,0,0,5, ;94
    db 6,4,3,2,1,0,0,6, ;95
    db 6,5,0,0,0,0,0,2, ;96
    db 6,5,0,0,0,0,0,3, ;97
    db 6,5,1,0,0,0,0,3, ;98
    db 6,5,1,0,0,0,0,4, ;99
    db 6,5,2,0,0,0,0,3, ;100
    db 6,5,2,0,0,0,0,4, ;101
    db 6,5,2,1,0,0,0,4, ;102
    db 6,5,2,1,0,0,0,5, ;103
    db 6,5,3,0,0,0,0,3, ;104
    db 6,5,3,0,0,0,0,4, ;105
    db 6,5,3,1,0,0,0,4, ;106
    db 6,5,3,1,0,0,0,5, ;107
    db 6,5,3,2,0,0,0,4, ;108
    db 6,5,3,2,0,0,0,5, ;109
    db 6,5,3,2,1,0,0,5, ;110
    db 6,5,3,2,1,0,0,6, ;111
    db 6,5,4,0,0,0,0,3, ;112
    db 6,5,4,0,0,0,0,4, ;113
    db 6,5,4,1,0,0,0,4, ;114
    db 6,5,4,1,0,0,0,5, ;115
    db 6,5,4,2,0,0,0,4, ;116
    db 6,5,4,2,0,0,0,5, ;117
    db 6,5,4,2,1,0,0,5, ;118
    db 6,5,4,2,1,0,0,6, ;119
    db 6,5,4,3,0,0,0,4, ;120
    db 6,5,4,3,0,0,0,5, ;121
    db 6,5,4,3,1,0,0,5, ;122
    db 6,5,4,3,1,0,0,6, ;123
    db 6,5,4,3,2,0,0,5, ;124
    db 6,5,4,3,2,0,0,6, ;125
    db 6,5,4,3,2,1,0,6, ;126
    db 6,5,4,3,2,1,0,7, ;127
    db 7,0,0,0,0,0,0,1, ;128
    db 7,0,0,0,0,0,0,2, ;129
    db 7,1,0,0,0,0,0,2, ;130
    db 7,1,0,0,0,0,0,3, ;131
    db 7,2,0,0,0,0,0,2, ;132
    db 7,2,0,0,0,0,0,3, ;133
    db 7,2,1,0,0,0,0,3, ;134
    db 7,2,1,0,0,0,0,4, ;135
    db 7,3,0,0,0,0,0,2, ;136
    db 7,3,0,0,0,0,0,3, ;137
    db 7,3,1,0,0,0,0,3, ;138
    db 7,3,1,0,0,0,0,4, ;139
    db 7,3,2,0,0,0,0,3, ;140
    db 7,3,2,0,0,0,0,4, ;141
    db 7,3,2,1,0,0,0,4, ;142
    db 7,3,2,1,0,0,0,5, ;143
    db 7,4,0,0,0,0,0,2, ;144
    db 7,4,0,0,0,0,0,3, ;145
    db 7,4,1,0,0,0,0,3, ;146
    db 7,4,1,0,0,0,0,4, ;147
    db 7,4,2,0,0,0,0,3, ;148
    db 7,4,2,0,0,0,0,4, ;149
    db 7,4,2,1,0,0,0,4, ;150
    db 7,4,2,1,0,0,0,5, ;151
    db 7,4,3,0,0,0,0,3, ;152
    db 7,4,3,0,0,0,0,4, ;153
    db 7,4,3,1,0,0,0,4, ;154
    db 7,4,3,1,0,0,0,5, ;155
    db 7,4,3,2,0,0,0,4, ;156
    db 7,4,3,2,0,0,0,5, ;157
    db 7,4,3,2,1,0,0,5, ;158
    db 7,4,3,2,1,0,0,6, ;159
    db 7,5,0,0,0,0,0,2, ;160
    db 7,5,0,0,0,0,0,3, ;161
    db 7,5,1,0,0,0,0,3, ;162
    db 7,5,1,0,0,0,0,4, ;163
    db 7,5,2,0,0,0,0,3, ;164
    db 7,5,2,0,0,0,0,4, ;165
    db 7,5,2,1,0,0,0,4, ;166
    db 7,5,2,1,0,0,0,5, ;167
    db 7,5,3,0,0,0,0,3, ;168
    db 7,5,3,0,0,0,0,4, ;169
    db 7,5,3,1,0,0,0,4, ;170
    db 7,5,3,1,0,0,0,5, ;171
    db 7,5,3,2,0,0,0,4, ;172
    db 7,5,3,2,0,0,0,5, ;173
    db 7,5,3,2,1,0,0,5, ;174
    db 7,5,3,2,1,0,0,6, ;175
    db 7,5,4,0,0,0,0,3, ;176
    db 7,5,4,0,0,0,0,4, ;177
    db 7,5,4,1,0,0,0,4, ;178
    db 7,5,4,1,0,0,0,5, ;179
    db 7,5,4,2,0,0,0,4, ;180
    db 7,5,4,2,0,0,0,5, ;181
    db 7,5,4,2,1,0,0,5, ;182
    db 7,5,4,2,1,0,0,6, ;183
    db 7,5,4,3,0,0,0,4, ;184
    db 7,5,4,3,0,0,0,5, ;185
    db 7,5,4,3,1,0,0,5, ;186
    db 7,5,4,3,1,0,0,6, ;187
    db 7,5,4,3,2,0,0,5, ;188
    db 7,5,4,3,2,0,0,6, ;189
    db 7,5,4,3,2,1,0,6, ;190
    db 7,5,4,3,2,1,0,7, ;191
    db 7,6,0,0,0,0,0,2, ;192
    db 7,6,0,0,0,0,0,3, ;193
    db 7,6,1,0,0,0,0,3, ;194
    db 7,6,1,0,0,0,0,4, ;195
    db 7,6,2,0,0,0,0,3, ;196
    db 7,6,2,0,0,0,0,4, ;197
    db 7,6,2,1,0,0,0,4, ;198
    db 7,6,2,1,0,0,0,5, ;199
    db 7,6,3,0,0,0,0,3, ;200
    db 7,6,3,0,0,0,0,4, ;201
    db 7,6,3,1,0,0,0,4, ;202
    db 7,6,3,1,0,0,0,5, ;203
    db 7,6,3,2,0,0,0,4, ;204
    db 7,6,3,2,0,0,0,5, ;205
    db 7,6,3,2,1,0,0,5, ;206
    db 7,6,3,2,1,0,0,6, ;207
    db 7,6,4,0,0,0,0,3, ;208
    db 7,6,4,0,0,0,0,4, ;209
    db 7,6,4,1,0,0,0,4, ;210
    db 7,6,4,1,0,0,0,5, ;211
    db 7,6,4,2,0,0,0,4, ;212
    db 7,6,4,2,0,0,0,5, ;213
    db 7,6,4,2,1,0,0,5, ;214
    db 7,6,4,2,1,0,0,6, ;215
    db 7,6,4,3,0,0,0,4, ;216
    db 7,6,4,3,0,0,0,5, ;217
    db 7,6,4,3,1,0,0,5, ;218
    db 7,6,4,3,1,0,0,6, ;219
    db 7,6,4,3,2,0,0,5, ;220
    db 7,6,4,3,2,0,0,6, ;221
    db 7,6,4,3,2,1,0,6, ;222
    db 7,6,4,3,2,1,0,7, ;223
    db 7,6,5,0,0,0,0,3, ;224
    db 7,6,5,0,0,0,0,4, ;225
    db 7,6,5,1,0,0,0,4, ;226
    db 7,6,5,1,0,0,0,5, ;227
    db 7,6,5,2,0,0,0,4, ;228
    db 7,6,5,2,0,0,0,5, ;229
    db 7,6,5,2,1,0,0,5, ;230
    db 7,6,5,2,1,0,0,6, ;231
    db 7,6,5,3,0,0,0,4, ;232
    db 7,6,5,3,0,0,0,5, ;233
    db 7,6,5,3,1,0,0,5, ;234
    db 7,6,5,3,1,0,0,6, ;235
    db 7,6,5,3,2,0,0,5, ;236
    db 7,6,5,3,2,0,0,6, ;237
    db 7,6,5,3,2,1,0,6, ;238
    db 7,6,5,3,2,1,0,7, ;239
    db 7,6,5,4,0,0,0,4, ;240
    db 7,6,5,4,0,0,0,5, ;241
    db 7,6,5,4,1,0,0,5, ;242
    db 7,6,5,4,1,0,0,6, ;243
    db 7,6,5,4,2,0,0,5, ;244
    db 7,6,5,4,2,0,0,6, ;245
    db 7,6,5,4,2,1,0,6, ;246
    db 7,6,5,4,2,1,0,7, ;247
    db 7,6,5,4,3,0,0,5, ;248
    db 7,6,5,4,3,0,0,6, ;249
    db 7,6,5,4,3,1,0,6, ;250
    db 7,6,5,4,3,1,0,7, ;251
    db 7,6,5,4,3,2,0,6, ;252
    db 7,6,5,4,3,2,0,7, ;253
    db 7,6,5,4,3,2,1,7, ;254
    db 7,6,5,4,3,2,1,8, ;255

;***********************************************************************
; Code
;***********************************************************************
SECTION .text



;***********************************************************************
;int32_t CavlcParamCal_sse2(int16_t*coffLevel, uint8_t* run, int16_t *Level, int32_t* total_coeffs , int32_t endIdx);
;***********************************************************************
WELS_EXTERN CavlcParamCal_sse2
    push ebx
    push edi
    push esi

    mov         eax,    [esp+16]    ;coffLevel
    mov         edi,    [esp+24]    ;Level
    mov         ebx,    [esp+32]    ;endIdx
    cmp         ebx,    3
    jne         .Level16
    pxor        xmm1,   xmm1
    movq        xmm0,   [eax]   ; removed QWORD
    jmp         .Cal_begin
.Level16:
    movdqa      xmm0,   [eax]
    movdqa      xmm1,   [eax+16]
.Cal_begin:
    movdqa      xmm2,   xmm0
    packsswb    xmm0,   xmm1
    movdqa      xmm4,   xmm0
    pxor        xmm3,   xmm3
    pcmpgtb     xmm0,   xmm3
    pcmpgtb     xmm3,   xmm4
    por         xmm0,   xmm3
    pmovmskb    edx,    xmm0
    cmp         edx,    0
    je near   .return
    movdqa      xmm6,   [sse2_b_1]
    pcmpeqw     xmm7,   xmm7    ;generate -1
    mov         ebx,    0xff
    ;pinsrw     xmm6,   ebx,    3

    mov       bl,   dh

    lea       ebx,  [byte_1pos_table+8*ebx]
    movq      xmm0, [ebx]
    pextrw    ecx,  xmm0, 3
    shr       ecx,  8
    mov       dh,   cl

.loopHighFind0:
    cmp       ecx,   0
    je        .loopHighFind0End
    ;mov       esi, [ebx]
    ;and       esi, 0xff
    movzx     esi, byte [ebx]
    add       esi, 8
    mov       esi, [eax+2*esi]
    mov       [edi], si
    add       edi,   2
    ;add       ebx,   1
    inc       ebx
    dec       ecx
    jmp       .loopHighFind0
.loopHighFind0End:
    mov       cl,   dh
    cmp       cl,   8
    pand      xmm0, xmm6
    jne       .LowByteFind0
    sub       edi,   2
    mov       esi,   [eax+16]
    mov       [edi], esi
    add       edi,   2
.LowByteFind0:
    and       edx,  0xff
    lea       ebx,  [byte_1pos_table+8*edx]
    movq      xmm1, [ebx]
    pextrw    esi,  xmm1, 3
    or        esi,  0xff
    or        ecx,  0xff00
    and       ecx,  esi
    shr       esi,  8
    pand      xmm1, xmm6
.loopLowFind0:
    cmp       esi, 0
    je        .loopLowFind0End
    ;mov       edx, [ebx]
    ;and       edx, 0xff
    movzx     edx,  byte [ebx]
    mov       edx, [eax+2*edx]
    mov       [edi], dx
    add       edi,   2
    ;add       ebx,   1
    inc       ebx
    dec       esi
    jmp       .loopLowFind0
.loopLowFind0End:
    cmp       ch,  8
    jne       .getLevelEnd
    sub       edi, 2
    mov       edx, [eax]
    mov       [edi], dx
.getLevelEnd:
    mov      edx, [esp+28]  ;total_coeffs
    ;mov      ebx,   ecx
    ;and      ebx,   0xff
    movzx    ebx,   byte cl
    add      cl,    ch
    mov      [edx], cl
;getRun
    movq     xmm5, [sse2_b8]
    paddb    xmm0, xmm5
    pxor     xmm2, xmm2
    pxor     xmm3, xmm3
    mov      eax,  8
    sub      eax,  ebx
    shl      eax,  3
    shl      ebx,  3
    pinsrw   xmm2, ebx, 0
    pinsrw   xmm3, eax, 0
    psllq    xmm0, xmm3
    psrlq    xmm0, xmm3
    movdqa   xmm4, xmm1
    psllq    xmm1, xmm2
    psrlq    xmm4, xmm3
    punpcklqdq xmm1, xmm4
    por      xmm0,  xmm1

    pextrw   eax,   xmm0, 0
    and      eax,   0xff
    inc      eax
    sub      al,    cl
    movdqa   xmm1,  xmm0
    paddb    xmm1,  xmm7
    psrldq   xmm0,  1
    psubb    xmm1,  xmm0
    mov      ecx,   [esp+20] ;run
    movdqa   [ecx], xmm1
;getRunEnd
.return:
    pop esi
    pop edi
    pop ebx
    ret
%endif
