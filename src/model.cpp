#include "model.hpp"

#include "happly.h"

using namespace gl;

struct Vertex {

    float position[3];
    float color[4];
};

void Model::load(const char* path)
{
    // Chargement des données du fichier .ply.
    // Ne modifier pas cette partie.
    happly::PLYData plyIn(path);

    happly::Element& vertex = plyIn.getElement("vertex");
    std::vector<float> positionX = vertex.getProperty<float>("x");
    std::vector<float> positionY = vertex.getProperty<float>("y");
    std::vector<float> positionZ = vertex.getProperty<float>("z");

    std::vector<unsigned char> colorRed   = vertex.getProperty<unsigned char>("red");
    std::vector<unsigned char> colorGreen = vertex.getProperty<unsigned char>("green");
    std::vector<unsigned char> colorBlue  = vertex.getProperty<unsigned char>("blue");

    // Tableau de faces, une face est un tableau d'indices.
    // Les faces sont toutes des triangles dans nos modèles (donc 3 indices par face).
    std::vector<std::vector<unsigned int>> facesIndices = plyIn.getFaceIndices<unsigned int>();

    int nVertices = positionX.size();
    std::vector<Vertex> vertices;
    float floatConversion = 255.0f;
    for (int i = 0; i < nVertices; i++ ) {

        Vertex v;

        v.position[0] = positionX[i];
        v.position[1] = positionY[i];
        v.position[2] = positionZ[i];


        v.color[0] = colorRed[i] / floatConversion;
        v.color[1] = colorGreen[i] / floatConversion;
        v.color[2] = colorBlue[i] / floatConversion;
        v.color[3] = 1.0f;
        vertices.push_back(v);
    }

    std::vector<unsigned int> elements;

    for (auto i : facesIndices) {
        for (auto j : i) {
            elements.push_back(j);
        }

    }

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(unsigned int), elements.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glVertexAttribPointer(1,4,GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
    glEnableVertexAttribArray( 0 );
    glEnableVertexAttribArray( 1 );
    glBindVertexArray(0);
    count_ = elements.size();
}

Model::~Model()
{
    release();
}

void Model::release()
{
    if (!vao_ && !vbo_ && !ebo_)
        return;
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ebo_);
    vao_ = vbo_ = ebo_ = 0;
    count_ = 0;
}

void Model::draw()
{
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES,count_,GL_UNSIGNED_INT,0);
}

