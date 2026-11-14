
.386
include type.inc
include cseg.inc
include dseg.inc
include macros.inc
include thunks.inc

include grs.inc
include plyrast.inc
include plyparam.inc

assume ds:_DATA
_DATA segment
last_ltab       dd      0deadbeefh
last_mask_u     dd      0deadbeefh
last_mask_v     dd      0deadbeefh
last_p_src      dd      0deadbeefh
last_clut       dd      0deadbeefh
last_shift_v    db      0ffh

dux_local       dd      ?
dvx_local       dd      ?
i_local         dd      ?
di_local        dd      ?
count           dd      ?

extd    _grd_ltab816
extd    _grd_canvas
_DATA ends

p_dest          equ     G2S_POLY_PARAMS_P
row             equ     G2S_POLY_PARAMS_CANVAS_ROW
dux             equ     G2S_POLY_PARAMS_DUX
dvx             equ     G2S_POLY_PARAMS_DVX
dix             equ     G2S_POLY_PARAMS_DIX
u               equ     G2S_POLY_PARAMS_LEFT_U
v               equ     G2S_POLY_PARAMS_LEFT_V
i               equ     G2S_POLY_PARAMS_LEFT_I
du              equ     G2S_POLY_PARAMS_LEFT_DU
dv              equ     G2S_POLY_PARAMS_LEFT_DV
di_left         equ     G2S_POLY_PARAMS_LEFT_DI
x_left          equ     G2S_POLY_PARAMS_LEFT_X
dx_left         equ     G2S_POLY_PARAMS_LEFT_DX
x_frac          equ     G2S_POLY_PARAMS_LEFT_X_FRAC
dx_frac         equ     G2S_POLY_PARAMS_LEFT_DX_FRAC
x_right         equ     G2S_POLY_PARAMS_RIGHT_X
dx_right        equ     G2S_POLY_PARAMS_RIGHT_DX

extrn _g2d_ppp:dword

_TEXT segment

_FUNCDEF_ASM opaque_clut_lit_8to16_raster
        push    ebp
        mov     ebp, _g2d_ppp
        push    edx
        push    eax
        mov     edx, [ebp]
        nop

        push    edi
        push    esi

        push    ecx
        push    ebx

        mov     ebx, dix[ebp]
        mov     ecx, i[ebp]

        mov     di_local, ebx
        mov     i_local, ecx

        mov     ebx, dux[ebp]
        mov     ecx, dvx[ebp]

        mov     dux_local, ebx
        mov     dvx_local, ecx

        mov     ebx, x_right[ebp]       ; fixed point
        mov     ecx, x_left[ebp]        ; int

        sar     ebx, 16                 ; convert to int
        mov     edi, p_dest[ebp]        ; beginning of canvas row

raster_loop:
        mov     count, edx              ; number of scanlines remaining
        xor     edx, edx

        sub     ecx, ebx                ; ecx = -num pixels in scanline
        jge     skip_pixel_loop

        mov     eax, u[ebp]
        lea     edi, [edi+2*ebx]        ; point to end of scanline

        mov     ebx, v[ebp]
        push    ebp

        mov     esi, eax                ; two copies of u
        mov     ebp, ebx                ; two copies of v

pixel_loop:
        shr     eax, 16                 ; convert u_temp to int
        and     ebx, 0deadbeefh         ; mask v_temp
mask_v:

        shr     ebx, 0ffh               ; set up for read from texture
shift_v:
        and     eax, 0deadbeefh         ; mask u_temp
mask_u:

        add     esi, dux_local          ; increment u
        add     ebp, dvx_local          ; increment v

        mov     dl, 0deadbeefh[ebx+eax] ; read pixel from texture
p_src:
        mov     eax, i_local

        shr     eax, 16
        mov     ebx, i_local

        shl     eax, 9
        add     ebx, di_local

        mov     i_local, ebx
        xor     ebx, ebx

        mov     bl, 0deadbeefh[eax+2*edx] ; read from ltab
ltab_lo:
        mov     dl, (0deadbeefh+1)[eax+2*edx]
ltab_hi:
        mov     bx, 0deadbeefh[2*ebx]   ; read from clut
clut_lo:
        add     bx, (0deadbeefh+200h)[2*edx]
