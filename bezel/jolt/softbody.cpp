//
// softbody.cpp
// As part of the Atlas project
// Copyright (c) 2026 Neutral Software. All rights reserved.
// atlasengine.org | github.com/neutralsoftware
// --------------------------------------------------
// Description: Softbody physics implementation for Bezel
// SPDX-License-Identifier: MIT
//

#include "atlas/units.h"
#include "atlas/object.h"
#include "bezel/bezel.h"
#include <cstddef>
#include <vector>

#ifndef BEZEL_NATIVE

namespace bezel {

void Softbody::createMesh(CoreObject *object) {
    this->object = object;
    std::vector<Position3d> surfaceVertices;
    surfaceVertices.reserve(object->vertices.size());

    for (const CoreVertex &vertex : object->vertices) {
        surfaceVertices.push_back(vertex.position);
    }

    std::vector<SoftbodyTriangle> surfaceTriangles;

    for (size_t i = 0; i + 2 < object->indices.size(); i += 3) {
        SoftbodyTriangle triangle;
        triangle.a = object->indices[i];
        triangle.b = object->indices[i + 1];
        triangle.c = object->indices[i + 2];
        surfaceTriangles.push_back(triangle);
    }
}

} // namespace bezel

#endif
