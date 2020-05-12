#include "hilma/io/obj.h"

#include <stdio.h>
#include <iostream>
#include <limits>
#include <iomanip>
#include <fstream>
#include <cstdio>
#include <cassert>

#define TINYOBJLOADER_IMPLEMENTATION
#include "hilma/io/tiny_obj_loader.h"

#ifndef HILMA_LINE_MAX
#define HILMA_LINE_MAX 2048
#endif

#include "hilma/fs.h"
#include "hilma/text.h"

namespace hilma {


Material InitMaterial (const tinyobj::material_t& _material) {
    Material mat;
    mat.name = toLower( toUnderscore( purifyString( _material.name ) ) );
    mat.diffuse = glm::vec3(_material.diffuse[0], _material.diffuse[1], _material.diffuse[2]);
    mat.diffuse_map = _material.diffuse_texname;

    mat.specular = glm::vec3(_material.specular[0], _material.specular[1], _material.specular[2]);
    mat.specular_map = _material.specular_texname;
    
    mat.emissive = glm::vec3(_material.emission[0], _material.emission[1], _material.emission[2]);
    mat.emissive_map = _material.emissive_texname;

    mat.roughness = _material.roughness;
    mat.roughness_map = _material.roughness_texname;

    mat.metallic = _material.metallic;
    mat.metallic_map = _material.metallic_texname;

    mat.normal_map = _material.normal_texname;
    mat.bump_map = _material.bump_texname;
    mat.opacity_map = _material.alpha_texname;
    mat.displacement_map = _material.displacement_texname;

    mat.sheen = _material.sheen;
    mat.sheen_map = _material.sheen_texname;

    mat.shininess = _material.shininess;

    mat.anisotropy = _material.anisotropy;
    mat.anisotropy_rotation = _material.anisotropy_rotation;

    mat.clearcoat_roughness = _material.clearcoat_roughness;
    mat.clearcoat_thickness = _material.clearcoat_thickness;

    mat.ior = _material.ior;
    mat.dissolve = _material.dissolve;
    mat.transmittance = glm::vec3(_material.transmittance[0], _material.transmittance[1], _material.transmittance[2]);
    mat.reflection_map = _material.reflection_texname;
    mat.specular_highlight_map = _material.specular_highlight_texname;

    mat.illumination_model = _material.illum;
    return mat;
}

glm::vec3 getVertex(const tinyobj::attrib_t& _attrib, int _index) {
    return glm::vec3(   _attrib.vertices[3 * _index + 0],
                        _attrib.vertices[3 * _index + 1],
                        _attrib.vertices[3 * _index + 2]);
}

glm::vec4 getColor(const tinyobj::attrib_t& _attrib, int _index) {
    return glm::vec4(   _attrib.colors[3 * _index + 0],
                        _attrib.colors[3 * _index + 1],
                        _attrib.colors[3 * _index + 2],
                        1.0);
}

glm::vec3 getNormal(const tinyobj::attrib_t& _attrib, int _index) {
    return glm::vec3(   _attrib.normals[3 * _index + 0],
                        _attrib.normals[3 * _index + 1],
                        _attrib.normals[3 * _index + 2]);
}

glm::vec2 getTexCoords(const tinyobj::attrib_t& _attrib, int _index) {
    return glm::vec2(   _attrib.texcoords[2 * _index], 
                        1.0f - _attrib.texcoords[2 * _index + 1]);
}

void calcNormal(const glm::vec3& _v0, const glm::vec3& _v1, const glm::vec3& _v2, glm::vec3& _N) {
    glm::vec3 v10 = _v1 - _v0;
    glm::vec3 v20 = _v2 - _v0;

    _N.x = v20.x * v10.z - v20.z * v10.y;
    _N.y = v20.z * v10.x - v20.x * v10.z;
    _N.z = v20.x * v10.y - v20.y * v10.x;
    
    _N = glm::normalize(_N);
}

// Check if `mesh_t` contains smoothing group id.
bool hasSmoothingGroup(const tinyobj::shape_t& shape) {
    for (size_t i = 0; i < shape.mesh.smoothing_group_ids.size(); i++)
        if (shape.mesh.smoothing_group_ids[i] > 0)
            return true;
        
    return false;
}

void computeSmoothingNormals(const tinyobj::attrib_t& _attrib, const tinyobj::shape_t& _shape, std::map<int, glm::vec3>& smoothVertexNormals) {
    smoothVertexNormals.clear();

    std::map<int, glm::vec3>::iterator iter;

    for (size_t f = 0; f < _shape.mesh.indices.size() / 3; f++) {
        // Get the three indexes of the face (all faces are triangular)
        tinyobj::index_t idx0 = _shape.mesh.indices[3 * f + 0];
        tinyobj::index_t idx1 = _shape.mesh.indices[3 * f + 1];
        tinyobj::index_t idx2 = _shape.mesh.indices[3 * f + 2];

        // Get the three vertex indexes and coordinates
        int vi[3];      // indexes
        vi[0] = idx0.vertex_index;
        vi[1] = idx1.vertex_index;
        vi[2] = idx2.vertex_index;

        glm::vec3 v[3];  // coordinates
        for (size_t i = 0; i < 3; i++)
            v[i] = getVertex(_attrib, vi[i]);

        // Compute the normal of the face
        glm::vec3 normal;
        calcNormal(v[0], v[1], v[2], normal);

        // Add the normal to the three vertexes
        for (size_t i = 0; i < 3; ++i) {
            iter = smoothVertexNormals.find(vi[i]);
            // add
            if (iter != smoothVertexNormals.end())
                iter->second += normal;
            else
                smoothVertexNormals[vi[i]] = normal;
        }
    }  // f

    // Normalize the normals, that is, make them unit vectors
    for (iter = smoothVertexNormals.begin(); iter != smoothVertexNormals.end(); iter++) {
        iter->second = glm::normalize(iter->second);
    }
}


bool loadObj( const std::string& _filename, Mesh& _mesh ) {

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;
    std::string base_dir = getBaseDir(_filename.c_str());
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, _filename.c_str(), base_dir.c_str());