clut_hi:
        mov     [edi+2*ecx], bx         ; write pixel

        xor     edx, edx
        mov     eax, esi                ; copy u to u_temp

        mov     ebx, ebp                ; copy v to v_temp
        inc     ecx                     ; increment count

        jnz     pixel_loop              ; continue until done

        pop     ebp
skip_pixel_loop:

        mov     edx, x_frac[ebp]
        mov     ecx, x_left[ebp]

        add     ecx, dx_left[ebp]       ; update x_left
        add     edx, dx_frac[ebp]

        sbb     ebx, ebx                ; -1 if there was a carry
        mov     x_frac[ebp], edx

        mov     edx, u[ebp]
        sub     ecx, ebx                ; adjust x_left for carry

        mov     esi, i[ebp]
        mov     eax, dix[ebp]

        add     edx, du[ebp]
        add     esi, di_left[ebp]

        and     eax, ebx                ; adjust di for carry
        mov     x_left[ebp], ecx

        add     esi, eax                ; adjust i for carry
        mov     eax, dux[ebp]

        mov     i[ebp], esi
        and     eax, ebx                ; adjust du for carry

        mov     i_local, esi
        add     edx, eax                ; adjust u for carry

        mov     eax, dvx[ebp]
        mov     u[ebp], edx

        and     eax, ebx                ; adjust dv for carry
        mov     esi, v[ebp]

        mov     ebx, x_right[ebp]
        mov     edx, count

        add     ebx, dx_right[ebp]
        add     esi, dv[ebp]

        add     esi, eax                ; adjust v for carry
        mov     x_right[ebp], ebx

        sar     ebx, 16
        mov     edi, p_dest[ebp]

        mov     eax, row[ebp]           ; canvas row, that is
        mov     v[ebp], esi

        add     edi, eax
        dec     edx

        mov     p_dest[ebp], edi
        jnz     raster_loop

        pop     ebx
        pop     ecx

        pop     esi
        pop     edi

        pop     eax
        pop     edx
        pop     ebp
        ret

_FUNCDEF opaque_clut_lit_8to16_il_init, 1
        push    esi
        push    edi

        mov     ebx, _grd_canvas
        mov     edi, last_clut

        mov     edx, _grd_ltab816
        mov     esi, last_ltab

        mov     ebx, GRS_CANVAS_GC+GRS_CONTEXT_FILL_PARM[ebx]
        cmp     ebx, edi

        jz      skip_clut

        mov     d ds:[clut_lo-4], ebx
        lea     ecx, [ebx+200h]
        mov     d ds:[clut_hi-4], ecx
        mov     last_clut, ebx
skip_clut:
        mov     ebx, GRS_BITMAP_BITS[eax]
        cmp     esi, edx

        jz      skip_ltab

        mov     d ds:[ltab_lo-4], edx
        lea     ecx, [edx+1]
        mov     d ds:[ltab_hi-4], ecx
        mov     last_ltab, edx
skip_ltab:
        mov     edx, GRS_BITMAP_W[eax]  ;width AND height
        mov     al, GRS_BITMAP_WLOG[eax]

        mov     cl, 16
        sub     edx, 010001h            ;convert to masks

        sub     cl, al                  ;v_shift = 16-bm->wlog
        mov     eax, edx

        and     edx, 0ffff0000h         ;v_mask  = fix_make(bm->h-1,0)
        and     eax, 0ffffh             ;u_mask  = bm->w -1

        mov     esi, last_mask_u
        mov     edi, last_mask_v

        cmp     esi, eax
        jz      skip_mask_u

        mov     d ds:[mask_u-4], eax
        mov     last_mask_u, eax
skip_mask_u:
        cmp     edi, edx
        jz      skip_mask_v

        mov     d ds:[mask_v-4], edx
        mov     last_mask_v, edx
skip_mask_v:

        mov     eax, last_p_src
        mov     dl, last_shift_v

        cmp     eax, ebx
        jz      skip_p_src

        mov     d ds:[p_src-4], ebx
        mov     last_p_src, ebx
skip_p_src:
        cmp     dl, cl
        jz      skip_shift_v

        mov     b ds:[shift_v-1], cl
        mov     last_shift_v, cl
skip_shift_v:

        pop     edi
        pop     esi

        ret

_TEXT ends
end
