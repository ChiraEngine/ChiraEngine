#include "MeshDynamic.h"

using namespace chira;

void MeshDynamic::render(glm::mat4 parentTransform) {
    this->mesh.render(transformToMatrix(parentTransform, this->position, this->rotation));
    Entity::render(parentTransform);
}

MeshDataBuilder* MeshDynamic::getMesh() {
    return &this->mesh;
}