    if (!warn.empty()) std::cout << "WARN: " << warn << std::endl;
    
    if (!err.empty()) std::cerr << err << std::endl;
    
    if (!ret) {
        std::cerr << "Failed to load " << _filename.c_str() << std::endl;
        return false;
    }

    for (size_t m = 0; m < materials.size(); m++)
        if (_mesh.materials.find( materials[m].name ) == _mesh.materials.end())
            _mesh.materials[ materials[m].name ] = InitMaterial( materials[m] );

    Mesh mesh;
    for (size_t s = 0; s < shapes.size(); s++) {
        mesh.name = shapes[s].name;;

        // std::string name = shapes[s].name;
        
        // if (name.empty())
        //     name = toString(s);

        // Check for smoothing group and compute smoothing normals
        std::map<int, glm::vec3> smoothVertexNormals;
        if (hasSmoothingGroup(shapes[s]) > 0)
            computeSmoothingNormals(attrib, shapes[s], smoothVertexNormals);

        // Mesh mesh;
        // Material mat;

        std::map<int, tinyobj::index_t> unique_indices;
        std::map<int, tinyobj::index_t>::iterator iter;
        
        int mi = -1;
        int mCounter = 0;
        INDEX_TYPE iCounter = 0;
        for (size_t i = 0; i < shapes[s].mesh.indices.size(); i++) {
            int f = (int)floor(i/3);

            tinyobj::index_t index = shapes[s].mesh.indices[i];
            int vi = index.vertex_index;
            int ni = index.normal_index;
            int ti = index.texcoord_index;

            // // Associate w material
            // if (shapes[s].mesh.material_ids.size() > 0) {
            //     int material_index = shapes[s].mesh.material_ids[f];

            //     if (mi != material_index) {

            //         // If there is a switch of material start a new mesh
            //         if (mi != -1 && mesh.getVertices().size() > 0) {

            //             // std::cout << "Adding model " << name  << "_" << toString(mCounter, 3, '0') << " w new material " << mat.name << std::endl;
                        
            //             // Add the model to the stack 
            //             addModel(_models, name + "_"+ toString(mCounter,3,'0'), mesh, mat, _verbose);
            //             mCounter++;

            //             // Restart the mesh
            //             iCounter = 0;
            //             mesh.clear();
            //             unique_indices.clear();
            //         }

            //         // assign the current material
            //         mi = material_index;
            //         mat = _materials[ materials[material_index].name ];
            //     }
            // }

            bool reuse = false;
            iter = unique_indices.find(vi);

            // if already exist 
            if (iter != unique_indices.end())
                // and have the same attributes
                if ((iter->second.normal_index == ni) &&
                    (iter->second.texcoord_index == ti) )
                    reuse = true;
            
            // Re use the vertex
            if (reuse)
                mesh.addFaceIndex( (INDEX_TYPE)iter->second.vertex_index );
            // Other wise create a new one
            else {
                unique_indices[vi].vertex_index = (int)iCounter;
                unique_indices[vi].normal_index = ni;
                unique_indices[vi].texcoord_index = ti;
                
                mesh.addVertex( getVertex(attrib, vi) );
                mesh.addColor( getColor(attrib, vi) );

                // If there is normals add them
                if (attrib.normals.size() > 0)
                    mesh.addNormal( getNormal(attrib, ni) );

                else if (smoothVertexNormals.size() > 0)
                    if ( smoothVertexNormals.find(vi) != smoothVertexNormals.end() )
                        mesh.addNormal( smoothVertexNormals.at(vi) );

                // If there is texcoords add them
                if (attrib.texcoords.size() > 0)
                    mesh.addTexCoord( getTexCoords(attrib, ti) );

                mesh.addFaceIndex( iCounter++ );
            }
        }

        // std::string meshName = name;
        // if (mCounter > 0)
        //     meshName = name + "_" + toString(mCounter, 3, '0');

    }

