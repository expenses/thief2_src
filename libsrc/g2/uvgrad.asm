; $Header: x:/prj/tech/libsrc/g2/RCS/uvgrad.asm 1.1 1998/04/28 14:32:12 KEVIN Exp $

.486
.nolist
include type.inc
include cseg.inc
include dseg.inc
include thunks.inc
include macros.inc

include grs.inc
include g2spoint.inc
include plyparam.inc


RASTER_LIST     equ G2S_POLY_PARAMS_RASTER_LIST
P_DEST          equ G2S_POLY_PARAMS_P
FLAGS           equ G2S_POLY_PARAMS_FLAGS
DIX             equ G2S_POLY_PARAMS_DIX
DUX             equ G2S_POLY_PARAMS_DUX
DVX             equ G2S_POLY_PARAMS_DVX
DIY             equ G2S_POLY_PARAMS_DIY
DUY             equ G2S_POLY_PARAMS_DUY
DVY             equ G2S_POLY_PARAMS_DVY
I_SCALE         equ G2S_POLY_PARAMS_I_SCALE
U_SCALE         equ G2S_POLY_PARAMS_U_SCALE
V_SCALE         equ G2S_POLY_PARAMS_V_SCALE
RASTER_FUNC     equ G2S_POLY_PARAMS_RASTER_FUNC
LE_FUNC         equ G2S_POLY_PARAMS_LE_FUNC
RE_FUNC         equ G2S_POLY_PARAMS_RE_FUNC
GRAD_FUNC       equ G2S_POLY_PARAMS_GRAD_FUNC

.list
assume cs:_TEXT, ds:_DATA

extrn _g2d_v0:dword
extrn _g2d_v1:dword
extrn _g2d_v2:dword
extrn _g2d_ppp:dword

public triangle_raster_reentry
public _uv_triangle_gradients
public _iuv_triangle_gradients

_DATA segment
extd            _grd_canvas

convert_buf     dq 0
convert_buf2    dq 0
denom           dq 0
dy_2_0          dq 0
dy_2_1          dq 0
dx_2_0          dq 0
dx_2_1          dq 0

re_v0           dd 0
re_v1           dd 0
le_v0           dd 0
le_v1           dd 0
e2_v0           dd 0
e2_v1           dd 0
e2_func         dd 0
n_rows          dd 0
n_rows2         dd 0

dx_20           dd 0
dy_20           dd 0
dx_21           dd 0
dy_21           dd 0

fix_unit        dd 65536.0
two_to_36       dd 51c00000h   ; convert float to 16.16 fix
_DATA ends


_TEXT segment
use_old_denom proc near private
        fld        denom
        jmp        L1
use_old_denom endp
_FUNCDEF_ASM triangle_raster
        push       edi
        push       ebx
        mov        edi, _g2d_v2
        mov        ebx, _g2d_v0
        push       edx
        push       eax
        mov        edx, d G2S_POINT_SX[edi]
        mov        eax, d G2S_POINT_SY[edi]
        push       esi
        push       ecx
        sub        edx, d G2S_POINT_SX[ebx]
        sub        eax, d G2S_POINT_SY[ebx]
        mov        esi, _g2d_v1
        mov        ecx, _g2d_ppp
        mov        dx_20, edx
        mov        dy_20, eax
        push       ebp
        mov        ebp, d FLAGS[ecx]
        test       ebp, ebp
        je         use_old_denom
        mov        eax, d G2S_POINT_SX[edi]
        mov        edx, d G2S_POINT_SY[edi]
        fild       dy_20
        sub        eax, d G2S_POINT_SX[esi]
        sub        edx, d G2S_POINT_SY[esi]
        fild       dx_20
        mov        dx_21, eax
        mov        dy_21, edx
        fild       dx_21
        fild       dy_21
        fxch       st(3)
        fst        dy_2_0
        fmul       st, st(1)
        fxch       st(2)
        fst        dx_2_0
        fmul       st, st(3)
        fxch       st(1)
        fstp       dx_2_1
        fsubp      st(1), st
        fxch       st(1)
        fstp       dy_2_1
        fdivr      fix_unit
L1:
        mov        eax, d G2S_POINT_SY[ebx]
        mov        edx, d G2S_POINT_SY[esi]
        cmp        eax, edx
        jg         L3
        add        edx, 0ffffh
        mov        ebp, d G2S_POINT_SY[edi]
        sar        edx, 16
        add        eax, 0ffffh
        sar        eax, 16
        add        ebp, 0ffffh
        sar        ebp, 16
        nop
        cmp        eax, ebp
        jle        L4
L2:
        mov        ecx, edi
        mov        edi, esi
        mov        esi, ebx
        mov        ebx, ecx
        mov        ecx, ebp
        mov        ebp, edx
        mov        edx, eax
        mov        eax, ecx
        jmp        L4
