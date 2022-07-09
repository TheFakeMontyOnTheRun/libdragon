#include "gl_internal.h"
#include <string.h>

extern gl_state_t state;

void gl_matrix_init()
{
    state.modelview_stack = (gl_matrix_stack_t) {
        .storage = state.modelview_stack_storage,
        .size = MODELVIEW_STACK_SIZE,
    };

    state.projection_stack = (gl_matrix_stack_t) {
        .storage = state.projection_stack_storage,
        .size = PROJECTION_STACK_SIZE,
    };

    state.texture_stack = (gl_matrix_stack_t) {
        .storage = state.texture_stack_storage,
        .size = TEXTURE_STACK_SIZE,
    };

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

gl_matrix_t * gl_matrix_stack_get_matrix(gl_matrix_stack_t *stack)
{
    return &stack->storage[stack->cur_depth];
}

void gl_update_current_matrix()
{
    state.current_matrix = gl_matrix_stack_get_matrix(state.current_matrix_stack);
}

void gl_matrix_mult(GLfloat *d, const gl_matrix_t *m, const GLfloat *v)
{
    d[0] = m->m[0][0] * v[0] + m->m[1][0] * v[1] + m->m[2][0] * v[2] + m->m[3][0] * v[3];
    d[1] = m->m[0][1] * v[0] + m->m[1][1] * v[1] + m->m[2][1] * v[2] + m->m[3][1] * v[3];
    d[2] = m->m[0][2] * v[0] + m->m[1][2] * v[1] + m->m[2][2] * v[2] + m->m[3][2] * v[3];
    d[3] = m->m[0][3] * v[0] + m->m[1][3] * v[1] + m->m[2][3] * v[2] + m->m[3][3] * v[3];
}

void gl_matrix_mult3x3(GLfloat *d, const gl_matrix_t *m, const GLfloat *v)
{
    d[0] = m->m[0][0] * v[0] + m->m[1][0] * v[1] + m->m[2][0] * v[2];
    d[1] = m->m[0][1] * v[0] + m->m[1][1] * v[1] + m->m[2][1] * v[2];
    d[2] = m->m[0][2] * v[0] + m->m[1][2] * v[1] + m->m[2][2] * v[2];
}

void gl_matrix_mult_full(gl_matrix_t *d, const gl_matrix_t *l, const gl_matrix_t *r)
{
    gl_matrix_mult(d->m[0], l, r->m[0]);
    gl_matrix_mult(d->m[1], l, r->m[1]);
    gl_matrix_mult(d->m[2], l, r->m[2]);
    gl_matrix_mult(d->m[3], l, r->m[3]);
}

void gl_update_final_matrix()
{
    if (state.matrix_mode != GL_TEXTURE) {
        gl_matrix_mult_full(&state.final_matrix, gl_matrix_stack_get_matrix(&state.projection_stack), gl_matrix_stack_get_matrix(&state.modelview_stack));
    }
}

void glMatrixMode(GLenum mode)
{
    switch (mode) {
    case GL_MODELVIEW:
        state.current_matrix_stack = &state.modelview_stack;
        break;
    case GL_PROJECTION:
        state.current_matrix_stack = &state.projection_stack;
        break;
    case GL_TEXTURE:
        state.current_matrix_stack = &state.texture_stack;
        break;
    default:
        gl_set_error(GL_INVALID_ENUM);
        return;
    }

    state.matrix_mode = mode;

    gl_update_current_matrix();
}

void glLoadMatrixf(const GLfloat *m)
{
    memcpy(state.current_matrix, m, sizeof(gl_matrix_t));
    gl_update_final_matrix();
}

void glLoadMatrixd(const GLdouble *m)
{
    for (size_t i = 0; i < 16; i++)
    {
        state.current_matrix->m[i/4][i%4] = m[i];
    }
    gl_update_final_matrix();
}

void glMultMatrixf(const GLfloat *m)
{
    gl_matrix_t tmp = *state.current_matrix;
    gl_matrix_mult_full(state.current_matrix, &tmp, (gl_matrix_t*)m);
    gl_update_final_matrix();
}

void glMultMatrixd(const GLdouble *m);

void glLoadIdentity(void)
{
    *state.current_matrix = (gl_matrix_t){ .m={
        {1,0,0,0},
        {0,1,0,0},
        {0,0,1,0},
        {0,0,0,1},
    }};

    gl_update_final_matrix();
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    float c = cosf(angle);
    float s = sinf(angle);
    float ic = 1.f - c;

    float mag = sqrtf(x*x + y*y + z*z);
    x /= mag;
    y /= mag;
    z /= mag;

    gl_matrix_t rotation = (gl_matrix_t){ .m={
        {x*x*ic+c,   y*x*ic+z*s, z*x*ic-y*s, 0.f},
        {x*y*ic-z*s, y*y*ic+c,   z*y*ic+x*s, 0.f},
        {x*z*ic+y*s, y*z*ic-x*s, z*z*ic+c,   0.f},
        {0.f,        0.f,        0.f,        1.f},
    }};

    glMultMatrixf(rotation.m[0]);
}
void glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    gl_matrix_t translation = (gl_matrix_t){ .m={
        {1.f, 0.f, 0.f, 0.f},
        {0.f, 1.f, 0.f, 0.f},
        {0.f, 0.f, 1.f, 0.f},
        {x,   y,   z,   1.f},
    }};

    glMultMatrixf(translation.m[0]);
}
void glTranslated(GLdouble x, GLdouble y, GLdouble z);

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    gl_matrix_t scale = (gl_matrix_t){ .m={
        {x,   0.f, 0.f, 0.f},
        {0.f, y,   0.f, 0.f},
        {0.f, 0.f, z,   0.f},
        {0.f, 0.f, 0.f, 1.f},
    }};

    glMultMatrixf(scale.m[0]);
}
void glScaled(GLdouble x, GLdouble y, GLdouble z);

void glFrustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f)
{
    gl_matrix_t frustum = (gl_matrix_t){ .m={
        {(2*n)/(r-l), 0.f,           0.f,            0.f},
        {0.f,         (2.f*n)/(t-b), 0.f,            0.f},
        {(r+l)/(r-l), (t+b)/(t-b),   -(f+n)/(f-n),   -1.f},
        {0.f,         0.f,           -(2*f*n)/(f-n), 0.f},
    }};

    glMultMatrixf(frustum.m[0]);

    state.persp_norm_factor = 2.0f / (n + f);
}

void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f)
{
    gl_matrix_t ortho = (gl_matrix_t){ .m={
        {2.0f/(r-l),   0.f,          0.f,          0.f},
        {0.f,          2.0f/(t-b),   0.f,          0.f},
        {0.f,          0.f,          2.0f/(f-n),   0.f},
        {-(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), 1.f},
    }};

    glMultMatrixf(ortho.m[0]);

    state.persp_norm_factor = 1.0f;
}

void glPushMatrix(void)
{
    gl_matrix_stack_t *stack = state.current_matrix_stack;

    int32_t new_depth = stack->cur_depth + 1;
    if (new_depth >= stack->size) {
        gl_set_error(GL_STACK_OVERFLOW);
        return;
    }

    stack->cur_depth = new_depth;
    memcpy(&stack->storage[new_depth], &stack->storage[new_depth-1], sizeof(gl_matrix_t));

    gl_update_current_matrix();
}

void glPopMatrix(void)
{
    gl_matrix_stack_t *stack = state.current_matrix_stack;

    int32_t new_depth = stack->cur_depth - 1;
    if (new_depth < 0) {
        gl_set_error(GL_STACK_UNDERFLOW);
        return;
    }

    stack->cur_depth = new_depth;

    gl_update_current_matrix();
}
