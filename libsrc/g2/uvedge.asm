.386
include type.inc
include cseg.inc
include dseg.inc

include plyparam.inc

extrn _g2d_v0:dword
extrn _g2d_v1:dword
extrn _g2d_p_edge_x:dword
extrn _g2d_p_edge_dx:dword
extrn _g2d_ppp:dword

public _gen_calc_edge_
public _gen_right_edge_asm
public _gen_uv_left_edge_asm

assume ds:_DATA
_DATA segment
conv            dq      0
conv2           dq      0
tmp_dx          dd      0
tmp_dy          dd      0
tmp_x           dd      0
tmp_fix         dd      0
hack36          dd      051900000h
hack52          dd      059900000h
fix_to_float    dd      037800000h
float_to_fix    dd      047800000h
_DATA ends

_TEXT segment

_gen_calc_edge_ proc near
        push    eax
        mov     eax, _g2d_v0
        push    edx
        mov     edx, _g2d_v1
        push    esi
        push    edi
        mov     esi, [eax]
        mov     edi, [eax+4]
        sub     esi, [edx]
        sub     edi, [edx+4]
        mov     tmp_dx, esi
        mov     tmp_dy, edi
        fild    tmp_dx
        fild    tmp_dy
        fdivp   st(1), st
        push    ebx
        push    ecx
        mov     ebx, _g2d_p_edge_x
        mov     ecx, _g2d_p_edge_dx
        mov     esi, [eax]
        mov     edi, [eax+4]
        neg     edi
        add     esi, 0ffffh
        and     edi, 0ffffh
        mov     eax, [ebx]
        mov     tmp_fix, edi
        mov     edx, [ecx]
        mov     eax, dword ptr conv
        fild    tmp_fix
        fld     st(1)
        fadd    hack36
        fxch    st(1)
        fmulp   st(2), st
        fstp    conv
        fadd    hack52
        mov     eax, ebx
        mov     ebx, dword ptr conv
        mov     [ecx], ebx
        pop     ecx
        fstp    conv
        mov     edx, dword ptr conv
        pop     ebx
        add     edx, esi
        pop     edi
        mov     [eax], edx
        pop     esi
        pop     edx
        pop     eax
        ret
_gen_calc_edge_ endp

_gen_right_edge_asm proc near
        push    eax
        mov     eax, _g2d_ppp
        push    edx
        push    ebx
        lea     edx, [eax+G2S_POLY_PARAMS_RIGHT_X]
        lea     ebx, [eax+G2S_POLY_PARAMS_RIGHT_DX]
        mov     _g2d_p_edge_x, edx
        mov     _g2d_p_edge_dx, ebx
        call    _gen_calc_edge_
        pop     ebx
        pop     edx
        pop     eax
        ret
_gen_right_edge_asm endp

_gen_uv_left_edge_asm proc near
        push    eax
        push    edx
        mov     eax, _g2d_v0
        mov     edx, _g2d_v1
        push    ebx
        mov     ebx, _g2d_ppp
        push    ebp
        push    esi
        push    edi
        push    ecx
        mov     esi, [eax]
        mov     edi, [eax+4]
        sub     esi, [edx]
        sub     edi, [edx+4]
        mov     tmp_dx, esi
        mov     tmp_dy, edi
        fild    dword ptr [ebx+G2S_POLY_PARAMS_DVY]
        fild    dword ptr [ebx+G2S_POLY_PARAMS_DUY]
        fild    tmp_dx
        fild    tmp_dy
        fld     dword ptr [eax+14h]
        fmul    float_to_fix
        fld     dword ptr [eax+18h]
        fmul    float_to_fix
        fxch    st(2)
        fdivp   st(3), st
        mov     edi, [eax+4]
        mov     esi, [eax]
        neg     edi
        mov     edx, esi
        and     edi, 0ffffh
        add     esi, 0ffffh
        neg     edx
        mov     tmp_fix, edi
        fmul    dword ptr [ebx+G2S_POLY_PARAMS_U_SCALE]
        fild    tmp_fix
        fxch    st(2)
        fmul    dword ptr [ebx+G2S_POLY_PARAMS_V_SCALE]
        fxch    st(2)
        fld     st(3)
        fxch    st(1)
        fmul    fix_to_float
        fxch    st(1)
        fadd    hack36
        fxch    st(1)
        fmul    st(4), st
        fld     st
        fmul    st, st(6)
        fxch    st(2)
        fstp    conv
        fmul    st, st(6)
        fxch    st(4)
        fadd    hack36
        fxch    st(2)
        faddp   st(1), st
        fxch    st(2)
        faddp   st(3), st
        fild    dword ptr [ebx+G2S_POLY_PARAMS_DUX]
        fxch    st(1)
        fstp    conv2
        mov     eax, dword ptr conv
        mov     edi, dword ptr conv2
        add     edi, esi
        mov     ecx, eax
        mov     esi, edi
        and     edi, 0ffff0000h
        sar     ecx, 10h
        add     edx, edi
        shl     esi, 10h
        mov     tmp_x, edx
        fild    tmp_x
        shl     eax, 10h
        mov     [ebx+G2S_POLY_PARAMS_LEFT_DX], ecx
        sar     edi, 10h
        mov     [ebx+G2S_POLY_PARAMS_LEFT_DX_FRAC], eax
        fmul    fix_to_float
        fild    dword ptr [ebx+G2S_POLY_PARAMS_DVX]
        fld     st(2)
        fmul    st, st(2)
        fxch    st(2)
        fmul    st, st(1)
        fxch    st(2)
        faddp   st(4), st
        fild    dword ptr [ebx+G2S_POLY_PARAMS_LEFT_DX]
        fxch    st(2)
        faddp   st(5), st
        fxch    st(3)
        fadd    hack52
        fxch    st(1)
        fmul    st(2), st
        fxch    st(4)
        fadd    hack52
        fxch    st(4)
        fmulp   st(3), st
        fxch    st(1)
        faddp   st(4), st
        fstp    conv
        faddp   st(3), st
        fxch    st(1)
        fadd    hack52
        fxch    st(1)
        fstp    conv2
        mov     eax, dword ptr conv
        mov     edx, dword ptr conv2
        fstp    conv
        fadd    hack52
        mov     [ebx+G2S_POLY_PARAMS_LEFT_X], edi
        mov     [ebx+G2S_POLY_PARAMS_LEFT_X_FRAC], esi
        mov     [ebx+G2S_POLY_PARAMS_LEFT_U], eax
        mov     [ebx+G2S_POLY_PARAMS_LEFT_V], edx
        pop     ecx
        pop     edi
        fstp    conv2
        mov     eax, dword ptr conv
        mov     edx, dword ptr conv2
        mov     [ebx+G2S_POLY_PARAMS_LEFT_DU], eax
        mov     [ebx+G2S_POLY_PARAMS_LEFT_DV], edx
        pop     esi
        pop     ebp
        pop     ebx
        pop     edx
        pop     eax
        ret
_gen_uv_left_edge_asm endp

_TEXT ends

end