L3:
        add        edx, 0ffffh
        mov        ebp, d G2S_POINT_SY[edi]
        sar        edx, 16
        add        eax, 0ffffh
        sar        eax, 16
        add        ebp, 0ffffh
        sar        ebp, 16
        nop
        cmp        edx, ebp
        jg         L2
        mov        ecx, esi
        mov        esi, edi
        mov        edi, ebx
        mov        ebx, ecx
        mov        ecx, edx
        mov        edx, ebp
        mov        ebp, eax
        mov        eax, ecx
L4:
        cmp        edx, ebp
        jg         L8
        mov        le_v0, ebx
        mov        le_v1, edi
        cmp        eax, edx
        je         L6
        mov        ecx, edx
        mov        re_v0, ebx
        sub        ecx, eax
        mov        re_v1, esi
        mov        n_rows, ecx
        mov        ecx, _g2d_ppp
        sub        ebp, edx
        mov        e2_v0, esi
        mov        n_rows2, ebp
        mov        edx, d RE_FUNC[ecx]
        mov        e2_v1, edi
        mov        e2_func, edx
        jmp        L9
L5:
        ffree      st(0)
        jmp        L12
L6:
        sub        ebp, edx
        je         L5
        mov        n_rows, ebp
        xor        edx, edx
        mov        re_v0, esi
        mov        re_v1, edi
        mov        n_rows2, edx
        nop
        mov        ecx, _g2d_ppp
        jmp        L9
L7:
        sub        edx, ebp
        xor        ebp, ebp
        mov        le_v0, edi
        mov        le_v1, esi
        mov        n_rows, edx
        mov        n_rows2, ebp
        mov        ecx, _g2d_ppp
        jmp        L9
L8:
        mov        re_v0, ebx
        mov        re_v1, esi
        cmp        eax, ebp
        je         L7
        mov        ecx, ebp
        mov        le_v0, ebx
        sub        ecx, eax
        mov        le_v1, edi
        mov        n_rows, ecx
        mov        ecx, _g2d_ppp
        sub        edx, ebp
        mov        e2_v0, edi
        mov        n_rows2, edx
        mov        edx, d LE_FUNC[ecx]
        mov        e2_v1, esi
        mov        e2_func, edx
L9:
        mov        edx, _grd_canvas
        mov        ebx, _g2d_v0
        mov        esi, _g2d_v1
        mov        edi, _g2d_v2
        mov        ebp, d GRS_CANVAS_YTAB[edx]
        test       ebp, ebp
        jne        L10
        mov        ebp, d GRS_BITMAP_BITS[edx]
        mov        edx, d GRS_BITMAP_ROW[edx]
        and        edx, 0ffffh
        imul       edx
        add        eax, ebp
        jmp        L11
L10:
        mov        eax, [ebp+4*eax]
        mov        edx, d GRS_BITMAP_BITS[edx]
        add        eax, edx
L11:
        mov        d P_DEST[ecx], eax
        jmp        d GRAD_FUNC[ecx]
triangle_raster_reentry:
        mov        esi, ecx
        mov        eax, le_v0
        mov        edx, le_v1
        mov        _g2d_v0, eax
        mov        _g2d_v1, edx
        call       d LE_FUNC[esi]
        mov        eax, re_v0
        mov        edx, re_v1
        mov        _g2d_v0, eax
        mov        _g2d_v1, edx
        call       d RE_FUNC[esi]
        mov        eax, n_rows
        mov        edi, n_rows2
        mov        d RASTER_LIST[esi], eax
        call       d RASTER_FUNC[esi]
        test       edi, edi
        je         L12
        mov        eax, e2_v0
        mov        edx, e2_v1
        mov        _g2d_v0, eax
        mov        _g2d_v1, edx
        call       e2_func
        mov        d RASTER_LIST[esi], edi
        call       d RASTER_FUNC[esi]
L12:
        pop        ebp
        pop        ecx
        pop        esi
        pop        eax
        pop        edx
        pop        ebx
        pop        edi
        ret
_FUNCDEF_ASM dummy_triangle_gradients
        ffree      st(0)
        jmp        triangle_raster_reentry
