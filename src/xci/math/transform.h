// transform.h created on 2023-11-25 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_MATH_TRANSFORM_H
#define XCI_MATH_TRANSFORM_H

#include "Mat4.h"
#include "Vec3.h"
#include <cmath>

namespace xci {


/// Build perspective projection matrix for OpenGL / Vulkan
/// \param fov_y    Vertical field of view, in radians
/// \param aspect   Aspect ratio (viewport width / height)
/// \param near_z   Min distance from camera
/// \param far_z    Max distance from camera
/// \returns Constructed matrix, column-major
/// Example: `auto m = perspective_projection(1.2, 4.0 / 3.0, 1.0, 100.0)`
template <typename T>
Mat4<T> perspective_projection(T fov_y, T aspect, T near_z, T far_z)
{
    const T t = std::tan(fov_y / T(2));
    const T fy = T(1) / t;
    const T fx = T(1) / (t * aspect);
    const T zs = -(far_z + near_z) / (far_z - near_z);
    const T zt = -(T(2) * far_z * near_z) / (far_z - near_z);
    return {
        fx, 0,  0,   0,
        0,  fy, 0,   0,
        0,  0,  zs, -1,
        0,  0,  zt,  0,
    };
}


/// Build view matrix for classic look-at camera
/// \param eye      Camera coords, e.g. {0,0,0}
/// \param target   Point where is the camera looking, e.g. {0,1,0} for positive Y direction from {0,0,0}
/// \param up_norm  Normalized up vector, e.g. {0,0,1} for Z-up
/// \returns Constructed matrix, column-major
/// Example: `auto m = lookat_view({0,0,0}, {0,1,0}, {0,0,1})`
template <typename T>
Mat4<T> look_at_view(const Vec3<T>& eye, const Vec3<T>& target, const Vec3<T>& up_norm)
{
    const Vec3<T> forward = (eye - target).normalize();   // Z axis
    const Vec3<T> right = up_norm.cross(forward).normalize();  // X axis
    const Vec3<T> up = forward.cross(right);  // Y axis
    const Vec3<T> p {-right.dot(eye), -up.dot(eye), -forward.dot(eye)};
    return {
        right.x, up.x, forward.x, 0,
        right.y, up.y, forward.y, 0,
        right.z, up.z, forward.z, 0,
        p.x, p.y, p.z, 1,
    };
}


/// Build view matrix for look-around camera (mouse controlled free look)
/// See https://en.wikipedia.org/wiki/Aircraft_principal_axes
///
/// \param eye      Camera coords, e.g. {0,0,0}
/// \param yaw      Rotation about Z axis, in radians, 0 .. 2*PI
/// \param pitch    Rotation about X axis, in radians, -0.5*PI .. 0.5*PI
/// \param roll     Rotation about Y axis, in radians, -0.5*PI .. 0.5*PI
/// \returns Constructed matrix, column-major
/// Example: `auto m = lookat_view({0,0,0}, {0,1,0}, {0,0,1})`
template <typename T>
Mat4<T> look_around_view(const Vec3<T>& eye, T yaw, T pitch, T roll)
{
    return ( Mat4<T>::translate(eye)
           * Mat4<T>::rot_z(std::cos(yaw), std::sin(yaw))
           * Mat4<T>::rot_y(std::cos(roll), std::sin(roll))
           * Mat4<T>::rot_x(std::cos(pitch), std::sin(pitch))).inverse();
}


} // namespace xci

#endif // include guard