    return true;
}

bool saveObj( const std::string& _filename, const Mesh& _mesh ) {
    FILE * obj_file = fopen(_filename.c_str(), "w");
    if (NULL == obj_file) {
        printf("IOError: %s could not be opened for writing...", _filename.c_str());
        return false;
    }

    // https://github.com/libigl/libigl/blob/master/include/igl/writeOBJ.cpp
    fprintf(obj_file,"# generated with Hilma by Patricio Gonzalez Vivo\n");
    fprintf(obj_file,"o %s\n", _mesh.getName().c_str());

    // TODO:
    //      - MATERIALS

    // Loop over V
    for(size_t i = 0; i < _mesh.vertices.size(); i++)
        fprintf(obj_file,"v %0.17g %0.17g %0.17g\n", _mesh.vertices[i].x, _mesh.vertices[i].y, _mesh.vertices[i].z); 
    // fprintf(obj_file,"\n");

    bool write_normals = _mesh.haveNormals();
    if (write_normals) {
        for (size_t i = 0; i < _mesh.normals.size(); i++)
            fprintf(obj_file,"vn %0.17g %0.17g %0.17g\n", _mesh.normals[i].x, _mesh.normals[i].y, _mesh.normals[i].z);
        // fprintf(obj_file,"\n");
    }

    bool write_texture_coords = _mesh.haveTexCoords();
    if (write_texture_coords) {
        for (size_t i = 0; i < _mesh.texcoords.size(); i++)
            fprintf(obj_file, "vt %0.17g %0.17g\n", _mesh.texcoords[i].x, _mesh.texcoords[i].y);
        // fprintf(obj_file,"\n");
    }

    if (_mesh.haveFaceIndices()) {
        std::vector<glm::ivec3> faces = _mesh.getTrianglesIndices();
        for (size_t i = 0; i < faces.size(); i++) {
            fprintf(obj_file,"f");

            for(int j = 0; j < 3; j++) {
                fprintf(obj_file," %u", faces[i][j] + 1);

                if(write_texture_coords)
                    fprintf(obj_file,"/%u", faces[i][j] + 1);
            
                if(write_normals) {
                    if (write_texture_coords)
                        fprintf(obj_file,"/%u", faces[i][j] + 1);
                    else
                        fprintf(obj_file,"//%u", faces[i][j] + 1);
                }
            }

            fprintf(obj_file,"\n");
        }
    }
    fclose(obj_file);

    return true;
};


}

