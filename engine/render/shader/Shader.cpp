#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>
#include <core/Logger.h>
#include <i18n/TranslationManager.h>
#include <resource/ShaderResource.h>
#include "UBO.h"

using namespace chira;

CHIRA_CREATE_LOG(SHADER);

Shader::Shader(std::string identifier_)
    : PropertiesResource(std::move(identifier_))
    , HandleObject<int>() {}

void Shader::compile(const nlohmann::json& properties) {
    this->handle = glCreateProgram();
    Serialize::fromJSON(this, properties);
    glLinkProgram(this->handle);
#if DEBUG
    this->checkForCompilationErrors();
#endif
    if (this->usesPV) {
        PerspectiveViewUBO::get()->bindToShader(this);
    }
    if (this->lit) {
        LightsUBO::get()->bindToShader(this);
    }
}

Shader::~Shader() {
    glDeleteProgram(this->handle);
}

void Shader::use() const {
    glUseProgram(this->handle);
}

void Shader::checkForCompilationErrors() const {
    int success = 0;
    char infoLog[512];
    glGetProgramiv(this->handle, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(this->handle, 512, nullptr, infoLog);
        LOG_SHADER.error(TRF("error.opengl.shader_linking", infoLog));
    }
}

void Shader::setUniform(std::string_view name, bool value) const {
    glUniform1i(glGetUniformLocation(this->handle, name.data()), static_cast<int>(value));
}

void Shader::setUniform(std::string_view name, int value) const {
    glUniform1i(glGetUniformLocation(this->handle, name.data()), value);
}

void Shader::setUniform(std::string_view name, unsigned int value) const {
    glUniform1ui(glGetUniformLocation(this->handle, name.data()), value);
}

void Shader::setUniform(std::string_view name, float value) const {
    glUniform1f(glGetUniformLocation(this->handle, name.data()), value);
}

void Shader::setUniform(std::string_view name, glm::vec2b value) const {
    glUniform2i(glGetUniformLocation(this->handle, name.data()), static_cast<int>(value.x), static_cast<int>(value.y));
}

void Shader::setUniform(std::string_view name, glm::vec2ui value) const {
    glUniform2ui(glGetUniformLocation(this->handle, name.data()), value.x, value.y);
}

void Shader::setUniform(std::string_view name, glm::vec2i value) const {
    glUniform2i(glGetUniformLocation(this->handle, name.data()), value.x, value.y);
}

void Shader::setUniform(std::string_view name, glm::vec2f value) const {
    glUniform2f(glGetUniformLocation(this->handle, name.data()), value.x, value.y);
}

void Shader::setUniform(std::string_view name, glm::vec3b value) const {
    glUniform3i(glGetUniformLocation(this->handle, name.data()), static_cast<int>(value.x), static_cast<int>(value.y), static_cast<int>(value.z));
}

void Shader::setUniform(std::string_view name, glm::vec3ui value) const {
    glUniform3ui(glGetUniformLocation(this->handle, name.data()), value.x, value.y, value.z);
}

void Shader::setUniform(std::string_view name, glm::vec3i value) const {
    glUniform3i(glGetUniformLocation(this->handle, name.data()), value.x, value.y, value.z);
}

void Shader::setUniform(std::string_view name, glm::vec3f value) const {
    glUniform3f(glGetUniformLocation(this->handle, name.data()), value.x, value.y, value.z);
}

void Shader::setUniform(std::string_view name, glm::vec4b value) const {
    glUniform4i(glGetUniformLocation(this->handle, name.data()), static_cast<int>(value.x), static_cast<int>(value.y), static_cast<int>(value.z), static_cast<int>(value.w));
}

void Shader::setUniform(std::string_view name, glm::vec4ui value) const {
    glUniform4ui(glGetUniformLocation(this->handle, name.data()), value.x, value.y, value.z, value.w);
}

void Shader::setUniform(std::string_view name, glm::vec4i value) const {
    glUniform4i(glGetUniformLocation(this->handle, name.data()), value.x, value.y, value.z, value.w);
}

void Shader::setUniform(std::string_view name, glm::vec4f value) const {
    glUniform4f(glGetUniformLocation(this->handle, name.data()), value.x, value.y, value.z, value.w);
}

void Shader::setUniform(std::string_view name, glm::mat4 value) const {
    glUniformMatrix4fv(glGetUniformLocation(this->handle, name.data()), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVertexShader(std::string path) {
    this->vertexPath = std::move(path);
    auto vert = Resource::getUniqueUncachedResource<ShaderResource>(this->vertexPath, GL_VERTEX_SHADER);
    glAttachShader(this->handle, vert->getHandle());
}

void Shader::setFragmentShader(std::string path) {
    this->fragmentPath = std::move(path);
    auto frag = Resource::getUniqueUncachedResource<ShaderResource>(this->fragmentPath, GL_FRAGMENT_SHADER);
    glAttachShader(this->handle, frag->getHandle());
}
