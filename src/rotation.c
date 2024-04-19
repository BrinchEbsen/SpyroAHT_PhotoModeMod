#include <common.h>
#include <symbols.h>

/*
mat44 EulerToMatrix(float x, float y, float z) {
    float sx = ig_sinf(-x); float sy = ig_sinf(-y); float sz = ig_sinf(-z);
    float cx = ig_cosf(-x); float cy = ig_cosf(-y); float cz = ig_cosf(-z);

    mat44 res = {
        { .x = cy*cz,               .y = (-cy)*sz,         .z = sy,       .w = 0},
        { .x = cx*sz + cz*sx*sy,    .y = cx*cz - sx*sy*sz, .z = (-cy)*sx, .w = 0},
        { .x = (-cx)*cz*sy + sx*sz, .y = cx*sy*sz + cz*sx, .z = cx*cy,    .w = 0},
        { .x = 0,                   .y = 0,                .z = 0,        .w = 1}
    };

    return res;
}
*/

EXVector RotateAroundPoint(EXVector *center, EXVector *p, mat44* m, bool inverse) {
    EXVector loc = {0};

    //Translate to local space
    loc.x = p->x - center->x;
    loc.y = p->y - center->y;
    loc.z = p->z - center->z;

    EXVector result;
    if (inverse) {
        result = (EXVector){
            .x = m->row0.x * loc.x + m->row1.x * loc.y + m->row2.x * loc.z,
            .y = m->row0.y * loc.x + m->row1.y * loc.y + m->row2.y * loc.z,
            .z = m->row0.z * loc.x + m->row1.z * loc.y + m->row2.z * loc.z,
            .w = 0
        };
    } else {
        result = (EXVector){
            .x = m->row0.x * loc.x + m->row0.y * loc.y + m->row0.z * loc.z,
            .y = m->row1.x * loc.x + m->row1.y * loc.y + m->row1.z * loc.z,
            .z = m->row2.x * loc.x + m->row2.y * loc.y + m->row2.z * loc.z,
            .w = 0
        };
    }

    //Translate back to global space
    result.x += center->x;
    result.y += center->y;
    result.z += center->z;

    return result;
}