#include <matrix.h>
#include <r3d.h>
#include <string.h>
#include <dbg.h>

extern void _r3_concat_matrix(mxs_matrix *m, mxs_angvec *a, r3e_order o);

static void (*rot_table[3])(mxs_matrix*, const mxs_matrix*, mxs_ang) = {
    mx_rot_x_mat, mx_rot_y_mat, mx_rot_z_mat
};

struct sMxStack {
    mxs_trans o2w;
    mxs_trans base[8];
    mxs_trans* cur;
    int size;
};

struct sMxStack stack;

void push_stack()
{
    if (stack.size == 8)
    {
        Warning(("little stack exceeded more than %d\n", 8));
    }
    else
    {
        stack.cur = (mxs_trans *)memcpy(stack.cur, &stack.o2w, sizeof(mxs_trans)) + 1;
        ++stack.size;
    }
}

void pop_stack()
{
    if (stack.size == 0)
    {
        Warning(("r3d_end_object called on empty stack\n"));
    }
    else
    {
        --stack.cur;
        memcpy(&stack, stack.cur, sizeof(mxs_trans));
        --stack.size;
    }
}

void stack_init()
{
    stack.size = 0;
    stack.cur = stack.base;
    mx_identity_trans(&stack.o2w);
}

void stack_start_object(mxs_vector *v)
{
    mxs_vector vtmp;

    push_stack();
    mx_mat_mul_vec(&vtmp, &stack.o2w.mat, v);
    mx_addeq_vec(&stack.o2w.vec, &vtmp);
}

void stack_start_object_angles(mxs_vector *v, mxs_angvec *a, r3e_order o)
{
    mxs_trans tmp;

    push_stack();
    _r3_concat_matrix(&tmp.mat, a, o);
    tmp.vec = *v;
    mx_mul_trans(&stack.o2w, (const mxs_trans *)stack.cur - 1, &tmp);
}

void stack_start_object_trans(mxs_trans *t)
{
    push_stack();
    mx_mul_trans(&stack.o2w, (const mxs_trans *)stack.cur - 1, t);
}

void stack_start_object_n(mxs_vector *v, unsigned short ang, int n)
{
    mxs_vector vtmp;

#ifdef DBG_ON
    if (n < 0 || n > 2)
    {
        Warning(("r3_start_object_n: angle arg outside of 0-2\n"));
        return;
    }
#endif

    push_stack();
    mx_mat_mul_vec(&vtmp, &stack.o2w.mat, v);
    mx_addeq_vec(&stack.o2w.vec, &vtmp);
    rot_table[n](&stack.o2w.mat, &stack.cur[-1].mat, ang);
}

void stack_end_object()
{
    pop_stack();
}

mxs_trans *stack_o2w()
{
    return &stack.o2w;
}

void stack_transform_o2w(mxs_vector *dst, mxs_vector *src)
{
    mx_trans_mul_vec(dst, &stack.o2w, src);
}
