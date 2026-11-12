// game/Convert.c -- Clock Tower 3 module 0x018518d0..0x01851bf0: ConvertMatrix2Quat, ConvertQuat2Matrix.
// One local run (symtab chunk 71: const_vector_A and the "divide by 0" literal) ties the two together.
// Both are VU0 macro-mode code in the ROM; rewritten here in scalar C from the disassembly, operation by
// operation (each ACC chain keeps the ROM's order of terms).
// A quaternion is { x, y, z, w }; a matrix is four rows of four floats, rows 0..2 the rotation.

#include "game/Convert.h"
#include "game/unassigned_data.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

// CT3 0x01937df0 (const_vector_A): x = the 1 of each 1 + diagonal sum, y = the 2 of the divisor 2s,
// w = the 1/2 of the leading component s/2.
static const float const_vector_A[4] = { 1.0f, 2.0f, 0.0f, 0.5f };

#define CV_ONE  const_vector_A[0]
#define CV_TWO  const_vector_A[1]
#define CV_HALF const_vector_A[3]

// The ROM's vsqrt takes the root of |x|.
static float vuSqrt(float x)
{
    return sqrtf(fabsf(x));
}

// CT3 0x018518d0
// Shepperd's method. With 1 + trace > 1: w = s/2 with s = sqrt(1 + trace), x y z the off-diagonal
// differences over 2s. Otherwise the largest diagonal element leads (x when m00 > m11 and m00 >= m22,
// else y when m11 >= m22, else z): it is s/2, the other two are sums over 2s, w a difference. A zero
// divisor (2s == 0) prints "divide by 0" and answers the unit quaternion; the trace branch returns before
// that test (s > 1 there).
void ConvertMatrix2Quat(float *q, float (*m)[4])
{
    float m00 = m[0][0], m01 = m[0][1], m02 = m[0][2];
    float m10 = m[1][0], m11 = m[1][1], m12 = m[1][2];
    float m20 = m[2][0], m21 = m[2][1], m22 = m[2][2];
    float sum, s, twoS, inv;

    sum = CV_ONE + m00;
    sum += CV_ONE * m11;
    sum += CV_ONE * m22;
    s = vuSqrt(sum);
    if (1.0f < sum) {
        inv = 1.0f / (CV_TWO * s);
        q[0] = m21 * inv - m12 * inv;
        q[1] = m02 * inv - m20 * inv;
        q[2] = m10 * inv - m01 * inv;
        q[3] = s * CV_HALF;
        return;
    }

    if (m11 < m00 && !(m00 < m22)) {
        // x leads
        sum = CV_ONE + m00;
        sum -= CV_ONE * m11;
        sum -= CV_ONE * m22;
        s = vuSqrt(sum);
        twoS = CV_TWO * s;
        inv = 1.0f / twoS;
        q[0] = s * CV_HALF;
        q[1] = m10 * inv + m01 * inv;
        q[2] = m20 * inv + m02 * inv;
        q[3] = m21 * inv - m12 * inv;
    } else if (!(m11 < m00) && !(m11 < m22)) {
        // y leads
        sum = CV_ONE + m11;
        sum -= CV_ONE * m22;
        sum -= CV_ONE * m00;
        s = vuSqrt(sum);
        twoS = CV_TWO * s;
        inv = 1.0f / twoS;
        q[0] = m01 * inv + m10 * inv;
        q[1] = s * CV_HALF;
        q[2] = m21 * inv + m12 * inv;
        q[3] = m02 * inv - m20 * inv;
    } else {
        // z leads
        sum = CV_ONE + m22;
        sum -= CV_ONE * m00;
        sum -= CV_ONE * m11;
        s = vuSqrt(sum);
        twoS = CV_TWO * s;
        inv = 1.0f / twoS;
        q[0] = m02 * inv + m20 * inv;
        q[1] = m12 * inv + m21 * inv;
        q[2] = s * CV_HALF;
        q[3] = m10 * inv - m01 * inv;
    }

    if (twoS == 0.0f) {
        printf("divide by 0");
        q[0] = 0.0f;
        q[1] = 0.0f;
        q[2] = 0.0f;
        q[3] = 1.0f;
    }
}

// CT3 0x01851b10
// w == 1 exactly copies matUnit; otherwise the rotation of the unit quaternion, row 3 = (0,0,0,1) and
// every row's w = 0 (with x2 = 2x ..):
//   0: 1 - z*z2 - y*y2,  x*y2 - w*z2,      w*y2 + x*z2
//   1: x*y2 + w*z2,      1 - z*z2 - x*x2,  y*z2 - w*x2
//   2: x*z2 - w*y2,      y*z2 + w*x2,      1 - x*x2 - y*y2
void ConvertQuat2Matrix(float (*m)[4], float *q)
{
    float x, y, z, w, x2, y2, z2;
    float r0[4], r1[4], r2[4];

    if (q[3] == 1.0f) {
        memcpy(m, matUnit, sizeof(float) * 16);
        return;
    }
    x = q[0];
    y = q[1];
    z = q[2];
    w = q[3];
    x2 = x + x;
    y2 = y + y;
    z2 = z + z;

    r1[0] = x * y2 + w * z2;
    r1[1] = 1.0f - z * z2 - x * x2;
    r1[2] = -(w * x2) + y * z2;
    r1[3] = 0.0f;
    r2[0] = x * z2 - w * y2;
    r2[1] = y * z2 + w * x2;
    r2[2] = 1.0f - x * x2 - y * y2;
    r2[3] = 0.0f;
    r0[0] = 1.0f - z * z2 - y * y2;
    r0[1] = x * y2 - w * z2;
    r0[2] = w * y2 + x * z2;
    r0[3] = 0.0f;

    // the ROM stores row 3 first, then 2, 1, 0
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
    memcpy(m[2], r2, sizeof(r2));
    memcpy(m[1], r1, sizeof(r1));
    memcpy(m[0], r0, sizeof(r0));
}
