#ifndef ROTATION_H
#define ROTATION_H
#include <common.h>

//Convert euler angles to rotation matrix
//mat44 EulerToMatrix(float x, float y, float z);

//Rotate a point around another point using a rotation matrix
EXVector RotateAroundPoint(EXVector *center, EXVector *p, mat44* m, bool inverse);

#endif /* ROTATION_H */