_uv_triangle_gradients:
_FUNCDEF_ASM uv_triangle_gradients
        fld        d G2S_POINT_U[edi]
        fsub       d G2S_POINT_U[esi]
        fld        st(1)
        fmul       d V_SCALE[ecx]
        fxch       st(1)
        fld        d G2S_POINT_U[edi]
        fxch       st(3)
        fmul       d U_SCALE[ecx]
        fxch       st(3)
        fld        st(1)
        fmul       dy_2_0
        fxch       st(1)
        fsub       d G2S_POINT_U[ebx]
        fxch       st(2)
        fmul       dx_2_0
        fld        d G2S_POINT_V[edi]
        fld        st(3)
        fmul       dy_2_1
        fxch       st(1)
        fsub       d G2S_POINT_V[esi]
        fxch       st(4)
        fmul       dx_2_1
        fxch       st(1)
        fsubp      st(3), st
        fld        d G2S_POINT_V[edi]
        fld        st(4)
        fmul       dy_2_0
        fxch       st(1)
        fsub       d G2S_POINT_V[ebx]
        fxch       st(5)
        fmul       dx_2_0
        fxch       st(3)
        fsubp      st(2), st
        fxch       st(3)
        fmul       st, st(6)
        fld        st(4)
        fmul       dy_2_1
        fxch       st(1)
        fadd       two_to_36
        fxch       st(5)
        fmul       dx_2_1
        fxch       st(1)
        fsubp      st(4), st
        fxch       st(1)
        fmulp      st(6), st
        fsubrp     st(1), st
        fxch       st(1)
        fmul       st, st(3)
        fxch       st(2)
        fstp       convert_buf
        fmulp      st(2), st
        fadd       two_to_36
        fxch       st(2)
        fadd       two_to_36
        fxch       st(1)
        fadd       two_to_36
        fxch       st(2)
        fstp       convert_buf2
        mov        eax, d convert_buf
        mov        edx, d convert_buf2
        fstp       convert_buf
        fstp       convert_buf2
        mov        esi, d convert_buf
        mov        edi, d convert_buf2
        mov        d DUX[ecx], eax
        mov        d DVX[ecx], edx
        mov        d DUY[ecx], esi
        mov        d DVY[ecx], edi
        jmp        triangle_raster_reentry
_iuv_triangle_gradients:
_FUNCDEF_ASM iuv_triangle_gradients
        fld        d G2S_POINT_U[edi]
        fsub       d G2S_POINT_U[esi]
        fld        st(1)
        fxch       st(2)
        fst        denom
        fmul       d V_SCALE[ecx]
        fxch       st(1)
        fld        d G2S_POINT_U[edi]
        fxch       st(3)
        fmul       d U_SCALE[ecx]
        fxch       st(3)
        fld        st(1)
        fmul       dy_2_0
        fxch       st(1)
        fsub       d G2S_POINT_U[ebx]
        fxch       st(2)
        fmul       dx_2_0
        fld        d G2S_POINT_V[edi]
        fld        st(3)
        fmul       dy_2_1
        fxch       st(1)
        fsub       d G2S_POINT_V[esi]
        fxch       st(4)
        fmul       dx_2_1
        fxch       st(1)
        fsubp      st(3), st
        fld        d G2S_POINT_V[edi]
        fld        st(4)
        fmul       dy_2_0
        fxch       st(1)
        fsub       d G2S_POINT_V[ebx]
        fxch       st(5)
        fmul       dx_2_0
        fxch       st(3)
        fsubp      st(2), st
        fxch       st(3)
        fmul       st, st(6)
        fld        st(4)
        fmul       dy_2_1
        fxch       st(1)
        fadd       two_to_36
        fxch       st(5)
        fmul       dx_2_1
        fxch       st(1)
        fsubp      st(4), st
        fxch       st(1)
        fmulp      st(6), st
        fsubrp     st(1), st
        fxch       st(1)
        fmul       st, st(3)
        fxch       st(2)
        fstp       convert_buf
        fmulp      st(2), st
        fld        d G2S_POINT_I[edi]
        fsub       d G2S_POINT_I[esi]
        fld        d G2S_POINT_I[edi]
        fsub       d G2S_POINT_I[ebx]
        fld        st(1)
        fmul       dy_2_0
        fld        denom
        fmul       d I_SCALE[ecx]
        fld        st(2)
        fmul       dy_2_1
        fxch       st(7)
        fadd       two_to_36
        fxch       st(4)
        fmul       dx_2_0
        fxch       st(7)
        fsubp      st(2), st
        fxch       st(2)
        fmul       dx_2_1
        fxch       st(4)
        fadd       two_to_36
        fxch       st(1)
        fmul       st, st(2)
        fxch       st(6)
        fsubp      st(4), st
        fxch       st(4)
        fadd       two_to_36
        fxch       st(4)
        fstp       convert_buf2
        mov        eax, d convert_buf
        mov        edx, d convert_buf2
        fmulp      st(2), st
        fxch       st(3)
        fadd       two_to_36
        fxch       st(3)
        fstp       convert_buf
        fadd       two_to_36
        fxch       st(1)
        fstp       convert_buf2
        mov        d DUX[ecx], eax
        mov        d DVX[ecx], edx
        mov        eax, d convert_buf
        mov        edx, d convert_buf2
        fstp       convert_buf2
        fstp       convert_buf
        mov        d DUY[ecx], eax
        mov        d DVY[ecx], edx
        mov        eax, d convert_buf
        mov        edx, d convert_buf2
        mov        d DIX[ecx], eax
        mov        d DIY[ecx], edx
        jmp        triangle_raster_reentry

_TEXT ends

END
