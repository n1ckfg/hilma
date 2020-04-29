#include <string>
#include <sstream>
#include <unistd.h>
#include <iostream>

#include "hilma/Mesh.h"
#include "hilma/io/PlyOps.h"

int main(int argc, char **argv) {

    int size = 1024;

    hilma::Mesh mesh;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float u = x/float(size-1);
            float v = y/float(size-1);
            mesh.addVertex(u, v, 1.0f);
        }
    }

    hilma::PlyOps::save("plane.ply", mesh, false);

    return 1;
}